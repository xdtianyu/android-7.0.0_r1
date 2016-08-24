# pylint: disable-msg=C0111

import logging
from datetime import datetime
import django.core
try:
    from django.db import models as dbmodels, connection
except django.core.exceptions.ImproperlyConfigured:
    raise ImportError('Django database not yet configured. Import either '
                       'setup_django_environment or '
                       'setup_django_lite_environment from '
                       'autotest_lib.frontend before any imports that '
                       'depend on django models.')
from xml.sax import saxutils
import common
from autotest_lib.frontend.afe import model_logic, model_attributes
from autotest_lib.frontend.afe import rdb_model_extensions
from autotest_lib.frontend import settings, thread_local
from autotest_lib.client.common_lib import enum, error, host_protections
from autotest_lib.client.common_lib import global_config
from autotest_lib.client.common_lib import host_queue_entry_states
from autotest_lib.client.common_lib import control_data, priorities, decorators
from autotest_lib.client.common_lib import site_utils
from autotest_lib.client.common_lib.cros.graphite import autotest_es
from autotest_lib.server import utils as server_utils

# job options and user preferences
DEFAULT_REBOOT_BEFORE = model_attributes.RebootBefore.IF_DIRTY
DEFAULT_REBOOT_AFTER = model_attributes.RebootBefore.NEVER


class AclAccessViolation(Exception):
    """\
    Raised when an operation is attempted with proper permissions as
    dictated by ACLs.
    """


class AtomicGroup(model_logic.ModelWithInvalid, dbmodels.Model):
    """\
    An atomic group defines a collection of hosts which must only be scheduled
    all at once.  Any host with a label having an atomic group will only be
    scheduled for a job at the same time as other hosts sharing that label.

    Required:
      name: A name for this atomic group, e.g. 'rack23' or 'funky_net'.
      max_number_of_machines: The maximum number of machines that will be
              scheduled at once when scheduling jobs to this atomic group.
              The job.synch_count is considered the minimum.

    Optional:
      description: Arbitrary text description of this group's purpose.
    """
    name = dbmodels.CharField(max_length=255, unique=True)
    description = dbmodels.TextField(blank=True)
    # This magic value is the default to simplify the scheduler logic.
    # It must be "large".  The common use of atomic groups is to want all
    # machines in the group to be used, limits on which subset used are
    # often chosen via dependency labels.
    # TODO(dennisjeffrey): Revisit this so we don't have to assume that
    # "infinity" is around 3.3 million.
    INFINITE_MACHINES = 333333333
    max_number_of_machines = dbmodels.IntegerField(default=INFINITE_MACHINES)
    invalid = dbmodels.BooleanField(default=False,
                                  editable=settings.FULL_ADMIN)

    name_field = 'name'
    objects = model_logic.ModelWithInvalidManager()
    valid_objects = model_logic.ValidObjectsManager()


    def enqueue_job(self, job, is_template=False):
        """Enqueue a job on an associated atomic group of hosts.

        @param job: A job to enqueue.
        @param is_template: Whether the status should be "Template".
        """
        queue_entry = HostQueueEntry.create(atomic_group=self, job=job,
                                            is_template=is_template)
        queue_entry.save()


    def clean_object(self):
        self.label_set.clear()


    class Meta:
        """Metadata for class AtomicGroup."""
        db_table = 'afe_atomic_groups'


    def __unicode__(self):
        return unicode(self.name)


class Label(model_logic.ModelWithInvalid, dbmodels.Model):
    """\
    Required:
      name: label name

    Optional:
      kernel_config: URL/path to kernel config for jobs run on this label.
      platform: If True, this is a platform label (defaults to False).
      only_if_needed: If True, a Host with this label can only be used if that
              label is requested by the job/test (either as the meta_host or
              in the job_dependencies).
      atomic_group: The atomic group associated with this label.
    """
    name = dbmodels.CharField(max_length=255, unique=True)
    kernel_config = dbmodels.CharField(max_length=255, blank=True)
    platform = dbmodels.BooleanField(default=False)
    invalid = dbmodels.BooleanField(default=False,
                                    editable=settings.FULL_ADMIN)
    only_if_needed = dbmodels.BooleanField(default=False)

    name_field = 'name'
    objects = model_logic.ModelWithInvalidManager()
    valid_objects = model_logic.ValidObjectsManager()
    atomic_group = dbmodels.ForeignKey(AtomicGroup, null=True, blank=True)


    def clean_object(self):
        self.host_set.clear()
        self.test_set.clear()


    def enqueue_job(self, job, atomic_group=None, is_template=False):
        """Enqueue a job on any host of this label.

        @param job: A job to enqueue.
        @param atomic_group: The associated atomic group.
        @param is_template: Whether the status should be "Template".
        """
        queue_entry = HostQueueEntry.create(meta_host=self, job=job,
                                            is_template=is_template,
                                            atomic_group=atomic_group)
        queue_entry.save()



    class Meta:
        """Metadata for class Label."""
        db_table = 'afe_labels'


    def __unicode__(self):
        return unicode(self.name)


class Shard(dbmodels.Model, model_logic.ModelExtensions):

    hostname = dbmodels.CharField(max_length=255, unique=True)

    name_field = 'hostname'

    labels = dbmodels.ManyToManyField(Label, blank=True,
                                      db_table='afe_shards_labels')

    class Meta:
        """Metadata for class ParameterizedJob."""
        db_table = 'afe_shards'


    def rpc_hostname(self):
        """Get the rpc hostname of the shard.

        @return: Just the shard hostname for all non-testing environments.
                 The address of the default gateway for vm testing environments.
        """
        # TODO: Figure out a better solution for testing. Since no 2 shards
        # can run on the same host, if the shard hostname is localhost we
        # conclude that it must be a vm in a test cluster. In such situations
        # a name of localhost:<port> is necessary to achieve the correct
        # afe links/redirection from the frontend (this happens through the
        # host), but for rpcs that are performed *on* the shard, they need to
        # use the address of the gateway.
        # In the virtual machine testing environment (i.e., puppylab), each
        # shard VM has a hostname like localhost:<port>. In the real cluster
        # environment, a shard node does not have 'localhost' for its hostname.
        # The following hostname substitution is needed only for the VM
        # in puppylab.
        # The 'hostname' should not be replaced in the case of real cluster.
        if site_utils.is_puppylab_vm(self.hostname):
            hostname = self.hostname.split(':')[0]
            return self.hostname.replace(
                    hostname, site_utils.DEFAULT_VM_GATEWAY)
        return self.hostname


class Drone(dbmodels.Model, model_logic.ModelExtensions):
    """
    A scheduler drone

    hostname: the drone's hostname
    """
    hostname = dbmodels.CharField(max_length=255, unique=True)

    name_field = 'hostname'
    objects = model_logic.ExtendedManager()


    def save(self, *args, **kwargs):
        if not User.current_user().is_superuser():
            raise Exception('Only superusers may edit drones')
        super(Drone, self).save(*args, **kwargs)


    def delete(self):
        if not User.current_user().is_superuser():
            raise Exception('Only superusers may delete drones')
        super(Drone, self).delete()


    class Meta:
        """Metadata for class Drone."""
        db_table = 'afe_drones'

    def __unicode__(self):
        return unicode(self.hostname)


class DroneSet(dbmodels.Model, model_logic.ModelExtensions):
    """
    A set of scheduler drones

    These will be used by the scheduler to decide what drones a job is allowed
    to run on.

    name: the drone set's name
    drones: the drones that are part of the set
    """
    DRONE_SETS_ENABLED = global_config.global_config.get_config_value(
            'SCHEDULER', 'drone_sets_enabled', type=bool, default=False)
    DEFAULT_DRONE_SET_NAME = global_config.global_config.get_config_value(
            'SCHEDULER', 'default_drone_set_name', default=None)

    name = dbmodels.CharField(max_length=255, unique=True)
    drones = dbmodels.ManyToManyField(Drone, db_table='afe_drone_sets_drones')

    name_field = 'name'
    objects = model_logic.ExtendedManager()


    def save(self, *args, **kwargs):
        if not User.current_user().is_superuser():
            raise Exception('Only superusers may edit drone sets')
        super(DroneSet, self).save(*args, **kwargs)


    def delete(self):
        if not User.current_user().is_superuser():
            raise Exception('Only superusers may delete drone sets')
        super(DroneSet, self).delete()


    @classmethod
    def drone_sets_enabled(cls):
        """Returns whether drone sets are enabled.

        @param cls: Implicit class object.
        """
        return cls.DRONE_SETS_ENABLED


    @classmethod
    def default_drone_set_name(cls):
        """Returns the default drone set name.

        @param cls: Implicit class object.
        """
        return cls.DEFAULT_DRONE_SET_NAME


    @classmethod
    def get_default(cls):
        """Gets the default drone set name, compatible with Job.add_object.

        @param cls: Implicit class object.
        """
        return cls.smart_get(cls.DEFAULT_DRONE_SET_NAME)


    @classmethod
    def resolve_name(cls, drone_set_name):
        """
        Returns the name of one of these, if not None, in order of preference:
        1) the drone set given,
        2) the current user's default drone set, or
        3) the global default drone set

        or returns None if drone sets are disabled

        @param cls: Implicit class object.
        @param drone_set_name: A drone set name.
        """
        if not cls.drone_sets_enabled():
            return None

        user = User.current_user()
        user_drone_set_name = user.drone_set and user.drone_set.name

        return drone_set_name or user_drone_set_name or cls.get_default().name


    def get_drone_hostnames(self):
        """
        Gets the hostnames of all drones in this drone set
        """
        return set(self.drones.all().values_list('hostname', flat=True))


    class Meta:
        """Metadata for class DroneSet."""
        db_table = 'afe_drone_sets'

    def __unicode__(self):
        return unicode(self.name)


