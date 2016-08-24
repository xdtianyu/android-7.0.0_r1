# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Rdb server module.
"""

import logging

import common

from django.core import exceptions as django_exceptions
from django.db.models import fields
from django.db.models import Q
from autotest_lib.client.common_lib.cros.graphite import autotest_stats
from autotest_lib.frontend.afe import models
from autotest_lib.scheduler import rdb_cache_manager
from autotest_lib.scheduler import rdb_hosts
from autotest_lib.scheduler import rdb_requests
from autotest_lib.scheduler import rdb_utils
from autotest_lib.server import utils


_timer = autotest_stats.Timer(rdb_utils.RDB_STATS_KEY)
_is_master = not utils.is_shard()


# Qeury managers: Provide a layer of abstraction over the database by
# encapsulating common query patterns used by the rdb.
class BaseHostQueryManager(object):
    """Base manager for host queries on all hosts.
    """

    host_objects = models.Host.objects


    def update_hosts(self, host_ids, **kwargs):
        """Update fields on a hosts.

        @param host_ids: A list of ids of hosts to update.
        @param kwargs: A key value dictionary corresponding to column, value
            in the host database.
        """
        self.host_objects.filter(id__in=host_ids).update(**kwargs)


    @rdb_hosts.return_rdb_host
    def get_hosts(self, ids):
        """Get host objects for the given ids.

        @param ids: The ids for which we need host objects.

        @returns: A list of RDBServerHostWrapper objects, ordered by host_id.
        """
        return self.host_objects.filter(id__in=ids).order_by('id')


    @rdb_hosts.return_rdb_host
    def find_hosts(self, deps, acls):
        """Finds valid hosts matching deps, acls.

        @param deps: A list of dependencies to match.
        @param acls: A list of acls, at least one of which must coincide with
            an acl group the chosen host is in.

        @return: A set of matching hosts available.
        """
        hosts_available = self.host_objects.filter(invalid=0)
        queries = [Q(labels__id=dep) for dep in deps]
        queries += [Q(aclgroup__id__in=acls)]
        for query in queries:
            hosts_available = hosts_available.filter(query)
        return set(hosts_available)


class AvailableHostQueryManager(BaseHostQueryManager):
    """Query manager for requests on un-leased, un-locked hosts.
    """

    host_objects = models.Host.leased_objects


# Request Handlers: Used in conjunction with requests in rdb_utils, these
# handlers acquire hosts for a request and record the acquisition in
# an response_map dictionary keyed on the request itself, with the host/hosts
# as values.
class BaseHostRequestHandler(object):
    """Handler for requests related to hosts, leased or unleased.

    This class is only capable of blindly returning host information.
    """

    def __init__(self):
        self.host_query_manager = BaseHostQueryManager()
        self.response_map = {}


    def update_response_map(self, request, response, append=False):
        """Record a response for a request.

        The response_map only contains requests that were either satisfied, or
        that ran into an exception. Often this translates to reserving hosts
        against a request. If the rdb hit an exception processing a request, the
        exception gets recorded in the map for the client to reraise.

        @param response: A response for the request.
        @param request: The request that has reserved these hosts.
        @param append: Boolean, whether to append new hosts in
                       |response| for existing request.
                       Will not append if existing response is
                       a list of exceptions.

        @raises RDBException: If an empty values is added to the map.
        """
        if not response:
            raise rdb_utils.RDBException('response_map dict can only contain '
                    'valid responses. Request %s, response %s is invalid.' %
                     (request, response))
        exist_response = self.response_map.setdefault(request, [])
        if exist_response and not append:
            raise rdb_utils.RDBException('Request %s already has response %s '
                                         'the rdb cannot return multiple '
                                         'responses for the same request.' %
                                         (request, response))
        if exist_response and append and not isinstance(
                exist_response[0], rdb_hosts.RDBHost):
            # Do not append if existing response contains exception.
            return
        exist_response.extend(response)


    def _check_response_map(self):
        """Verify that we never give the same host to different requests.

        @raises RDBException: If the same host is assigned to multiple requests.
        """
        unique_hosts = set([])
        for request, response in self.response_map.iteritems():
            # Each value in the response map can only either be a list of
            # RDBHosts or a list of RDBExceptions, not a mix of both.
            if isinstance(response[0], rdb_hosts.RDBHost):
                if any([host in unique_hosts for host in response]):
                    raise rdb_utils.RDBException(
                            'Assigning the same host to multiple requests. New '
                            'hosts %s, request %s, response_map: %s' %
                            (response, request, self.response_map))
                else:
                    unique_hosts = unique_hosts.union(response)


    def _record_exceptions(self, request, exceptions):
        """Record a list of exceptions for a request.

        @param request: The request for which the exceptions were hit.
        @param exceptions: The exceptions hit while processing the request.
        """
        rdb_exceptions = [rdb_utils.RDBException(ex) for ex in exceptions]
        self.update_response_map(request, rdb_exceptions)


    def get_response(self):
        """Convert all RDBServerHostWrapper objects to host info dictionaries.

        @return: A dictionary mapping requests to a list of matching host_infos.

        @raises RDBException: If the same host is assigned to multiple requests.
        """
        self._check_response_map()
        for request, response in self.response_map.iteritems():
            self.response_map[request] = [reply.wire_format()
                                          for reply in response]
        return self.response_map


    def update_hosts(self, update_requests):
        """Updates host tables with a payload.

        @param update_requests: A list of update requests, as defined in
            rdb_requests.UpdateHostRequest.
        """
        # Last payload for a host_id wins in the case of conflicting requests.
        unique_host_requests = {}
        for request in update_requests:
            if unique_host_requests.get(request.host_id):
                unique_host_requests[request.host_id].update(request.payload)
            else:
                unique_host_requests[request.host_id] = request.payload

        # Batch similar payloads so we can do them in one table scan.
        similar_requests = {}
        for host_id, payload in unique_host_requests.iteritems():
            similar_requests.setdefault(payload, []).append(host_id)

        # If fields of the update don't match columns in the database,
        # record the exception in the response map. This also means later
        # updates will get applied even if previous updates fail.
        for payload, hosts in similar_requests.iteritems():
            try:
                response = self.host_query_manager.update_hosts(hosts, **payload)
            except (django_exceptions.FieldError,
                    fields.FieldDoesNotExist, ValueError) as e:
                for host in hosts:
                    # Since update requests have a consistent hash this will map
                    # to the same key as the original request.
                    request = rdb_requests.UpdateHostRequest(
                            host_id=host, payload=payload).get_request()
                    self._record_exceptions(request, [e])


    def batch_get_hosts(self, host_requests):
        """Get hosts matching the requests.

        This method does not acquire the hosts, i.e it reserves hosts against
        requests leaving their leased state untouched.

        @param host_requests: A list of requests, as defined in
            rdb_utils.BaseHostRequest.
        """
        host_ids = set([request.host_id for request in host_requests])
        host_map = {}

        # This list will not contain available hosts if executed using
        # an AvailableHostQueryManager.
        for host in self.host_query_manager.get_hosts(host_ids):
            host_map[host.id] = host
        for request in host_requests:
            if request.host_id in host_map:
                self.update_response_map(request, [host_map[request.host_id]])
            else:
                logging.warning('rdb could not get host for request: %s, it '
                                'is already leased or locked', request)


class AvailableHostRequestHandler(BaseHostRequestHandler):
    """Handler for requests related to available (unleased and unlocked) hosts.

    This class is capable of acquiring or validating hosts for requests.
    """


    def __init__(self):
        self.host_query_manager = AvailableHostQueryManager()
        self.cache = rdb_cache_manager.RDBHostCacheManager()
        self.response_map = {}
        self.unsatisfied_requests = 0
        self.leased_hosts_count = 0
        self.request_accountant = None


    @_timer.decorate
    def lease_hosts(self, hosts):
        """Leases a list of hosts.

        @param hosts: A list of RDBServerHostWrapper instances to lease.

        @return: The list of RDBServerHostWrappers that were successfully
            leased.
        """
        #TODO(beeps): crbug.com/353183.
        unleased_hosts = set(hosts)
        leased_hosts = set([])
        for host in unleased_hosts:
            try:
                host.lease()
            except rdb_utils.RDBException as e:
                logging.error('Unable to lease host %s: %s', host.hostname, e)
            else:
                leased_hosts.add(host)
        return list(leased_hosts)


    @classmethod
    def valid_host_assignment(cls, request, host):
        """Check if a host, request pairing is valid.

        @param request: The request to match against the host.
        @param host: An RDBServerHostWrapper instance.

        @return: True if the host, request assignment is valid.

        @raises RDBException: If the request already has another host_ids
            associated with it.
        """
        if request.host_id and request.host_id != host.id:
            raise rdb_utils.RDBException(
                    'Cannot assign a different host for request: %s, it '
                    'already has one: %s ' % (request, host.id))

        # Getting all labels and acls might result in large queries, so
        # bail early if the host is already leased.
        if host.leased:
            return False
        # If a host is invalid it must be a one time host added to the
        # afe specifically for this purpose, so it doesn't require acl checking.
        acl_match = (request.acls.intersection(host.acls) or host.invalid)
        label_match = (request.deps.intersection(host.labels) == request.deps)
        return acl_match and label_match


    @classmethod
    def _sort_hosts_by_preferred_deps(cls, hosts, preferred_deps):
        """Sort hosts in the order of how many preferred deps it has.

        This allows rdb always choose the hosts with the most preferred deps
        for a request. One important use case is including cros-version as
        a preferred dependence. By choosing a host with the same cros-version,
        we can save the time on provisioning it. Note this is not guaranteed
        if preferred_deps contains other labels as well.

        @param hosts: A list of hosts to sort.
        @param preferred_deps: A list of deps that are preferred.

        @return: A list of sorted hosts.

        """
        hosts = sorted(
                hosts,
                key=lambda host: len(set(preferred_deps) & set(host.labels)),
                reverse=True)
        return hosts


    @rdb_cache_manager.memoize_hosts
    def _acquire_hosts(self, request, hosts_required, is_acquire_min_duts=False,
                       **kwargs):
        """Acquire hosts for a group of similar requests.

        Find and acquire hosts that can satisfy a group of requests.
        1. If the caching decorator doesn't pass in a list of matching hosts
           via the MEMOIZE_KEY this method will directly check the database for
           matching hosts.
        2. If all matching hosts are not leased for this request, the remaining
           hosts are returned to the caching decorator, to place in the cache.

        @param hosts_required: Number of hosts required to satisfy request.
        @param request: The request for hosts.
        @param is_acquire_min_duts: Boolean. Indicate whether this is to
                                    acquire minimum required duts, only used
                                    for stats purpose.

        @return: The list of excess matching hosts.
        """
        hosts = kwargs.get(rdb_cache_manager.MEMOIZE_KEY, [])
        if not hosts:
            hosts = self.host_query_manager.find_hosts(
                            request.deps, request.acls)

        # <-----[:attempt_lease_hosts](evicted)--------> <-(returned, cached)->
        # |   -leased_hosts-  |   -stale cached hosts-  | -unleased matching- |
        # --used this request---used by earlier request----------unused--------
        hosts = self._sort_hosts_by_preferred_deps(
                hosts, request.preferred_deps)
        attempt_lease_hosts = min(len(hosts), hosts_required)
        leased_host_count = 0
        if attempt_lease_hosts:
            leased_hosts = self.lease_hosts(hosts[:attempt_lease_hosts])
            if leased_hosts:
                self.update_response_map(request, leased_hosts, append=True)

            # [:attempt_leased_hosts] - leased_hosts will include hosts that
            # failed leasing, most likely because they're already leased, so
            # don't cache them again.
            leased_host_count = len(leased_hosts)
            failed_leasing = attempt_lease_hosts - leased_host_count
            if failed_leasing > 0:
                # For the sake of simplicity this calculation assumes that
                # leasing only fails if there's a stale cached host already
                # leased by a previous request, ergo, we can only get here
                # through a cache hit.
                line_length = len(hosts)
                self.cache.stale_entries.append(
                        (float(failed_leasing)/line_length) * 100)
            self.leased_hosts_count += leased_host_count
        if is_acquire_min_duts:
            self.request_accountant.record_acquire_min_duts(
                    request, hosts_required, leased_host_count)
        self.unsatisfied_requests += max(hosts_required - leased_host_count, 0)
        # Cache the unleased matching hosts against the request.
        return hosts[attempt_lease_hosts:]


    @_timer.decorate
    def batch_acquire_hosts(self, host_requests):
        """Acquire hosts for a list of requests.

        The act of acquisition involves finding and leasing a set of hosts
        that match the parameters of a request. Each acquired host is added
        to the response_map dictionary as an RDBServerHostWrapper.

        @param host_requests: A list of requests to acquire hosts.
        """
        distinct_requests = 0

        logging.debug('Processing %s host acquisition requests',
                      len(host_requests))

        self.request_accountant = rdb_utils.RequestAccountant(host_requests)
        # First pass tries to satisfy min_duts for each suite.
        for request in self.request_accountant.requests:
            to_acquire = self.request_accountant.get_min_duts(request)
            if to_acquire > 0:
                self._acquire_hosts(request, to_acquire,
                                    is_acquire_min_duts=True)
            distinct_requests += 1

        # Second pass tries to allocate duts to the rest unsatisfied requests.
        for request in self.request_accountant.requests:
            to_acquire = self.request_accountant.get_duts(request)
            if to_acquire > 0:
                self._acquire_hosts(request, to_acquire,
                                    is_acquire_min_duts=False)

        self.cache.record_stats()
        logging.debug('Host acquisition stats: distinct requests: %s, leased '
                      'hosts: %s, unsatisfied requests: %s', distinct_requests,
                      self.leased_hosts_count, self.unsatisfied_requests)
        autotest_stats.Gauge(rdb_utils.RDB_STATS_KEY).send(
                'leased_hosts', self.leased_hosts_count)
        autotest_stats.Gauge(rdb_utils.RDB_STATS_KEY).send(
                'unsatisfied_requests', self.unsatisfied_requests)


    @_timer.decorate
    def batch_validate_hosts(self, requests):
        """Validate requests with hosts.

        Reserve all hosts, check each one for validity and discard invalid
        request-host pairings. Lease the remaining hsots.

        @param requests: A list of requests to validate.

        @raises RDBException: If multiple hosts or the wrong host is returned
            for a response.
        """
        # The following cases are possible for frontend requests:
        # 1. Multiple requests for 1 host, with different acls/deps/priority:
        #    These form distinct requests because they hash differently.
        #    The response map will contain entries like: {r1: h1, r2: h1}
        #    after the batch_get_hosts call. There are 2 sub-cases:
        #        a. Same deps/acls, different priority:
        #           Since we sort the requests based on priority, the
        #           higher priority request r1, will lease h1. The
        #           validation of r2, h1 will fail because of the r1 lease.
        #        b. Different deps/acls, only one of which matches the host:
        #           The matching request will lease h1. The other host
        #           pairing will get dropped from the response map.
        # 2. Multiple requests with the same acls/deps/priority and 1 host:
        #    These all have the same request hash, so the response map will
        #    contain: {r: h}, regardless of the number of r's. If this is not
        #    a valid host assignment it will get dropped from the response.
        self.batch_get_hosts(set(requests))
        for request in sorted(self.response_map.keys(),
                key=lambda request: request.priority, reverse=True):
            hosts = self.response_map[request]
            if len(hosts) > 1:
                raise rdb_utils.RDBException('Got multiple hosts for a single '
                        'request. Hosts: %s, request %s.' % (hosts, request))
            # Job-shard is 1:1 mapping. Because a job can only belongs
            # to one shard, or belongs to master, we disallow frontend job
            # that spans hosts on and off shards or across multiple shards,
            # which would otherwise break the 1:1 mapping.
            # As such, on master, if a request asks for multiple hosts and
            # if any host is found on shard, we assume other requested hosts
            # would also be on the same shard.  We can safely drop this request.
            ignore_request = _is_master and any(
                    [host.shard_id for host in hosts])
            if (not ignore_request and
                    (self.valid_host_assignment(request, hosts[0]) and
                        self.lease_hosts(hosts))):
                continue
            del self.response_map[request]
            logging.warning('Request %s was not able to lease host %s',
                            request, hosts[0])


# Request dispatchers: Create the appropriate request handler, send a list
# of requests to one of its methods. The corresponding request handler in
# rdb_lib must understand how to match each request with a response from a
# dispatcher, the easiest way to achieve this is to returned the response_map
# attribute of the request handler, after making the appropriate requests.
def get_hosts(host_requests):
    """Get host information about the requested hosts.

    @param host_requests: A list of requests as defined in BaseHostRequest.
    @return: A dictionary mapping each request to a list of hosts.
    """
    rdb_handler = BaseHostRequestHandler()
    rdb_handler.batch_get_hosts(host_requests)
    return rdb_handler.get_response()


def update_hosts(update_requests):
    """Update hosts.

    @param update_requests: A list of updates to host tables
        as defined in UpdateHostRequest.
    """
    rdb_handler = BaseHostRequestHandler()
    rdb_handler.update_hosts(update_requests)
    return rdb_handler.get_response()


def rdb_host_request_dispatcher(host_requests):
    """Dispatcher for all host acquisition queries.

    @param host_requests: A list of requests for acquiring hosts, as defined in
        AcquireHostRequest.
    @return: A dictionary mapping each request to a list of hosts, or
        an empty list if none could satisfy the request. Eg:
        {AcquireHostRequest.template: [host_info_dictionaries]}
    """
    validation_requests = []
    require_hosts_requests = []

    # Validation requests are made by a job scheduled against a specific host
    # specific host (eg: through the frontend) and only require the rdb to
    # match the parameters of the host against the request. Acquisition
    # requests are made by jobs that need hosts (eg: suites) and the rdb needs
    # to find hosts matching the parameters of the request.
    for request in host_requests:
        if request.host_id:
            validation_requests.append(request)
        else:
            require_hosts_requests.append(request)

    rdb_handler = AvailableHostRequestHandler()
    rdb_handler.batch_validate_hosts(validation_requests)
    rdb_handler.batch_acquire_hosts(require_hosts_requests)
    return rdb_handler.get_response()
