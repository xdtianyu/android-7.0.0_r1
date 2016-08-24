# pylint: disable-msg=C0111

"""\
Functions to expose over the RPC interface.

For all modify* and delete* functions that ask for an 'id' parameter to
identify the object to operate on, the id may be either
 * the database row ID
 * the name of the object (label name, hostname, user login, etc.)
 * a dictionary containing uniquely identifying field (this option should seldom
   be used)

When specifying foreign key fields (i.e. adding hosts to a label, or adding
users to an ACL group), the given value may be either the database row ID or the
name of the object.

All get* functions return lists of dictionaries.  Each dictionary represents one
object and maps field names to values.

Some examples:
modify_host(2, hostname='myhost') # modify hostname of host with database ID 2
modify_host('ipaj2', hostname='myhost') # modify hostname of host 'ipaj2'
modify_test('sleeptest', test_type='Client', params=', seconds=60')
delete_acl_group(1) # delete by ID
delete_acl_group('Everyone') # delete by name
acl_group_add_users('Everyone', ['mbligh', 'showard'])
get_jobs(owner='showard', status='Queued')

See doctests/001_rpc_test.txt for (lots) more examples.
"""

__author__ = 'showard@google.com (Steve Howard)'

import sys
import datetime
import logging

from django.db.models import Count
import common
from autotest_lib.client.common_lib import priorities
from autotest_lib.client.common_lib.cros import dev_server
from autotest_lib.client.common_lib.cros.graphite import autotest_stats
from autotest_lib.frontend.afe import control_file, rpc_utils
from autotest_lib.frontend.afe import models, model_logic, model_attributes
from autotest_lib.frontend.afe import site_rpc_interface
from autotest_lib.frontend.tko import models as tko_models
from autotest_lib.frontend.tko import rpc_interface as tko_rpc_interface
from autotest_lib.server import frontend
from autotest_lib.server import utils
from autotest_lib.server.cros import provision
from autotest_lib.server.cros.dynamic_suite import tools
from autotest_lib.site_utils import status_history


_timer = autotest_stats.Timer('rpc_interface')

def get_parameterized_autoupdate_image_url(job):
    """Get the parameterized autoupdate image url from a parameterized job."""
    known_test_obj = models.Test.smart_get('autoupdate_ParameterizedJob')
    image_parameter = known_test_obj.testparameter_set.get(test=known_test_obj,
                                                           name='image')
    para_set = job.parameterized_job.parameterizedjobparameter_set
    job_test_para = para_set.get(test_parameter=image_parameter)
    return job_test_para.parameter_value


# labels

def modify_label(id, **data):
    """Modify a label.

    @param id: id or name of a label. More often a label name.
    @param data: New data for a label.
    """
    label_model = models.Label.smart_get(id)
    label_model.update_object(data)

    # Master forwards the RPC to shards
    if not utils.is_shard():
        rpc_utils.fanout_rpc(label_model.host_set.all(), 'modify_label', False,
                             id=id, **data)


def delete_label(id):
    """Delete a label.

    @param id: id or name of a label. More often a label name.
    """
    label_model = models.Label.smart_get(id)
    # Hosts that have the label to be deleted. Save this info before
    # the label is deleted to use it later.
    hosts = []
    for h in label_model.host_set.all():
        hosts.append(models.Host.smart_get(h.id))
    label_model.delete()

    # Master forwards the RPC to shards
    if not utils.is_shard():
        rpc_utils.fanout_rpc(hosts, 'delete_label', False, id=id)


def add_label(name, ignore_exception_if_exists=False, **kwargs):
    """Adds a new label of a given name.

    @param name: label name.
    @param ignore_exception_if_exists: If True and the exception was
        thrown due to the duplicated label name when adding a label,
        then suppress the exception. Default is False.
    @param kwargs: keyword args that store more info about a label
        other than the name.
    @return: int/long id of a new label.
    """
    # models.Label.add_object() throws model_logic.ValidationError
    # when it is given a label name that already exists.
    # However, ValidationError can be thrown with different errors,
    # and those errors should be thrown up to the call chain.
    try:
        label = models.Label.add_object(name=name, **kwargs)
    except:
        exc_info = sys.exc_info()
        if ignore_exception_if_exists:
            label = rpc_utils.get_label(name)
            # If the exception is raised not because of duplicated
            # "name", then raise the original exception.
            if label is None:
                raise exc_info[0], exc_info[1], exc_info[2]
        else:
            raise exc_info[0], exc_info[1], exc_info[2]
    return label.id


def add_label_to_hosts(id, hosts):
    """Adds a label of the given id to the given hosts only in local DB.

    @param id: id or name of a label. More often a label name.
    @param hosts: The hostnames of hosts that need the label.

    @raises models.Label.DoesNotExist: If the label with id doesn't exist.
    """
    label = models.Label.smart_get(id)
    host_objs = models.Host.smart_get_bulk(hosts)
    if label.platform:
        models.Host.check_no_platform(host_objs)
    label.host_set.add(*host_objs)


@rpc_utils.route_rpc_to_master
def label_add_hosts(id, hosts):
    """Adds a label with the given id to the given hosts.

    This method should be run only on master not shards.
    The given label will be created if it doesn't exist, provided the `id`
    supplied is a label name not an int/long id.

    @param id: id or name of a label. More often a label name.
    @param hosts: A list of hostnames or ids. More often hostnames.

    @raises ValueError: If the id specified is an int/long (label id)
                        while the label does not exist.
    """
    try:
        label = models.Label.smart_get(id)
    except models.Label.DoesNotExist:
        # This matches the type checks in smart_get, which is a hack
        # in and off itself. The aim here is to create any non-existent
        # label, which we cannot do if the 'id' specified isn't a label name.
        if isinstance(id, basestring):
            label = models.Label.smart_get(add_label(id))
        else:
            raise ValueError('Label id (%s) does not exist. Please specify '
                             'the argument, id, as a string (label name).'
                             % id)
    add_label_to_hosts(id, hosts)

    host_objs = models.Host.smart_get_bulk(hosts)
    # Make sure the label exists on the shard with the same id
    # as it is on the master.
    # It is possible that the label is already in a shard because
    # we are adding a new label only to shards of hosts that the label
    # is going to be attached.
    # For example, we add a label L1 to a host in shard S1.
    # Master and S1 will have L1 but other shards won't.
    # Later, when we add the same label L1 to hosts in shards S1 and S2,
    # S1 already has the label but S2 doesn't.
    # S2 should have the new label without any problem.
    # We ignore exception in such a case.
    rpc_utils.fanout_rpc(
            host_objs, 'add_label', include_hostnames=False,
            name=label.name, ignore_exception_if_exists=True,
            id=label.id, platform=label.platform)
    rpc_utils.fanout_rpc(host_objs, 'add_label_to_hosts', id=id)


def remove_label_from_hosts(id, hosts):
    """Removes a label of the given id from the given hosts only in local DB.

    @param id: id or name of a label.
    @param hosts: The hostnames of hosts that need to remove the label from.
    """
    host_objs = models.Host.smart_get_bulk(hosts)
    models.Label.smart_get(id).host_set.remove(*host_objs)