class User(dbmodels.Model, model_logic.ModelExtensions):
    """\
    Required:
    login :user login name

    Optional:
    access_level: 0=User (default), 1=Admin, 100=Root
    """
    ACCESS_ROOT = 100
    ACCESS_ADMIN = 1
    ACCESS_USER = 0

    AUTOTEST_SYSTEM = 'autotest_system'

    login = dbmodels.CharField(max_length=255, unique=True)
    access_level = dbmodels.IntegerField(default=ACCESS_USER, blank=True)

    # user preferences
    reboot_before = dbmodels.SmallIntegerField(
        choices=model_attributes.RebootBefore.choices(), blank=True,
        default=DEFAULT_REBOOT_BEFORE)
    reboot_after = dbmodels.SmallIntegerField(
        choices=model_attributes.RebootAfter.choices(), blank=True,
        default=DEFAULT_REBOOT_AFTER)
    drone_set = dbmodels.ForeignKey(DroneSet, null=True, blank=True)
    show_experimental = dbmodels.BooleanField(default=False)

    name_field = 'login'
    objects = model_logic.ExtendedManager()


    def save(self, *args, **kwargs):
        # is this a new object being saved for the first time?
        first_time = (self.id is None)
        user = thread_local.get_user()
        if user and not user.is_superuser() and user.login != self.login:
            raise AclAccessViolation("You cannot modify user " + self.login)
        super(User, self).save(*args, **kwargs)
        if first_time:
            everyone = AclGroup.objects.get(name='Everyone')
            everyone.users.add(self)


    def is_superuser(self):
        """Returns whether the user has superuser access."""
        return self.access_level >= self.ACCESS_ROOT


    @classmethod
    def current_user(cls):
        """Returns the current user.

        @param cls: Implicit class object.
        """
        user = thread_local.get_user()
        if user is None:
            user, _ = cls.objects.get_or_create(login=cls.AUTOTEST_SYSTEM)
            user.access_level = cls.ACCESS_ROOT
            user.save()
        return user


    @classmethod
    def get_record(cls, data):
        """Check the database for an identical record.

        Check for a record with matching id and login. If one exists,
        return it. If one does not exist there is a possibility that
        the following cases have happened:
        1. Same id, different login
            We received: "1 chromeos-test"
            And we have: "1 debug-user"
        In this case we need to delete "1 debug_user" and insert
        "1 chromeos-test".

        2. Same login, different id:
            We received: "1 chromeos-test"
            And we have: "2 chromeos-test"
        In this case we need to delete "2 chromeos-test" and insert
        "1 chromeos-test".

        As long as this method deletes bad records and raises the
        DoesNotExist exception the caller will handle creating the
        new record.

        @raises: DoesNotExist, if a record with the matching login and id
                does not exist.
        """

        # Both the id and login should be uniqe but there are cases when
        # we might already have a user with the same login/id because
        # current_user will proactively create a user record if it doesn't
        # exist. Since we want to avoid conflict between the master and
        # shard, just delete any existing user records that don't match
        # what we're about to deserialize from the master.
        try:
            return cls.objects.get(login=data['login'], id=data['id'])
        except cls.DoesNotExist:
            cls.delete_matching_record(login=data['login'])
            cls.delete_matching_record(id=data['id'])
            raise


    class Meta:
        """Metadata for class User."""
        db_table = 'afe_users'

    def __unicode__(self):
        return unicode(self.login)


class Host(model_logic.ModelWithInvalid, rdb_model_extensions.AbstractHostModel,
           model_logic.ModelWithAttributes):
    """\
    Required:
    hostname

    optional:
    locked: if true, host is locked and will not be queued

    Internal:
    From AbstractHostModel:
        synch_id: currently unused
        status: string describing status of host
        invalid: true if the host has been deleted
        protection: indicates what can be done to this host during repair
        lock_time: DateTime at which the host was locked
        dirty: true if the host has been used without being rebooted
    Local:
        locked_by: user that locked the host, or null if the host is unlocked
    """

    SERIALIZATION_LINKS_TO_FOLLOW = set(['aclgroup_set',
                                         'hostattribute_set',
                                         'labels',
                                         'shard'])
    SERIALIZATION_LOCAL_LINKS_TO_UPDATE = set(['invalid'])


    def custom_deserialize_relation(self, link, data):
        assert link == 'shard', 'Link %s should not be deserialized' % link
        self.shard = Shard.deserialize(data)


    # Note: Only specify foreign keys here, specify all native host columns in
    # rdb_model_extensions instead.
    Protection = host_protections.Protection
    labels = dbmodels.ManyToManyField(Label, blank=True,
                                      db_table='afe_hosts_labels')
    locked_by = dbmodels.ForeignKey(User, null=True, blank=True, editable=False)
    name_field = 'hostname'
    objects = model_logic.ModelWithInvalidManager()
    valid_objects = model_logic.ValidObjectsManager()
    leased_objects = model_logic.LeasedHostManager()

    shard = dbmodels.ForeignKey(Shard, blank=True, null=True)

    def __init__(self, *args, **kwargs):
        super(Host, self).__init__(*args, **kwargs)
        self._record_attributes(['status'])


    @staticmethod
    def create_one_time_host(hostname):
        """Creates a one-time host.

        @param hostname: The name for the host.
        """
        query = Host.objects.filter(hostname=hostname)
        if query.count() == 0:
            host = Host(hostname=hostname, invalid=True)
            host.do_validate()
        else:
            host = query[0]
            if not host.invalid:
                raise model_logic.ValidationError({
                    'hostname' : '%s already exists in the autotest DB.  '
                        'Select it rather than entering it as a one time '
                        'host.' % hostname
                    })
        host.protection = host_protections.Protection.DO_NOT_REPAIR
        host.locked = False
        host.save()
        host.clean_object()
        return host


    @classmethod
    def assign_to_shard(cls, shard, known_ids):
        """Assigns hosts to a shard.

        For all labels that have been assigned to a shard, all hosts that
        have at least one of the shard's labels are assigned to the shard.
        Hosts that are assigned to the shard but aren't already present on the
        shard are returned.

        Board to shard mapping is many-to-one. Many different boards can be
        hosted in a shard. However, DUTs of a single board cannot be distributed
        into more than one shard.

        @param shard: The shard object to assign labels/hosts for.
        @param known_ids: List of all host-ids the shard already knows.
                          This is used to figure out which hosts should be sent
                          to the shard. If shard_ids were used instead, hosts
                          would only be transferred once, even if the client
                          failed persisting them.
                          The number of hosts usually lies in O(100), so the
                          overhead is acceptable.

        @returns the hosts objects that should be sent to the shard.
        """

        # Disclaimer: concurrent heartbeats should theoretically not occur in
        # the current setup. As they may be introduced in the near future,
        # this comment will be left here.

        # Sending stuff twice is acceptable, but forgetting something isn't.
        # Detecting duplicates on the client is easy, but here it's harder. The
        # following options were considered:
        # - SELECT ... WHERE and then UPDATE ... WHERE: Update might update more
        #   than select returned, as concurrently more hosts might have been
        #   inserted
        # - UPDATE and then SELECT WHERE shard=shard: select always returns all
        #   hosts for the shard, this is overhead
        # - SELECT and then UPDATE only selected without requerying afterwards:
        #   returns the old state of the records.
        host_ids = set(Host.objects.filter(
            labels__in=shard.labels.all(),
            leased=False
            ).exclude(
            id__in=known_ids,
            ).values_list('pk', flat=True))

        if host_ids:
            Host.objects.filter(pk__in=host_ids).update(shard=shard)
            return list(Host.objects.filter(pk__in=host_ids).all())
        return []

    def resurrect_object(self, old_object):
        super(Host, self).resurrect_object(old_object)
        # invalid hosts can be in use by the scheduler (as one-time hosts), so
        # don't change the status
        self.status = old_object.status


    def clean_object(self):
        self.aclgroup_set.clear()
        self.labels.clear()


    def record_state(self, type_str, state, value, other_metadata=None):
        """Record metadata in elasticsearch.

        @param type_str: sets the _type field in elasticsearch db.
        @param state: string representing what state we are recording,
                      e.g. 'locked'
        @param value: value of the state, e.g. True
        @param other_metadata: Other metadata to store in metaDB.
        """
        metadata = {
            state: value,
            'hostname': self.hostname,
        }
        if other_metadata:
            metadata = dict(metadata.items() + other_metadata.items())
        autotest_es.post(use_http=True, type_str=type_str, metadata=metadata)


    def save(self, *args, **kwargs):
        # extra spaces in the hostname can be a sneaky source of errors
        self.hostname = self.hostname.strip()
        # is this a new object being saved for the first time?
        first_time = (self.id is None)
        if not first_time:
            AclGroup.check_for_acl_violation_hosts([self])
        # If locked is changed, send its status and user made the change to
        # metaDB. Locks are important in host history because if a device is
        # locked then we don't really care what state it is in.
        if self.locked and not self.locked_by:
            self.locked_by = User.current_user()
            if not self.lock_time:
                self.lock_time = datetime.now()
            self.record_state('lock_history', 'locked', self.locked,
                              {'changed_by': self.locked_by.login,
                               'lock_reason': self.lock_reason})
            self.dirty = True
        elif not self.locked and self.locked_by:
            self.record_state('lock_history', 'locked', self.locked,
                              {'changed_by': self.locked_by.login})
            self.locked_by = None
            self.lock_time = None
        super(Host, self).save(*args, **kwargs)
        if first_time:
            everyone = AclGroup.objects.get(name='Everyone')
            everyone.hosts.add(self)
        self._check_for_updated_attributes()


    def delete(self):
        AclGroup.check_for_acl_violation_hosts([self])
        for queue_entry in self.hostqueueentry_set.all():
            queue_entry.deleted = True
            queue_entry.abort()
        super(Host, self).delete()


    def on_attribute_changed(self, attribute, old_value):
        assert attribute == 'status'
        logging.info(self.hostname + ' -> ' + self.status)


    def enqueue_job(self, job, atomic_group=None, is_template=False):
        """Enqueue a job on this host.

        @param job: A job to enqueue.
        @param atomic_group: The associated atomic group.
        @param is_template: Whther the status should be "Template".
        """
        queue_entry = HostQueueEntry.create(host=self, job=job,
                                            is_template=is_template,
                                            atomic_group=atomic_group)
        # allow recovery of dead hosts from the frontend
        if not self.active_queue_entry() and self.is_dead():
            self.status = Host.Status.READY
            self.save()
        queue_entry.save()

        block = IneligibleHostQueue(job=job, host=self)
        block.save()


    def platform(self):
        """The platform of the host."""
        # TODO(showard): slighly hacky?
        platforms = self.labels.filter(platform=True)
        if len(platforms) == 0:
            return None
        return platforms[0]
    platform.short_description = 'Platform'


    @classmethod
    def check_no_platform(cls, hosts):
        """Verify the specified hosts have no associated platforms.

        @param cls: Implicit class object.
        @param hosts: The hosts to verify.
        @raises model_logic.ValidationError if any hosts already have a
            platform.
        """
        Host.objects.populate_relationships(hosts, Label, 'label_list')
        errors = []
        for host in hosts:
            platforms = [label.name for label in host.label_list
                         if label.platform]
            if platforms:
                # do a join, just in case this host has multiple platforms,
                # we'll be able to see it
                errors.append('Host %s already has a platform: %s' % (
                              host.hostname, ', '.join(platforms)))
        if errors:
            raise model_logic.ValidationError({'labels': '; '.join(errors)})


    def is_dead(self):
        """Returns whether the host is dead (has status repair failed)."""
        return self.status == Host.Status.REPAIR_FAILED


    def active_queue_entry(self):
        """Returns the active queue entry for this host, or None if none."""
        active = list(self.hostqueueentry_set.filter(active=True))
        if not active:
            return None
        assert len(active) == 1, ('More than one active entry for '
                                  'host ' + self.hostname)
        return active[0]


    def _get_attribute_model_and_args(self, attribute):
        return HostAttribute, dict(host=self, attribute=attribute)


    @classmethod
    def get_attribute_model(cls):
        """Return the attribute model.

        Override method in parent class. See ModelExtensions for details.
        @returns: The attribute model of Host.
        """
        return HostAttribute


    class Meta:
        """Metadata for the Host class."""
        db_table = 'afe_hosts'


    def __unicode__(self):
        return unicode(self.hostname)


