# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
#
# Expects to be run in an environment with sudo and no interactive password
# prompt, such as within the Chromium OS development chroot.


"""This file provides core logic for servo verify/repair process."""


import httplib
import logging
import socket
import time
import xmlrpclib

from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib import global_config
from autotest_lib.client.common_lib import lsbrelease_utils
from autotest_lib.client.common_lib.cros import autoupdater
from autotest_lib.client.common_lib.cros import dev_server
from autotest_lib.client.common_lib.cros import retry
from autotest_lib.client.common_lib.cros.graphite import autotest_stats
from autotest_lib.client.common_lib.cros.network import ping_runner
from autotest_lib.client.cros import constants as client_constants
from autotest_lib.server import site_utils as server_site_utils
from autotest_lib.server.cros import dnsname_mangler
from autotest_lib.server.cros.servo import servo
from autotest_lib.server.cros.dynamic_suite import frontend_wrappers
from autotest_lib.server.hosts import ssh_host
from autotest_lib.site_utils.rpm_control_system import rpm_client


# Names of the host attributes in the database that represent the values for
# the servo_host and servo_port for a servo connected to the DUT.
SERVO_HOST_ATTR = 'servo_host'
SERVO_PORT_ATTR = 'servo_port'

_CONFIG = global_config.global_config

class ServoHostException(error.AutoservError):
    """This is the base class for exceptions raised by ServoHost."""
    pass


class ServoHostVerifyFailure(ServoHostException):
    """Raised when servo verification fails."""
    pass


class ServoHostRepairFailure(ServoHostException):
    """Raised when a repair method fails to repair a servo host."""
    pass


class ServoHostRepairMethodNA(ServoHostException):
    """Raised when a repair method is not applicable."""
    pass


class ServoHostRepairTotalFailure(ServoHostException):
    """Raised if all attempts to repair a servo host fail."""
    pass


def make_servo_hostname(dut_hostname):
    """Given a DUT's hostname, return the hostname of its servo.

    @param dut_hostname: hostname of a DUT.

    @return hostname of the DUT's servo.

    """
    host_parts = dut_hostname.split('.')
    host_parts[0] = host_parts[0] + '-servo'
    return '.'.join(host_parts)