@rpc_utils.route_rpc_to_master
def label_remove_hosts(id, hosts):
    """Removes a label of the given id from the given hosts.

    This method should be run only on master not shards.

    @param id: id or name of a label.
    @param hosts: A list of hostnames or ids. More often hostnames.
    """
    host_objs = models.Host.smart_get_bulk(hosts)
    remove_label_from_hosts(id, hosts)

    rpc_utils.fanout_rpc(host_objs, 'remove_label_from_hosts', id=id)


def get_labels(exclude_filters=(), **filter_data):
    """\
    @param exclude_filters: A sequence of dictionaries of filters.

    @returns A sequence of nested dictionaries of label information.
    """
    labels = models.Label.query_objects(filter_data)
    for exclude_filter in exclude_filters:
        labels = labels.exclude(**exclude_filter)
    return rpc_utils.prepare_rows_as_nested_dicts(labels, ('atomic_group',))


# atomic groups

def add_atomic_group(name, max_number_of_machines=None, description=None):
    return models.AtomicGroup.add_object(
            name=name, max_number_of_machines=max_number_of_machines,
            description=description).id


def modify_atomic_group(id, **data):
    models.AtomicGroup.smart_get(id).update_object(data)


def delete_atomic_group(id):
    models.AtomicGroup.smart_get(id).delete()


def atomic_group_add_labels(id, labels):
    label_objs = models.Label.smart_get_bulk(labels)
    models.AtomicGroup.smart_get(id).label_set.add(*label_objs)


def atomic_group_remove_labels(id, labels):
    label_objs = models.Label.smart_get_bulk(labels)
    models.AtomicGroup.smart_get(id).label_set.remove(*label_objs)


def get_atomic_groups(**filter_data):
    return rpc_utils.prepare_for_serialization(
            models.AtomicGroup.list_objects(filter_data))


# hosts

def add_host(hostname, status=None, locked=None, lock_reason='', protection=None):
    if locked and not lock_reason:
        raise model_logic.ValidationError(
            {'locked': 'Please provide a reason for locking when adding host.'})

    return models.Host.add_object(hostname=hostname, status=status,
                                  locked=locked, lock_reason=lock_reason,
                                  protection=protection).id


@rpc_utils.route_rpc_to_master
def modify_host(id, **kwargs):
    """Modify local attributes of a host.

    If this is called on the master, but the host is assigned to a shard, this
    will call `modify_host_local` RPC to the responsible shard. This means if
    a host is being locked using this function, this change will also propagate
    to shards.
    When this is called on a shard, the shard just routes the RPC to the master
    and does nothing.

    @param id: id of the host to modify.
    @param kwargs: key=value pairs of values to set on the host.
    """
    rpc_utils.check_modify_host(kwargs)
    host = models.Host.smart_get(id)
    try:
        rpc_utils.check_modify_host_locking(host, kwargs)
    except model_logic.ValidationError as e:
        if not kwargs.get('force_modify_locking', False):
            raise
        logging.exception('The following exception will be ignored and lock '
                          'modification will be enforced. %s', e)

    # This is required to make `lock_time` for a host be exactly same
    # between the master and a shard.
    if kwargs.get('locked', None) and 'lock_time' not in kwargs:
        kwargs['lock_time'] = datetime.datetime.now()
    host.update_object(kwargs)

    # force_modifying_locking is not an internal field in database, remove.
    kwargs.pop('force_modify_locking', None)
    rpc_utils.fanout_rpc([host], 'modify_host_local',
                         include_hostnames=False, id=id, **kwargs)


def modify_host_local(id, **kwargs):
    """Modify host attributes in local DB.

    @param id: Host id.
    @param kwargs: key=value pairs of values to set on the host.
    """
    models.Host.smart_get(id).update_object(kwargs)


@rpc_utils.route_rpc_to_master
def modify_hosts(host_filter_data, update_data):
    """Modify local attributes of multiple hosts.

    If this is called on the master, but one of the hosts in that match the
    filters is assigned to a shard, this will call `modify_hosts_local` RPC
    to the responsible shard.
    When this is called on a shard, the shard just routes the RPC to the master
    and does nothing.

    The filters are always applied on the master, not on the shards. This means
    if the states of a host differ on the master and a shard, the state on the
    master will be used. I.e. this means:
    A host was synced to Shard 1. On Shard 1 the status of the host was set to
    'Repair Failed'.
    - A call to modify_hosts with host_filter_data={'status': 'Ready'} will
    update the host (both on the shard and on the master), because the state
    of the host as the master knows it is still 'Ready'.
    - A call to modify_hosts with host_filter_data={'status': 'Repair failed'
    will not update the host, because the filter doesn't apply on the master.

    @param host_filter_data: Filters out which hosts to modify.
    @param update_data: A dictionary with the changes to make to the hosts.
    """
    update_data = update_data.copy()
    rpc_utils.check_modify_host(update_data)
    hosts = models.Host.query_objects(host_filter_data)

    affected_shard_hostnames = set()
    affected_host_ids = []

    # Check all hosts before changing data for exception safety.
    for host in hosts:
        try:
            rpc_utils.check_modify_host_locking(host, update_data)
        except model_logic.ValidationError as e:
            if not update_data.get('force_modify_locking', False):
                raise
            logging.exception('The following exception will be ignored and '
                              'lock modification will be enforced. %s', e)

        if host.shard:
            affected_shard_hostnames.add(host.shard.rpc_hostname())
            affected_host_ids.append(host.id)

    # This is required to make `lock_time` for a host be exactly same
    # between the master and a shard.
    if update_data.get('locked', None) and 'lock_time' not in update_data:
        update_data['lock_time'] = datetime.datetime.now()
    for host in hosts:
        host.update_object(update_data)

    update_data.pop('force_modify_locking', None)
    # Caution: Changing the filter from the original here. See docstring.
    rpc_utils.run_rpc_on_multiple_hostnames(
            'modify_hosts_local', affected_shard_hostnames,
            host_filter_data={'id__in': affected_host_ids},
            update_data=update_data)


def modify_hosts_local(host_filter_data, update_data):
    """Modify attributes of hosts in local DB.

    @param host_filter_data: Filters out which hosts to modify.
    @param update_data: A dictionary with the changes to make to the hosts.
    """
    for host in models.Host.query_objects(host_filter_data):
        host.update_object(update_data)


def add_labels_to_host(id, labels):
    """Adds labels to a given host only in local DB.

    @param id: id or hostname for a host.
    @param labels: ids or names for labels.
    """
    label_objs = models.Label.smart_get_bulk(labels)
    models.Host.smart_get(id).labels.add(*label_objs)


@rpc_utils.route_rpc_to_master
def host_add_labels(id, labels):
    """Adds labels to a given host.

    @param id: id or hostname for a host.
    @param labels: ids or names for labels.

    @raises ValidationError: If adding more than one platform label.
    """
    label_objs = models.Label.smart_get_bulk(labels)
    platforms = [label.name for label in label_objs if label.platform]
    if len(platforms) > 1:
        raise model_logic.ValidationError(
            {'labels': 'Adding more than one platform label: %s' %
                       ', '.join(platforms)})

    host_obj = models.Host.smart_get(id)
    if len(platforms) == 1:
        models.Host.check_no_platform([host_obj])
    add_labels_to_host(id, labels)

    rpc_utils.fanout_rpc([host_obj], 'add_labels_to_host', False,
                         id=id, labels=labels)