class HostAttribute(dbmodels.Model, model_logic.ModelExtensions):
    """Arbitrary keyvals associated with hosts."""

    SERIALIZATION_LINKS_TO_KEEP = set(['host'])
    SERIALIZATION_LOCAL_LINKS_TO_UPDATE = set(['value'])
    host = dbmodels.ForeignKey(Host)
    attribute = dbmodels.CharField(max_length=90)
    value = dbmodels.CharField(max_length=300)

    objects = model_logic.ExtendedManager()

    class Meta:
        """Metadata for the HostAttribute class."""
        db_table = 'afe_host_attributes'


    @classmethod
    def get_record(cls, data):
        """Check the database for an identical record.

        Use host_id and attribute to search for a existing record.

        @raises: DoesNotExist, if no record found
        @raises: MultipleObjectsReturned if multiple records found.
        """
        # TODO(fdeng): We should use host_id and attribute together as
        #              a primary key in the db.
        return cls.objects.get(host_id=data['host_id'],
                               attribute=data['attribute'])


    @classmethod
    def deserialize(cls, data):
        """Override deserialize in parent class.

        Do not deserialize id as id is not kept consistent on master and shards.

        @param data: A dictionary of data to deserialize.

        @returns: A HostAttribute object.
        """
        if data:
            data.pop('id')
        return super(HostAttribute, cls).deserialize(data)


class Test(dbmodels.Model, model_logic.ModelExtensions):
    """\
    Required:
    author: author name
    description: description of the test
    name: test name
    time: short, medium, long
    test_class: This describes the class for your the test belongs in.
    test_category: This describes the category for your tests
    test_type: Client or Server
    path: path to pass to run_test()
    sync_count:  is a number >=1 (1 being the default). If it's 1, then it's an
                 async job. If it's >1 it's sync job for that number of machines
                 i.e. if sync_count = 2 it is a sync job that requires two
                 machines.
    Optional:
    dependencies: What the test requires to run. Comma deliminated list
    dependency_labels: many-to-many relationship with labels corresponding to
                       test dependencies.
    experimental: If this is set to True production servers will ignore the test
    run_verify: Whether or not the scheduler should run the verify stage
    run_reset: Whether or not the scheduler should run the reset stage
    test_retry: Number of times to retry test if the test did not complete
                successfully. (optional, default: 0)
    """
    TestTime = enum.Enum('SHORT', 'MEDIUM', 'LONG', start_value=1)

    name = dbmodels.CharField(max_length=255, unique=True)
    author = dbmodels.CharField(max_length=255)
    test_class = dbmodels.CharField(max_length=255)
    test_category = dbmodels.CharField(max_length=255)
    dependencies = dbmodels.CharField(max_length=255, blank=True)
    description = dbmodels.TextField(blank=True)
    experimental = dbmodels.BooleanField(default=True)
    run_verify = dbmodels.BooleanField(default=False)
    test_time = dbmodels.SmallIntegerField(choices=TestTime.choices(),
                                           default=TestTime.MEDIUM)
    test_type = dbmodels.SmallIntegerField(
        choices=control_data.CONTROL_TYPE.choices())
    sync_count = dbmodels.IntegerField(default=1)
    path = dbmodels.CharField(max_length=255, unique=True)
    test_retry = dbmodels.IntegerField(blank=True, default=0)
    run_reset = dbmodels.BooleanField(default=True)

    dependency_labels = (
        dbmodels.ManyToManyField(Label, blank=True,
                                 db_table='afe_autotests_dependency_labels'))
    name_field = 'name'
    objects = model_logic.ExtendedManager()


    def admin_description(self):
        """Returns a string representing the admin description."""
        escaped_description = saxutils.escape(self.description)
        return '<span style="white-space:pre">%s</span>' % escaped_description
    admin_description.allow_tags = True
    admin_description.short_description = 'Description'


    class Meta:
        """Metadata for class Test."""
        db_table = 'afe_autotests'

    def __unicode__(self):
        return unicode(self.name)


class TestParameter(dbmodels.Model):
    """
    A declared parameter of a test
    """
    test = dbmodels.ForeignKey(Test)
    name = dbmodels.CharField(max_length=255)

    class Meta:
        """Metadata for class TestParameter."""
        db_table = 'afe_test_parameters'
        unique_together = ('test', 'name')

    def __unicode__(self):
        return u'%s (%s)' % (self.name, self.test.name)


