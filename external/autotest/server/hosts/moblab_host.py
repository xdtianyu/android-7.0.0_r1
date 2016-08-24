# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
import common
import logging
import os
import re
import tempfile
import time
import urllib2

from autotest_lib.client.common_lib import error, global_config
from autotest_lib.server.cros.dynamic_suite import frontend_wrappers
from autotest_lib.server.hosts import cros_host


AUTOTEST_INSTALL_DIR = global_config.global_config.get_config_value(
        'SCHEDULER', 'drone_installation_directory')
#'/usr/local/autotest'
SHADOW_CONFIG_PATH = '%s/shadow_config.ini' % AUTOTEST_INSTALL_DIR
ATEST_PATH = '%s/cli/atest' % AUTOTEST_INSTALL_DIR
SUBNET_DUT_SEARCH_RE = (
        r'/?.*\((?P<ip>192.168.231.*)\) at '
        '(?P<mac>[0-9a-fA-F][0-9a-fA-F]:){5}([0-9a-fA-F][0-9a-fA-F])')
MOBLAB_IMAGE_STORAGE = '/mnt/moblab/static'
MOBLAB_BOTO_LOCATION = '/home/moblab/.boto'
MOBLAB_LAUNCH_CONTROL_KEY_LOCATION = '/home/moblab/.launch_control_key'
MOBLAB_AUTODIR = '/usr/local/autodir'
DHCPD_LEASE_FILE = '/var/lib/dhcp/dhcpd.leases'
MOBLAB_SERVICES = ['moblab-scheduler-init',
                   'moblab-database-init',
                   'moblab-devserver-init',
                   'moblab-gsoffloader-init',
                   'moblab-gsoffloader_s-init']
MOBLAB_PROCESSES = ['apache2', 'dhcpd']
DUT_VERIFY_SLEEP_SECS = 5
DUT_VERIFY_TIMEOUT = 15 * 60
MOBLAB_TMP_DIR = '/mnt/moblab/tmp'