def remove_labels_from_host(id, labels):
    """Removes labels from a given host only in local DB.

    @param id: id or hostname for a host.
    @param labels: ids or names for labels.
    """
    label_objs = models.Label.smart_get_bulk(labels)
    models.Host.smart_get(id).labels.remove(*label_objs)


@rpc_utils.route_rpc_to_master
def host_remove_labels(id, labels):
    """Removes labels from a given host.

    @param id: id or hostname for a host.
    @param labels: ids or names for labels.
    """
    remove_labels_from_host(id, labels)

    host_obj = models.Host.smart_get(id)
    rpc_utils.fanout_rpc([host_obj], 'remove_labels_from_host', False,
                         id=id, labels=labels)


def get_host_attribute(attribute, **host_filter_data):
    """
    @param attribute: string name of attribute
    @param host_filter_data: filter data to apply to Hosts to choose hosts to
                             act upon
    """
    hosts = rpc_utils.get_host_query((), False, False, True, host_filter_data)
    hosts = list(hosts)
    models.Host.objects.populate_relationships(hosts, models.HostAttribute,
                                               'attribute_list')
    host_attr_dicts = []
    for host_obj in hosts:
        for attr_obj in host_obj.attribute_list:
            if attr_obj.attribute == attribute:
                host_attr_dicts.append(attr_obj.get_object_dict())
    return rpc_utils.prepare_for_serialization(host_attr_dicts)


def set_host_attribute(attribute, value, **host_filter_data):
    """
    @param attribute: string name of attribute
    @param value: string, or None to delete an attribute
    @param host_filter_data: filter data to apply to Hosts to choose hosts to
                             act upon
    """
    assert host_filter_data # disallow accidental actions on all hosts
    hosts = models.Host.query_objects(host_filter_data)
    models.AclGroup.check_for_acl_violation_hosts(hosts)
    for host in hosts:
        host.set_or_delete_attribute(attribute, value)

    # Master forwards this RPC to shards.
    if not utils.is_shard():
        rpc_utils.fanout_rpc(hosts, 'set_host_attribute', False,
                attribute=attribute, value=value, **host_filter_data)


@rpc_utils.forward_single_host_rpc_to_shard
def delete_host(id):
    models.Host.smart_get(id).delete()


def get_hosts(multiple_labels=(), exclude_only_if_needed_labels=False,
              exclude_atomic_group_hosts=False, valid_only=True,
              include_current_job=False, **filter_data):
    """Get a list of dictionaries which contains the information of hosts.

    @param multiple_labels: match hosts in all of the labels given.  Should
            be a list of label names.
    @param exclude_only_if_needed_labels: Exclude hosts with at least one
            "only_if_needed" label applied.
    @param exclude_atomic_group_hosts: Exclude hosts that have one or more
            atomic group labels associated with them.
    @param include_current_job: Set to True to include ids of currently running
            job and special task.
    """
    hosts = rpc_utils.get_host_query(multiple_labels,
                                     exclude_only_if_needed_labels,
                                     exclude_atomic_group_hosts,
                                     valid_only, filter_data)
    hosts = list(hosts)
    models.Host.objects.populate_relationships(hosts, models.Label,
                                               'label_list')
    models.Host.objects.populate_relationships(hosts, models.AclGroup,
                                               'acl_list')
    models.Host.objects.populate_relationships(hosts, models.HostAttribute,
                                               'attribute_list')
    host_dicts = []
    for host_obj in hosts:
        host_dict = host_obj.get_object_dict()
        host_dict['labels'] = [label.name for label in host_obj.label_list]
        host_dict['platform'], host_dict['atomic_group'] = (rpc_utils.
                find_platform_and_atomic_group(host_obj))
        host_dict['acls'] = [acl.name for acl in host_obj.acl_list]
        host_dict['attributes'] = dict((attribute.attribute, attribute.value)
                                       for attribute in host_obj.attribute_list)
        if include_current_job:
            host_dict['current_job'] = None
            host_dict['current_special_task'] = None
            entries = models.HostQueueEntry.objects.filter(
                    host_id=host_dict['id'], active=True, complete=False)
            if entries:
                host_dict['current_job'] = (
                        entries[0].get_object_dict()['job'])
            tasks = models.SpecialTask.objects.filter(
                    host_id=host_dict['id'], is_active=True, is_complete=False)
            if tasks:
                host_dict['current_special_task'] = (
                        '%d-%s' % (tasks[0].get_object_dict()['id'],
                                   tasks[0].get_object_dict()['task'].lower()))
        host_dicts.append(host_dict)
    return rpc_utils.prepare_for_serialization(host_dicts)


def get_num_hosts(multiple_labels=(), exclude_only_if_needed_labels=False,
                  exclude_atomic_group_hosts=False, valid_only=True,
                  **filter_data):
    """
    Same parameters as get_hosts().

    @returns The number of matching hosts.
    """
    hosts = rpc_utils.get_host_query(multiple_labels,
                                     exclude_only_if_needed_labels,
                                     exclude_atomic_group_hosts,
                                     valid_only, filter_data)
    return hosts.count()


# tests

def add_test(name, test_type, path, author=None, dependencies=None,
             experimental=True, run_verify=None, test_class=None,
             test_time=None, test_category=None, description=None,
             sync_count=1):
    return models.Test.add_object(name=name, test_type=test_type, path=path,
                                  author=author, dependencies=dependencies,
                                  experimental=experimental,
                                  run_verify=run_verify, test_time=test_time,
                                  test_category=test_category,
                                  sync_count=sync_count,
                                  test_class=test_class,
                                  description=description).id


def modify_test(id, **data):
    models.Test.smart_get(id).update_object(data)


def delete_test(id):
    models.Test.smart_get(id).delete()


def get_tests(**filter_data):
    return rpc_utils.prepare_for_serialization(
        models.Test.list_objects(filter_data))


@_timer.decorate
def get_tests_status_counts_by_job_name_label(job_name_prefix, label_name):
    """Gets the counts of all passed and failed tests from the matching jobs.

    @param job_name_prefix: Name prefix of the jobs to get the summary from, e.g.,
            'butterfly-release/R40-6457.21.0/bvt-cq/'.
    @param label_name: Label that must be set in the jobs, e.g.,
            'cros-version:butterfly-release/R40-6457.21.0'.

    @returns A summary of the counts of all the passed and failed tests.
    """
    job_ids = list(models.Job.objects.filter(
            name__startswith=job_name_prefix,
            dependency_labels__name=label_name).values_list(
                'pk', flat=True))
    summary = {'passed': 0, 'failed': 0}
    if not job_ids:
        return summary

    counts = (tko_models.TestView.objects.filter(
            afe_job_id__in=job_ids).exclude(
                test_name='SERVER_JOB').exclude(
                    test_name__startswith='CLIENT_JOB').values(
                        'status').annotate(
                            count=Count('status')))
    for status in counts:
        if status['status'] == 'GOOD':
            summary['passed'] += status['count']
        else:
            summary['failed'] += status['count']
    return summary


# profilers

def add_profiler(name, description=None):
    return models.Profiler.add_object(name=name, description=description).id


def modify_profiler(id, **data):
    models.Profiler.smart_get(id).update_object(data)


def delete_profiler(id):
    models.Profiler.smart_get(id).delete()


def get_profilers(**filter_data):
    return rpc_utils.prepare_for_serialization(
        models.Profiler.list_objects(filter_data))