class Profiler(dbmodels.Model, model_logic.ModelExtensions):
    """\
    Required:
    name: profiler name
    test_type: Client or Server

    Optional:
    description: arbirary text description
    """
    name = dbmodels.CharField(max_length=255, unique=True)
    description = dbmodels.TextField(blank=True)

    name_field = 'name'
    objects = model_logic.ExtendedManager()


    class Meta:
        """Metadata for class Profiler."""
        db_table = 'afe_profilers'

    def __unicode__(self):
        return unicode(self.name)


class AclGroup(dbmodels.Model, model_logic.ModelExtensions):
    """\
    Required:
    name: name of ACL group

    Optional:
    description: arbitrary description of group
    """

    SERIALIZATION_LINKS_TO_FOLLOW = set(['users'])

    name = dbmodels.CharField(max_length=255, unique=True)
    description = dbmodels.CharField(max_length=255, blank=True)
    users = dbmodels.ManyToManyField(User, blank=False,
                                     db_table='afe_acl_groups_users')
    hosts = dbmodels.ManyToManyField(Host, blank=True,
                                     db_table='afe_acl_groups_hosts')

    name_field = 'name'
    objects = model_logic.ExtendedManager()

    @staticmethod
    def check_for_acl_violation_hosts(hosts):
        """Verify the current user has access to the specified hosts.

        @param hosts: The hosts to verify against.
        @raises AclAccessViolation if the current user doesn't have access
            to a host.
        """
        user = User.current_user()
        if user.is_superuser():
            return
        accessible_host_ids = set(
            host.id for host in Host.objects.filter(aclgroup__users=user))
        for host in hosts:
            # Check if the user has access to this host,
            # but only if it is not a metahost or a one-time-host.
            no_access = (isinstance(host, Host)
                         and not host.invalid
                         and int(host.id) not in accessible_host_ids)
            if no_access:
                raise AclAccessViolation("%s does not have access to %s" %
                                         (str(user), str(host)))


    @staticmethod
    def check_abort_permissions(queue_entries):
        """Look for queue entries that aren't abortable by the current user.

        An entry is not abortable if:
           * the job isn't owned by this user, and
           * the machine isn't ACL-accessible, or
           * the machine is in the "Everyone" ACL

        @param queue_entries: The queue entries to check.
        @raises AclAccessViolation if a queue entry is not abortable by the
            current user.
        """
        user = User.current_user()
        if user.is_superuser():
            return
        not_owned = queue_entries.exclude(job__owner=user.login)
        # I do this using ID sets instead of just Django filters because
        # filtering on M2M dbmodels is broken in Django 0.96. It's better in
        # 1.0.
        # TODO: Use Django filters, now that we're using 1.0.
        accessible_ids = set(
            entry.id for entry
            in not_owned.filter(host__aclgroup__users__login=user.login))
        public_ids = set(entry.id for entry
                         in not_owned.filter(host__aclgroup__name='Everyone'))
        cannot_abort = [entry for entry in not_owned.select_related()
                        if entry.id not in accessible_ids
                        or entry.id in public_ids]
        if len(cannot_abort) == 0:
            return
        entry_names = ', '.join('%s-%s/%s' % (entry.job.id, entry.job.owner,
                                              entry.host_or_metahost_name())
                                for entry in cannot_abort)
        raise AclAccessViolation('You cannot abort the following job entries: '
                                 + entry_names)


    def check_for_acl_violation_acl_group(self):
        """Verifies the current user has acces to this ACL group.

        @raises AclAccessViolation if the current user doesn't have access to
            this ACL group.
        """
        user = User.current_user()
        if user.is_superuser():
            return
        if self.name == 'Everyone':
            raise AclAccessViolation("You cannot modify 'Everyone'!")
        if not user in self.users.all():
            raise AclAccessViolation("You do not have access to %s"
                                     % self.name)

    @staticmethod
    def on_host_membership_change():
        """Invoked when host membership changes."""
        everyone = AclGroup.objects.get(name='Everyone')

        # find hosts that aren't in any ACL group and add them to Everyone
        # TODO(showard): this is a bit of a hack, since the fact that this query
        # works is kind of a coincidence of Django internals.  This trick
        # doesn't work in general (on all foreign key relationships).  I'll
        # replace it with a better technique when the need arises.
        orphaned_hosts = Host.valid_objects.filter(aclgroup__id__isnull=True)
        everyone.hosts.add(*orphaned_hosts.distinct())

        # find hosts in both Everyone and another ACL group, and remove them
        # from Everyone
        hosts_in_everyone = Host.valid_objects.filter(aclgroup__name='Everyone')
        acled_hosts = set()
        for host in hosts_in_everyone:
            # Has an ACL group other than Everyone
            if host.aclgroup_set.count() > 1:
                acled_hosts.add(host)
        everyone.hosts.remove(*acled_hosts)


    def delete(self):
        if (self.name == 'Everyone'):
            raise AclAccessViolation("You cannot delete 'Everyone'!")
        self.check_for_acl_violation_acl_group()
        super(AclGroup, self).delete()
        self.on_host_membership_change()


    def add_current_user_if_empty(self):
        """Adds the current user if the set of users is empty."""
        if not self.users.count():
            self.users.add(User.current_user())


    def perform_after_save(self, change):
        """Called after a save.

        @param change: Whether there was a change.
        """
        if not change:
            self.users.add(User.current_user())
        self.add_current_user_if_empty()
        self.on_host_membership_change()


    def save(self, *args, **kwargs):
        change = bool(self.id)
        if change:
            # Check the original object for an ACL violation
            AclGroup.objects.get(id=self.id).check_for_acl_violation_acl_group()
        super(AclGroup, self).save(*args, **kwargs)
        self.perform_after_save(change)


    class Meta:
        """Metadata for class AclGroup."""
        db_table = 'afe_acl_groups'

    def __unicode__(self):
        return unicode(self.name)


class Kernel(dbmodels.Model):
    """
    A kernel configuration for a parameterized job
    """
    version = dbmodels.CharField(max_length=255)
    cmdline = dbmodels.CharField(max_length=255, blank=True)

    @classmethod
    def create_kernels(cls, kernel_list):
        """Creates all kernels in the kernel list.

        @param cls: Implicit class object.
        @param kernel_list: A list of dictionaries that describe the kernels,
            in the same format as the 'kernel' argument to
            rpc_interface.generate_control_file.
        @return A list of the created kernels.
        """
        if not kernel_list:
            return None
        return [cls._create(kernel) for kernel in kernel_list]


    @classmethod
    def _create(cls, kernel_dict):
        version = kernel_dict.pop('version')
        cmdline = kernel_dict.pop('cmdline', '')

        if kernel_dict:
            raise Exception('Extraneous kernel arguments remain: %r'
                            % kernel_dict)

        kernel, _ = cls.objects.get_or_create(version=version,
                                              cmdline=cmdline)
        return kernel


    class Meta:
        """Metadata for class Kernel."""
        db_table = 'afe_kernels'
        unique_together = ('version', 'cmdline')

    def __unicode__(self):
        return u'%s %s' % (self.version, self.cmdline)


class ParameterizedJob(dbmodels.Model):
    """
    Auxiliary configuration for a parameterized job.
    """
    test = dbmodels.ForeignKey(Test)
    label = dbmodels.ForeignKey(Label, null=True)
    use_container = dbmodels.BooleanField(default=False)
    profile_only = dbmodels.BooleanField(default=False)
    upload_kernel_config = dbmodels.BooleanField(default=False)

    kernels = dbmodels.ManyToManyField(
            Kernel, db_table='afe_parameterized_job_kernels')
    profilers = dbmodels.ManyToManyField(
            Profiler, through='ParameterizedJobProfiler')


    @classmethod
    def smart_get(cls, id_or_name, *args, **kwargs):
        """For compatibility with Job.add_object.

        @param cls: Implicit class object.
        @param id_or_name: The ID or name to get.
        @param args: Non-keyword arguments.
        @param kwargs: Keyword arguments.
        """
        return cls.objects.get(pk=id_or_name)


    def job(self):
        """Returns the job if it exists, or else None."""
        jobs = self.job_set.all()
        assert jobs.count() <= 1
        return jobs and jobs[0] or None


    class Meta:
        """Metadata for class ParameterizedJob."""
        db_table = 'afe_parameterized_jobs'

    def __unicode__(self):
        return u'%s (parameterized) - %s' % (self.test.name, self.job())


class ParameterizedJobProfiler(dbmodels.Model):
    """
    A profiler to run on a parameterized job
    """
    parameterized_job = dbmodels.ForeignKey(ParameterizedJob)
    profiler = dbmodels.ForeignKey(Profiler)

    class Meta:
        """Metedata for class ParameterizedJobProfiler."""
        db_table = 'afe_parameterized_jobs_profilers'
        unique_together = ('parameterized_job', 'profiler')