class ServoHost(ssh_host.SSHHost):
    """Host class for a host that controls a servo, e.g. beaglebone."""

    # Timeout for getting the value of 'pwr_button'.
    PWR_BUTTON_CMD_TIMEOUT_SECS = 15
    # Timeout for rebooting servo host.
    REBOOT_TIMEOUT_SECS = 90
    HOST_DOWN_TIMEOUT_SECS = 60
    # Delay after rebooting for servod to become fully functional.
    REBOOT_DELAY_SECS = 20
    # Servod process name.
    SERVOD_PROCESS = 'servod'
    # Timeout for initializing servo signals.
    INITIALIZE_SERVO_TIMEOUT_SECS = 30

    _MAX_POWER_CYCLE_ATTEMPTS = 3
    _timer = autotest_stats.Timer('servo_host')


    def _initialize(self, servo_host='localhost', servo_port=9999,
                    required_by_test=True, is_in_lab=None, *args, **dargs):
        """Initialize a ServoHost instance.

        A ServoHost instance represents a host that controls a servo.

        @param servo_host: Name of the host where the servod process
                           is running.
        @param servo_port: Port the servod process is listening on.
        @param required_by_test: True if servo is required by test.
        @param is_in_lab: True if the servo host is in Cros Lab. Default is set
                          to None, for which utils.host_is_in_lab_zone will be
                          called to check if the servo host is in Cros lab.

        """
        super(ServoHost, self)._initialize(hostname=servo_host,
                                           *args, **dargs)
        if is_in_lab is None:
            self._is_in_lab = utils.host_is_in_lab_zone(self.hostname)
        else:
            self._is_in_lab = is_in_lab
        self._is_localhost = (self.hostname == 'localhost')
        remote = 'http://%s:%s' % (self.hostname, servo_port)
        self._servod_server = xmlrpclib.ServerProxy(remote)
        # Commands on the servo host must be run by the superuser. Our account
        # on Beaglebone is root, but locally we might be running as a
        # different user. If so - `sudo ' will have to be added to the
        # commands.
        if self._is_localhost:
            self._sudo_required = utils.system_output('id -u') != '0'
        else:
            self._sudo_required = False
        # Create a cache of Servo object. This must be called at the end of
        # _initialize to make sure all attributes are set.
        self._servo = None
        self.required_by_test = required_by_test
        try:
            self.verify()
        except Exception:
            if required_by_test:
                if not self.is_in_lab():
                    raise
                else:
                    self.repair()


    def is_in_lab(self):
        """Check whether the servo host is a lab device.

        @returns: True if the servo host is in Cros Lab, otherwise False.

        """
        return self._is_in_lab


    def is_localhost(self):
        """Checks whether the servo host points to localhost.

        @returns: True if it points to localhost, otherwise False.

        """
        return self._is_localhost


    def get_servod_server_proxy(self):
        """Return a proxy that can be used to communicate with servod server.

        @returns: An xmlrpclib.ServerProxy that is connected to the servod
                  server on the host.

        """
        return self._servod_server


    def get_wait_up_processes(self):
        """Get the list of local processes to wait for in wait_up.

        Override get_wait_up_processes in
        autotest_lib.client.common_lib.hosts.base_classes.Host.
        Wait for servod process to go up. Called by base class when
        rebooting the device.

        """
        processes = [self.SERVOD_PROCESS]
        return processes


    def _is_cros_host(self):
        """Check if a servo host is running chromeos.

        @return: True if the servo host is running chromeos.
            False if it isn't, or we don't have enough information.
        """
        try:
            result = self.run('grep -q CHROMEOS /etc/lsb-release',
                              ignore_status=True, timeout=10)
        except (error.AutoservRunError, error.AutoservSSHTimeout):
            return False
        return result.exit_status == 0


    def make_ssh_command(self, user='root', port=22, opts='', hosts_file=None,
                         connect_timeout=None, alive_interval=None):
        """Override default make_ssh_command to use tuned options.

        Tuning changes:
          - ConnectTimeout=30; maximum of 30 seconds allowed for an SSH
          connection failure. Consistency with remote_access.py.

          - ServerAliveInterval=180; which causes SSH to ping connection every
          180 seconds. In conjunction with ServerAliveCountMax ensures
          that if the connection dies, Autotest will bail out quickly.

          - ServerAliveCountMax=3; consistency with remote_access.py.

          - ConnectAttempts=4; reduce flakiness in connection errors;
          consistency with remote_access.py.

          - UserKnownHostsFile=/dev/null; we don't care about the keys.

          - SSH protocol forced to 2; needed for ServerAliveInterval.

        @param user User name to use for the ssh connection.
        @param port Port on the target host to use for ssh connection.
        @param opts Additional options to the ssh command.
        @param hosts_file Ignored.
        @param connect_timeout Ignored.
        @param alive_interval Ignored.

        @returns: An ssh command with the requested settings.

        """
        base_command = ('/usr/bin/ssh -a -x %s -o StrictHostKeyChecking=no'
                        ' -o UserKnownHostsFile=/dev/null -o BatchMode=yes'
                        ' -o ConnectTimeout=30 -o ServerAliveInterval=180'
                        ' -o ServerAliveCountMax=3 -o ConnectionAttempts=4'
                        ' -o Protocol=2 -l %s -p %d')
        return base_command % (opts, user, port)


    def _make_scp_cmd(self, sources, dest):
        """Format scp command.

        Given a list of source paths and a destination path, produces the
        appropriate scp command for encoding it. Remote paths must be
        pre-encoded. Overrides _make_scp_cmd in AbstractSSHHost
        to allow additional ssh options.

        @param sources: A list of source paths to copy from.
        @param dest: Destination path to copy to.

        @returns: An scp command that copies |sources| on local machine to
                  |dest| on the remote servo host.

        """
        command = ('scp -rq %s -o BatchMode=yes -o StrictHostKeyChecking=no '
                   '-o UserKnownHostsFile=/dev/null -P %d %s "%s"')
        return command % (self.master_ssh_option,
                          self.port, ' '.join(sources), dest)


    def run(self, command, timeout=3600, ignore_status=False,
            stdout_tee=utils.TEE_TO_LOGS, stderr_tee=utils.TEE_TO_LOGS,
            connect_timeout=30, options='', stdin=None, verbose=True, args=()):
        """Run a command on the servo host.

        Extends method `run` in SSHHost. If the servo host is a remote device,
        it will call `run` in SSHost without changing anything.
        If the servo host is 'localhost', it will call utils.system_output.

        @param command: The command line string.
        @param timeout: Time limit in seconds before attempting to
                        kill the running process. The run() function
                        will take a few seconds longer than 'timeout'
                        to complete if it has to kill the process.
        @param ignore_status: Do not raise an exception, no matter
                              what the exit code of the command is.
        @param stdout_tee/stderr_tee: Where to tee the stdout/stderr.
        @param connect_timeout: SSH connection timeout (in seconds)
                                Ignored if host is 'localhost'.
        @param options: String with additional ssh command options
                        Ignored if host is 'localhost'.
        @param stdin: Stdin to pass (a string) to the executed command.
        @param verbose: Log the commands.
        @param args: Sequence of strings to pass as arguments to command by
                     quoting them in " and escaping their contents if necessary.

        @returns: A utils.CmdResult object.

        @raises AutoservRunError if the command failed.
        @raises AutoservSSHTimeout SSH connection has timed out. Only applies
                when servo host is not 'localhost'.

        """
        run_args = {'command': command, 'timeout': timeout,
                    'ignore_status': ignore_status, 'stdout_tee': stdout_tee,
                    'stderr_tee': stderr_tee, 'stdin': stdin,
                    'verbose': verbose, 'args': args}
        if self.is_localhost():
            if self._sudo_required:
                run_args['command'] = 'sudo -n %s' % command
            try:
                return utils.run(**run_args)
            except error.CmdError as e:
                logging.error(e)
                raise error.AutoservRunError('command execution error',
                                             e.result_obj)
        else:
            run_args['connect_timeout'] = connect_timeout
            run_args['options'] = options
            return super(ServoHost, self).run(**run_args)


    @_timer.decorate
    def _check_servod(self):
        """A sanity check of the servod state."""
        msg_prefix = 'Servod error: %s'
        error_msg = None
        try:
            timeout, _ = retry.timeout(
                    self._servod_server.get, args=('pwr_button', ),
                    timeout_sec=self.PWR_BUTTON_CMD_TIMEOUT_SECS)
            if timeout:
                error_msg = msg_prefix % 'Request timed out.'
        except (socket.error, xmlrpclib.Error, httplib.BadStatusLine) as e:
            error_msg = msg_prefix % e
        if error_msg:
            raise ServoHostVerifyFailure(error_msg)


    def _check_servo_config(self):
        """Check if config file exists for servod.

        If servod config file does not exist, there is no need to verify if
        servo is working. The servo could be attached to a board not supported
        yet.

        @raises ServoHostVerifyFailure if /var/lib/servod/config does not exist.

        """
        if self._is_localhost:
            return
        try:
            self.run('test -f /var/lib/servod/config')
        except (error.AutoservRunError, error.AutoservSSHTimeout) as e:
            if not self._is_cros_host():
                logging.info('Ignoring servo config check failure, either %s '
                             'is not running chromeos or we cannot find enough '
                             'information about the host.', self.hostname)
                return
            raise ServoHostVerifyFailure(
                    'Servo config file check failed for %s: %s' %
                    (self.hostname, e))


    def _check_servod_status(self):
        """Check if servod process is running.

        If servod is not running, there is no need to verify if servo is
        working. Check the process before making any servod call can avoid
        long timeout that eventually fail any servod call.
        If the servo host is set to localhost, failure of servod status check
        will be ignored, as servo call may use ssh tunnel.

        @raises ServoHostVerifyFailure if servod process does not exist.

        """
        try:
            pids = [str(int(s)) for s in
                    self.run('pgrep servod').stdout.strip().split('\n')]
            logging.info('servod is running, PID=%s', ','.join(pids))
        except (error.AutoservRunError, error.AutoservSSHTimeout) as e:
            if self._is_localhost:
                logging.info('Ignoring servod status check failure. servo host '
                             'is set to localhost, servo call may use ssh '
                             'tunnel to go through.')
            else:
                raise ServoHostVerifyFailure(
                        'Servod status check failed for %s: %s' %
                        (self.hostname, e))


    def get_release_version(self):
        """Get the value of attribute CHROMEOS_RELEASE_VERSION from lsb-release.

        @returns The version string in lsb-release, under attribute
                 CHROMEOS_RELEASE_VERSION.
        """
        lsb_release_content = self.run(
                    'cat "%s"' % client_constants.LSB_RELEASE).stdout.strip()
        return lsbrelease_utils.get_chromeos_release_version(
                    lsb_release_content=lsb_release_content)


    @_timer.decorate
    def _update_image(self):
        """Update the image on the servo host, if needed.

        This method recognizes the following cases:
          * If the Host is not running Chrome OS, do nothing.
          * If a previously triggered update is now complete, reboot
            to the new version.
          * If the host is processing a previously triggered update,
            do nothing.
          * If the host is running a version of Chrome OS different
            from the default for servo Hosts, trigger an update, but
            don't wait for it to complete.

        @raises dev_server.DevServerException: If all the devservers are down.
        @raises site_utils.ParseBuildNameException: If the devserver returns
            an invalid build name.
        @raises autoupdater.ChromiumOSError: If something goes wrong in the
            checking update engine client status or applying an update.
        @raises AutoservRunError: If the update_engine_client isn't present on
            the host, and the host is a cros_host.

        """
        # servod could be running in a Ubuntu workstation.
        if not self._is_cros_host():
            logging.info('Not attempting an update, either %s is not running '
                         'chromeos or we cannot find enough information about '
                         'the host.', self.hostname)
            return

        if lsbrelease_utils.is_moblab():
            logging.info('Not attempting an update, %s is running moblab.',
                         self.hostname)
            return

        board = _CONFIG.get_config_value(
                'CROS', 'servo_board')
        afe = frontend_wrappers.RetryingAFE(timeout_min=5, delay_sec=10)
        target_version = afe.run('get_stable_version', board=board)
        build_pattern = _CONFIG.get_config_value(
                'CROS', 'stable_build_pattern')
        target_build = build_pattern % (board, target_version)
        target_build_number = server_site_utils.ParseBuildName(
                target_build)[3]
        ds = dev_server.ImageServer.resolve(self.hostname)
        url = ds.get_update_url(target_build)

        updater = autoupdater.ChromiumOSUpdater(update_url=url, host=self)
        current_build_number = self.get_release_version()
        status = updater.check_update_status()

        if status == autoupdater.UPDATER_NEED_REBOOT:
            logging.info('Rebooting beaglebone host %s with build %s',
                         self.hostname, current_build_number)
            kwargs = {
                'reboot_cmd': 'sleep 1 ; reboot & sleep 10; reboot -f',
                'fastsync': True,
                'label': None,
                'wait': False,
            }
            # Do not wait for reboot to complete. Otherwise, self.reboot call
            # will log reboot failure if servo does not come back. The logged
            # reboot failure will lead to test job failure. If the test does not
            # require servo, we don't want servo failure to fail the test with
            # error: `Host did not return from reboot` in status.log
            # If servo does not come back after reboot, exception needs to be
            # raised, so test requires servo should fail.
            self.reboot(**kwargs)
            if self.wait_up(timeout=120):
                current_build_number = self.get_release_version()
                logging.info('servo host %s back from reboot, with build %s',
                             self.hostname, current_build_number)
            else:
                raise error.AutoservHostError(
                            'servo host %s failed to come back from reboot.' %
                             self.hostname)

        if status in autoupdater.UPDATER_PROCESSING_UPDATE:
            logging.info('servo host %s already processing an update, update '
                         'engine client status=%s', self.hostname, status)
        elif current_build_number != target_build_number:
            logging.info('Using devserver url: %s to trigger update on '
                         'servo host %s, from %s to %s', url, self.hostname,
                         current_build_number, target_build_number)
            try:
                ds.stage_artifacts(target_build,
                                   artifacts=['full_payload'])
            except Exception as e:
                logging.error('Staging artifacts failed: %s', str(e))
                logging.error('Abandoning update for this cycle.')
            else:
                try:
                    updater.trigger_update()
                except autoupdater.RootFSUpdateError as e:
                    trigger_download_status = 'failed with %s' % str(e)
                    autotest_stats.Counter(
                            'servo_host.RootFSUpdateError').increment()
                else:
                    trigger_download_status = 'passed'
                logging.info('Triggered download and update %s for %s, '
                             'update engine currently in status %s',
                             trigger_download_status, self.hostname,
                             updater.check_update_status())
        else:
            logging.info('servo host %s does not require an update.',
                         self.hostname)


    def verify_software(self):
        """Update the servo host and verify it's in a good state.

        It overrides the base class function for verify_software.
        If an update is available, downloads and applies it. Then verifies:
            1) Whether basic servo command can run successfully.
            2) Whether USB is in a good state. crbug.com/225932

        @raises ServoHostVerifyFailure if servo host does not pass the checks.

        """
        logging.info('Applying an update to the servo host, if necessary.')
        self._update_image()
        self._check_servo_config()
        self._check_servod_status()

        # If servo is already initialized, we don't need to do it again, call
        # _check_servod should be enough.
        if self._servo:
            self._check_servod()
        else:
            self._servo = servo.Servo(servo_host=self)
            timeout, _ = retry.timeout(
                    self._servo.initialize_dut,
                    timeout_sec=self.INITIALIZE_SERVO_TIMEOUT_SECS)
            if timeout:
                raise ServoHostVerifyFailure('Servo initialize timed out.')
        logging.info('Sanity checks pass on servo host %s', self.hostname)


    def _repair_with_sysrq_reboot(self):
        """Reboot with magic SysRq key."""
        self.reboot(timeout=self.REBOOT_TIMEOUT_SECS,
                    label=None,
                    down_timeout=self.HOST_DOWN_TIMEOUT_SECS,
                    reboot_cmd='echo "b" > /proc/sysrq-trigger &',
                    fastsync=True)
        time.sleep(self.REBOOT_DELAY_SECS)


    def has_power(self):
        """Return whether or not the servo host is powered by PoE."""
        # TODO(fdeng): See crbug.com/302791
        # For now, assume all servo hosts in the lab have power.
        return self.is_in_lab()


    def power_cycle(self):
        """Cycle power to this host via PoE if it is a lab device.

        @raises ServoHostRepairFailure if it fails to power cycle the
                servo host.

        """
        if self.has_power():
            try:
                rpm_client.set_power(self.hostname, 'CYCLE')
            except (socket.error, xmlrpclib.Error,
                    httplib.BadStatusLine,
                    rpm_client.RemotePowerException) as e:
                raise ServoHostRepairFailure(
                        'Power cycling %s failed: %s' % (self.hostname, e))
        else:
            logging.info('Skipping power cycling, not a lab device.')


    def _powercycle_to_repair(self):
        """Power cycle the servo host using PoE.

        @raises ServoHostRepairFailure if it fails to fix the servo host.
        @raises ServoHostRepairMethodNA if it does not support power.

        """
        if not self.has_power():
            raise ServoHostRepairMethodNA('%s does not support power.' %
                                          self.hostname)
        logging.info('Attempting repair via PoE powercycle.')
        failed_cycles = 0
        self.power_cycle()
        while not self.wait_up(timeout=self.REBOOT_TIMEOUT_SECS):
            failed_cycles += 1
            if failed_cycles >= self._MAX_POWER_CYCLE_ATTEMPTS:
                raise ServoHostRepairFailure(
                        'Powercycled host %s %d times; device did not come back'
                        ' online.' % (self.hostname, failed_cycles))
            self.power_cycle()
        logging.info('Powercycling was successful after %d failures.',
                     failed_cycles)
        # Allow some time for servod to get started.
        time.sleep(self.REBOOT_DELAY_SECS)


    def repair(self):
        """Attempt to repair servo host.

        This overrides the base class function for repair.
        Note if the host is not in Cros Lab, the repair procedure
        will be skipped.

        @raises ServoHostRepairTotalFailure if all attempts fail.

        """
        if not self.is_in_lab():
            logging.warning('Skip repairing servo host %s: Not a lab device.',
                         self.hostname)
            return
        logging.info('Attempting to repair servo host %s.', self.hostname)
        # Reset the cache to guarantee servo initialization being called later.
        self._servo = None
        # TODO(dshi): add self._powercycle_to_repair back to repair_funcs
        # after crbug.com/336606 is fixed.
        repair_funcs = [self._repair_with_sysrq_reboot,]
        errors = []
        for repair_func in repair_funcs:
            counter_prefix = 'servo_host_repair.%s.' % repair_func.__name__
            try:
                repair_func()
                self.verify()
                autotest_stats.Counter(counter_prefix + 'SUCCEEDED').increment()
                return
            except ServoHostRepairMethodNA as e:
                logging.warning('Repair method NA: %s', e)
                autotest_stats.Counter(counter_prefix + 'RepairNA').increment()
                errors.append(str(e))
            except Exception as e:
                logging.warning('Failed to repair servo: %s', e)
                autotest_stats.Counter(counter_prefix + 'FAILED').increment()
                errors.append(str(e))
        autotest_stats.Counter('servo_host_repair.Full_Repair_Failed'). \
                increment()
        raise ServoHostRepairTotalFailure(
                'All attempts at repairing the servo failed:\n%s' %
                '\n'.join(errors))


    def get_servo(self):
        """Get the cached servo.Servo object.

        @return: a servo.Servo object.
        """
        return self._servo