# users

def add_user(login, access_level=None):
    return models.User.add_object(login=login, access_level=access_level).id


def modify_user(id, **data):
    models.User.smart_get(id).update_object(data)


def delete_user(id):
    models.User.smart_get(id).delete()


def get_users(**filter_data):
    return rpc_utils.prepare_for_serialization(
        models.User.list_objects(filter_data))


# acl groups

def add_acl_group(name, description=None):
    group = models.AclGroup.add_object(name=name, description=description)
    group.users.add(models.User.current_user())
    return group.id


def modify_acl_group(id, **data):
    group = models.AclGroup.smart_get(id)
    group.check_for_acl_violation_acl_group()
    group.update_object(data)
    group.add_current_user_if_empty()


def acl_group_add_users(id, users):
    group = models.AclGroup.smart_get(id)
    group.check_for_acl_violation_acl_group()
    users = models.User.smart_get_bulk(users)
    group.users.add(*users)


def acl_group_remove_users(id, users):
    group = models.AclGroup.smart_get(id)
    group.check_for_acl_violation_acl_group()
    users = models.User.smart_get_bulk(users)
    group.users.remove(*users)
    group.add_current_user_if_empty()


def acl_group_add_hosts(id, hosts):
    group = models.AclGroup.smart_get(id)
    group.check_for_acl_violation_acl_group()
    hosts = models.Host.smart_get_bulk(hosts)
    group.hosts.add(*hosts)
    group.on_host_membership_change()


def acl_group_remove_hosts(id, hosts):
    group = models.AclGroup.smart_get(id)
    group.check_for_acl_violation_acl_group()
    hosts = models.Host.smart_get_bulk(hosts)
    group.hosts.remove(*hosts)
    group.on_host_membership_change()


def delete_acl_group(id):
    models.AclGroup.smart_get(id).delete()


def get_acl_groups(**filter_data):
    acl_groups = models.AclGroup.list_objects(filter_data)
    for acl_group in acl_groups:
        acl_group_obj = models.AclGroup.objects.get(id=acl_group['id'])
        acl_group['users'] = [user.login
                              for user in acl_group_obj.users.all()]
        acl_group['hosts'] = [host.hostname
                              for host in acl_group_obj.hosts.all()]
    return rpc_utils.prepare_for_serialization(acl_groups)


# jobs

def generate_control_file(tests=(), kernel=None, label=None, profilers=(),
                          client_control_file='', use_container=False,
                          profile_only=None, upload_kernel_config=False,
                          db_tests=True):
    """
    Generates a client-side control file to load a kernel and run tests.

    @param tests List of tests to run. See db_tests for more information.
    @param kernel A list of kernel info dictionaries configuring which kernels
        to boot for this job and other options for them
    @param label Name of label to grab kernel config from.
    @param profilers List of profilers to activate during the job.
    @param client_control_file The contents of a client-side control file to
        run at the end of all tests.  If this is supplied, all tests must be
        client side.
        TODO: in the future we should support server control files directly
        to wrap with a kernel.  That'll require changing the parameter
        name and adding a boolean to indicate if it is a client or server
        control file.
    @param use_container unused argument today.  TODO: Enable containers
        on the host during a client side test.
    @param profile_only A boolean that indicates what default profile_only
        mode to use in the control file. Passing None will generate a
        control file that does not explcitly set the default mode at all.
    @param upload_kernel_config: if enabled it will generate server control
            file code that uploads the kernel config file to the client and
            tells the client of the new (local) path when compiling the kernel;
            the tests must be server side tests
    @param db_tests: if True, the test object can be found in the database
                     backing the test model. In this case, tests is a tuple
                     of test IDs which are used to retrieve the test objects
                     from the database. If False, tests is a tuple of test
                     dictionaries stored client-side in the AFE.

    @returns a dict with the following keys:
        control_file: str, The control file text.
        is_server: bool, is the control file a server-side control file?
        synch_count: How many machines the job uses per autoserv execution.
            synch_count == 1 means the job is asynchronous.
        dependencies: A list of the names of labels on which the job depends.
    """
    if not tests and not client_control_file:
        return dict(control_file='', is_server=False, synch_count=1,
                    dependencies=[])

    cf_info, test_objects, profiler_objects, label = (
        rpc_utils.prepare_generate_control_file(tests, kernel, label,
                                                profilers, db_tests))
    cf_info['control_file'] = control_file.generate_control(
        tests=test_objects, kernels=kernel, platform=label,
        profilers=profiler_objects, is_server=cf_info['is_server'],
        client_control_file=client_control_file, profile_only=profile_only,
        upload_kernel_config=upload_kernel_config)
    return cf_info


def create_parameterized_job(name, priority, test, parameters, kernel=None,
                             label=None, profilers=(), profiler_parameters=None,
                             use_container=False, profile_only=None,
                             upload_kernel_config=False, hosts=(),
                             meta_hosts=(), one_time_hosts=(),
                             atomic_group_name=None, synch_count=None,
                             is_template=False, timeout=None,
                             timeout_mins=None, max_runtime_mins=None,
                             run_verify=False, email_list='', dependencies=(),
                             reboot_before=None, reboot_after=None,
                             parse_failed_repair=None, hostless=False,
                             keyvals=None, drone_set=None, run_reset=True,
                             require_ssp=None):
    """
    Creates and enqueues a parameterized job.

    Most parameters a combination of the parameters for generate_control_file()
    and create_job(), with the exception of:

    @param test name or ID of the test to run
    @param parameters a map of parameter name ->
                          tuple of (param value, param type)
    @param profiler_parameters a dictionary of parameters for the profilers:
                                   key: profiler name
                                   value: dict of param name -> tuple of
                                                                (param value,
                                                                 param type)
    """
    # Save the values of the passed arguments here. What we're going to do with
    # them is pass them all to rpc_utils.get_create_job_common_args(), which
    # will extract the subset of these arguments that apply for
    # rpc_utils.create_job_common(), which we then pass in to that function.
    args = locals()

    # Set up the parameterized job configs
    test_obj = models.Test.smart_get(test)
    control_type = test_obj.test_type

    try:
        label = models.Label.smart_get(label)
    except models.Label.DoesNotExist:
        label = None

    kernel_objs = models.Kernel.create_kernels(kernel)
    profiler_objs = [models.Profiler.smart_get(profiler)
                     for profiler in profilers]

    parameterized_job = models.ParameterizedJob.objects.create(
            test=test_obj, label=label, use_container=use_container,
            profile_only=profile_only,
            upload_kernel_config=upload_kernel_config)
    parameterized_job.kernels.add(*kernel_objs)

    for profiler in profiler_objs:
        parameterized_profiler = models.ParameterizedJobProfiler.objects.create(
                parameterized_job=parameterized_job,
                profiler=profiler)
        profiler_params = profiler_parameters.get(profiler.name, {})
        for name, (value, param_type) in profiler_params.iteritems():
            models.ParameterizedJobProfilerParameter.objects.create(
                    parameterized_job_profiler=parameterized_profiler,
                    parameter_name=name,
                    parameter_value=value,
                    parameter_type=param_type)

    try:
        for parameter in test_obj.testparameter_set.all():
            if parameter.name in parameters:
                param_value, param_type = parameters.pop(parameter.name)
                parameterized_job.parameterizedjobparameter_set.create(
                        test_parameter=parameter, parameter_value=param_value,
                        parameter_type=param_type)

        if parameters:
            raise Exception('Extra parameters remain: %r' % parameters)

        return rpc_utils.create_job_common(
                parameterized_job=parameterized_job.id,
                control_type=control_type,
                **rpc_utils.get_create_job_common_args(args))
    except:
        parameterized_job.delete()
        raise