class ParameterizedJobProfilerParameter(dbmodels.Model):
    """
    A parameter for a profiler in a parameterized job
    """
    parameterized_job_profiler = dbmodels.ForeignKey(ParameterizedJobProfiler)
    parameter_name = dbmodels.CharField(max_length=255)
    parameter_value = dbmodels.TextField()
    parameter_type = dbmodels.CharField(
            max_length=8, choices=model_attributes.ParameterTypes.choices())

    class Meta:
        """Metadata for class ParameterizedJobProfilerParameter."""
        db_table = 'afe_parameterized_job_profiler_parameters'
        unique_together = ('parameterized_job_profiler', 'parameter_name')

    def __unicode__(self):
        return u'%s - %s' % (self.parameterized_job_profiler.profiler.name,
                             self.parameter_name)


class ParameterizedJobParameter(dbmodels.Model):
    """
    Parameters for a parameterized job
    """
    parameterized_job = dbmodels.ForeignKey(ParameterizedJob)
    test_parameter = dbmodels.ForeignKey(TestParameter)
    parameter_value = dbmodels.TextField()
    parameter_type = dbmodels.CharField(
            max_length=8, choices=model_attributes.ParameterTypes.choices())

    class Meta:
        """Metadata for class ParameterizedJobParameter."""
        db_table = 'afe_parameterized_job_parameters'
        unique_together = ('parameterized_job', 'test_parameter')

    def __unicode__(self):
        return u'%s - %s' % (self.parameterized_job.job().name,
                             self.test_parameter.name)


class JobManager(model_logic.ExtendedManager):
    'Custom manager to provide efficient status counts querying.'
    def get_status_counts(self, job_ids):
        """Returns a dict mapping the given job IDs to their status count dicts.

        @param job_ids: A list of job IDs.
        """
        if not job_ids:
            return {}
        id_list = '(%s)' % ','.join(str(job_id) for job_id in job_ids)
        cursor = connection.cursor()
        cursor.execute("""
            SELECT job_id, status, aborted, complete, COUNT(*)
            FROM afe_host_queue_entries
            WHERE job_id IN %s
            GROUP BY job_id, status, aborted, complete
            """ % id_list)
        all_job_counts = dict((job_id, {}) for job_id in job_ids)
        for job_id, status, aborted, complete, count in cursor.fetchall():
            job_dict = all_job_counts[job_id]
            full_status = HostQueueEntry.compute_full_status(status, aborted,
                                                             complete)
            job_dict.setdefault(full_status, 0)
            job_dict[full_status] += count
        return all_job_counts