class MoblabHost(cros_host.CrosHost):
    """Moblab specific host class."""


    def _initialize(self, *args, **dargs):
        super(MoblabHost, self)._initialize(*args, **dargs)
        # Clear the Moblab Image Storage so that staging an image is properly
        # tested.
        if dargs.get('retain_image_storage') is not True:
            self.run('rm -rf %s/*' % MOBLAB_IMAGE_STORAGE)
        self._dhcpd_leasefile = None
        self.web_address = dargs.get('web_address', self.hostname)
        timeout_min = dargs.get('rpc_timeout_min', 1)
        self.afe = frontend_wrappers.RetryingAFE(timeout_min=timeout_min,
                                                 user='moblab',
                                                 server=self.web_address)
        self.tko = frontend_wrappers.RetryingTKO(timeout_min=timeout_min,
                                                 user='moblab',
                                                 server=self.web_address)


    @staticmethod
    def check_host(host, timeout=10):
        """
        Check if the given host is an moblab host.

        @param host: An ssh host representing a device.
        @param timeout: The timeout for the run command.


        @return: True if the host device has adb.

        @raises AutoservRunError: If the command failed.
        @raises AutoservSSHTimeout: Ssh connection has timed out.
        """
        try:
            result = host.run(
                    'grep -q moblab /etc/lsb-release && '
                    '! test -f /mnt/stateful_partition/.android_tester',
                    ignore_status=True, timeout=timeout)
        except (error.AutoservRunError, error.AutoservSSHTimeout):
            return False
        return result.exit_status == 0


    def install_boto_file(self, boto_path=''):
        """Install a boto file on the Moblab device.

        @param boto_path: Path to the boto file to install. If None, sends the
                          boto file in the current HOME directory.

        @raises error.TestError if the boto file does not exist.
        """
        if not boto_path:
            boto_path = os.path.join(os.getenv('HOME'), '.boto')
        if not os.path.exists(boto_path):
            raise error.TestError('Boto File:%s does not exist.' % boto_path)
        self.send_file(boto_path, MOBLAB_BOTO_LOCATION)
        self.run('chown moblab:moblab %s' % MOBLAB_BOTO_LOCATION)


    def get_autodir(self):
        """Return the directory to install autotest for client side tests."""
        return self.autodir or MOBLAB_AUTODIR


    def run_as_moblab(self, command, **kwargs):
        """Moblab commands should be ran as the moblab user not root.

        @param command: Command to run as user moblab.
        """
        command = "su - moblab -c '%s'" % command
        return self.run(command, **kwargs)


    def reboot(self, **dargs):
        """Reboot the Moblab Host and wait for its services to restart."""
        super(MoblabHost, self).reboot(**dargs)
        # In general after a reboot, we want to wait till the web frontend
        # and other Autotest services are up before executing. However should
        # something be wrong with these services, repair needs to be able
        # to continue and reimage the device.
        try:
            self.wait_afe_up()
        except (urllib2.HTTPError, urllib2.URLError) as e:
            logging.error('DUT has rebooted but AFE has failed to load.: %s',
                          e)


    def wait_afe_up(self, timeout_min=5):
        """Wait till the AFE is up and loaded.

        Attempt to reach the Moblab's AFE and database through its RPC
        interface.

        @param timeout_min: Minutes to wait for the AFE to respond. Default is
                            5 minutes.

        @raises urllib2.HTTPError if AFE does not respond within the timeout.
        """
        # Use a new AFE object with a longer timeout to wait for the AFE to
        # load.
        afe = frontend_wrappers.RetryingAFE(timeout_min=timeout_min,
                                            server=self.hostname)
        # Verify the AFE can handle a simple request.
        afe.get_hosts()


    def _wake_devices(self):
        """Search the subnet and attempt to ping any available duts.

        Fills up the arp table with entries about devices on the subnet.

        Either uses fping or directly pings devices listed in the dhcpd lease
        file.
        """
        fping_result = self.run('fping -g 192.168.231.100 192.168.231.120',
                                ignore_status=True)
        # If fping is not on the system, ping entries in the dhcpd lease file.
        if fping_result.exit_status == 127:
            leases = set(self.run('grep ^lease %s' % DHCPD_LEASE_FILE,
                                  ignore_status=True).stdout.splitlines())
            for lease in leases:
                ip = re.match('lease (?P<ip>.*) {', lease).groups('ip')
                self.run('ping %s -w 1' % ip, ignore_status=True)


    def add_dut(self, hostname):
        """Add a DUT hostname to the AFE.

        @param hostname: DUT hostname to add.
        """
        result = self.run_as_moblab('%s host create %s' % (ATEST_PATH,
                                                           hostname))
        logging.debug('atest host create output for host %s:\n%s',
                      hostname, result.stdout)


    def find_and_add_duts(self):
        """Discover DUTs on the testing subnet and add them to the AFE.

        Runs 'arp -a' on the Moblab host and parses the output to discover DUTs
        and if they are not already in the AFE, adds them.
        """
        self._wake_devices()
        existing_hosts = [host.hostname for host in self.afe.get_hosts()]
        arp_command = self.run('arp -a')
        for line in arp_command.stdout.splitlines():
            match = re.match(SUBNET_DUT_SEARCH_RE, line)
            if match:
                dut_hostname = match.group('ip')
                if dut_hostname in existing_hosts:
                    break
                self.add_dut(dut_hostname)


    def verify_software(self):
        """Verify working software on a Chrome OS system.

        Tests for the following conditions:
         1. All conditions tested by the parent version of this
            function.
         2. Ensures that Moblab services are running.
         3. Ensures that both DUTs successfully run Verify.

        """
        # In case cleanup or powerwash wiped the autodir, create an empty
        # directory.
        self.run('mkdir -p %s' % MOBLAB_AUTODIR)
        super(MoblabHost, self).verify_software()
        self._verify_moblab_services()
        self._verify_duts()


    def _verify_moblab_services(self):
        """Verify the required Moblab services are up and running.

        @raises AutoservError if any moblab service is not running.
        """
        for service in MOBLAB_SERVICES:
            if not self.upstart_status(service):
                raise error.AutoservError('Moblab service: %s is not running.'
                                          % service)
        for process in MOBLAB_PROCESSES:
            try:
                self.run('pgrep %s' % process)
            except error.AutoservRunError:
                raise error.AutoservError('Moblab process: %s is not running.'
                                          % process)


    def _verify_duts(self):
        """Verify the Moblab DUTs are up and running.

        @raises AutoservError if no DUTs are in the Ready State.
        """
        # Add the DUTs if they have not yet been added.
        self.find_and_add_duts()
        # Ensure a boto file is installed in case this Moblab was wiped in
        # repair.
        self.install_boto_file()
        hosts = self.afe.reverify_hosts()
        logging.debug('DUTs scheduled for reverification: %s', hosts)
        # Wait till all pending special tasks are completed.
        total_time = 0
        while (self.afe.get_special_tasks(is_complete=False) and
               total_time < DUT_VERIFY_TIMEOUT):
            total_time = total_time + DUT_VERIFY_SLEEP_SECS
            time.sleep(DUT_VERIFY_SLEEP_SECS)
        if not self.afe.get_hosts(status='Ready'):
            for host in self.afe.get_hosts():
                logging.error('DUT: %s Status: %s', host, host.status)
            raise error.AutoservError('Moblab has 0 Ready DUTs')


    def check_device(self):
        """Moblab specific check_device.

        Runs after a repair method has been attempted:
        * Reboots the moblab to start its services.
        * Creates the autotest client directory in case powerwash was used to
          wipe stateful and repair.
        * Reinstall the dhcp lease file if it was preserved.
        """
        # Moblab requires a reboot to initialize it's services prior to
        # verification.
        self.reboot()
        self.wait_afe_up()
        # Stateful could have been wiped so setup an empty autotest client
        # directory.
        self.run('mkdir -p %s' % self.get_autodir(), ignore_status=True)
        # Restore the dhcpd lease file if it was backed up.
        # TODO (sbasi) - Currently this is required for repairs but may need
        # to be expanded to regular installs as well.
        if self._dhcpd_leasefile:
            self.send_file(self._dhcpd_leasefile.name, DHCPD_LEASE_FILE)
            self.run('chown dhcp:dhcp %s' % DHCPD_LEASE_FILE)
        super(MoblabHost, self).check_device()


    def repair(self):
        """Moblab specific repair.

        Preserves the dhcp lease file prior to repairing the device.
        """
        try:
            temp = tempfile.TemporaryFile()
            self.get_file(DHCPD_LEASE_FILE, temp.name)
            self._dhcpd_leasefile = temp
        except error.AutoservRunError:
            logging.debug('Failed to retrieve dhcpd lease file from host.')
        super(MoblabHost, self).repair()


    def get_platform(self):
        """Determine the correct platform label for this host.

        For Moblab devices '_moblab' is appended.

        @returns a string representing this host's platform.
        """
        return super(MoblabHost, self).get_platform() + '_moblab'


    def make_tmp_dir(self, base=MOBLAB_TMP_DIR):
        """Creates a temporary directory.

        @param base: The directory where it should be created.

        @return Path to a newly created temporary directory.
        """
        self.run('mkdir -p %s' % base)
        return self.run('mktemp -d -p %s' % base).stdout.strip()