def create_job_page_handler(name, priority, control_file, control_type,
                            image=None, hostless=False, firmware_rw_build=None,
                            firmware_ro_build=None, test_source_build=None,
                            **kwargs):
    """\
    Create and enqueue a job.

    @param name name of this job
    @param priority Integer priority of this job.  Higher is more important.
    @param control_file String contents of the control file.
    @param control_type Type of control file, Client or Server.
    @param image: ChromeOS build to be installed in the dut. Default to None.
    @param firmware_rw_build: Firmware build to update RW firmware. Default to
                              None, i.e., RW firmware will not be updated.
    @param firmware_ro_build: Firmware build to update RO firmware. Default to
                              None, i.e., RO firmware will not be updated.
    @param test_source_build: Build to be used to retrieve test code. Default
                              to None.
    @param kwargs extra args that will be required by create_suite_job or
                  create_job.

    @returns The created Job id number.
    """
    control_file = rpc_utils.encode_ascii(control_file)
    if not control_file:
        raise model_logic.ValidationError({
                'control_file' : "Control file cannot be empty"})

    if image and hostless:
        builds = {}
        builds[provision.CROS_VERSION_PREFIX] = image
        if firmware_rw_build:
            builds[provision.FW_RW_VERSION_PREFIX] = firmware_rw_build
        if firmware_ro_build:
            builds[provision.FW_RO_VERSION_PREFIX] = firmware_ro_build
        return site_rpc_interface.create_suite_job(
                name=name, control_file=control_file, priority=priority,
                builds=builds, test_source_build=test_source_build, **kwargs)
    return create_job(name, priority, control_file, control_type, image=image,
                      hostless=hostless, **kwargs)


@rpc_utils.route_rpc_to_master
def create_job(name, priority, control_file, control_type,
               hosts=(), meta_hosts=(), one_time_hosts=(),
               atomic_group_name=None, synch_count=None, is_template=False,
               timeout=None, timeout_mins=None, max_runtime_mins=None,
               run_verify=False, email_list='', dependencies=(),
               reboot_before=None, reboot_after=None, parse_failed_repair=None,
               hostless=False, keyvals=None, drone_set=None, image=None,
               parent_job_id=None, test_retry=0, run_reset=True,
               require_ssp=None, args=(), **kwargs):
    """\
    Create and enqueue a job.

    @param name name of this job
    @param priority Integer priority of this job.  Higher is more important.
    @param control_file String contents of the control file.
    @param control_type Type of control file, Client or Server.
    @param synch_count How many machines the job uses per autoserv execution.
        synch_count == 1 means the job is asynchronous.  If an atomic group is
        given this value is treated as a minimum.
    @param is_template If true then create a template job.
    @param timeout Hours after this call returns until the job times out.
    @param timeout_mins Minutes after this call returns until the job times
        out.
    @param max_runtime_mins Minutes from job starting time until job times out
    @param run_verify Should the host be verified before running the test?
    @param email_list String containing emails to mail when the job is done
    @param dependencies List of label names on which this job depends
    @param reboot_before Never, If dirty, or Always
    @param reboot_after Never, If all tests passed, or Always
    @param parse_failed_repair if true, results of failed repairs launched by
        this job will be parsed as part of the job.
    @param hostless if true, create a hostless job
    @param keyvals dict of keyvals to associate with the job
    @param hosts List of hosts to run job on.
    @param meta_hosts List where each entry is a label name, and for each entry
        one host will be chosen from that label to run the job on.
    @param one_time_hosts List of hosts not in the database to run the job on.
    @param atomic_group_name The name of an atomic group to schedule the job on.
    @param drone_set The name of the drone set to run this test on.
    @param image OS image to install before running job.
    @param parent_job_id id of a job considered to be parent of created job.
    @param test_retry Number of times to retry test if the test did not
        complete successfully. (optional, default: 0)
    @param run_reset Should the host be reset before running the test?
    @param require_ssp Set to True to require server-side packaging to run the
                       test. If it's set to None, drone will still try to run
                       the server side with server-side packaging. If the
                       autotest-server package doesn't exist for the build or
                       image is not set, drone will run the test without server-
                       side packaging. Default is None.
    @param args A list of args to be injected into control file.
    @param kwargs extra keyword args. NOT USED.

    @returns The created Job id number.
    """
    if args:
        control_file = tools.inject_vars({'args': args}, control_file)

    if image is None:
        return rpc_utils.create_job_common(
                **rpc_utils.get_create_job_common_args(locals()))

    # Translate the image name, in case its a relative build name.
    ds = dev_server.ImageServer.resolve(image)
    image = ds.translate(image)

    # When image is supplied use a known parameterized test already in the
    # database to pass the OS image path from the front end, through the
    # scheduler, and finally to autoserv as the --image parameter.

    # The test autoupdate_ParameterizedJob is in afe_autotests and used to
    # instantiate a Test object and from there a ParameterizedJob.
    known_test_obj = models.Test.smart_get('autoupdate_ParameterizedJob')
    known_parameterized_job = models.ParameterizedJob.objects.create(
            test=known_test_obj)

    # autoupdate_ParameterizedJob has a single parameter, the image parameter,
    # stored in the table afe_test_parameters.  We retrieve and set this
    # instance of the parameter to the OS image path.
    image_parameter = known_test_obj.testparameter_set.get(test=known_test_obj,
                                                           name='image')
    known_parameterized_job.parameterizedjobparameter_set.create(
            test_parameter=image_parameter, parameter_value=image,
            parameter_type='string')

    # TODO(crbug.com/502638): save firmware build etc to parameterized_job.

    # By passing a parameterized_job to create_job_common the job entry in
    # the afe_jobs table will have the field parameterized_job_id set.
    # The scheduler uses this id in the afe_parameterized_jobs table to
    # match this job to our known test, and then with the
    # afe_parameterized_job_parameters table to get the actual image path.
    return rpc_utils.create_job_common(
            parameterized_job=known_parameterized_job.id,
            **rpc_utils.get_create_job_common_args(locals()))


def abort_host_queue_entries(**filter_data):
    """\
    Abort a set of host queue entries.

    @return: A list of dictionaries, each contains information
             about an aborted HQE.
    """
    query = models.HostQueueEntry.query_objects(filter_data)

    # Dont allow aborts on:
    #   1. Jobs that have already completed (whether or not they were aborted)
    #   2. Jobs that we have already been aborted (but may not have completed)
    query = query.filter(complete=False).filter(aborted=False)
    models.AclGroup.check_abort_permissions(query)
    host_queue_entries = list(query.select_related())
    rpc_utils.check_abort_synchronous_jobs(host_queue_entries)

    models.HostQueueEntry.abort_host_queue_entries(host_queue_entries)
    hqe_info = [{'HostQueueEntry': hqe.id, 'Job': hqe.job_id,
                 'Job name': hqe.job.name} for hqe in host_queue_entries]
    return hqe_info