class Job(dbmodels.Model, model_logic.ModelExtensions):
    """\
    owner: username of job owner
    name: job name (does not have to be unique)
    priority: Integer priority value.  Higher is more important.
    control_file: contents of control file
    control_type: Client or Server
    created_on: date of job creation
    submitted_on: date of job submission
    synch_count: how many hosts should be used per autoserv execution
    run_verify: Whether or not to run the verify phase
    run_reset: Whether or not to run the reset phase
    timeout: DEPRECATED - hours from queuing time until job times out
    timeout_mins: minutes from job queuing time until the job times out
    max_runtime_hrs: DEPRECATED - hours from job starting time until job
                     times out
    max_runtime_mins: minutes from job starting time until job times out
    email_list: list of people to email on completion delimited by any of:
                white space, ',', ':', ';'
    dependency_labels: many-to-many relationship with labels corresponding to
                       job dependencies
    reboot_before: Never, If dirty, or Always
    reboot_after: Never, If all tests passed, or Always
    parse_failed_repair: if True, a failed repair launched by this job will have
    its results parsed as part of the job.
    drone_set: The set of drones to run this job on
    parent_job: Parent job (optional)
    test_retry: Number of times to retry test if the test did not complete
                successfully. (optional, default: 0)
    require_ssp: Require server-side packaging unless require_ssp is set to
                 False. (optional, default: None)
    """

    # TODO: Investigate, if jobkeyval_set is really needed.
    # dynamic_suite will write them into an attached file for the drone, but
    # it doesn't seem like they are actually used. If they aren't used, remove
    # jobkeyval_set here.
    SERIALIZATION_LINKS_TO_FOLLOW = set(['dependency_labels',
                                         'hostqueueentry_set',
                                         'jobkeyval_set',
                                         'shard'])

    # SQL for selecting jobs that should be sent to shard.
    # We use raw sql as django filters were not optimized.
    # The following jobs are excluded by the SQL.
    #     - Non-aborted jobs known to shard as specified in |known_ids|.
    #       Note for jobs aborted on master, even if already known to shard,
    #       will be sent to shard again so that shard can abort them.
    #     - Completed jobs
    #     - Active jobs
    #     - Jobs without host_queue_entries
    NON_ABORTED_KNOWN_JOBS = '(t2.aborted = 0 AND t1.id IN (%(known_ids)s))'

    SQL_SHARD_JOBS = (
        'SELECT DISTINCT(t1.id) FROM afe_jobs t1 '
        'INNER JOIN afe_host_queue_entries t2  ON '
        '  (t1.id = t2.job_id AND t2.complete != 1 AND t2.active != 1 '
        '   %(check_known_jobs)s) '
        'LEFT OUTER JOIN afe_jobs_dependency_labels t3 ON (t1.id = t3.job_id) '
        'WHERE (t3.label_id IN  '
        '  (SELECT label_id FROM afe_shards_labels '
        '   WHERE shard_id = %(shard_id)s) '
        '  OR t2.meta_host IN '
        '  (SELECT label_id FROM afe_shards_labels '
        '   WHERE shard_id = %(shard_id)s))'
        )

    # Jobs can be created with assigned hosts and have no dependency
    # labels nor meta_host.
    # We are looking for:
    #     - a job whose hqe's meta_host is null
    #     - a job whose hqe has a host
    #     - one of the host's labels matches the shard's label.
    # Non-aborted known jobs, completed jobs, active jobs, jobs
    # without hqe are exluded as we do with SQL_SHARD_JOBS.
    SQL_SHARD_JOBS_WITH_HOSTS = (
        'SELECT DISTINCT(t1.id) FROM afe_jobs t1 '
        'INNER JOIN afe_host_queue_entries t2 ON '
        '  (t1.id = t2.job_id AND t2.complete != 1 AND t2.active != 1 '
        '   AND t2.meta_host IS NULL AND t2.host_id IS NOT NULL '
        '   %(check_known_jobs)s) '
        'LEFT OUTER JOIN afe_hosts_labels t3 ON (t2.host_id = t3.host_id) '
        'WHERE (t3.label_id IN '
        '  (SELECT label_id FROM afe_shards_labels '
        '   WHERE shard_id = %(shard_id)s))'
        )

    # Even if we had filters about complete, active and aborted
    # bits in the above two SQLs, there is a chance that
    # the result may still contain a job with an hqe with 'complete=1'
    # or 'active=1' or 'aborted=0 and afe_job.id in known jobs.'
    # This happens when a job has two (or more) hqes and at least
    # one hqe has different bits than others.
    # We use a second sql to ensure we exclude all un-desired jobs.
    SQL_JOBS_TO_EXCLUDE =(
        'SELECT t1.id FROM afe_jobs t1 '
        'INNER JOIN afe_host_queue_entries t2 ON '
        '  (t1.id = t2.job_id) '
        'WHERE (t1.id in (%(candidates)s) '
        '  AND (t2.complete=1 OR t2.active=1 '
        '  %(check_known_jobs)s))'
        )

    def _deserialize_relation(self, link, data):
        if link in ['hostqueueentry_set', 'jobkeyval_set']:
            for obj in data:
                obj['job_id'] = self.id

        super(Job, self)._deserialize_relation(link, data)


    def custom_deserialize_relation(self, link, data):
        assert link == 'shard', 'Link %s should not be deserialized' % link
        self.shard = Shard.deserialize(data)


    def sanity_check_update_from_shard(self, shard, updated_serialized):
        # If the job got aborted on the master after the client fetched it
        # no shard_id will be set. The shard might still push updates though,
        # as the job might complete before the abort bit syncs to the shard.
        # Alternative considered: The master scheduler could be changed to not
        # set aborted jobs to completed that are sharded out. But that would
        # require database queries and seemed more complicated to implement.
        # This seems safe to do, as there won't be updates pushed from the wrong
        # shards should be powered off and wiped hen they are removed from the
        # master.
        if self.shard_id and self.shard_id != shard.id:
            raise error.UnallowedRecordsSentToMaster(
                'Job id=%s is assigned to shard (%s). Cannot update it with %s '
                'from shard %s.' % (self.id, self.shard_id, updated_serialized,
                                    shard.id))


    # TIMEOUT is deprecated.
    DEFAULT_TIMEOUT = global_config.global_config.get_config_value(
        'AUTOTEST_WEB', 'job_timeout_default', default=24)
    DEFAULT_TIMEOUT_MINS = global_config.global_config.get_config_value(
        'AUTOTEST_WEB', 'job_timeout_mins_default', default=24*60)
    # MAX_RUNTIME_HRS is deprecated. Will be removed after switch to mins is
    # completed.
    DEFAULT_MAX_RUNTIME_HRS = global_config.global_config.get_config_value(
        'AUTOTEST_WEB', 'job_max_runtime_hrs_default', default=72)
    DEFAULT_MAX_RUNTIME_MINS = global_config.global_config.get_config_value(
        'AUTOTEST_WEB', 'job_max_runtime_mins_default', default=72*60)
    DEFAULT_PARSE_FAILED_REPAIR = global_config.global_config.get_config_value(
        'AUTOTEST_WEB', 'parse_failed_repair_default', type=bool,
        default=False)

    owner = dbmodels.CharField(max_length=255)
    name = dbmodels.CharField(max_length=255)
    priority = dbmodels.SmallIntegerField(default=priorities.Priority.DEFAULT)
    control_file = dbmodels.TextField(null=True, blank=True)
    control_type = dbmodels.SmallIntegerField(
        choices=control_data.CONTROL_TYPE.choices(),
        blank=True, # to allow 0
        default=control_data.CONTROL_TYPE.CLIENT)
    created_on = dbmodels.DateTimeField()
    synch_count = dbmodels.IntegerField(blank=True, default=0)
    timeout = dbmodels.IntegerField(default=DEFAULT_TIMEOUT)
    run_verify = dbmodels.BooleanField(default=False)
    email_list = dbmodels.CharField(max_length=250, blank=True)
    dependency_labels = (
            dbmodels.ManyToManyField(Label, blank=True,
                                     db_table='afe_jobs_dependency_labels'))
    reboot_before = dbmodels.SmallIntegerField(
        choices=model_attributes.RebootBefore.choices(), blank=True,
        default=DEFAULT_REBOOT_BEFORE)
    reboot_after = dbmodels.SmallIntegerField(
        choices=model_attributes.RebootAfter.choices(), blank=True,
        default=DEFAULT_REBOOT_AFTER)
    parse_failed_repair = dbmodels.BooleanField(
        default=DEFAULT_PARSE_FAILED_REPAIR)
    # max_runtime_hrs is deprecated. Will be removed after switch to mins is
    # completed.
    max_runtime_hrs = dbmodels.IntegerField(default=DEFAULT_MAX_RUNTIME_HRS)
    max_runtime_mins = dbmodels.IntegerField(default=DEFAULT_MAX_RUNTIME_MINS)
    drone_set = dbmodels.ForeignKey(DroneSet, null=True, blank=True)

    parameterized_job = dbmodels.ForeignKey(ParameterizedJob, null=True,
                                            blank=True)

    parent_job = dbmodels.ForeignKey('self', blank=True, null=True)

    test_retry = dbmodels.IntegerField(blank=True, default=0)

    run_reset = dbmodels.BooleanField(default=True)

    timeout_mins = dbmodels.IntegerField(default=DEFAULT_TIMEOUT_MINS)

    # If this is None on the master, a slave should be found.
    # If this is None on a slave, it should be synced back to the master
    shard = dbmodels.ForeignKey(Shard, blank=True, null=True)

    # If this is None, server-side packaging will be used for server side test,
    # unless it's disabled in global config AUTOSERV/enable_ssp_container.
    require_ssp = dbmodels.NullBooleanField(default=None, blank=True, null=True)

    # custom manager
    objects = JobManager()


    @decorators.cached_property
    def labels(self):
        """All the labels of this job"""
        # We need to convert dependency_labels to a list, because all() gives us
        # back an iterator, and storing/caching an iterator means we'd only be
        # able to read from it once.
        return list(self.dependency_labels.all())


    def is_server_job(self):
        """Returns whether this job is of type server."""
        return self.control_type == control_data.CONTROL_TYPE.SERVER


    @classmethod
    def parameterized_jobs_enabled(cls):
        """Returns whether parameterized jobs are enabled.

        @param cls: Implicit class object.
        """
        return global_config.global_config.get_config_value(
                'AUTOTEST_WEB', 'parameterized_jobs', type=bool)


    @classmethod
    def check_parameterized_job(cls, control_file, parameterized_job):
        """Checks that the job is valid given the global config settings.

        First, either control_file must be set, or parameterized_job must be
        set, but not both. Second, parameterized_job must be set if and only if
        the parameterized_jobs option in the global config is set to True.

        @param cls: Implict class object.
        @param control_file: A control file.
        @param parameterized_job: A parameterized job.
        """
        if not (bool(control_file) ^ bool(parameterized_job)):
            raise Exception('Job must have either control file or '
                            'parameterization, but not both')

        parameterized_jobs_enabled = cls.parameterized_jobs_enabled()
        if control_file and parameterized_jobs_enabled:
            raise Exception('Control file specified, but parameterized jobs '
                            'are enabled')
        if parameterized_job and not parameterized_jobs_enabled:
            raise Exception('Parameterized job specified, but parameterized '
                            'jobs are not enabled')


    @classmethod
    def create(cls, owner, options, hosts):
        """Creates a job.

        The job is created by taking some information (the listed args) and
        filling in the rest of the necessary information.

        @param cls: Implicit class object.
        @param owner: The owner for the job.
        @param options: An options object.
        @param hosts: The hosts to use.
        """
        AclGroup.check_for_acl_violation_hosts(hosts)

        control_file = options.get('control_file')
        parameterized_job = options.get('parameterized_job')

        # The current implementation of parameterized jobs requires that only
        # control files or parameterized jobs are used. Using the image
        # parameter on autoupdate_ParameterizedJob doesn't mix pure
        # parameterized jobs and control files jobs, it does muck enough with
        # normal jobs by adding a parameterized id to them that this check will
        # fail. So for now we just skip this check.
        # cls.check_parameterized_job(control_file=control_file,
        #                             parameterized_job=parameterized_job)
        user = User.current_user()
        if options.get('reboot_before') is None:
            options['reboot_before'] = user.get_reboot_before_display()
        if options.get('reboot_after') is None:
            options['reboot_after'] = user.get_reboot_after_display()

        drone_set = DroneSet.resolve_name(options.get('drone_set'))

        if options.get('timeout_mins') is None and options.get('timeout'):
            options['timeout_mins'] = options['timeout'] * 60

        job = cls.add_object(
            owner=owner,
            name=options['name'],
            priority=options['priority'],
            control_file=control_file,
            control_type=options['control_type'],
            synch_count=options.get('synch_count'),
            # timeout needs to be deleted in the future.
            timeout=options.get('timeout'),
            timeout_mins=options.get('timeout_mins'),
            max_runtime_mins=options.get('max_runtime_mins'),
            run_verify=options.get('run_verify'),
            email_list=options.get('email_list'),
            reboot_before=options.get('reboot_before'),
            reboot_after=options.get('reboot_after'),
            parse_failed_repair=options.get('parse_failed_repair'),
            created_on=datetime.now(),
            drone_set=drone_set,
            parameterized_job=parameterized_job,
            parent_job=options.get('parent_job_id'),
            test_retry=options.get('test_retry'),
            run_reset=options.get('run_reset'),
            require_ssp=options.get('require_ssp'))

        job.dependency_labels = options['dependencies']

        if options.get('keyvals'):
            for key, value in options['keyvals'].iteritems():
                JobKeyval.objects.create(job=job, key=key, value=value)

        return job


    @classmethod
    def assign_to_shard(cls, shard, known_ids):
        """Assigns unassigned jobs to a shard.

        For all labels that have been assigned to this shard, all jobs that
        have this label, are assigned to this shard.

        Jobs that are assigned to the shard but aren't already present on the
        shard are returned.

        @param shard: The shard to assign jobs to.
        @param known_ids: List of all ids of incomplete jobs, the shard already
                          knows about.
                          This is used to figure out which jobs should be sent
                          to the shard. If shard_ids were used instead, jobs
                          would only be transferred once, even if the client
                          failed persisting them.
                          The number of unfinished jobs usually lies in O(1000).
                          Assuming one id takes 8 chars in the json, this means
                          overhead that lies in the lower kilobyte range.
                          A not in query with 5000 id's takes about 30ms.

        @returns The job objects that should be sent to the shard.
        """
        # Disclaimer: Concurrent heartbeats should not occur in today's setup.
        # If this changes or they are triggered manually, this applies:
        # Jobs may be returned more than once by concurrent calls of this
        # function, as there is a race condition between SELECT and UPDATE.
        job_ids = set([])
        check_known_jobs_exclude = ''
        check_known_jobs_include = ''

        if known_ids:
            check_known_jobs = (
                    cls.NON_ABORTED_KNOWN_JOBS %
                    {'known_ids': ','.join([str(i) for i in known_ids])})
            check_known_jobs_exclude = 'AND NOT ' + check_known_jobs
            check_known_jobs_include = 'OR ' + check_known_jobs

        for sql in [cls.SQL_SHARD_JOBS, cls.SQL_SHARD_JOBS_WITH_HOSTS]:
            query = Job.objects.raw(sql % {
                    'check_known_jobs': check_known_jobs_exclude,
                    'shard_id': shard.id})
            job_ids |= set([j.id for j in query])

        if job_ids:
            query = Job.objects.raw(
                    cls.SQL_JOBS_TO_EXCLUDE %
                    {'check_known_jobs': check_known_jobs_include,
                     'candidates': ','.join([str(i) for i in job_ids])})
            job_ids -= set([j.id for j in query])

        if job_ids:
            Job.objects.filter(pk__in=job_ids).update(shard=shard)
            return list(Job.objects.filter(pk__in=job_ids).all())
        return []


    def save(self, *args, **kwargs):
        # The current implementation of parameterized jobs requires that only
        # control files or parameterized jobs are used. Using the image
        # parameter on autoupdate_ParameterizedJob doesn't mix pure
        # parameterized jobs and control files jobs, it does muck enough with
        # normal jobs by adding a parameterized id to them that this check will
        # fail. So for now we just skip this check.
        # cls.check_parameterized_job(control_file=self.control_file,
        #                             parameterized_job=self.parameterized_job)
        super(Job, self).save(*args, **kwargs)


    def queue(self, hosts, atomic_group=None, is_template=False):
        """Enqueue a job on the given hosts.

        @param hosts: The hosts to use.
        @param atomic_group: The associated atomic group.
        @param is_template: Whether the status should be "Template".
        """
        if not hosts:
            if atomic_group:
                # No hosts or labels are required to queue an atomic group
                # Job.  However, if they are given, we respect them below.
                atomic_group.enqueue_job(self, is_template=is_template)
            else:
                # hostless job
                entry = HostQueueEntry.create(job=self, is_template=is_template)
                entry.save()
            return

        for host in hosts:
            host.enqueue_job(self, atomic_group=atomic_group,
                             is_template=is_template)


    def create_recurring_job(self, start_date, loop_period, loop_count, owner):
        """Creates a recurring job.

        @param start_date: The starting date of the job.
        @param loop_period: How often to re-run the job, in seconds.
        @param loop_count: The re-run count.
        @param owner: The owner of the job.
        """
        rec = RecurringRun(job=self, start_date=start_date,
                           loop_period=loop_period,
                           loop_count=loop_count,
                           owner=User.objects.get(login=owner))
        rec.save()
        return rec.id


    def user(self):
        """Gets the user of this job, or None if it doesn't exist."""
        try:
            return User.objects.get(login=self.owner)
        except self.DoesNotExist:
            return None


    def abort(self):
        """Aborts this job."""
        for queue_entry in self.hostqueueentry_set.all():
            queue_entry.abort()


    def tag(self):
        """Returns a string tag for this job."""
        return server_utils.get_job_tag(self.id, self.owner)


    def keyval_dict(self):
        """Returns all keyvals for this job as a dictionary."""
        return dict((keyval.key, keyval.value)
                    for keyval in self.jobkeyval_set.all())


    @classmethod
    def get_attribute_model(cls):
        """Return the attribute model.

        Override method in parent class. This class is called when
        deserializing the one-to-many relationship betwen Job and JobKeyval.
        On deserialization, we will try to clear any existing job keyvals
        associated with a job to avoid any inconsistency.
        Though Job doesn't implement ModelWithAttribute, we still treat
        it as an attribute model for this purpose.

        @returns: The attribute model of Job.
        """
        return JobKeyval


    class Meta:
        """Metadata for class Job."""
        db_table = 'afe_jobs'

    def __unicode__(self):
        return u'%s (%s-%s)' % (self.name, self.id, self.owner)


