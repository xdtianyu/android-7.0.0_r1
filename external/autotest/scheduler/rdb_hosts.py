# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""RDB Host objects.

RDBHost: Basic host object, capable of retrieving fields of a host that
correspond to columns of the host table.

RDBServerHostWrapper: Server side host adapters that help in making a raw
database host object more ameanable to the classes and functions in the rdb
and/or rdb clients.

RDBClientHostWrapper: Scheduler host proxy that converts host information
returned by the rdb into a client host object capable of proxying updates
back to the rdb.
"""

import logging
import time
from django.core import exceptions as django_exceptions

import common
from autotest_lib.frontend.afe import rdb_model_extensions as rdb_models
from autotest_lib.frontend.afe import models as afe_models
from autotest_lib.scheduler import rdb_requests
from autotest_lib.scheduler import rdb_utils
from autotest_lib.site_utils import metadata_reporter
from autotest_lib.site_utils.suite_scheduler import constants


class RDBHost(object):
    """A python host object representing a django model for the host."""

    required_fields = set(
            rdb_models.AbstractHostModel.get_basic_field_names() + ['id'])


    def _update_attributes(self, new_attributes):
        """Updates attributes based on an input dictionary.

        Since reads are not proxied to the rdb this method caches updates to
        the host tables as class attributes.

        @param new_attributes: A dictionary of attributes to update.
        """
        for name, value in new_attributes.iteritems():
            setattr(self, name, value)


    def __init__(self, **kwargs):
        if self.required_fields - set(kwargs.keys()):
            raise rdb_utils.RDBException('Creating %s requires %s, got %s '
                    % (self.__class__, self.required_fields, kwargs.keys()))
        self._update_attributes(kwargs)


    @classmethod
    def get_required_fields_from_host(cls, host):
        """Returns all required attributes of the host parsed into a dict.

        Required attributes are defined as the attributes required to
        create an RDBHost, and mirror the columns of the host table.

        @param host: A host object containing all required fields as attributes.
        """
        required_fields_map = {}
        try:
            for field in cls.required_fields:
                required_fields_map[field] = getattr(host, field)
        except AttributeError as e:
            raise rdb_utils.RDBException('Required %s' % e)
        required_fields_map['id'] = host.id
        return required_fields_map


    def wire_format(self):
        """Returns information about this host object.

        @return: A dictionary of fields representing the host.
        """
        return RDBHost.get_required_fields_from_host(self)


class RDBServerHostWrapper(RDBHost):
    """A host wrapper for the base host object.

    This object contains all the attributes of the raw database columns,
    and a few more that make the task of host assignment easier. It handles
    the following duties:
        1. Serialization of the host object and foreign keys
        2. Conversion of label ids to label names, and retrieval of platform
        3. Checking the leased bit/status of a host before leasing it out.
    """

    def __init__(self, host):
        """Create an RDBServerHostWrapper.

        @param host: An instance of the Host model class.
        """
        host_fields = RDBHost.get_required_fields_from_host(host)
        super(RDBServerHostWrapper, self).__init__(**host_fields)
        self.labels = rdb_utils.LabelIterator(host.labels.all())
        self.acls = [aclgroup.id for aclgroup in host.aclgroup_set.all()]
        self.protection = host.protection
        platform = host.platform()
        # Platform needs to be a method, not an attribute, for
        # backwards compatibility with the rest of the host model.
        self.platform_name = platform.name if platform else None
        self.shard_id = host.shard_id


    def refresh(self, fields=None):
        """Refresh the attributes on this instance.

        @param fields: A list of fieldnames to refresh. If None
            all the required fields of the host are refreshed.

        @raises RDBException: If refreshing a field fails.
        """
        # TODO: This is mainly required for cache correctness. If it turns
        # into a bottleneck, cache host_ids instead of rdbhosts and rebuild
        # the hosts once before leasing them out. The important part is to not
        # trust the leased bit on a cached host.
        fields = self.required_fields if not fields else fields
        try:
            refreshed_fields = afe_models.Host.objects.filter(
                    id=self.id).values(*fields)[0]
        except django_exceptions.FieldError as e:
            raise rdb_utils.RDBException('Couldn\'t refresh fields %s: %s' %
                    fields, e)
        self._update_attributes(refreshed_fields)


    def lease(self):
        """Set the leased bit on the host object, and in the database.

        @raises RDBException: If the host is already leased.
        """
        self.refresh(fields=['leased'])
        if self.leased:
            raise rdb_utils.RDBException('Host %s is already leased' %
                                         self.hostname)
        self.leased = True
        # TODO: Avoid leaking django out of rdb.QueryManagers. This is still
        # preferable to calling save() on the host object because we're only
        # updating/refreshing a single indexed attribute, the leased bit.
        afe_models.Host.objects.filter(id=self.id).update(leased=self.leased)


    def wire_format(self, unwrap_foreign_keys=True):
        """Returns all information needed to scheduler jobs on the host.

        @param unwrap_foreign_keys: If true this method will retrieve and
            serialize foreign keys of the original host, which are stored
            in the RDBServerHostWrapper as iterators.

        @return: A dictionary of host information.
        """
        host_info = super(RDBServerHostWrapper, self).wire_format()

        if unwrap_foreign_keys:
            host_info['labels'] = self.labels.get_label_names()
            host_info['acls'] = self.acls
            host_info['platform_name'] = self.platform_name
            host_info['protection'] = self.protection
        return host_info


class RDBClientHostWrapper(RDBHost):
    """A client host wrapper for the base host object.

    This wrapper is used whenever the queue entry needs direct access
    to the host.
    """

    def __init__(self, **kwargs):

        # This class is designed to only check for the bare minimum
        # attributes on a host, so if a client tries accessing an
        # unpopulated foreign key it will result in an exception. Doing
        # so makes it easier to add fields to the rdb host without
        # updating all the clients.
        super(RDBClientHostWrapper, self).__init__(**kwargs)

        # TODO(beeps): Remove this once we transition to urls
        from autotest_lib.scheduler import rdb
        self.update_request_manager = rdb_requests.RDBRequestManager(
                rdb_requests.UpdateHostRequest, rdb.update_hosts)
        self.dbg_str = ''
        self.metadata = {}


    def _update(self, payload):
        """Send an update to rdb, save the attributes of the payload locally.

        @param: A dictionary representing 'key':value of the update required.

        @raises RDBException: If the update fails.
        """
        logging.info('Host %s in %s updating %s through rdb on behalf of: %s ',
                     self.hostname, self.status, payload, self.dbg_str)
        self.update_request_manager.add_request(host_id=self.id,
                payload=payload)
        for response in self.update_request_manager.response():
            if response:
                raise rdb_utils.RDBException('Host %s unable to perform update '
                        '%s through rdb on behalf of %s: %s',  self.hostname,
                        payload, self.dbg_str, response)
        super(RDBClientHostWrapper, self)._update_attributes(payload)


    def record_state(self, type_str, state, value):
        """Record metadata in elasticsearch.

        @param type_str: sets the _type field in elasticsearch db.
        @param state: string representing what state we are recording,
                      e.g. 'status'
        @param value: value of the state, e.g. 'running'
        """
        metadata = {
            state: value,
            'hostname': self.hostname,
            'board': self.board,
            'pools': self.pools,
            'dbg_str': self.dbg_str,
            '_type': type_str,
            'time_recorded': time.time(),
        }
        metadata.update(self.metadata)
        metadata_reporter.queue(metadata)


    def set_status(self, status):
        """Proxy for setting the status of a host via the rdb.

        @param status: The new status.
        """
        self._update({'status': status})
        self.record_state('host_history', 'status', status)


    def update_field(self, fieldname, value):
        """Proxy for updating a field on the host.

        @param fieldname: The fieldname as a string.
        @param value: The value to assign to the field.
        """
        self._update({fieldname: value})


    def platform_and_labels(self):
        """Get the platform and labels on this host.

        @return: A tuple containing a list of label names and the platform name.
        """
        platform = self.platform_name
        labels = [label for label in self.labels if label != platform]
        return platform, labels


    def platform(self):
        """Get the name of the platform of this host.

        @return: A string representing the name of the platform.
        """
        return self.platform_name


    def find_labels_start_with(self, search_string):
        """Find all labels started with given string.

        @param search_string: A string to match the beginning of the label.
        @return: A list of all matched labels.
        """
        try:
            return [l for l in self.labels if l.startswith(search_string)]
        except AttributeError:
            return []


    @property
    def board(self):
        """Get the names of the board of this host.

        @return: A string of the name of the board, e.g., lumpy.
        """
        boards = self.find_labels_start_with(constants.Labels.BOARD_PREFIX)
        return (boards[0][len(constants.Labels.BOARD_PREFIX):] if boards
                else None)


    @property
    def pools(self):
        """Get the names of the pools of this host.

        @return: A list of pool names that the host is assigned to.
        """
        return [label[len(constants.Labels.POOL_PREFIX):] for label in
                self.find_labels_start_with(constants.Labels.POOL_PREFIX)]


    def get_object_dict(self, **kwargs):
        """Serialize the attributes of this object into a dict.

        This method is called through frontend code to get a serialized
        version of this object.

        @param kwargs:
            extra_fields: Extra fields, outside the columns of a host table.

        @return: A dictionary representing the fields of this host object.
        """
        # TODO(beeps): Implement support for extra fields. Currently nothing
        # requires them.
        return self.wire_format()


    def save(self):
        """Save any local data a client of this host object might have saved.

        Setting attributes on a model before calling its save() method is a
        common django pattern. Most, if not all updates to the host happen
        either through set status or update_field. Though we keep the internal
        state of the RDBClientHostWrapper consistent through these updates
        we need a bulk save method such as this one to save any attributes of
        this host another model might have set on it before calling its own
        save method. Eg:
            task = ST.objects.get(id=12)
            task.host.status = 'Running'
            task.save() -> this should result in the hosts status changing to
            Running.

        Functions like add_host_to_labels will have to update this host object
        differently, as that is another level of foreign key indirection.
        """
        self._update(self.get_required_fields_from_host(self))


def return_rdb_host(func):
    """Decorator for functions that return a list of Host objects.

    @param func: The decorated function.
    @return: A functions capable of converting each host_object to a
        rdb_hosts.RDBServerHostWrapper.
    """
    def get_rdb_host(*args, **kwargs):
        """Takes a list of hosts and returns a list of host_infos.

        @param hosts: A list of hosts. Each host is assumed to contain
            all the fields in a host_info defined above.
        @return: A list of rdb_hosts.RDBServerHostWrappers, one per host, or an
            empty list is no hosts were found..
        """
        hosts = func(*args, **kwargs)
        return [RDBServerHostWrapper(host) for host in hosts]
    return get_rdb_host