def abort_special_tasks(**filter_data):
    """\
    Abort the special task, or tasks, specified in the filter.
    """
    query = models.SpecialTask.query_objects(filter_data)
    special_tasks = query.filter(is_active=True)
    for task in special_tasks:
        task.abort()


def _call_special_tasks_on_hosts(task, hosts):
    """\
    Schedules a set of hosts for a special task.

    @returns A list of hostnames that a special task was created for.
    """
    models.AclGroup.check_for_acl_violation_hosts(hosts)
    shard_host_map = rpc_utils.bucket_hosts_by_shard(hosts)
    if shard_host_map and not utils.is_shard():
        raise ValueError('The following hosts are on shards, please '
                         'follow the link to the shards and create jobs '
                         'there instead. %s.' % shard_host_map)
    for host in hosts:
        models.SpecialTask.schedule_special_task(host, task)
    return list(sorted(host.hostname for host in hosts))


def _forward_special_tasks_on_hosts(task, rpc, **filter_data):
    """Forward special tasks to corresponding shards.

    For master, when special tasks are fired on hosts that are sharded,
    forward the RPC to corresponding shards.

    For shard, create special task records in local DB.

    @param task: Enum value of frontend.afe.models.SpecialTask.Task
    @param rpc: RPC name to forward.
    @param filter_data: Filter keywords to be used for DB query.

    @return: A list of hostnames that a special task was created for.
    """
    hosts = models.Host.query_objects(filter_data)
    shard_host_map = rpc_utils.bucket_hosts_by_shard(hosts, rpc_hostnames=True)

    # Filter out hosts on a shard from those on the master, forward
    # rpcs to the shard with an additional hostname__in filter, and
    # create a local SpecialTask for each remaining host.
    if shard_host_map and not utils.is_shard():
        hosts = [h for h in hosts if h.shard is None]
        for shard, hostnames in shard_host_map.iteritems():

            # The main client of this module is the frontend website, and
            # it invokes it with an 'id' or an 'id__in' filter. Regardless,
            # the 'hostname' filter should narrow down the list of hosts on
            # each shard even though we supply all the ids in filter_data.
            # This method uses hostname instead of id because it fits better
            # with the overall architecture of redirection functions in
            # rpc_utils.
            shard_filter = filter_data.copy()
            shard_filter['hostname__in'] = hostnames
            rpc_utils.run_rpc_on_multiple_hostnames(
                    rpc, [shard], **shard_filter)

    # There is a race condition here if someone assigns a shard to one of these
    # hosts before we create the task. The host will stay on the master if:
    # 1. The host is not Ready
    # 2. The host is Ready but has a task
    # But if the host is Ready and doesn't have a task yet, it will get sent
    # to the shard as we're creating a task here.

    # Given that we only rarely verify Ready hosts it isn't worth putting this
    # entire method in a transaction. The worst case scenario is that we have
    # a verify running on a Ready host while the shard is using it, if the
    # verify fails no subsequent tasks will be created against the host on the
    # master, and verifies are safe enough that this is OK.
    return _call_special_tasks_on_hosts(task, hosts)


def reverify_hosts(**filter_data):
    """\
    Schedules a set of hosts for verify.

    @returns A list of hostnames that a verify task was created for.
    """
    return _forward_special_tasks_on_hosts(
            models.SpecialTask.Task.VERIFY, 'reverify_hosts', **filter_data)


def repair_hosts(**filter_data):
    """\
    Schedules a set of hosts for repair.

    @returns A list of hostnames that a repair task was created for.
    """
    return _forward_special_tasks_on_hosts(
            models.SpecialTask.Task.REPAIR, 'repair_hosts', **filter_data)


def get_jobs(not_yet_run=False, running=False, finished=False,
             suite=False, sub=False, standalone=False, **filter_data):
    """\
    Extra status filter args for get_jobs:
    -not_yet_run: Include only jobs that have not yet started running.
    -running: Include only jobs that have start running but for which not
    all hosts have completed.
    -finished: Include only jobs for which all hosts have completed (or
    aborted).

    Extra type filter args for get_jobs:
    -suite: Include only jobs with child jobs.
    -sub: Include only jobs with a parent job.
    -standalone: Inlcude only jobs with no child or parent jobs.
    At most one of these three fields should be specified.
    """
    extra_args = rpc_utils.extra_job_status_filters(not_yet_run,
                                                    running,
                                                    finished)
    filter_data['extra_args'] = rpc_utils.extra_job_type_filters(extra_args,
                                                                 suite,
                                                                 sub,
                                                                 standalone)
    job_dicts = []
    jobs = list(models.Job.query_objects(filter_data))
    models.Job.objects.populate_relationships(jobs, models.Label,
                                              'dependencies')
    models.Job.objects.populate_relationships(jobs, models.JobKeyval, 'keyvals')
    for job in jobs:
        job_dict = job.get_object_dict()
        job_dict['dependencies'] = ','.join(label.name
                                            for label in job.dependencies)
        job_dict['keyvals'] = dict((keyval.key, keyval.value)
                                   for keyval in job.keyvals)
        if job.parameterized_job:
            job_dict['image'] = get_parameterized_autoupdate_image_url(job)
        job_dicts.append(job_dict)
    return rpc_utils.prepare_for_serialization(job_dicts)


def get_num_jobs(not_yet_run=False, running=False, finished=False,
                 suite=False, sub=False, standalone=False,
                 **filter_data):
    """\
    See get_jobs() for documentation of extra filter parameters.
    """
    extra_args = rpc_utils.extra_job_status_filters(not_yet_run,
                                                    running,
                                                    finished)
    filter_data['extra_args'] = rpc_utils.extra_job_type_filters(extra_args,
                                                                 suite,
                                                                 sub,
                                                                 standalone)
    return models.Job.query_count(filter_data)


def get_jobs_summary(**filter_data):
    """\
    Like get_jobs(), but adds 'status_counts' and 'result_counts' field.

    'status_counts' filed is a dictionary mapping status strings to the number
    of hosts currently with that status, i.e. {'Queued' : 4, 'Running' : 2}.

    'result_counts' field is piped to tko's rpc_interface and has the return
    format specified under get_group_counts.
    """
    jobs = get_jobs(**filter_data)
    ids = [job['id'] for job in jobs]
    all_status_counts = models.Job.objects.get_status_counts(ids)
    for job in jobs:
        job['status_counts'] = all_status_counts[job['id']]
        job['result_counts'] = tko_rpc_interface.get_status_counts(
                ['afe_job_id', 'afe_job_id'],
                header_groups=[['afe_job_id'], ['afe_job_id']],
                **{'afe_job_id': job['id']})
    return rpc_utils.prepare_for_serialization(jobs)