class JobKeyval(dbmodels.Model, model_logic.ModelExtensions):
    """Keyvals associated with jobs"""

    SERIALIZATION_LINKS_TO_KEEP = set(['job'])
    SERIALIZATION_LOCAL_LINKS_TO_UPDATE = set(['value'])

    job = dbmodels.ForeignKey(Job)
    key = dbmodels.CharField(max_length=90)
    value = dbmodels.CharField(max_length=300)

    objects = model_logic.ExtendedManager()


    @classmethod
    def get_record(cls, data):
        """Check the database for an identical record.

        Use job_id and key to search for a existing record.

        @raises: DoesNotExist, if no record found
        @raises: MultipleObjectsReturned if multiple records found.
        """
        # TODO(fdeng): We should use job_id and key together as
        #              a primary key in the db.
        return cls.objects.get(job_id=data['job_id'], key=data['key'])


    @classmethod
    def deserialize(cls, data):
        """Override deserialize in parent class.

        Do not deserialize id as id is not kept consistent on master and shards.

        @param data: A dictionary of data to deserialize.

        @returns: A JobKeyval object.
        """
        if data:
            data.pop('id')
        return super(JobKeyval, cls).deserialize(data)


    class Meta:
        """Metadata for class JobKeyval."""
        db_table = 'afe_job_keyvals'


class IneligibleHostQueue(dbmodels.Model, model_logic.ModelExtensions):
    """Represents an ineligible host queue."""
    job = dbmodels.ForeignKey(Job)
    host = dbmodels.ForeignKey(Host)

    objects = model_logic.ExtendedManager()

    class Meta:
        """Metadata for class IneligibleHostQueue."""
        db_table = 'afe_ineligible_host_queues'


class HostQueueEntry(dbmodels.Model, model_logic.ModelExtensions):
    """Represents a host queue entry."""

    SERIALIZATION_LINKS_TO_FOLLOW = set(['meta_host'])
    SERIALIZATION_LINKS_TO_KEEP = set(['host'])
    SERIALIZATION_LOCAL_LINKS_TO_UPDATE = set(['aborted'])


    def custom_deserialize_relation(self, link, data):
        assert link == 'meta_host'
        self.meta_host = Label.deserialize(data)


    def sanity_check_update_from_shard(self, shard, updated_serialized,
                                       job_ids_sent):
        if self.job_id not in job_ids_sent:
            raise error.UnallowedRecordsSentToMaster(
                'Sent HostQueueEntry without corresponding '
                'job entry: %s' % updated_serialized)


    Status = host_queue_entry_states.Status
    ACTIVE_STATUSES = host_queue_entry_states.ACTIVE_STATUSES
    COMPLETE_STATUSES = host_queue_entry_states.COMPLETE_STATUSES

    job = dbmodels.ForeignKey(Job)
    host = dbmodels.ForeignKey(Host, blank=True, null=True)
    status = dbmodels.CharField(max_length=255)
    meta_host = dbmodels.ForeignKey(Label, blank=True, null=True,
                                    db_column='meta_host')
    active = dbmodels.BooleanField(default=False)
    complete = dbmodels.BooleanField(default=False)
    deleted = dbmodels.BooleanField(default=False)
    execution_subdir = dbmodels.CharField(max_length=255, blank=True,
                                          default='')
    # If atomic_group is set, this is a virtual HostQueueEntry that will
    # be expanded into many actual hosts within the group at schedule time.
    atomic_group = dbmodels.ForeignKey(AtomicGroup, blank=True, null=True)
    aborted = dbmodels.BooleanField(default=False)
    started_on = dbmodels.DateTimeField(null=True, blank=True)
    finished_on = dbmodels.DateTimeField(null=True, blank=True)

    objects = model_logic.ExtendedManager()


    def __init__(self, *args, **kwargs):
        super(HostQueueEntry, self).__init__(*args, **kwargs)
        self._record_attributes(['status'])


    @classmethod
    def create(cls, job, host=None, meta_host=None, atomic_group=None,
                 is_template=False):
        """Creates a new host queue entry.

        @param cls: Implicit class object.
        @param job: The associated job.
        @param host: The associated host.
        @param meta_host: The associated meta host.
        @param atomic_group: The associated atomic group.
        @param is_template: Whether the status should be "Template".
        """
        if is_template:
            status = cls.Status.TEMPLATE
        else:
            status = cls.Status.QUEUED

        return cls(job=job, host=host, meta_host=meta_host,
                   atomic_group=atomic_group, status=status)


    def save(self, *args, **kwargs):
        self._set_active_and_complete()
        super(HostQueueEntry, self).save(*args, **kwargs)
        self._check_for_updated_attributes()


    def execution_path(self):
        """
        Path to this entry's results (relative to the base results directory).
        """
        return server_utils.get_hqe_exec_path(self.job.tag(),
                                              self.execution_subdir)


    def host_or_metahost_name(self):
        """Returns the first non-None name found in priority order.

        The priority order checked is: (1) host name; (2) meta host name; and
        (3) atomic group name.
        """
        if self.host:
            return self.host.hostname
        elif self.meta_host:
            return self.meta_host.name
        else:
            assert self.atomic_group, "no host, meta_host or atomic group!"
            return self.atomic_group.name


    def _set_active_and_complete(self):
        if self.status in self.ACTIVE_STATUSES:
            self.active, self.complete = True, False
        elif self.status in self.COMPLETE_STATUSES:
            self.active, self.complete = False, True
        else:
            self.active, self.complete = False, False


    def on_attribute_changed(self, attribute, old_value):
        assert attribute == 'status'
        logging.info('%s/%d (%d) -> %s', self.host, self.job.id, self.id,
                     self.status)


    def is_meta_host_entry(self):
        'True if this is a entry has a meta_host instead of a host.'
        return self.host is None and self.meta_host is not None


    # This code is shared between rpc_interface and models.HostQueueEntry.
    # Sadly due to circular imports between the 2 (crbug.com/230100) making it
    # a class method was the best way to refactor it. Attempting to put it in
    # rpc_utils or a new utils module failed as that would require us to import
    # models.py but to call it from here we would have to import the utils.py
    # thus creating a cycle.
    @classmethod
    def abort_host_queue_entries(cls, host_queue_entries):
        """Aborts a collection of host_queue_entries.

        Abort these host queue entry and all host queue entries of jobs created
        by them.

        @param host_queue_entries: List of host queue entries we want to abort.
        """
        # This isn't completely immune to race conditions since it's not atomic,
        # but it should be safe given the scheduler's behavior.

        # TODO(milleral): crbug.com/230100
        # The |abort_host_queue_entries| rpc does nearly exactly this,
        # however, trying to re-use the code generates some horrible
        # circular import error.  I'd be nice to refactor things around
        # sometime so the code could be reused.

        # Fixpoint algorithm to find the whole tree of HQEs to abort to
        # minimize the total number of database queries:
        children = set()
        new_children = set(host_queue_entries)
        while new_children:
            children.update(new_children)
            new_child_ids = [hqe.job_id for hqe in new_children]
            new_children = HostQueueEntry.objects.filter(
                    job__parent_job__in=new_child_ids,
                    complete=False, aborted=False).all()
            # To handle circular parental relationships
            new_children = set(new_children) - children

        # Associate a user with the host queue entries that we're about
        # to abort so that we can look up who to blame for the aborts.
        now = datetime.now()
        user = User.current_user()
        aborted_hqes = [AbortedHostQueueEntry(queue_entry=hqe,
                aborted_by=user, aborted_on=now) for hqe in children]
        AbortedHostQueueEntry.objects.bulk_create(aborted_hqes)
        # Bulk update all of the HQEs to set the abort bit.
        child_ids = [hqe.id for hqe in children]
        HostQueueEntry.objects.filter(id__in=child_ids).update(aborted=True)


    def abort(self):
        """ Aborts this host queue entry.

        Abort this host queue entry and all host queue entries of jobs created by
        this one.

        """
        if not self.complete and not self.aborted:
            HostQueueEntry.abort_host_queue_entries([self])


    @classmethod
    def compute_full_status(cls, status, aborted, complete):
        """Returns a modified status msg if the host queue entry was aborted.

        @param cls: Implicit class object.
        @param status: The original status message.
        @param aborted: Whether the host queue entry was aborted.
        @param complete: Whether the host queue entry was completed.
        """
        if aborted and not complete:
            return 'Aborted (%s)' % status
        return status


    def full_status(self):
        """Returns the full status of this host queue entry, as a string."""
        return self.compute_full_status(self.status, self.aborted,
                                        self.complete)


    def _postprocess_object_dict(self, object_dict):
        object_dict['full_status'] = self.full_status()


    class Meta:
        """Metadata for class HostQueueEntry."""
        db_table = 'afe_host_queue_entries'



    def __unicode__(self):
        hostname = None
        if self.host:
            hostname = self.host.hostname
        return u"%s/%d (%d)" % (hostname, self.job.id, self.id)