def create_servo_host(dut, servo_args, try_lab_servo=False):
    """Create a ServoHost object.

    The `servo_args` parameter is a dictionary specifying optional
    Servo client parameter overrides (i.e. a specific host or port).
    When specified, the caller requires that an exception be raised
    unless both the ServoHost and the Servo are successfully
    created.

    There are three possible cases:
    1. If the DUT is in the Cros test lab then the ServoHost object
       is only created for the host in the lab.  Alternate host or
       port settings in `servo_host` will be ignored.
    2. When not case 1., but `servo_args` is not `None`, then create
       a ServoHost object using `servo_args`.
    3. Otherwise, return `None`.

    When the `try_lab_servo` parameter is false, it indicates that a
    ServoHost should not be created for a device in the Cros test
    lab.  The setting of `servo_args` takes precedence over the
    setting of `try_lab_servo`.

    @param dut: host name of the host that servo connects. It can be used to
                lookup the servo in test lab using naming convention.
    @param servo_args: A dictionary that contains args for creating
                       a ServoHost object,
                       e.g. {'servo_host': '172.11.11.111',
                             'servo_port': 9999}.
                       See comments above.
    @param try_lab_servo: Boolean. Whether to create ServoHost for a device
                          in test lab. See above.

    @returns: A ServoHost object or None. See comments above.

    """
    required_by_test = servo_args is not None
    if not utils.is_in_container():
        is_moblab = utils.is_moblab()
    else:
        is_moblab = _CONFIG.get_config_value(
                'SSP', 'is_moblab', type=bool, default=False)
    if not is_moblab:
        dut_is_hostname = not dnsname_mangler.is_ip_address(dut)
        if dut_is_hostname:
            lab_servo_hostname = make_servo_hostname(dut)
            is_in_lab = utils.host_is_in_lab_zone(lab_servo_hostname)
        else:
            is_in_lab = False
    else:
        # Servos on Moblab are not in the actual lab.
        is_in_lab = False
        afe = frontend_wrappers.RetryingAFE(timeout_min=5, delay_sec=10)
        hosts = afe.get_hosts(hostname=dut)
        if hosts and SERVO_HOST_ATTR in hosts[0].attributes:
            servo_args = {}
            servo_args[SERVO_HOST_ATTR] = hosts[0].attributes[SERVO_HOST_ATTR]
            servo_args[SERVO_PORT_ATTR] = hosts[0].attributes.get(
                    SERVO_PORT_ATTR, 9999)
            if (utils.is_in_container() and
                servo_args[SERVO_HOST_ATTR] in ['localhost', '127.0.0.1']):
                servo_args[SERVO_HOST_ATTR] = _CONFIG.get_config_value(
                        'SSP', 'host_container_ip', type=str, default=None)

    if not is_in_lab:
        if not required_by_test:
            return None
        return ServoHost(required_by_test=True, is_in_lab=False, **servo_args)
    elif servo_args is not None or try_lab_servo:
        # Technically, this duplicates the SSH ping done early in the servo
        # proxy initialization code.  However, this ping ends in a couple
        # seconds when if fails, rather than the 60 seconds it takes to decide
        # that an SSH ping has timed out.  Specifically, that timeout happens
        # when our servo DNS name resolves, but there is no host at that IP.
        # TODO(dshi): crbug.com/380773 Remove this ping check once the bug is
        #             fixed. Autotest should not try to verify servo if servo is
        #             not required for the test.
        ping_config = ping_runner.PingConfig(
                lab_servo_hostname, count=3,
                ignore_result=True, ignore_status=True)
        logging.info('Pinging servo at %s', lab_servo_hostname)
        host_is_up = ping_runner.PingRunner().ping(ping_config).received > 0
        if host_is_up:
            return ServoHost(servo_host=lab_servo_hostname, is_in_lab=is_in_lab,
                             required_by_test=required_by_test)
    else:
        return None