def get_info_for_clone(id, preserve_metahosts, queue_entry_filter_data=None):
    """\
    Retrieves all the information needed to clone a job.
    """
    job = models.Job.objects.get(id=id)
    job_info = rpc_utils.get_job_info(job,
                                      preserve_metahosts,
                                      queue_entry_filter_data)

    host_dicts = []
    for host in job_info['hosts']:
        host_dict = get_hosts(id=host.id)[0]
        other_labels = host_dict['labels']
        if host_dict['platform']:
            other_labels.remove(host_dict['platform'])
        host_dict['other_labels'] = ', '.join(other_labels)
        host_dicts.append(host_dict)

    for host in job_info['one_time_hosts']:
        host_dict = dict(hostname=host.hostname,
                         id=host.id,
                         platform='(one-time host)',
                         locked_text='')
        host_dicts.append(host_dict)

    # convert keys from Label objects to strings (names of labels)
    meta_host_counts = dict((meta_host.name, count) for meta_host, count
                            in job_info['meta_host_counts'].iteritems())

    info = dict(job=job.get_object_dict(),
                meta_host_counts=meta_host_counts,
                hosts=host_dicts)
    info['job']['dependencies'] = job_info['dependencies']
    if job_info['atomic_group']:
        info['atomic_group_name'] = (job_info['atomic_group']).name
    else:
        info['atomic_group_name'] = None
    info['hostless'] = job_info['hostless']
    info['drone_set'] = job.drone_set and job.drone_set.name

    if job.parameterized_job:
        info['job']['image'] = get_parameterized_autoupdate_image_url(job)

    return rpc_utils.prepare_for_serialization(info)


# host queue entries

def get_host_queue_entries(start_time=None, end_time=None, **filter_data):
    """\
    @returns A sequence of nested dictionaries of host and job information.
    """
    filter_data = rpc_utils.inject_times_to_filter('started_on__gte',
                                                   'started_on__lte',
                                                   start_time,
                                                   end_time,
                                                   **filter_data)
    return rpc_utils.prepare_rows_as_nested_dicts(
            models.HostQueueEntry.query_objects(filter_data),
            ('host', 'atomic_group', 'job'))


def get_num_host_queue_entries(start_time=None, end_time=None, **filter_data):
    """\
    Get the number of host queue entries associated with this job.
    """
    filter_data = rpc_utils.inject_times_to_filter('started_on__gte',
                                                   'started_on__lte',
                                                   start_time,
                                                   end_time,
                                                   **filter_data)
    return models.HostQueueEntry.query_count(filter_data)


def get_hqe_percentage_complete(**filter_data):
    """
    Computes the fraction of host queue entries matching the given filter data
    that are complete.
    """
    query = models.HostQueueEntry.query_objects(filter_data)
    complete_count = query.filter(complete=True).count()
    total_count = query.count()
    if total_count == 0:
        return 1
    return float(complete_count) / total_count


# special tasks

def get_special_tasks(**filter_data):
    """Get special task entries from the local database.

    Query the special tasks table for tasks matching the given
    `filter_data`, and return a list of the results.  No attempt is
    made to forward the call to shards; the buck will stop here.
    The caller is expected to know the target shard for such reasons
    as:
      * The caller is a service (such as gs_offloader) configured
        to operate on behalf of one specific shard, and no other.
      * The caller has a host as a parameter, and knows that this is
        the shard assigned to that host.

    @param filter_data  Filter keywords to pass to the underlying
                        database query.

    """
    return rpc_utils.prepare_rows_as_nested_dicts(
            models.SpecialTask.query_objects(filter_data),
            ('host', 'queue_entry'))


def get_host_special_tasks(host_id, **filter_data):
    """Get special task entries for a given host.

    Query the special tasks table for tasks that ran on the host
    given by `host_id` and matching the given `filter_data`.
    Return a list of the results.  If the host is assigned to a
    shard, forward this call to that shard.

    @param host_id      Id in the database of the target host.
    @param filter_data  Filter keywords to pass to the underlying
                        database query.

    """
    # Retrieve host data even if the host is in an invalid state.
    host = models.Host.smart_get(host_id, False)
    if not host.shard:
        return get_special_tasks(host_id=host_id, **filter_data)
    else:
        # The return values from AFE methods are post-processed
        # objects that aren't JSON-serializable.  So, we have to
        # call AFE.run() to get the raw, serializable output from
        # the shard.
        shard_afe = frontend.AFE(server=host.shard.rpc_hostname())
        return shard_afe.run('get_special_tasks',
                             host_id=host_id, **filter_data)


def get_num_special_tasks(**kwargs):
    """Get the number of special task entries from the local database.

    Query the special tasks table for tasks matching the given 'kwargs',
    and return the number of the results. No attempt is made to forward
    the call to shards; the buck will stop here.

    @param kwargs    Filter keywords to pass to the underlying database query.

    """
    return models.SpecialTask.query_count(kwargs)


def get_host_num_special_tasks(host, **kwargs):
    """Get special task entries for a given host.

    Query the special tasks table for tasks that ran on the host
    given by 'host' and matching the given 'kwargs'.
    Return a list of the results.  If the host is assigned to a
    shard, forward this call to that shard.

    @param host      id or name of a host. More often a hostname.
    @param kwargs    Filter keywords to pass to the underlying database query.

    """
    # Retrieve host data even if the host is in an invalid state.
    host_model = models.Host.smart_get(host, False)
    if not host_model.shard:
        return get_num_special_tasks(host=host, **kwargs)
    else:
        shard_afe = frontend.AFE(server=host_model.shard.rpc_hostname())
        return shard_afe.run('get_num_special_tasks', host=host, **kwargs)


def get_status_task(host_id, end_time):
    """Get the "status task" for a host from the local shard.

    Returns a single special task representing the given host's
    "status task".  The status task is a completed special task that
    identifies whether the corresponding host was working or broken
    when it completed.  A successful task indicates a working host;
    a failed task indicates broken.

    This call will not be forward to a shard; the receiving server
    must be the shard that owns the host.

    @param host_id      Id in the database of the target host.
    @param end_time     Time reference for the host's status.

    @return A single task; its status (successful or not)
            corresponds to the status of the host (working or
            broken) at the given time.  If no task is found, return
            `None`.

    """
    tasklist = rpc_utils.prepare_rows_as_nested_dicts(
            status_history.get_status_task(host_id, end_time),
            ('host', 'queue_entry'))
    return tasklist[0] if tasklist else None


def get_host_status_task(host_id, end_time):
    """Get the "status task" for a host from its owning shard.

    Finds the given host's owning shard, and forwards to it a call
    to `get_status_task()` (see above).

    @param host_id      Id in the database of the target host.
    @param end_time     Time reference for the host's status.

    @return A single task; its status (successful or not)
            corresponds to the status of the host (working or
            broken) at the given time.  If no task is found, return
            `None`.

    """
    host = models.Host.smart_get(host_id)
    if not host.shard:
        return get_status_task(host_id, end_time)
    else:
        # The return values from AFE methods are post-processed
        # objects that aren't JSON-serializable.  So, we have to
        # call AFE.run() to get the raw, serializable output from
        # the shard.
        shard_afe = frontend.AFE(server=host.shard.rpc_hostname())
        return shard_afe.run('get_status_task',
                             host_id=host_id, end_time=end_time)


def get_host_diagnosis_interval(host_id, end_time, success):
    """Find a "diagnosis interval" for a given host.

    A "diagnosis interval" identifies a start and end time where
    the host went from "working" to "broken", or vice versa.  The
    interval's starting time is the starting time of the last status
    task with the old status; the end time is the finish time of the
    first status task with the new status.

    This routine finds the most recent diagnosis interval for the
    given host prior to `end_time`, with a starting status matching
    `success`.  If `success` is true, the interval will start with a
    successful status task; if false the interval will start with a
    failed status task.

    @param host_id      Id in the database of the target host.
    @param end_time     Time reference for the diagnosis interval.
    @param success      Whether the diagnosis interval should start
                        with a successful or failed status task.

    @return A list of two strings.  The first is the timestamp for
            the beginning of the interval; the second is the
            timestamp for the end.  If the host has never changed
            state, the list is empty.

    """
    host = models.Host.smart_get(host_id)
    if not host.shard or utils.is_shard():
        return status_history.get_diagnosis_interval(
                host_id, end_time, success)
    else:
        shard_afe = frontend.AFE(server=host.shard.rpc_hostname())
        return shard_afe.get_host_diagnosis_interval(
                host_id, end_time, success)