class AbortedHostQueueEntry(dbmodels.Model, model_logic.ModelExtensions):
    """Represents an aborted host queue entry."""
    queue_entry = dbmodels.OneToOneField(HostQueueEntry, primary_key=True)
    aborted_by = dbmodels.ForeignKey(User)
    aborted_on = dbmodels.DateTimeField()

    objects = model_logic.ExtendedManager()


    def save(self, *args, **kwargs):
        self.aborted_on = datetime.now()
        super(AbortedHostQueueEntry, self).save(*args, **kwargs)

    class Meta:
        """Metadata for class AbortedHostQueueEntry."""
        db_table = 'afe_aborted_host_queue_entries'


class RecurringRun(dbmodels.Model, model_logic.ModelExtensions):
    """\
    job: job to use as a template
    owner: owner of the instantiated template
    start_date: Run the job at scheduled date
    loop_period: Re-run (loop) the job periodically
                 (in every loop_period seconds)
    loop_count: Re-run (loop) count
    """

    job = dbmodels.ForeignKey(Job)
    owner = dbmodels.ForeignKey(User)
    start_date = dbmodels.DateTimeField()
    loop_period = dbmodels.IntegerField(blank=True)
    loop_count = dbmodels.IntegerField(blank=True)

    objects = model_logic.ExtendedManager()

    class Meta:
        """Metadata for class RecurringRun."""
        db_table = 'afe_recurring_run'

    def __unicode__(self):
        return u'RecurringRun(job %s, start %s, period %s, count %s)' % (
            self.job.id, self.start_date, self.loop_period, self.loop_count)


class SpecialTask(dbmodels.Model, model_logic.ModelExtensions):
    """\
    Tasks to run on hosts at the next time they are in the Ready state. Use this
    for high-priority tasks, such as forced repair or forced reinstall.

    host: host to run this task on
    task: special task to run
    time_requested: date and time the request for this task was made
    is_active: task is currently running
    is_complete: task has finished running
    is_aborted: task was aborted
    time_started: date and time the task started
    time_finished: date and time the task finished
    queue_entry: Host queue entry waiting on this task (or None, if task was not
                 started in preparation of a job)
    """
    Task = enum.Enum('Verify', 'Cleanup', 'Repair', 'Reset', 'Provision',
                     string_values=True)

    host = dbmodels.ForeignKey(Host, blank=False, null=False)
    task = dbmodels.CharField(max_length=64, choices=Task.choices(),
                              blank=False, null=False)
    requested_by = dbmodels.ForeignKey(User)
    time_requested = dbmodels.DateTimeField(auto_now_add=True, blank=False,
                                            null=False)
    is_active = dbmodels.BooleanField(default=False, blank=False, null=False)
    is_complete = dbmodels.BooleanField(default=False, blank=False, null=False)
    is_aborted = dbmodels.BooleanField(default=False, blank=False, null=False)
    time_started = dbmodels.DateTimeField(null=True, blank=True)
    queue_entry = dbmodels.ForeignKey(HostQueueEntry, blank=True, null=True)
    success = dbmodels.BooleanField(default=False, blank=False, null=False)
    time_finished = dbmodels.DateTimeField(null=True, blank=True)

    objects = model_logic.ExtendedManager()


    def save(self, **kwargs):
        if self.queue_entry:
            self.requested_by = User.objects.get(
                    login=self.queue_entry.job.owner)
        super(SpecialTask, self).save(**kwargs)


    def execution_path(self):
        """Returns the execution path for a special task."""
        return server_utils.get_special_task_exec_path(
                self.host.hostname, self.id, self.task, self.time_requested)


    # property to emulate HostQueueEntry.status
    @property
    def status(self):
        """Returns a host queue entry status appropriate for a speical task."""
        return server_utils.get_special_task_status(
                self.is_complete, self.success, self.is_active)


    # property to emulate HostQueueEntry.started_on
    @property
    def started_on(self):
        """Returns the time at which this special task started."""
        return self.time_started


    @classmethod
    def schedule_special_task(cls, host, task):
        """Schedules a special task on a host if not already scheduled.

        @param cls: Implicit class object.
        @param host: The host to use.
        @param task: The task to schedule.
        """
        existing_tasks = SpecialTask.objects.filter(host__id=host.id, task=task,
                                                    is_active=False,
                                                    is_complete=False)
        if existing_tasks:
            return existing_tasks[0]

        special_task = SpecialTask(host=host, task=task,
                                   requested_by=User.current_user())
        special_task.save()
        return special_task


    def abort(self):
        """ Abort this special task."""
        self.is_aborted = True
        self.save()


    def activate(self):
        """
        Sets a task as active and sets the time started to the current time.
        """
        logging.info('Starting: %s', self)
        self.is_active = True
        self.time_started = datetime.now()
        self.save()


    def finish(self, success):
        """Sets a task as completed.

        @param success: Whether or not the task was successful.
        """
        logging.info('Finished: %s', self)
        self.is_active = False
        self.is_complete = True
        self.success = success
        if self.time_started:
            self.time_finished = datetime.now()
        self.save()


    class Meta:
        """Metadata for class SpecialTask."""
        db_table = 'afe_special_tasks'


    def __unicode__(self):
        result = u'Special Task %s (host %s, task %s, time %s)' % (
            self.id, self.host, self.task, self.time_requested)
        if self.is_complete:
            result += u' (completed)'
        elif self.is_active:
            result += u' (active)'

        return result


class StableVersion(dbmodels.Model, model_logic.ModelExtensions):

    board = dbmodels.CharField(max_length=255, unique=True)
    version = dbmodels.CharField(max_length=255)

    class Meta:
        """Metadata for class StableVersion."""
        db_table = 'afe_stable_versions'
