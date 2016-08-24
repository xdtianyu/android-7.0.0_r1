#pylint: disable-msg=C0111

import cPickle
import logging
import os
import time

import common
from autotest_lib.scheduler import drone_utility, email_manager
from autotest_lib.client.bin import local_host
from autotest_lib.client.common_lib import error, global_config, utils
from autotest_lib.client.common_lib.cros.graphite import autotest_stats


AUTOTEST_INSTALL_DIR = global_config.global_config.get_config_value('SCHEDULER',
                                                 'drone_installation_directory')
DEFAULT_CONTAINER_PATH = global_config.global_config.get_config_value(
        'AUTOSERV', 'container_path')

class DroneUnreachable(Exception):
    """The drone is non-sshable."""
    pass


class _BaseAbstractDrone(object):
    """
    Attributes:
    * allowed_users: set of usernames allowed to use this drone.  if None,
            any user can use this drone.
    """
    def __init__(self, timestamp_remote_calls=True):
        """Instantiate an abstract drone.

        @param timestamp_remote_calls: If true, drone_utility is invoked with
            the --call_time option and the current time. Currently this is only
            used for testing.
        """
        self._calls = []
        self.hostname = None
        self.enabled = True
        self.max_processes = 0
        self.active_processes = 0
        self.allowed_users = None
        self._autotest_install_dir = AUTOTEST_INSTALL_DIR
        self._host = None
        self.timestamp_remote_calls = timestamp_remote_calls
        # If drone supports server-side packaging. The property support_ssp will
        # init self._support_ssp later.
        self._support_ssp = None


    def shutdown(self):
        pass


    @property
    def _drone_utility_path(self):
        return os.path.join(self._autotest_install_dir,
                            'scheduler', 'drone_utility.py')


    def used_capacity(self):
        """Gets the capacity used by this drone

        Returns a tuple of (percentage_full, -max_capacity). This is to aid
        direct comparisons, so that a 0/10 drone is considered less heavily
        loaded than a 0/2 drone.

        This value should never be used directly. It should only be used in
        direct comparisons using the basic comparison operators, or using the
        cmp() function.
        """
        if self.max_processes == 0:
            return (1.0, 0)
        return (float(self.active_processes) / self.max_processes,
                -self.max_processes)


    def usable_by(self, user):
        if self.allowed_users is None:
            return True
        return user in self.allowed_users


    def _execute_calls_impl(self, calls):
        if not self._host:
            raise ValueError('Drone cannot execute calls without a host.')
        drone_utility_cmd = self._drone_utility_path
        if self.timestamp_remote_calls:
            drone_utility_cmd = '%s --call_time %s' % (
                    drone_utility_cmd, time.time())
        logging.info("Running drone_utility on %s", self.hostname)
        result = self._host.run('python %s' % drone_utility_cmd,
                                stdin=cPickle.dumps(calls), stdout_tee=None,
                                connect_timeout=300)
        try:
            return cPickle.loads(result.stdout)
        except Exception: # cPickle.loads can throw all kinds of exceptions
            logging.critical('Invalid response:\n---\n%s\n---', result.stdout)
            raise


    def _execute_calls(self, calls):
        autotest_stats.Gauge('drone_execute_call_count').send(
                    self.hostname.replace('.', '_'), len(calls))
        return_message = self._execute_calls_impl(calls)
        for warning in return_message['warnings']:
            subject = 'Warning from drone %s' % self.hostname
            logging.warning(subject + '\n' + warning)
            email_manager.manager.enqueue_notify_email(subject, warning)
        return return_message['results']


    def get_calls(self):
        """Returns the calls queued against this drone.

        @return: A list of calls queued against the drone.
        """
        return self._calls


    def call(self, method, *args, **kwargs):
        return self._execute_calls(
            [drone_utility.call(method, *args, **kwargs)])


    def queue_call(self, method, *args, **kwargs):
        self._calls.append(drone_utility.call(method, *args, **kwargs))


    def clear_call_queue(self):
        self._calls = []


    def execute_queued_calls(self):
        if not self._calls:
            return
        results = self._execute_calls(self._calls)
        self.clear_call_queue()
        return results


    def set_autotest_install_dir(self, path):
        pass


    @property
    def support_ssp(self):
        """Check if the drone supports server-side packaging with container.

        @return: True if the drone supports server-side packaging with container
        """
        if not self._host:
            raise ValueError('Can not determine if drone supports server-side '
                             'packaging before host is set.')
        if self._support_ssp is None:
            try:
                # TODO(crbug.com/471316): We need a better way to check if drone
                # supports container, and install/upgrade base container. The
                # check of base container folder is not reliable and shall be
                # obsoleted once that bug is fixed.
                self._host.run('which lxc-start')
                # Test if base container is setup.
                base_container = os.path.join(DEFAULT_CONTAINER_PATH, 'base')
                # SSP uses privileged containers, sudo access is required. If
                # the process can't run sudo command without password, SSP can't
                # work properly. sudo command option -n will avoid user input.
                # If password is required, the command will fail and raise
                # AutoservRunError exception.
                self._host.run('sudo -n ls "%s"' %  base_container)
                self._support_ssp = True
            except (error.AutoservRunError, error.AutotestHostRunError):
                # Local drone raises AutotestHostRunError, while remote drone
                # raises AutoservRunError.
                self._support_ssp = False
        return self._support_ssp


SiteDrone = utils.import_site_class(
   __file__, 'autotest_lib.scheduler.site_drones',
   '_SiteAbstractDrone', _BaseAbstractDrone)


class _AbstractDrone(SiteDrone):
    pass


class _LocalDrone(_AbstractDrone):
    def __init__(self, timestamp_remote_calls=True):
        super(_LocalDrone, self).__init__(
                timestamp_remote_calls=timestamp_remote_calls)
        self.hostname = 'localhost'
        self._host = local_host.LocalHost()
        self._drone_utility = drone_utility.DroneUtility()


    def send_file_to(self, drone, source_path, destination_path,
                     can_fail=False):
        if drone.hostname == self.hostname:
            self.queue_call('copy_file_or_directory', source_path,
                            destination_path)
        else:
            self.queue_call('send_file_to', drone.hostname, source_path,
                            destination_path, can_fail)


class _RemoteDrone(_AbstractDrone):
    def __init__(self, hostname, timestamp_remote_calls=True):
        super(_RemoteDrone, self).__init__(
                timestamp_remote_calls=timestamp_remote_calls)
        self.hostname = hostname
        self._host = drone_utility.create_host(hostname)
        if not self._host.is_up():
            logging.error('Drone %s is unpingable, kicking out', hostname)
            raise DroneUnreachable


    def set_autotest_install_dir(self, path):
        self._autotest_install_dir = path


    def shutdown(self):
        super(_RemoteDrone, self).shutdown()
        self._host.close()


    def send_file_to(self, drone, source_path, destination_path,
                     can_fail=False):
        if drone.hostname == self.hostname:
            self.queue_call('copy_file_or_directory', source_path,
                            destination_path)
        elif isinstance(drone, _LocalDrone):
            drone.queue_call('get_file_from', self.hostname, source_path,
                             destination_path)
        else:
            self.queue_call('send_file_to', drone.hostname, source_path,
                            destination_path, can_fail)


def get_drone(hostname):
    """
    Use this factory method to get drone objects.
    """
    if hostname == 'localhost':
        return _LocalDrone()
    try:
        return _RemoteDrone(hostname)
    except DroneUnreachable:
        return None