# support for host detail view

def get_host_queue_entries_and_special_tasks(host, query_start=None,
                                             query_limit=None, start_time=None,
                                             end_time=None):
    """
    @returns an interleaved list of HostQueueEntries and SpecialTasks,
            in approximate run order.  each dict contains keys for type, host,
            job, status, started_on, execution_path, and ID.
    """
    total_limit = None
    if query_limit is not None:
        total_limit = query_start + query_limit
    filter_data_common = {'host': host,
                          'query_limit': total_limit,
                          'sort_by': ['-id']}

    filter_data_special_tasks = rpc_utils.inject_times_to_filter(
            'time_started__gte', 'time_started__lte', start_time, end_time,
            **filter_data_common)

    queue_entries = get_host_queue_entries(
            start_time, end_time, **filter_data_common)
    special_tasks = get_host_special_tasks(host, **filter_data_special_tasks)

    interleaved_entries = rpc_utils.interleave_entries(queue_entries,
                                                       special_tasks)
    if query_start is not None:
        interleaved_entries = interleaved_entries[query_start:]
    if query_limit is not None:
        interleaved_entries = interleaved_entries[:query_limit]
    return rpc_utils.prepare_host_queue_entries_and_special_tasks(
            interleaved_entries, queue_entries)


def get_num_host_queue_entries_and_special_tasks(host, start_time=None,
                                                 end_time=None):
    filter_data_common = {'host': host}

    filter_data_queue_entries, filter_data_special_tasks = (
            rpc_utils.inject_times_to_hqe_special_tasks_filters(
                    filter_data_common, start_time, end_time))

    return (models.HostQueueEntry.query_count(filter_data_queue_entries)
            + get_host_num_special_tasks(**filter_data_special_tasks))


# recurring run

def get_recurring(**filter_data):
    return rpc_utils.prepare_rows_as_nested_dicts(
            models.RecurringRun.query_objects(filter_data),
            ('job', 'owner'))


def get_num_recurring(**filter_data):
    return models.RecurringRun.query_count(filter_data)


def delete_recurring_runs(**filter_data):
    to_delete = models.RecurringRun.query_objects(filter_data)
    to_delete.delete()


def create_recurring_run(job_id, start_date, loop_period, loop_count):
    owner = models.User.current_user().login
    job = models.Job.objects.get(id=job_id)
    return job.create_recurring_job(start_date=start_date,
                                    loop_period=loop_period,
                                    loop_count=loop_count,
                                    owner=owner)


# other

def echo(data=""):
    """\
    Returns a passed in string. For doing a basic test to see if RPC calls
    can successfully be made.
    """
    return data


def get_motd():
    """\
    Returns the message of the day as a string.
    """
    return rpc_utils.get_motd()


def get_static_data():
    """\
    Returns a dictionary containing a bunch of data that shouldn't change
    often and is otherwise inaccessible.  This includes:

    priorities: List of job priority choices.
    default_priority: Default priority value for new jobs.
    users: Sorted list of all users.
    labels: Sorted list of labels not start with 'cros-version' and
            'fw-version'.
    atomic_groups: Sorted list of all atomic groups.
    tests: Sorted list of all tests.
    profilers: Sorted list of all profilers.
    current_user: Logged-in username.
    host_statuses: Sorted list of possible Host statuses.
    job_statuses: Sorted list of possible HostQueueEntry statuses.
    job_timeout_default: The default job timeout length in minutes.
    parse_failed_repair_default: Default value for the parse_failed_repair job
            option.
    reboot_before_options: A list of valid RebootBefore string enums.
    reboot_after_options: A list of valid RebootAfter string enums.
    motd: Server's message of the day.
    status_dictionary: A mapping from one word job status names to a more
            informative description.
    """

    job_fields = models.Job.get_field_dict()
    default_drone_set_name = models.DroneSet.default_drone_set_name()
    drone_sets = ([default_drone_set_name] +
                  sorted(drone_set.name for drone_set in
                         models.DroneSet.objects.exclude(
                                 name=default_drone_set_name)))

    result = {}
    result['priorities'] = priorities.Priority.choices()
    default_priority = priorities.Priority.DEFAULT
    result['default_priority'] = 'Default'
    result['max_schedulable_priority'] = priorities.Priority.DEFAULT
    result['users'] = get_users(sort_by=['login'])

    label_exclude_filters = [{'name__startswith': 'cros-version'},
                             {'name__startswith': 'fw-version'},
                             {'name__startswith': 'fwrw-version'},
                             {'name__startswith': 'fwro-version'}]
    result['labels'] = get_labels(
        label_exclude_filters,
        sort_by=['-platform', 'name'])

    result['atomic_groups'] = get_atomic_groups(sort_by=['name'])
    result['tests'] = get_tests(sort_by=['name'])
    result['profilers'] = get_profilers(sort_by=['name'])
    result['current_user'] = rpc_utils.prepare_for_serialization(
        models.User.current_user().get_object_dict())
    result['host_statuses'] = sorted(models.Host.Status.names)
    result['job_statuses'] = sorted(models.HostQueueEntry.Status.names)
    result['job_timeout_mins_default'] = models.Job.DEFAULT_TIMEOUT_MINS
    result['job_max_runtime_mins_default'] = (
        models.Job.DEFAULT_MAX_RUNTIME_MINS)
    result['parse_failed_repair_default'] = bool(
        models.Job.DEFAULT_PARSE_FAILED_REPAIR)
    result['reboot_before_options'] = model_attributes.RebootBefore.names
    result['reboot_after_options'] = model_attributes.RebootAfter.names
    result['motd'] = rpc_utils.get_motd()
    result['drone_sets_enabled'] = models.DroneSet.drone_sets_enabled()
    result['drone_sets'] = drone_sets
    result['parameterized_jobs'] = models.Job.parameterized_jobs_enabled()

    result['status_dictionary'] = {"Aborted": "Aborted",
                                   "Verifying": "Verifying Host",
                                   "Provisioning": "Provisioning Host",
                                   "Pending": "Waiting on other hosts",
                                   "Running": "Running autoserv",
                                   "Completed": "Autoserv completed",
                                   "Failed": "Failed to complete",
                                   "Queued": "Queued",
                                   "Starting": "Next in host's queue",
                                   "Stopped": "Other host(s) failed verify",
                                   "Parsing": "Awaiting parse of final results",
                                   "Gathering": "Gathering log files",
                                   "Template": "Template job for recurring run",
                                   "Waiting": "Waiting for scheduler action",
                                   "Archiving": "Archiving results",
                                   "Resetting": "Resetting hosts"}

    result['wmatrix_url'] = rpc_utils.get_wmatrix_url()
    result['is_moblab'] = bool(utils.is_moblab())

    return result


def get_server_time():
    return datetime.datetime.now().strftime("%Y-%m-%d %H:%M")
