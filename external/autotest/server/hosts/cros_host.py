# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import functools
import json
import logging
import os
import re
import time

import common
from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib import autotemp
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib import global_config
from autotest_lib.client.common_lib import lsbrelease_utils
from autotest_lib.client.common_lib.cros import autoupdater
from autotest_lib.client.common_lib.cros import dev_server
from autotest_lib.client.common_lib.cros.graphite import autotest_es
from autotest_lib.client.common_lib.cros.graphite import autotest_stats
from autotest_lib.client.cros import constants as client_constants
from autotest_lib.client.cros import cros_ui
from autotest_lib.client.cros.audio import cras_utils
from autotest_lib.client.cros.input_playback import input_playback
from autotest_lib.client.cros.video import constants as video_test_constants
from autotest_lib.server import afe_utils
from autotest_lib.server import autoserv_parser
from autotest_lib.server import autotest
from autotest_lib.server import constants
from autotest_lib.server import crashcollect
from autotest_lib.server import utils as server_utils
from autotest_lib.server.cros import provision
from autotest_lib.server.cros.dynamic_suite import constants as ds_constants
from autotest_lib.server.cros.dynamic_suite import tools, frontend_wrappers
from autotest_lib.server.cros.faft.config.config import Config as FAFTConfig
from autotest_lib.server.cros.servo import plankton
from autotest_lib.server.hosts import abstract_ssh
from autotest_lib.server.hosts import chameleon_host
from autotest_lib.server.hosts import plankton_host
from autotest_lib.server.hosts import servo_host
from autotest_lib.site_utils.rpm_control_system import rpm_client


CONFIG = global_config.global_config

LUCID_SLEEP_BOARDS = ['samus', 'lulu']

# A file to indicate provision failure and require Repair job to powerwash the
# dut.
PROVISION_FAILED = '/var/tmp/provision_failed'

class FactoryImageCheckerException(error.AutoservError):
    """Exception raised when an image is a factory image."""
    pass


class CrosHost(abstract_ssh.AbstractSSHHost):
    """Chromium OS specific subclass of Host."""

    VERSION_PREFIX = provision.CROS_VERSION_PREFIX

    _parser = autoserv_parser.autoserv_parser
    _AFE = frontend_wrappers.RetryingAFE(timeout_min=5, delay_sec=10)

    # Timeout values (in seconds) associated with various Chrome OS
    # state changes.
    #
    # In general, a good rule of thumb is that the timeout can be up
    # to twice the typical measured value on the slowest platform.
    # The times here have not necessarily been empirically tested to
    # meet this criterion.
    #
    # SLEEP_TIMEOUT:  Time to allow for suspend to memory.
    # RESUME_TIMEOUT: Time to allow for resume after suspend, plus
    #   time to restart the netwowrk.
    # SHUTDOWN_TIMEOUT: Time to allow for shut down.
    # BOOT_TIMEOUT: Time to allow for boot from power off.  Among
    #   other things, this must account for the 30 second dev-mode
    #   screen delay, time to start the network on the DUT, and the
    #   ssh timeout of 120 seconds.
    # USB_BOOT_TIMEOUT: Time to allow for boot from a USB device,
    #   including the 30 second dev-mode delay and time to start the
    #   network.
    # INSTALL_TIMEOUT: Time to allow for chromeos-install.
    # POWERWASH_BOOT_TIMEOUT: Time to allow for a reboot that
    #   includes powerwash.

    SLEEP_TIMEOUT = 2
    RESUME_TIMEOUT = 10
    SHUTDOWN_TIMEOUT = 10
    BOOT_TIMEOUT = 150
    USB_BOOT_TIMEOUT = 300
    INSTALL_TIMEOUT = 480
    POWERWASH_BOOT_TIMEOUT = 60

    # Minimum OS version that supports server side packaging. Older builds may
    # not have server side package built or with Autotest code change to support
    # server-side packaging.
    MIN_VERSION_SUPPORT_SSP = CONFIG.get_config_value(
            'AUTOSERV', 'min_version_support_ssp', type=int)

    # REBOOT_TIMEOUT: How long to wait for a reboot.
    #
    # We have a long timeout to ensure we don't flakily fail due to other
    # issues. Shorter timeouts are vetted in platform_RebootAfterUpdate.
    # TODO(sbasi - crbug.com/276094) Restore to 5 mins once the 'host did not
    # return from reboot' bug is solved.
    REBOOT_TIMEOUT = 480

    # _USB_POWER_TIMEOUT: Time to allow for USB to power toggle ON and OFF.
    # _POWER_CYCLE_TIMEOUT: Time to allow for manual power cycle.
    _USB_POWER_TIMEOUT = 5
    _POWER_CYCLE_TIMEOUT = 10

    _RPM_RECOVERY_BOARDS = CONFIG.get_config_value('CROS',
            'rpm_recovery_boards', type=str).split(',')

    _MAX_POWER_CYCLE_ATTEMPTS = 6
    _LAB_MACHINE_FILE = '/mnt/stateful_partition/.labmachine'
    _RPM_HOSTNAME_REGEX = ('chromeos(\d+)(-row(\d+))?-rack(\d+[a-z]*)'
                           '-host(\d+)')
    _LIGHTSENSOR_FILES = [ "in_illuminance0_input",
                           "in_illuminance_input",
                           "in_illuminance0_raw",
                           "in_illuminance_raw",
                           "illuminance0_input"]
    _LIGHTSENSOR_SEARCH_DIR = '/sys/bus/iio/devices'
    _LABEL_FUNCTIONS = []
    _DETECTABLE_LABELS = []
    label_decorator = functools.partial(server_utils.add_label_detector,
                                        _LABEL_FUNCTIONS,
                                        _DETECTABLE_LABELS)

    # Constants used in ping_wait_up() and ping_wait_down().
    #
    # _PING_WAIT_COUNT is the approximate number of polling
    # cycles to use when waiting for a host state change.
    #
    # _PING_STATUS_DOWN and _PING_STATUS_UP are names used
    # for arguments to the internal _ping_wait_for_status()
    # method.
    _PING_WAIT_COUNT = 40
    _PING_STATUS_DOWN = False
    _PING_STATUS_UP = True

    # Allowed values for the power_method argument.

    # POWER_CONTROL_RPM: Passed as default arg for power_off/on/cycle() methods.
    # POWER_CONTROL_SERVO: Used in set_power() and power_cycle() methods.
    # POWER_CONTROL_MANUAL: Used in set_power() and power_cycle() methods.
    POWER_CONTROL_RPM = 'RPM'
    POWER_CONTROL_SERVO = 'servoj10'
    POWER_CONTROL_MANUAL = 'manual'

    POWER_CONTROL_VALID_ARGS = (POWER_CONTROL_RPM,
                                POWER_CONTROL_SERVO,
                                POWER_CONTROL_MANUAL)

    _RPM_OUTLET_CHANGED = 'outlet_changed'

    # URL pattern to download firmware image.
    _FW_IMAGE_URL_PATTERN = CONFIG.get_config_value(
            'CROS', 'firmware_url_pattern', type=str)

    # File that has a list of directories to be collected
    _LOGS_TO_COLLECT_FILE = os.path.join(
            common.client_dir, 'common_lib', 'logs_to_collect')

    # Prefix of logging message w.r.t. crash collection
    _CRASHLOGS_PREFIX = 'collect_crashlogs'

    # Time duration waiting for host up/down check
    _CHECK_HOST_UP_TIMEOUT_SECS = 15

    # A command that interacts with kernel and hardware (e.g., rm, mkdir, etc)
    # might not be completely done deep through the hardware when the machine
    # is powered down right after the command returns.
    # We should wait for a few seconds to make them done. Finger crossed.
    _SAFE_WAIT_SECS = 10


    @staticmethod
    def check_host(host, timeout=10):
        """
        Check if the given host is a chrome-os host.

        @param host: An ssh host representing a device.
        @param timeout: The timeout for the run command.

        @return: True if the host device is chromeos.

        """
        try:
            result = host.run(
                    'grep -q CHROMEOS /etc/lsb-release && '
                    '! test -f /mnt/stateful_partition/.android_tester && '
                    '! grep -q moblab /etc/lsb-release',
                    ignore_status=True, timeout=timeout)
        except (error.AutoservRunError, error.AutoservSSHTimeout):
            return False
        return result.exit_status == 0


    @staticmethod
    def _extract_arguments(args_dict, key_subset):
        """Extract options from `args_dict` and return a subset result.

        Take the provided dictionary of argument options and return
        a subset that represent standard arguments needed to construct
        a test-assistant object (chameleon or servo) for a host. The
        intent is to provide standard argument processing from
        CrosHost for tests that require a test-assistant board
        to operate.

        @param args_dict Dictionary from which to extract the arguments.
        @param key_subset Tuple of keys to extract from the args_dict, e.g.
          ('servo_host', 'servo_port').
        """
        result = {}
        for arg in key_subset:
            if arg in args_dict:
                result[arg] = args_dict[arg]
        return result


    @staticmethod
    def get_chameleon_arguments(args_dict):
        """Extract chameleon options from `args_dict` and return the result.

        Recommended usage:
        ~~~~~~~~
            args_dict = utils.args_to_dict(args)
            chameleon_args = hosts.CrosHost.get_chameleon_arguments(args_dict)
            host = hosts.create_host(machine, chameleon_args=chameleon_args)
        ~~~~~~~~

        @param args_dict Dictionary from which to extract the chameleon
          arguments.
        """
        return CrosHost._extract_arguments(
                args_dict, ('chameleon_host', 'chameleon_port'))


    @staticmethod
    def get_plankton_arguments(args_dict):
        """Extract chameleon options from `args_dict` and return the result.

        Recommended usage:
        ~~~~~~~~
            args_dict = utils.args_to_dict(args)
            plankon_args = hosts.CrosHost.get_plankton_arguments(args_dict)
            host = hosts.create_host(machine, plankton_args=polankton_args)
        ~~~~~~~~

        @param args_dict Dictionary from which to extract the plankton
          arguments.
        """
        args = CrosHost._extract_arguments(
                args_dict, ('plankton_host', 'plankton_port'))
        return args


    @staticmethod
    def get_servo_arguments(args_dict):
        """Extract servo options from `args_dict` and return the result.

        Recommended usage:
        ~~~~~~~~
            args_dict = utils.args_to_dict(args)
            servo_args = hosts.CrosHost.get_servo_arguments(args_dict)
            host = hosts.create_host(machine, servo_args=servo_args)
        ~~~~~~~~

        @param args_dict Dictionary from which to extract the servo
          arguments.
        """
        return CrosHost._extract_arguments(
                args_dict, ('servo_host', 'servo_port'))


    def _initialize(self, hostname, chameleon_args=None, servo_args=None, plankton_args=None,
                    try_lab_servo=False, ssh_verbosity_flag='', ssh_options='',
                    *args, **dargs):
        """Initialize superclasses, |self.chameleon|, and |self.servo|.

        This method will attempt to create the test-assistant object
        (chameleon/servo) when it is needed by the test. Check
        the docstring of chameleon_host.create_chameleon_host and
        servo_host.create_servo_host for how this is determined.

        @param hostname: Hostname of the dut.
        @param chameleon_args: A dictionary that contains args for creating
                               a ChameleonHost. See chameleon_host for details.
        @param servo_args: A dictionary that contains args for creating
                           a ServoHost object. See servo_host for details.
        @param try_lab_servo: Boolean, False indicates that ServoHost should
                              not be created for a device in Cros test lab.
                              See servo_host for details.
        @param ssh_verbosity_flag: String, to pass to the ssh command to control
                                   verbosity.
        @param ssh_options: String, other ssh options to pass to the ssh
                            command.
        """
        super(CrosHost, self)._initialize(hostname=hostname,
                                          *args, **dargs)
        # self.env is a dictionary of environment variable settings
        # to be exported for commands run on the host.
        # LIBC_FATAL_STDERR_ can be useful for diagnosing certain
        # errors that might happen.
        self.env['LIBC_FATAL_STDERR_'] = '1'
        self._ssh_verbosity_flag = ssh_verbosity_flag
        self._ssh_options = ssh_options
        # TODO(fdeng): We need to simplify the
        # process of servo and servo_host initialization.
        # crbug.com/298432
        self._servo_host =  servo_host.create_servo_host(
                dut=self.hostname, servo_args=servo_args,
                try_lab_servo=try_lab_servo)
        # TODO(waihong): Do the simplication on Chameleon too.
        self._chameleon_host = chameleon_host.create_chameleon_host(
                dut=self.hostname, chameleon_args=chameleon_args)
        # Add plankton host if plankton args were added on command line
        self._plankton_host = plankton_host.create_plankton_host(plankton_args)

        if self._servo_host is not None:
            self.servo = self._servo_host.get_servo()
        else:
            self.servo = None

        if self._chameleon_host:
            self.chameleon = self._chameleon_host.create_chameleon_board()
        else:
            self.chameleon = None

        if self._plankton_host:
            self.plankton_servo = self._plankton_host.get_servo()
            logging.info('plankton_servo: %r', self.plankton_servo)
            # Create the plankton object used to access the ec uart
            self.plankton = plankton.Plankton(self.plankton_servo,
                    self._plankton_host.get_servod_server_proxy())
        else:
            self.plankton = None


    def get_repair_image_name(self, image_type='cros'):
        """Generate a image_name from variables in the global config.

        image_type is used to differentiate different images. Default is CrOS,
        in which case, repair image's name follows the naming convention defined
        in global setting CROS/stable_build_pattern.
        If the image_type is not `cros`, the repair image will be looked up
        using key `board_name/image_type`, e.g., daisy_spring/firmware.

        @param image_type: Type of the image. Default is `cros`.

        @returns a str of $board-version/$BUILD. Returns None if stable version
                for the board and the default are both not set, e.g., stable
                firmware version for a new board.

        """
        board = self._get_board_from_afe()
        if board is None:
            raise error.AutoservError('DUT has no board attribute, '
                                      'cannot be repaired.')
        if image_type != 'cros':
            board = '%s/%s' % (board, image_type)
        stable_version = afe_utils.get_stable_version(board=board)
        if image_type == 'cros':
            build_pattern = CONFIG.get_config_value(
                    'CROS', 'stable_build_pattern')
            stable_version = build_pattern % (board, stable_version)
        elif image_type == 'firmware':
            # If firmware stable version is not specified, `stable_version`
            # from the RPC is the default stable version for CrOS image.
            # firmware stable version must be from firmware branch, thus its
            # value must be like board-firmware/R31-1234.0.0. Check if
            # firmware exists in the stable version, if not, return None.
            if 'firmware' not in stable_version:
                return None
        return stable_version


    def lookup_job_repo_url(self):
        """Looks up the job_repo_url for the host.

        @returns job_repo_url from AFE or None if not found.

        @raises KeyError if the host does not have a job_repo_url
        """
        hosts = self._AFE.get_hosts(hostname=self.hostname)
        if hosts and ds_constants.JOB_REPO_URL in hosts[0].attributes:
            return hosts[0].attributes[ds_constants.JOB_REPO_URL]
        else:
            return None


    def clear_job_repo_url(self):
        """Clear host attribute job_repo_url."""
        if not afe_utils.host_in_lab(self):
            return
        self.update_job_repo_url(None, None)


    def update_job_repo_url(self, devserver_url, image_name):
        """
        Updates the job_repo_url host attribute and asserts it's value.

        @param devserver_url: The devserver to use in the job_repo_url.
        @param image_name: The name of the image to use in the job_repo_url.

        @raises AutoservError: If we failed to update the job_repo_url.
        """
        repo_url = None
        if devserver_url and image_name:
            repo_url = tools.get_package_url(devserver_url, image_name)
        self._AFE.set_host_attribute(ds_constants.JOB_REPO_URL, repo_url,
                                     hostname=self.hostname)
        if self.lookup_job_repo_url() != repo_url:
            raise error.AutoservError('Failed to update job_repo_url with %s, '
                                      'host %s' % (repo_url, self.hostname))


    def add_job_repo_url(self, image_name):
        """Add cros_version labels and host attribute job_repo_url.

        @param image_name: The name of the image e.g.
                lumpy-release/R27-3837.0.0

        """
        if not afe_utils.host_in_lab(self):
            return

        devserver_url = dev_server.ImageServer.resolve(image_name,
                                                       self.hostname).url()
        self.update_job_repo_url(devserver_url, image_name)


    def verify_job_repo_url(self, tag=''):
        """
        Make sure job_repo_url of this host is valid.

        Eg: The job_repo_url "http://lmn.cd.ab.xyx:8080/static/\
        lumpy-release/R29-4279.0.0/autotest/packages" claims to have the
        autotest package for lumpy-release/R29-4279.0.0. If this isn't the case,
        download and extract it. If the devserver embedded in the url is
        unresponsive, update the job_repo_url of the host after staging it on
        another devserver.

        @param job_repo_url: A url pointing to the devserver where the autotest
            package for this build should be staged.
        @param tag: The tag from the server job, in the format
                    <job_id>-<user>/<hostname>, or <hostless> for a server job.

        @raises DevServerException: If we could not resolve a devserver.
        @raises AutoservError: If we're unable to save the new job_repo_url as
            a result of choosing a new devserver because the old one failed to
            respond to a health check.
        @raises urllib2.URLError: If the devserver embedded in job_repo_url
                                  doesn't respond within the timeout.
        """
        job_repo_url = self.lookup_job_repo_url()
        if not job_repo_url:
            logging.warning('No job repo url set on host %s', self.hostname)
            return

        logging.info('Verifying job repo url %s', job_repo_url)
        devserver_url, image_name = tools.get_devserver_build_from_package_url(
            job_repo_url)

        ds = dev_server.ImageServer(devserver_url)

        logging.info('Staging autotest artifacts for %s on devserver %s',
            image_name, ds.url())

        start_time = time.time()
        ds.stage_artifacts(image_name, ['autotest_packages'])
        stage_time = time.time() - start_time

        # Record how much of the verification time comes from a devserver
        # restage. If we're doing things right we should not see multiple
        # devservers for a given board/build/branch path.
        try:
            board, build_type, branch = server_utils.ParseBuildName(
                                                image_name)[:3]
        except server_utils.ParseBuildNameException:
            pass
        else:
            devserver = devserver_url[
                devserver_url.find('/') + 2:devserver_url.rfind(':')]
            stats_key = {
                'board': board,
                'build_type': build_type,
                'branch': branch,
                'devserver': devserver.replace('.', '_'),
            }
            autotest_stats.Gauge('verify_job_repo_url').send(
                '%(board)s.%(build_type)s.%(branch)s.%(devserver)s' % stats_key,
                stage_time)


    def stage_server_side_package(self, image=None):
        """Stage autotest server-side package on devserver.

        @param image: Full path of an OS image to install or a build name.

        @return: A url to the autotest server-side package.
        """
        if image:
            image_name = tools.get_build_from_image(image)
            if not image_name:
                raise error.AutoservError(
                        'Failed to parse build name from %s' % image)
            ds = dev_server.ImageServer.resolve(image_name)
        else:
            job_repo_url = self.lookup_job_repo_url()
            if job_repo_url:
                devserver_url, image_name = (
                    tools.get_devserver_build_from_package_url(job_repo_url))
                ds = dev_server.ImageServer(devserver_url)
            else:
                labels = self._AFE.get_labels(
                        name__startswith=ds_constants.VERSION_PREFIX,
                        host__hostname=self.hostname)
                if not labels:
                    raise error.AutoservError(
                            'Failed to stage server-side package. The host has '
                            'no job_report_url attribute or version label.')
                image_name = labels[0].name[len(ds_constants.VERSION_PREFIX):]
                ds = dev_server.ImageServer.resolve(image_name)

        # Get the OS version of the build, for any build older than
        # MIN_VERSION_SUPPORT_SSP, server side packaging is not supported.
        match = re.match('.*/R\d+-(\d+)\.', image_name)
        if match and int(match.group(1)) < self.MIN_VERSION_SUPPORT_SSP:
            logging.warn('Build %s is older than %s. Server side packaging is '
                         'disabled.', image_name, self.MIN_VERSION_SUPPORT_SSP)
            return None

        ds.stage_artifacts(image_name, ['autotest_server_package'])
        return '%s/static/%s/%s' % (ds.url(), image_name,
                                    'autotest_server_package.tar.bz2')


    def _try_stateful_update(self, update_url, force_update, updater):
        """Try to use stateful update to initialize DUT.

        When DUT is already running the same version that machine_install
        tries to install, stateful update is a much faster way to clean up
        the DUT for testing, compared to a full reimage. It is implemeted
        by calling autoupdater.run_update, but skipping updating root, as
        updating the kernel is time consuming and not necessary.

        @param update_url: url of the image.
        @param force_update: Set to True to update the image even if the DUT
            is running the same version.
        @param updater: ChromiumOSUpdater instance used to update the DUT.
        @returns: True if the DUT was updated with stateful update.

        """
        # Stop service ap-update-manager to prevent rebooting during autoupdate.
        # The service is used in jetstream boards, but not other CrOS devices.
        self.run('sudo stop ap-update-manager', ignore_status=True)

        # TODO(jrbarnette):  Yes, I hate this re.match() test case.
        # It's better than the alternative:  see crbug.com/360944.
        image_name = autoupdater.url_to_image_name(update_url)
        release_pattern = r'^.*-release/R[0-9]+-[0-9]+\.[0-9]+\.0$'
        if not re.match(release_pattern, image_name):
            return False
        if not updater.check_version():
            return False
        if not force_update:
            logging.info('Canceling stateful update because the new and '
                         'old versions are the same.')
            return False
        # Following folders should be rebuilt after stateful update.
        # A test file is used to confirm each folder gets rebuilt after
        # the stateful update.
        folders_to_check = ['/var', '/home', '/mnt/stateful_partition']
        test_file = '.test_file_to_be_deleted'
        for folder in folders_to_check:
            touch_path = os.path.join(folder, test_file)
            self.run('touch %s' % touch_path)

        updater.run_update(update_root=False)

        # Reboot to complete stateful update.
        self.reboot(timeout=self.REBOOT_TIMEOUT, wait=True)
        check_file_cmd = 'test -f %s; echo $?'
        for folder in folders_to_check:
            test_file_path = os.path.join(folder, test_file)
            result = self.run(check_file_cmd % test_file_path,
                              ignore_status=True)
            if result.exit_status == 1:
                return False
        return True


    def _post_update_processing(self, updater, expected_kernel=None):
        """After the DUT is updated, confirm machine_install succeeded.

        @param updater: ChromiumOSUpdater instance used to update the DUT.
        @param expected_kernel: kernel expected to be active after reboot,
            or `None` to skip rollback checking.

        """
        # Touch the lab machine file to leave a marker that
        # distinguishes this image from other test images.
        # Afterwards, we must re-run the autoreboot script because
        # it depends on the _LAB_MACHINE_FILE.
        autoreboot_cmd = ('FILE="%s" ; [ -f "$FILE" ] || '
                          '( touch "$FILE" ; start autoreboot )')
        self.run(autoreboot_cmd % self._LAB_MACHINE_FILE)
        updater.verify_boot_expectations(
                expected_kernel, rollback_message=
                'Build %s failed to boot on %s; system rolled back to previous '
                'build' % (updater.update_version, self.hostname))
        # Check that we've got the build we meant to install.
        if not updater.check_version_to_confirm_install():
            raise autoupdater.ChromiumOSError(
                'Failed to update %s to build %s; found build '
                '%s instead' % (self.hostname,
                                updater.update_version,
                                self.get_release_version()))

        logging.debug('Cleaning up old autotest directories.')
        try:
            installed_autodir = autotest.Autotest.get_installed_autodir(self)
            self.run('rm -rf ' + installed_autodir)
        except autotest.AutodirNotFoundError:
            logging.debug('No autotest installed directory found.')


    def _stage_image_for_update(self, image_name=None):
        """Stage a build on a devserver and return the update_url and devserver.

        @param image_name: a name like lumpy-release/R27-3837.0.0
        @returns a tuple with an update URL like:
            http://172.22.50.205:8082/update/lumpy-release/R27-3837.0.0
            and the devserver instance.
        """
        if not image_name:
            image_name = self.get_repair_image_name()

        logging.info('Staging build for AU: %s', image_name)
        devserver = dev_server.ImageServer.resolve(image_name, self.hostname)
        devserver.trigger_download(image_name, synchronous=False)
        return (tools.image_url_pattern() % (devserver.url(), image_name),
                devserver)


    def stage_image_for_servo(self, image_name=None):
        """Stage a build on a devserver and return the update_url.

        @param image_name: a name like lumpy-release/R27-3837.0.0
        @returns an update URL like:
            http://172.22.50.205:8082/update/lumpy-release/R27-3837.0.0
        """
        if not image_name:
            image_name = self.get_repair_image_name()
        logging.info('Staging build for servo install: %s', image_name)
        devserver = dev_server.ImageServer.resolve(image_name, self.hostname)
        devserver.stage_artifacts(image_name, ['test_image'])
        return devserver.get_test_image_url(image_name)


    def stage_factory_image_for_servo(self, image_name):
        """Stage a build on a devserver and return the update_url.

        @param image_name: a name like <baord>/4262.204.0

        @return: An update URL, eg:
            http://<devserver>/static/canary-channel/\
            <board>/4262.204.0/factory_test/chromiumos_factory_image.bin

        @raises: ValueError if the factory artifact name is missing from
                 the config.

        """
        if not image_name:
            logging.error('Need an image_name to stage a factory image.')
            return

        factory_artifact = CONFIG.get_config_value(
                'CROS', 'factory_artifact', type=str, default='')
        if not factory_artifact:
            raise ValueError('Cannot retrieve the factory artifact name from '
                             'autotest config, and hence cannot stage factory '
                             'artifacts.')

        logging.info('Staging build for servo install: %s', image_name)
        devserver = dev_server.ImageServer.resolve(image_name, self.hostname)
        devserver.stage_artifacts(
                image_name,
                [factory_artifact],
                archive_url=None)

        return tools.factory_image_url_pattern() % (devserver.url(), image_name)


    def machine_install(self, update_url=None, force_update=False,
                        local_devserver=False, repair=False,
                        force_full_update=False):
        """Install the DUT.

        Use stateful update if the DUT is already running the same build.
        Stateful update does not update kernel and tends to run much faster
        than a full reimage. If the DUT is running a different build, or it
        failed to do a stateful update, full update, including kernel update,
        will be applied to the DUT.

        Once a host enters machine_install its host attribute job_repo_url
        (used for package install) will be removed and then updated.

        @param update_url: The url to use for the update
                pattern: http://$devserver:###/update/$build
                If update_url is None and repair is True we will install the
                stable image listed in afe_stable_versions table. If the table
                is not setup, global_config value under CROS.stable_cros_version
                will be used instead.
        @param force_update: Force an update even if the version installed
                is the same. Default:False
        @param local_devserver: Used by test_that to allow people to
                use their local devserver. Default: False
        @param repair: Forces update to repair image. Implies force_update.
        @param force_full_update: If True, do not attempt to run stateful
                update, force a full reimage. If False, try stateful update
                first when the dut is already installed with the same version.
        @raises autoupdater.ChromiumOSError

        @returns Name of the image installed.
        """
        devserver = None
        if repair:
            update_url, devserver = self._stage_image_for_update()
            force_update = True

        if not update_url and not self._parser.options.image:
            raise error.AutoservError(
                    'There is no update URL, nor a method to get one.')

        if not update_url and self._parser.options.image:
            # This is the base case where we have no given update URL i.e.
            # dynamic suites logic etc. This is the most flexible case where we
            # can serve an update from any of our fleet of devservers.
            requested_build = self._parser.options.image
            if not requested_build.startswith('http://'):
                logging.debug('Update will be staged for this installation')
                update_url, devserver = self._stage_image_for_update(
                        requested_build)
            else:
                update_url = requested_build

        logging.debug('Update URL is %s', update_url)

        # Remove cros-version and job_repo_url host attribute from host.
        self.clear_job_repo_url()

        # Create a file to indicate if provision fails. The file will be removed
        # by stateful update or full install.
        self.run('touch %s' % PROVISION_FAILED)

        update_complete = False
        updater = autoupdater.ChromiumOSUpdater(
                 update_url, host=self, local_devserver=local_devserver)
        if not force_full_update:
            try:
                # If the DUT is already running the same build, try stateful
                # update first as it's much quicker than a full re-image.
                update_complete = self._try_stateful_update(
                        update_url, force_update, updater)
            except Exception as e:
                logging.exception(e)

        inactive_kernel = None
        if update_complete or (not force_update and updater.check_version()):
            logging.info('Install complete without full update')
        else:
            logging.info('DUT requires full update.')
            self.reboot(timeout=self.REBOOT_TIMEOUT, wait=True)
            # Stop service ap-update-manager to prevent rebooting during
            # autoupdate. The service is used in jetstream boards, but not other
            # CrOS devices.
            self.run('sudo stop ap-update-manager', ignore_status=True)

            num_of_attempts = provision.FLAKY_DEVSERVER_ATTEMPTS

            while num_of_attempts > 0:
                num_of_attempts -= 1
                try:
                    updater.run_update()
                except Exception:
                    logging.warn('Autoupdate did not complete.')
                    # Do additional check for the devserver health. Ideally,
                    # the autoupdater.py could raise an exception when it
                    # detected network flake but that would require
                    # instrumenting the update engine and parsing it log.
                    if (num_of_attempts <= 0 or
                            devserver is None or
                            dev_server.DevServer.devserver_healthy(
                                    devserver.url())):
                        raise

                    logging.warn('Devserver looks unhealthy. Trying another')
                    update_url, devserver = self._stage_image_for_update(
                            requested_build)
                    logging.debug('New Update URL is %s', update_url)
                    updater = autoupdater.ChromiumOSUpdater(
                            update_url, host=self,
                            local_devserver=local_devserver)
                else:
                    break

            # Give it some time in case of IO issues.
            time.sleep(10)

            # Figure out active and inactive kernel.
            active_kernel, inactive_kernel = updater.get_kernel_state()

            # Ensure inactive kernel has higher priority than active.
            if (updater.get_kernel_priority(inactive_kernel)
                    < updater.get_kernel_priority(active_kernel)):
                raise autoupdater.ChromiumOSError(
                    'Update failed. The priority of the inactive kernel'
                    ' partition is less than that of the active kernel'
                    ' partition.')

            # Updater has returned successfully; reboot the host.
            self.reboot(timeout=self.REBOOT_TIMEOUT, wait=True)

        self._post_update_processing(updater, inactive_kernel)
        image_name = autoupdater.url_to_image_name(update_url)
        self.add_job_repo_url(image_name)
        return image_name


    def _clear_fw_version_labels(self, rw_only):
        """Clear firmware version labels from the machine.

        @param rw_only: True to only clear fwrw_version; otherewise, clear
                        both fwro_version and fwrw_version.
        """
        labels = self._AFE.get_labels(
                name__startswith=provision.FW_RW_VERSION_PREFIX,
                host__hostname=self.hostname)
        if not rw_only:
            labels = labels + self._AFE.get_labels(
                    name__startswith=provision.FW_RO_VERSION_PREFIX,
                    host__hostname=self.hostname)
        for label in labels:
            label.remove_hosts(hosts=[self.hostname])


    def _add_fw_version_label(self, build, rw_only):
        """Add firmware version label to the machine.

        @param build: Build of firmware.
        @param rw_only: True to only add fwrw_version; otherwise, add both
                        fwro_version and fwrw_version.

        """
        fw_label = provision.fwrw_version_to_label(build)
        self._AFE.run('label_add_hosts', id=fw_label, hosts=[self.hostname])
        if not rw_only:
            fw_label = provision.fwro_version_to_label(build)
            self._AFE.run('label_add_hosts', id=fw_label, hosts=[self.hostname])


    def firmware_install(self, build=None, rw_only=False):
        """Install firmware to the DUT.

        Use stateful update if the DUT is already running the same build.
        Stateful update does not update kernel and tends to run much faster
        than a full reimage. If the DUT is running a different build, or it
        failed to do a stateful update, full update, including kernel update,
        will be applied to the DUT.

        Once a host enters firmware_install its fw[ro|rw]_version label will
        be removed. After the firmware is updated successfully, a new
        fw[ro|rw]_version label will be added to the host.

        @param build: The build version to which we want to provision the
                      firmware of the machine,
                      e.g. 'link-firmware/R22-2695.1.144'.
        @param rw_only: True to only install firmware to its RW portions. Keep
                        the RO portions unchanged.

        TODO(dshi): After bug 381718 is fixed, update here with corresponding
                    exceptions that could be raised.

        """
        if not self.servo:
            raise error.TestError('Host %s does not have servo.' %
                                  self.hostname)

        # TODO(fdeng): use host.get_board() after
        # crbug.com/271834 is fixed.
        board = self._get_board_from_afe()

        # If build is not set, try to install firmware from stable CrOS.
        if not build:
            build = self.get_repair_image_name(image_type='firmware')
            if not build:
                raise error.TestError(
                        'Failed to find stable firmware build for %s.',
                        self.hostname)
            logging.info('Will install firmware from build %s.', build)

        config = FAFTConfig(board)
        if config.use_u_boot:
            ap_image = 'image-%s.bin' % board
        else: # Depthcharge platform
            ap_image = 'image.bin'
        ec_image = 'ec.bin'
        ds = dev_server.ImageServer.resolve(build, self.hostname)
        ds.stage_artifacts(build, ['firmware'])

        tmpd = autotemp.tempdir(unique_id='fwimage')
        try:
            fwurl = self._FW_IMAGE_URL_PATTERN % (ds.url(), build)
            local_tarball = os.path.join(tmpd.name, os.path.basename(fwurl))
            server_utils.system('wget -O %s %s' % (local_tarball, fwurl),
                                timeout=60)
            server_utils.system('tar xf %s -C %s %s %s' %
                                (local_tarball, tmpd.name, ap_image, ec_image),
                                timeout=60)
            server_utils.system('tar xf %s  --wildcards -C %s "dts/*"' %
                                (local_tarball, tmpd.name),
                                timeout=60, ignore_status=True)

            self._clear_fw_version_labels(rw_only)
            logging.info('Will re-program EC %snow', 'RW ' if rw_only else '')
            self.servo.program_ec(os.path.join(tmpd.name, ec_image), rw_only)
            logging.info('Will re-program BIOS %snow', 'RW ' if rw_only else '')
            self.servo.program_bios(os.path.join(tmpd.name, ap_image), rw_only)
            self.servo.get_power_state_controller().reset()
            time.sleep(self.servo.BOOT_DELAY)
            self._add_fw_version_label(build, rw_only)
        finally:
            tmpd.clean()


    def show_update_engine_log(self):
        """Output update engine log."""
        logging.debug('Dumping %s', client_constants.UPDATE_ENGINE_LOG)
        self.run('cat %s' % client_constants.UPDATE_ENGINE_LOG)


    def _get_board_from_afe(self):
        """Retrieve this host's board from its labels in the AFE.

        Looks for a host label of the form "board:<board>", and
        returns the "<board>" part of the label.  `None` is returned
        if there is not a single, unique label matching the pattern.

        @returns board from label, or `None`.
        """
        return server_utils.get_board_from_afe(self.hostname, self._AFE)


    def get_build(self):
        """Retrieve the current build for this Host from the AFE.

        Looks through this host's labels in the AFE to determine its build.
        This method is replaced by afe_utils.get_build. It's kept here to
        maintain backwards compatibility for test control files in older CrOS
        builds (R48, R49 etc.) still call host.get_build, e.g.,
        `provision_AutoUpdate.double`.
        TODO(sbasi): Once R50 falls into release branch, this method can be
        removed.

        @returns The current build or None if it could not find it or if there
                 were multiple build labels assigned to this host.
        """
        return afe_utils.get_build(self)


    def _install_repair(self):
        """Attempt to repair this host using the update-engine.

        If the host is up, try installing the DUT with a stable
        "repair" version of Chrome OS as defined in afe_stable_versions table.
        If the table is not setup, global_config value under
        CROS.stable_cros_version will be used instead.

        @raises AutoservRepairMethodNA if the DUT is not reachable.
        @raises ChromiumOSError if the install failed for some reason.

        """
        if not self.is_up():
            raise error.AutoservRepairMethodNA('DUT unreachable for install.')
        logging.info('Attempting to reimage machine to repair image.')
        try:
            afe_utils.machine_install_and_update_labels(self, repair=True)
        except autoupdater.ChromiumOSError as e:
            logging.exception(e)
            logging.info('Repair via install failed.')
            raise


    def _install_repair_with_powerwash(self):
        """Attempt to powerwash first then repair this host using update-engine.

        update-engine may fail due to a bad image. In such case, powerwash
        may help to cleanup the DUT for update-engine to work again.

        @raises AutoservRepairMethodNA if the DUT is not reachable.
        @raises ChromiumOSError if the install failed for some reason.

        """
        if not self.is_up():
            raise error.AutoservRepairMethodNA('DUT unreachable for install.')

        logging.info('Attempting to powerwash the DUT.')
        self.run('echo "fast safe" > '
                 '/mnt/stateful_partition/factory_install_reset')
        self.reboot(timeout=self.POWERWASH_BOOT_TIMEOUT, wait=True)
        if not self.is_up():
            logging.error('Powerwash failed. DUT did not come back after '
                          'reboot.')
            raise error.AutoservRepairFailure(
                    'DUT failed to boot from powerwash after %d seconds' %
                    self.POWERWASH_BOOT_TIMEOUT)

        logging.info('Powerwash succeeded.')
        self._install_repair()


    def servo_install(self, image_url=None, usb_boot_timeout=USB_BOOT_TIMEOUT,
                      install_timeout=INSTALL_TIMEOUT):
        """
        Re-install the OS on the DUT by:
        1) installing a test image on a USB storage device attached to the Servo
                board,
        2) booting that image in recovery mode, and then
        3) installing the image with chromeos-install.

        @param image_url: If specified use as the url to install on the DUT.
                otherwise boot the currently staged image on the USB stick.
        @param usb_boot_timeout: The usb_boot_timeout to use during reimage.
                Factory images need a longer usb_boot_timeout than regular
                cros images.
        @param install_timeout: The timeout to use when installing the chromeos
                image. Factory images need a longer install_timeout.

        @raises AutoservError if the image fails to boot.

        """
        usb_boot_timer_key = ('servo_install.usb_boot_timeout_%s'
                              % usb_boot_timeout)
        logging.info('Downloading image to USB, then booting from it. Usb boot '
                     'timeout = %s', usb_boot_timeout)
        timer = autotest_stats.Timer(usb_boot_timer_key)
        timer.start()
        self.servo.install_recovery_image(image_url)
        if not self.wait_up(timeout=usb_boot_timeout):
            raise error.AutoservRepairFailure(
                    'DUT failed to boot from USB after %d seconds' %
                    usb_boot_timeout)
        timer.stop()

        # The new chromeos-tpm-recovery has been merged since R44-7073.0.0.
        # In old CrOS images, this command fails. Skip the error.
        logging.info('Resetting the TPM status')
        try:
            self.run('chromeos-tpm-recovery')
        except error.AutoservRunError:
            logging.warn('chromeos-tpm-recovery is too old.')


        install_timer_key = ('servo_install.install_timeout_%s'
                             % install_timeout)
        timer = autotest_stats.Timer(install_timer_key)
        timer.start()
        logging.info('Installing image through chromeos-install.')
        self.run('chromeos-install --yes', timeout=install_timeout)
        self.halt()
        timer.stop()

        logging.info('Power cycling DUT through servo.')
        self.servo.get_power_state_controller().power_off()
        self.servo.switch_usbkey('off')
        # N.B. The Servo API requires that we use power_on() here
        # for two reasons:
        #  1) After turning on a DUT in recovery mode, you must turn
        #     it off and then on with power_on() once more to
        #     disable recovery mode (this is a Parrot specific
        #     requirement).
        #  2) After power_off(), the only way to turn on is with
        #     power_on() (this is a Storm specific requirement).
        self.servo.get_power_state_controller().power_on()

        logging.info('Waiting for DUT to come back up.')
        if not self.wait_up(timeout=self.BOOT_TIMEOUT):
            raise error.AutoservError('DUT failed to reboot installed '
                                      'test image after %d seconds' %
                                      self.BOOT_TIMEOUT)


    def _setup_servo(self):
        """Try to force to create servo object if it's not set up yet.
        """
        if self.servo:
            return

        try:
            # Setting servo_args to {} will force it to create the servo_host
            # object if possible.
            self._servo_host = servo_host.create_servo_host(
                    dut=self.hostname, servo_args={})
            if self._servo_host:
                self.servo = self._servo_host.get_servo()
            else:
                logging.error('Failed to create servo_host object.')
        except Exception as e:
            logging.error('Failed to create servo object: %s', e)


    def _servo_repair_reinstall(self):
        """Reinstall the DUT utilizing servo and a test image.

        Re-install the OS on the DUT by:
        1) installing a test image on a USB storage device attached to the Servo
                board,
        2) booting that image in recovery mode,
        3) resetting the TPM status, and then
        4) installing the image with chromeos-install.

        @raises AutoservRepairMethodNA if the device does not have servo
                support.

        """
        if not self.servo:
            raise error.AutoservRepairMethodNA('Repair Reinstall NA: '
                                               'DUT has no servo support.')

        logging.info('Attempting to recovery servo enabled device with '
                     'servo_repair_reinstall')

        image_url = self.stage_image_for_servo()
        self.servo_install(image_url)


    def _is_firmware_repair_supported(self):
        """Check if the firmware repair is supported.

        The firmware repair is only applicable to DUTs in pools listed in
        global config CROS/pools_support_firmware_repair.

        @return: True if it is supported; otherwise False.
        """
        logging.info('Checking if host %s can be repaired with firmware '
                     'repair.', self.hostname)
        pools = server_utils.get_labels_from_afe(self.hostname, 'pool:',
                                                 self._AFE)
        pools_support_firmware_repair = CONFIG.get_config_value('CROS',
                'pools_support_firmware_repair', type=str).split(',')

        return (pools and pools_support_firmware_repair and
                set(pools).intersection(set(pools_support_firmware_repair)))


    def _firmware_repair(self):
        """Reinstall the firmware image using servo.

        This repair function attempts to install the stable firmware specified
        by the stable firmware version.
        Then reset the DUT and try to verify it. If verify fails, it will try to
        install the CrOS image using servo.
        """
        if not self._is_firmware_repair_supported():
            logging.info('Host is not in pools that support firmware repair.')
            raise error.AutoservRepairMethodNA(
                    'Firmware repair is not applicable to host %s.' %
                    self.hostname)

        # To repair a DUT connected to a moblab, try to create a servo object if
        # it was failed to be created earlier as there may be a servo_host host
        # attribute for this host.
        if utils.is_moblab():
            self._setup_servo()

        if not self.servo:
            raise error.AutoservRepairMethodNA('Repair Reinstall NA: '
                                               'DUT has no servo support.')

        logging.info('Attempting to recovery servo enabled device with '
                     'firmware_repair.')
        self.firmware_install()

        logging.info('Firmware repaired. Check if the DUT can boot. If not, '
                     'reinstall the CrOS using servo.')
        try:
            self.verify()
        except Exception as e:
            logging.warn('Failed to verify DUT, error: %s. Will try to repair '
                         'the DUT with servo_repair_reinstall.', e)
            self._servo_repair_reinstall()


    def _servo_repair_power(self):
        """Attempt to repair DUT using an attached Servo.

        Attempt to power cycle the DUT via cold_reset.

        @raises AutoservRepairMethodNA if the device does not have servo
                support.
        @raises AutoservRepairFailure if the repair fails for any reason.
        """
        if not self.servo:
            raise error.AutoservRepairMethodNA('Repair Power NA: '
                                               'DUT has no servo support.')

        logging.info('Attempting to recover servo enabled device by '
                     'powering cycling with cold reset.')
        self.servo.get_power_state_controller().reset()
        if self.wait_up(self.BOOT_TIMEOUT):
            return

        raise error.AutoservRepairFailure('DUT did not boot after long_press.')


    def _powercycle_to_repair(self):
        """Utilize the RPM Infrastructure to bring the host back up.

        If the host is not up/repaired after the first powercycle we utilize
        auto fallback to the last good install by powercycling and rebooting the
        host 6 times.

        @raises AutoservRepairMethodNA if the device does not support remote
                power.
        @raises AutoservRepairFailure if the repair fails for any reason.

        """
        if not self.has_power():
            raise error.AutoservRepairMethodNA('Device does not support power.')

        logging.info('Attempting repair via RPM powercycle.')
        failed_cycles = 0
        self.power_cycle()
        while not self.wait_up(timeout=self.BOOT_TIMEOUT):
            failed_cycles += 1
            if failed_cycles >= self._MAX_POWER_CYCLE_ATTEMPTS:
                raise error.AutoservRepairFailure(
                        'Powercycled host %s %d times; device did not come back'
                        ' online.' % (self.hostname, failed_cycles))
            self.power_cycle()
        if failed_cycles == 0:
            logging.info('Powercycling was successful first time.')
        else:
            logging.info('Powercycling was successful after %d failures.',
                         failed_cycles)


    def _reboot_repair(self):
        """SSH to this host and reboot."""
        if not self.is_up(self._CHECK_HOST_UP_TIMEOUT_SECS):
            raise error.AutoservRepairMethodNA('DUT unreachable for reboot.')
        logging.info('Attempting repair via SSH reboot.')
        self.reboot(timeout=self.BOOT_TIMEOUT, wait=True)


    def check_device(self):
        """Check if a device is ssh-able, and if so, clean and verify it.

        @raise AutoservSSHTimeout: If the ssh ping times out.
        @raise AutoservSshPermissionDeniedError: If ssh ping fails due to
                                                 permissions.
        @raise AutoservSshPingHostError: For other AutoservRunErrors during
                                         ssh_ping.
        @raises AutoservError: As appropriate, during cleanup and verify.
        """
        self.ssh_ping()
        self.cleanup()
        self.verify()


    def confirm_servo(self):
        """Confirm servo is initialized and verified.

        @raise AutoservError: If servo is not initialized and verified.
        """
        if self.servo and self._servo_host.required_by_test:
            return

        # Force to re-create the servo object to make sure servo is verified.
        logging.debug('Rebuilding the servo object.')
        self.servo = None
        self._servo_host = None
        self._setup_servo()
        if not self.servo:
            raise error.AutoservError('Failed to create servo object.')


    def _is_last_provision_failed(self):
        """Checks if the last provision job failed.

        @return: True if there exists file /var/tmp/provision_failed, which
                 indicates the last provision job failed.
                 False if the file does not exist or the dut can't be reached.
        """
        try:
            result = self.run('test -f %s' % PROVISION_FAILED,
                              ignore_status=True, timeout=5)
            return result.exit_status == 0
        except (error.AutoservRunError, error.AutoservSSHTimeout):
            # Default to False, for repair to try all repair method if the dut
            # can't be reached.
            return False


    def repair(self):
        """Attempt to get the DUT to pass `self.verify()`.

        This overrides the base class function for repair; it does
        not call back to the parent class, but instead offers a
        simplified implementation based on the capabilities in the
        Chrome OS test lab.

        It first verifies and repairs servo if it is a DUT in CrOS
        lab and a servo is attached.

        This escalates in order through the following procedures and verifies
        the status using `self.check_device()` after each of them. This is done
        until both the repair and the veryfing step succeed.

        Escalation order of repair procedures from less intrusive to
        more intrusive repairs:
          1. SSH to the DUT and reboot.
          2. If there's a servo for the DUT, try to power the DUT off and
             on.
          3. If the DUT can be power-cycled via RPM, try to repair
             by power-cycling.
          4. Try to re-install to a known stable image using
             auto-update.
          5. If there's a servo for the DUT, try to re-install via
             the servo.

        As with the parent method, the last operation performed on
        the DUT must be to call `self.check_device()`; If that call fails the
        exception it raises is passed back to the caller.

        @raises AutoservRepairTotalFailure if the repair process fails to
                fix the DUT.
        @raises ServoHostRepairTotalFailure if the repair process fails to
                fix the servo host if one is attached to the DUT.
        @raises AutoservSshPermissionDeniedError if it is unable
                to ssh to the servo host due to permission error.

        """
        # Caution: Deleting shards relies on repair to always reboot the DUT.

        # To repair a DUT connected to a moblab, try to create a servo object if
        # it was failed to be created earlier as there may be a servo_host host
        # attribute for this host.
        if utils.is_moblab():
            self._setup_servo()

        if self._servo_host and not self.servo:
            try:
                self._servo_host.repair()
            except Exception as e:
                logging.error('Could not create a healthy servo: %s', e)
            self.servo = self._servo_host.get_servo()

        self.try_collect_crashlogs()

        # TODO(scottz): This should use something similar to label_decorator,
        # but needs to be populated in order so DUTs are repaired with the
        # least amount of effort.
        force_powerwash = self._is_last_provision_failed()
        if force_powerwash:
            logging.info('Last provision failed, try powerwash first.')
            autotest_stats.Counter(
                    'repair_force_powerwash.TOTAL').increment()
            repair_funcs = [self._firmware_repair,
                            self._install_repair_with_powerwash,
                            self._servo_repair_reinstall]
        else:
            repair_funcs = [self._reboot_repair,
                            self._servo_repair_power,
                            self._firmware_repair,
                            self._powercycle_to_repair,
                            self._install_repair,
                            self._install_repair_with_powerwash,
                            self._servo_repair_reinstall]
        errors = []
        board = self._get_board_from_afe()
        for repair_func in repair_funcs:
            try:
                repair_func()
                self.try_collect_crashlogs()
                self.check_device()
                autotest_stats.Counter(
                        '%s.SUCCEEDED' % repair_func.__name__).increment()
                if board:
                    autotest_stats.Counter(
                            '%s.%s.SUCCEEDED' % (repair_func.__name__,
                                             board)).increment()
                if force_powerwash:
                    autotest_stats.Counter(
                            'repair_force_powerwash.SUCCEEDED').increment()
                return
            except error.AutoservRepairMethodNA as e:
                autotest_stats.Counter(
                        '%s.RepairNA' % repair_func.__name__).increment()
                if board:
                    autotest_stats.Counter(
                            '%s.%s.RepairNA' % (repair_func.__name__,
                                                board)).increment()
                logging.warning('Repair function NA: %s', e)
                errors.append(str(e))
            except Exception as e:
                autotest_stats.Counter(
                        '%s.FAILED' % repair_func.__name__).increment()
                if board:
                    autotest_stats.Counter(
                            '%s.%s.FAILED' % (repair_func.__name__,
                                              board)).increment()
                logging.warning('Failed to repair device: %s', e)
                errors.append(str(e))

        if force_powerwash:
            autotest_stats.Counter(
                    'repair_force_powerwash.FAILED').increment()
        autotest_stats.Counter('Full_Repair_Failed').increment()
        if board:
            autotest_stats.Counter(
                    'Full_Repair_Failed.%s' % board).increment()
        raise error.AutoservRepairTotalFailure(
                'All attempts at repairing the device failed:\n%s' %
                '\n'.join(errors))


    def try_collect_crashlogs(self, check_host_up=True):
        """
        Check if a host is up and logs need to be collected from the host,
        if yes, collect them.

        @param check_host_up: Flag for checking host is up. Default is True.
        """
        try:
            crash_job = self._need_crash_logs()
            if crash_job:
                logging.debug('%s: Job %s was crashed', self._CRASHLOGS_PREFIX,
                              crash_job)
                if not check_host_up or self.is_up(
                        self._CHECK_HOST_UP_TIMEOUT_SECS):
                    self._collect_crashlogs(crash_job)
                    logging.debug('%s: Completed collecting logs for the '
                                  'crashed job %s', self._CRASHLOGS_PREFIX,
                                  crash_job)
        except Exception as e:
            # Exception should not result in repair failure.
            # Therefore, suppress all exceptions here.
            logging.error('%s: Failed while trying to collect crash-logs: %s',
                          self._CRASHLOGS_PREFIX, e)


    def _need_crash_logs(self):
        """Get the value of need_crash_logs attribute of this host.

        @return: Value string of need_crash_logs attribute
                 None if there is no need_crash_logs attribute
        """
        attrs = self._AFE.get_host_attribute(constants.CRASHLOGS_HOST_ATTRIBUTE,
                                             hostname=self.hostname)
        assert len(attrs) < 2
        return attrs[0].value if attrs else None


    def _collect_crashlogs(self, job_id):
        """Grab logs from the host where a job was crashed.

        First, check if PRIOR_LOGS_DIR exists in the host.
        If yes, collect them.
        Otherwise, check if a lab-machine marker (_LAB_MACHINE_FILE) exists
        in the host.
        If yes, the host was repaired automatically, and we collect normal
        system logs.

        @param job_id: Id of the job that was crashed.
        """
        crashlogs_dir = crashcollect.get_crashinfo_dir(self,
                constants.CRASHLOGS_DEST_DIR_PREFIX)
        flag_prior_logs = False

        if self.path_exists(client_constants.PRIOR_LOGS_DIR):
            flag_prior_logs = True
            self._collect_prior_logs(crashlogs_dir)
        elif self.path_exists(self._LAB_MACHINE_FILE):
            self._collect_system_logs(crashlogs_dir)
        else:
            logging.warning('%s: Host was manually re-installed without '
                            '--lab_preserve_log option. Skip collecting '
                            'crash-logs.', self._CRASHLOGS_PREFIX)

        # We make crash collection be one-time effort.
        # _collect_prior_logs() and _collect_system_logs() will not throw
        # any exception, and following codes will be executed even when
        # those methods fail.
        # _collect_crashlogs() is called only when the host is up (refer
        # to try_collect_crashlogs()). We assume _collect_prior_logs() and
        # _collect_system_logs() fail rarely when the host is up.
        # In addition, it is not clear how many times we should try crash
        # collection again while not triggering next repair unnecessarily.
        # Threfore, we try crash collection one time.

        # Create a marker file as soon as log collection is done.
        # Leave the job id to this marker for gs_offloader to consume.
        marker_file = os.path.join(crashlogs_dir, constants.CRASHLOGS_MARKER)
        with open(marker_file, 'a') as f:
            f.write('%s\n' % job_id)

        # Remove need_crash_logs attribute
        logging.debug('%s: Remove attribute need_crash_logs from host %s',
                      self._CRASHLOGS_PREFIX, self.hostname)
        self._AFE.set_host_attribute(constants.CRASHLOGS_HOST_ATTRIBUTE,
                                     None, hostname=self.hostname)

        if flag_prior_logs:
            logging.debug('%s: Remove %s from host %s', self._CRASHLOGS_PREFIX,
                          client_constants.PRIOR_LOGS_DIR, self.hostname)
            self.run('rm -rf %s; sync' % client_constants.PRIOR_LOGS_DIR)
            # Wait for a few seconds to make sure the prior command is
            # done deep through storage.
            time.sleep(self._SAFE_WAIT_SECS)


    def _collect_prior_logs(self, crashlogs_dir):
        """Grab prior logs that were stashed before re-installing a host.

        @param crashlogs_dir: Directory path where crash-logs are stored.
        """
        logging.debug('%s: Found %s, collecting them...',
                      self._CRASHLOGS_PREFIX, client_constants.PRIOR_LOGS_DIR)
        try:
            self.collect_logs(client_constants.PRIOR_LOGS_DIR,
                              crashlogs_dir, False)
            logging.debug('%s: %s is collected',
                          self._CRASHLOGS_PREFIX, client_constants.PRIOR_LOGS_DIR)
        except Exception as e:
            logging.error('%s: Failed to collect %s: %s',
                          self._CRASHLOGS_PREFIX, client_constants.PRIOR_LOGS_DIR,
                          e)


    def _collect_system_logs(self, crashlogs_dir):
        """Grab normal system logs from a host.

        @param crashlogs_dir: Directory path where crash-logs are stored.
        """
        logging.debug('%s: Found %s, collecting system logs...',
                      self._CRASHLOGS_PREFIX, self._LAB_MACHINE_FILE)
        sources = server_utils.parse_simple_config(self._LOGS_TO_COLLECT_FILE)
        for src in sources:
            try:
                if self.path_exists(src):
                    logging.debug('%s: Collecting %s...',
                                  self._CRASHLOGS_PREFIX, src)
                    dest = server_utils.concat_path_except_last(
                            crashlogs_dir, src)
                    self.collect_logs(src, dest, False)
                    logging.debug('%s: %s is collected',
                                  self._CRASHLOGS_PREFIX, src)
            except Exception as e:
                logging.error('%s: Failed to collect %s: %s',
                              self._CRASHLOGS_PREFIX, src, e)


    def close(self):
        super(CrosHost, self).close()


    def get_power_supply_info(self):
        """Get the output of power_supply_info.

        power_supply_info outputs the info of each power supply, e.g.,
        Device: Line Power
          online:                  no
          type:                    Mains
          voltage (V):             0
          current (A):             0
        Device: Battery
          state:                   Discharging
          percentage:              95.9276
          technology:              Li-ion

        Above output shows two devices, Line Power and Battery, with details of
        each device listed. This function parses the output into a dictionary,
        with key being the device name, and value being a dictionary of details
        of the device info.

        @return: The dictionary of power_supply_info, e.g.,
                 {'Line Power': {'online': 'yes', 'type': 'main'},
                  'Battery': {'vendor': 'xyz', 'percentage': '100'}}
        @raise error.AutoservRunError if power_supply_info tool is not found in
               the DUT. Caller should handle this error to avoid false failure
               on verification.
        """
        result = self.run('power_supply_info').stdout.strip()
        info = {}
        device_name = None
        device_info = {}
        for line in result.split('\n'):
            pair = [v.strip() for v in line.split(':')]
            if len(pair) != 2:
                continue
            if pair[0] == 'Device':
                if device_name:
                    info[device_name] = device_info
                device_name = pair[1]
                device_info = {}
            else:
                device_info[pair[0]] = pair[1]
        if device_name and not device_name in info:
            info[device_name] = device_info
        return info


    def get_battery_percentage(self):
        """Get the battery percentage.

        @return: The percentage of battery level, value range from 0-100. Return
                 None if the battery info cannot be retrieved.
        """
        try:
            info = self.get_power_supply_info()
            logging.info(info)
            return float(info['Battery']['percentage'])
        except (KeyError, ValueError, error.AutoservRunError):
            return None


    def is_ac_connected(self):
        """Check if the dut has power adapter connected and charging.

        @return: True if power adapter is connected and charging.
        """
        try:
            info = self.get_power_supply_info()
            return info['Line Power']['online'] == 'yes'
        except (KeyError, error.AutoservRunError):
            return None


    def _cleanup_poweron(self):
        """Special cleanup method to make sure hosts always get power back."""
        afe = frontend_wrappers.RetryingAFE(timeout_min=5, delay_sec=10)
        hosts = afe.get_hosts(hostname=self.hostname)
        if not hosts or not (self._RPM_OUTLET_CHANGED in
                             hosts[0].attributes):
            return
        logging.debug('This host has recently interacted with the RPM'
                      ' Infrastructure. Ensuring power is on.')
        try:
            self.power_on()
            afe.set_host_attribute(self._RPM_OUTLET_CHANGED, None,
                                   hostname=self.hostname)
        except rpm_client.RemotePowerException:
            logging.error('Failed to turn Power On for this host after '
                          'cleanup through the RPM Infrastructure.')
            autotest_es.post(
                    type_str='RPM_poweron_failure',
                    metadata={'hostname': self.hostname})

            battery_percentage = self.get_battery_percentage()
            if battery_percentage and battery_percentage < 50:
                raise
            elif self.is_ac_connected():
                logging.info('The device has power adapter connected and '
                             'charging. No need to try to turn RPM on '
                             'again.')
                afe.set_host_attribute(self._RPM_OUTLET_CHANGED, None,
                                       hostname=self.hostname)
            logging.info('Battery level is now at %s%%. The device may '
                         'still have enough power to run test, so no '
                         'exception will be raised.', battery_percentage)


    def _is_factory_image(self):
        """Checks if the image on the DUT is a factory image.

        @return: True if the image on the DUT is a factory image.
                 False otherwise.
        """
        result = self.run('[ -f /root/.factory_test ]', ignore_status=True)
        return result.exit_status == 0


    def _restart_ui(self):
        """Restart the Chrome UI.

        @raises: FactoryImageCheckerException for factory images, since
                 we cannot attempt to restart ui on them.
                 error.AutoservRunError for any other type of error that
                 occurs while restarting ui.
        """
        if self._is_factory_image():
            raise FactoryImageCheckerException('Cannot restart ui on factory '
                                               'images')

        # TODO(jrbarnette):  The command to stop/start the ui job
        # should live inside cros_ui, too.  However that would seem
        # to imply interface changes to the existing start()/restart()
        # functions, which is a bridge too far (for now).
        prompt = cros_ui.get_chrome_session_ident(self)
        self.run('stop ui; start ui')
        cros_ui.wait_for_chrome_ready(prompt, self)


    def get_release_version(self):
        """Get the value of attribute CHROMEOS_RELEASE_VERSION from lsb-release.

        @returns The version string in lsb-release, under attribute
                 CHROMEOS_RELEASE_VERSION.
        """
        lsb_release_content = self.run(
                    'cat "%s"' % client_constants.LSB_RELEASE).stdout.strip()
        return lsbrelease_utils.get_chromeos_release_version(
                    lsb_release_content=lsb_release_content)


    def verify_cros_version_label(self):
        """ Make sure host's cros-version label match the actual image in dut.

        Remove any cros-version: label that doesn't match that installed in
        the dut.

        @param raise_error: Set to True to raise exception if any mismatch found

        @raise error.AutoservError: If any mismatch between cros-version label
                                    and the build installed in dut is found.
        """
        labels = self._AFE.get_labels(
                name__startswith=ds_constants.VERSION_PREFIX,
                host__hostname=self.hostname)
        mismatch_found = False
        if labels:
            # Get CHROMEOS_RELEASE_VERSION from lsb-release, e.g., 6908.0.0.
            # Note that it's different from cros-version label, which has
            # builder and branch info, e.g.,
            # cros-version:peppy-release/R43-6908.0.0
            release_version = self.get_release_version()
            host_list = [self.hostname]
            for label in labels:
                # Remove any cros-version label that does not match
                # release_version.
                build_version = label.name[len(ds_constants.VERSION_PREFIX):]
                if not utils.version_match(build_version, release_version):
                    logging.warn('cros-version label "%s" does not match '
                                 'release version %s. Removing the label.',
                                 label.name, release_version)
                    label.remove_hosts(hosts=host_list)
                    mismatch_found = True
        if mismatch_found:
            autotest_es.post(use_http=True,
                             type_str='cros_version_label_mismatch',
                             metadata={'hostname': self.hostname})
            raise error.AutoservError('The host has wrong cros-version label.')


    def verify_tpm_status(self):
        """ Verify the host's TPM is in a good state.

        @raise error.AutoservError: If state is not good.
        """
        # This cryptohome command emits status information in JSON format. It
        # looks something like this:
        # {
        #    "installattrs": {
        #       "first_install": false,
        #       "initialized": true,
        #       "invalid": false,
        #       "lockbox_index": 536870916,
        #       "lockbox_nvram_version": 2,
        #       "secure": true,
        #       "size": 0,
        #       "version": 1
        #    },
        #    "mounts": [ {
        #       "enterprise": false,
        #       "keysets": [ {
        #          "current": true,
        #          "index": 0,
        #          "last_activity": 1330111359,
        #          "ok": true,
        #          "scrypt": true,
        #          "tpm": false
        #       } ],
        #       "mounted": true,
        #       "owner": "dbb3dd34edb181245130e136be51fa08478d3909"
        #    } ],
        #    "tpm": {
        #       "being_owned": false,
        #       "can_connect": true,
        #       "can_decrypt": false,
        #       "can_encrypt": false,
        #       "can_load_srk": true,
        #       "can_load_srk_pubkey": true,
        #       "enabled": true,
        #       "has_context": true,
        #       "has_cryptohome_key": false,
        #       "has_key_handle": false,
        #       "last_error": 0,
        #       "owned": true
        #    }
        # }
        output = self.run('cryptohome --action=status').stdout.strip()
        try:
            status = json.loads(output)
        except ValueError:
            logging.error('TPM_VERIFY: Cryptohome did not return valid status.')
            return
        try:
            tpm = status['tpm']
            if (not tpm['enabled'] or not tpm['can_connect'] or
                (tpm['owned'] and not tpm['can_load_srk']) or
                (tpm['can_load_srk'] and not tpm['can_load_srk_pubkey'])):
                logging.error('TPM_VERIFY: The host TPM is in a bad state.')
                raise error.AutoservError('The host TPM is in a bad state.')
            else:
                logging.debug('TPM_VERIFY: The host TPM is in a good state.')
        except KeyError:
            logging.error('TPM_VERIFY: Cryptohome did not return valid status.')


    def verify_firmware_status(self):
        """Verify the host's firmware is in a good state.

        @raise error.AutoservError: If state is not good.
        """
        if self._is_firmware_repair_supported():
            try:
                # Read the AP firmware and dump the sections we are interested.
                cmd = ('mkdir /tmp/verify_firmware; '
                       'cd /tmp/verify_firmware; '
                       'for section in VBLOCK_A VBLOCK_B FW_MAIN_A FW_MAIN_B; '
                       'do flashrom -r image.bin -i $section:$section; '
                       'done')
                self.run(cmd)

                # Verify the firmware blocks A and B.
                cmd = ('vbutil_firmware --verify /tmp/verify_firmware/VBLOCK_%c'
                       ' --signpubkey /usr/share/vboot/devkeys/root_key.vbpubk'
                       ' --fv /tmp/verify_firmware/FW_MAIN_%c')
                for c in ('A', 'B'):
                    rv = self.run(cmd % (c, c), ignore_status=True)
                    if rv.exit_status:
                       raise error.AutoservError(
                               'Firmware %c is in a bad state.' % c)
            finally:
                # Remove the tempoary files.
                self.run('rm -rf /tmp/verify_firmware')
        else:
            logging.info('Do not care about firmware status when the host '
                         'is not in pools that support firmware repair.')


    def verify_filesystem_write_status(self):
        """Verify the DUT's filesystem is read/writable

        @raise error.AutoservError: if filesystem is not writable.
        """
        # try to create & delete a file
        filename = "/mnt/stateful_partition/test.txt"
        cmd = 'touch %s && rm %s' % (filename, filename)
        rv = self.run(command=cmd, ignore_status=True)

        if rv.exit_status == 1:
            raise error.AutoservError('DUT filesystem is read-only.')


    def cleanup(self):
        self.run('rm -f %s' % client_constants.CLEANUP_LOGS_PAUSED_FILE)
        try:
            self._restart_ui()
        except (error.AutotestRunError, error.AutoservRunError,
                FactoryImageCheckerException):
            logging.warning('Unable to restart ui, rebooting device.')
            # Since restarting the UI fails fall back to normal Autotest
            # cleanup routines, i.e. reboot the machine.
            super(CrosHost, self).cleanup()
        # Check if the rpm outlet was manipulated.
        if self.has_power():
            self._cleanup_poweron()
        self.verify_cros_version_label()


    def reboot(self, **dargs):
        """
        This function reboots the site host. The more generic
        RemoteHost.reboot() performs sync and sleeps for 5
        seconds. This is not necessary for Chrome OS devices as the
        sync should be finished in a short time during the reboot
        command.
        """
        if 'reboot_cmd' not in dargs:
            reboot_timeout = dargs.get('reboot_timeout', 10)
            dargs['reboot_cmd'] = ('sleep 1; '
                                   'reboot & sleep %d; '
                                   'reboot -f' % reboot_timeout)
        # Enable fastsync to avoid running extra sync commands before reboot.
        if 'fastsync' not in dargs:
            dargs['fastsync'] = True

        # For purposes of logging reboot times:
        # Get the board name i.e. 'daisy_spring'
        board_fullname = self.get_board()

        # Strip the prefix and add it to dargs.
        dargs['board'] = board_fullname[board_fullname.find(':')+1:]
        super(CrosHost, self).reboot(**dargs)


    def suspend(self, **dargs):
        """
        This function suspends the site host.
        """
        suspend_time = dargs.get('suspend_time', 60)
        dargs['timeout'] = suspend_time
        if 'suspend_cmd' not in dargs:
            dargs['suspend_cmd'] = ' && '.join([
                'echo 0 > /sys/class/rtc/rtc0/wakealarm',
                'echo +%d > /sys/class/rtc/rtc0/wakealarm' % suspend_time,
                'powerd_dbus_suspend --delay=0'])
        super(CrosHost, self).suspend(**dargs)


    def upstart_status(self, service_name):
        """Check the status of an upstart init script.

        @param service_name: Service to look up.

        @returns True if the service is running, False otherwise.
        """
        return self.run('status %s | grep start/running' %
                        service_name).stdout.strip() != ''


    def verify_software(self):
        """Verify working software on a Chrome OS system.

        Tests for the following conditions:
         1. All conditions tested by the parent version of this
            function.
         2. Sufficient space in /mnt/stateful_partition.
         3. Sufficient space in /mnt/stateful_partition/encrypted.
         4. update_engine answers a simple status request over DBus.

        """
        # Check if a job was crashed on this host.
        # If yes, avoid verification until crash-logs are collected.
        if self._need_crash_logs():
            raise error.AutoservCrashLogCollectRequired(
                    'Need to collect crash-logs before verification')

        super(CrosHost, self).verify_software()
        default_kilo_inodes_required = CONFIG.get_config_value(
                'SERVER', 'kilo_inodes_required', type=int, default=100)
        board = self.get_board().replace(ds_constants.BOARD_PREFIX, '')
        kilo_inodes_required = CONFIG.get_config_value(
                'SERVER', 'kilo_inodes_required_%s' % board,
                type=int, default=default_kilo_inodes_required)
        self.check_inodes('/mnt/stateful_partition', kilo_inodes_required)
        self.check_diskspace(
            '/mnt/stateful_partition',
            CONFIG.get_config_value(
                'SERVER', 'gb_diskspace_required', type=float,
                default=20.0))
        encrypted_stateful_path = '/mnt/stateful_partition/encrypted'
        # Not all targets build with encrypted stateful support.
        if self.path_exists(encrypted_stateful_path):
            self.check_diskspace(
                encrypted_stateful_path,
                CONFIG.get_config_value(
                    'SERVER', 'gb_encrypted_diskspace_required', type=float,
                    default=0.1))

        if not self.upstart_status('system-services'):
            raise error.AutoservError('Chrome failed to reach login. '
                                      'System services not running.')

        # Factory images don't run update engine,
        # goofy controls dbus on these DUTs.
        if not self._is_factory_image():
            self.run('update_engine_client --status')
        # Makes sure python is present, loads and can use built in functions.
        # We have seen cases where importing cPickle fails with undefined
        # symbols in cPickle.so.
        self.run('python -c "import cPickle"')

        self.verify_cros_version_label()

        self.verify_tpm_status()

        self.verify_firmware_status()

        self.verify_filesystem_write_status()


    def verify_hardware(self):
        """Verify hardware system of a Chrome OS system.

        Check following hardware conditions:
        1. Battery level.
        2. Is power adapter connected.
        """
        logging.info('Battery percentage: %s', self.get_battery_percentage())
        if self.is_ac_connected() is None:
            logging.info('Can not determine if the device has power adapter '
                         'connected.')
        else:
            logging.info('Device %s power adapter connected and charging.',
                         'has' if self.is_ac_connected() else 'does not have')


    def make_ssh_command(self, user='root', port=22, opts='', hosts_file=None,
                         connect_timeout=None, alive_interval=None):
        """Override default make_ssh_command to use options tuned for Chrome OS.

        Tuning changes:
          - ConnectTimeout=30; maximum of 30 seconds allowed for an SSH
          connection failure.  Consistency with remote_access.sh.

          - ServerAliveInterval=900; which causes SSH to ping connection every
          900 seconds. In conjunction with ServerAliveCountMax ensures
          that if the connection dies, Autotest will bail out.
          Originally tried 60 secs, but saw frequent job ABORTS where
          the test completed successfully. Later increased from 180 seconds to
          900 seconds to account for tests where the DUT is suspended for
          longer periods of time.

          - ServerAliveCountMax=3; consistency with remote_access.sh.

          - ConnectAttempts=4; reduce flakiness in connection errors;
          consistency with remote_access.sh.

          - UserKnownHostsFile=/dev/null; we don't care about the keys.
          Host keys change with every new installation, don't waste
          memory/space saving them.

          - SSH protocol forced to 2; needed for ServerAliveInterval.

        @param user User name to use for the ssh connection.
        @param port Port on the target host to use for ssh connection.
        @param opts Additional options to the ssh command.
        @param hosts_file Ignored.
        @param connect_timeout Ignored.
        @param alive_interval Ignored.
        """
        base_command = ('/usr/bin/ssh -a -x %s %s %s'
                        ' -o StrictHostKeyChecking=no'
                        ' -o UserKnownHostsFile=/dev/null -o BatchMode=yes'
                        ' -o ConnectTimeout=30 -o ServerAliveInterval=900'
                        ' -o ServerAliveCountMax=3 -o ConnectionAttempts=4'
                        ' -o Protocol=2 -l %s -p %d')
        return base_command % (self._ssh_verbosity_flag, self._ssh_options,
                               opts, user, port)
    def syslog(self, message, tag='autotest'):
        """Logs a message to syslog on host.

        @param message String message to log into syslog
        @param tag String tag prefix for syslog

        """
        self.run('logger -t "%s" "%s"' % (tag, message))


    def _ping_check_status(self, status):
        """Ping the host once, and return whether it has a given status.

        @param status Check the ping status against this value.
        @return True iff `status` and the result of ping are the same
                (i.e. both True or both False).

        """
        ping_val = utils.ping(self.hostname, tries=1, deadline=1)
        return not (status ^ (ping_val == 0))

    def _ping_wait_for_status(self, status, timeout):
        """Wait for the host to have a given status (UP or DOWN).

        Status is checked by polling.  Polling will not last longer
        than the number of seconds in `timeout`.  The polling
        interval will be long enough that only approximately
        _PING_WAIT_COUNT polling cycles will be executed, subject
        to a maximum interval of about one minute.

        @param status Waiting will stop immediately if `ping` of the
                      host returns this status.
        @param timeout Poll for at most this many seconds.
        @return True iff the host status from `ping` matched the
                requested status at the time of return.

        """
        # _ping_check_status() takes about 1 second, hence the
        # "- 1" in the formula below.
        poll_interval = min(int(timeout / self._PING_WAIT_COUNT), 60) - 1
        end_time = time.time() + timeout
        while time.time() <= end_time:
            if self._ping_check_status(status):
                return True
            if poll_interval > 0:
                time.sleep(poll_interval)

        # The last thing we did was sleep(poll_interval), so it may
        # have been too long since the last `ping`.  Check one more
        # time, just to be sure.
        return self._ping_check_status(status)

    def ping_wait_up(self, timeout):
        """Wait for the host to respond to `ping`.

        N.B.  This method is not a reliable substitute for
        `wait_up()`, because a host that responds to ping will not
        necessarily respond to ssh.  This method should only be used
        if the target DUT can be considered functional even if it
        can't be reached via ssh.

        @param timeout Minimum time to allow before declaring the
                       host to be non-responsive.
        @return True iff the host answered to ping before the timeout.

        """
        return self._ping_wait_for_status(self._PING_STATUS_UP, timeout)

    def ping_wait_down(self, timeout):
        """Wait until the host no longer responds to `ping`.

        This function can be used as a slightly faster version of
        `wait_down()`, by avoiding potentially long ssh timeouts.

        @param timeout Minimum time to allow for the host to become
                       non-responsive.
        @return True iff the host quit answering ping before the
                timeout.

        """
        return self._ping_wait_for_status(self._PING_STATUS_DOWN, timeout)

    def test_wait_for_sleep(self, sleep_timeout=None):
        """Wait for the client to enter low-power sleep mode.

        The test for "is asleep" can't distinguish a system that is
        powered off; to confirm that the unit was asleep, it is
        necessary to force resume, and then call
        `test_wait_for_resume()`.

        This function is expected to be called from a test as part
        of a sequence like the following:

        ~~~~~~~~
            boot_id = host.get_boot_id()
            # trigger sleep on the host
            host.test_wait_for_sleep()
            # trigger resume on the host
            host.test_wait_for_resume(boot_id)
        ~~~~~~~~

        @param sleep_timeout time limit in seconds to allow the host sleep.

        @exception TestFail The host did not go to sleep within
                            the allowed time.
        """
        if sleep_timeout is None:
            sleep_timeout = self.SLEEP_TIMEOUT

        if not self.ping_wait_down(timeout=sleep_timeout):
            raise error.TestFail(
                'client failed to sleep after %d seconds' % sleep_timeout)


    def test_wait_for_resume(self, old_boot_id, resume_timeout=None):
        """Wait for the client to resume from low-power sleep mode.

        The `old_boot_id` parameter should be the value from
        `get_boot_id()` obtained prior to entering sleep mode.  A
        `TestFail` exception is raised if the boot id changes.

        See @ref test_wait_for_sleep for more on this function's
        usage.

        @param old_boot_id A boot id value obtained before the
                               target host went to sleep.
        @param resume_timeout time limit in seconds to allow the host up.

        @exception TestFail The host did not respond within the
                            allowed time.
        @exception TestFail The host responded, but the boot id test
                            indicated a reboot rather than a sleep
                            cycle.
        """
        if resume_timeout is None:
            resume_timeout = self.RESUME_TIMEOUT

        if not self.wait_up(timeout=resume_timeout):
            raise error.TestFail(
                'client failed to resume from sleep after %d seconds' %
                    resume_timeout)
        else:
            new_boot_id = self.get_boot_id()
            if new_boot_id != old_boot_id:
                logging.error('client rebooted (old boot %s, new boot %s)',
                              old_boot_id, new_boot_id)
                raise error.TestFail(
                    'client rebooted, but sleep was expected')


    def test_wait_for_shutdown(self, shutdown_timeout=None):
        """Wait for the client to shut down.

        The test for "has shut down" can't distinguish a system that
        is merely asleep; to confirm that the unit was down, it is
        necessary to force boot, and then call test_wait_for_boot().

        This function is expected to be called from a test as part
        of a sequence like the following:

        ~~~~~~~~
            boot_id = host.get_boot_id()
            # trigger shutdown on the host
            host.test_wait_for_shutdown()
            # trigger boot on the host
            host.test_wait_for_boot(boot_id)
        ~~~~~~~~

        @param shutdown_timeout time limit in seconds to allow the host down.
        @exception TestFail The host did not shut down within the
                            allowed time.
        """
        if shutdown_timeout is None:
            shutdown_timeout = self.SHUTDOWN_TIMEOUT

        if not self.ping_wait_down(timeout=shutdown_timeout):
            raise error.TestFail(
                'client failed to shut down after %d seconds' %
                    shutdown_timeout)


    def test_wait_for_boot(self, old_boot_id=None):
        """Wait for the client to boot from cold power.

        The `old_boot_id` parameter should be the value from
        `get_boot_id()` obtained prior to shutting down.  A
        `TestFail` exception is raised if the boot id does not
        change.  The boot id test is omitted if `old_boot_id` is not
        specified.

        See @ref test_wait_for_shutdown for more on this function's
        usage.

        @param old_boot_id A boot id value obtained before the
                               shut down.

        @exception TestFail The host did not respond within the
                            allowed time.
        @exception TestFail The host responded, but the boot id test
                            indicated that there was no reboot.
        """
        if not self.wait_up(timeout=self.REBOOT_TIMEOUT):
            raise error.TestFail(
                'client failed to reboot after %d seconds' %
                    self.REBOOT_TIMEOUT)
        elif old_boot_id:
            if self.get_boot_id() == old_boot_id:
                logging.error('client not rebooted (boot %s)',
                              old_boot_id)
                raise error.TestFail(
                    'client is back up, but did not reboot')


    @staticmethod
    def check_for_rpm_support(hostname):
        """For a given hostname, return whether or not it is powered by an RPM.

        @param hostname: hostname to check for rpm support.

        @return None if this host does not follows the defined naming format
                for RPM powered DUT's in the lab. If it does follow the format,
                it returns a regular expression MatchObject instead.
        """
        return re.match(CrosHost._RPM_HOSTNAME_REGEX, hostname)


    def has_power(self):
        """For this host, return whether or not it is powered by an RPM.

        @return True if this host is in the CROS lab and follows the defined
                naming format.
        """
        return CrosHost.check_for_rpm_support(self.hostname)


    def _set_power(self, state, power_method):
        """Sets the power to the host via RPM, Servo or manual.

        @param state Specifies which power state to set to DUT
        @param power_method Specifies which method of power control to
                            use. By default "RPM" will be used. Valid values
                            are the strings "RPM", "manual", "servoj10".

        """
        ACCEPTABLE_STATES = ['ON', 'OFF']

        if state.upper() not in ACCEPTABLE_STATES:
            raise error.TestError('State must be one of: %s.'
                                   % (ACCEPTABLE_STATES,))

        if power_method == self.POWER_CONTROL_SERVO:
            logging.info('Setting servo port J10 to %s', state)
            self.servo.set('prtctl3_pwren', state.lower())
            time.sleep(self._USB_POWER_TIMEOUT)
        elif power_method == self.POWER_CONTROL_MANUAL:
            logging.info('You have %d seconds to set the AC power to %s.',
                         self._POWER_CYCLE_TIMEOUT, state)
            time.sleep(self._POWER_CYCLE_TIMEOUT)
        else:
            if not self.has_power():
                raise error.TestFail('DUT does not have RPM connected.')
            afe = frontend_wrappers.RetryingAFE(timeout_min=5, delay_sec=10)
            afe.set_host_attribute(self._RPM_OUTLET_CHANGED, True,
                                   hostname=self.hostname)
            rpm_client.set_power(self.hostname, state.upper(), timeout_mins=5)


    def power_off(self, power_method=POWER_CONTROL_RPM):
        """Turn off power to this host via RPM, Servo or manual.

        @param power_method Specifies which method of power control to
                            use. By default "RPM" will be used. Valid values
                            are the strings "RPM", "manual", "servoj10".

        """
        self._set_power('OFF', power_method)


    def power_on(self, power_method=POWER_CONTROL_RPM):
        """Turn on power to this host via RPM, Servo or manual.

        @param power_method Specifies which method of power control to
                            use. By default "RPM" will be used. Valid values
                            are the strings "RPM", "manual", "servoj10".

        """
        self._set_power('ON', power_method)


    def power_cycle(self, power_method=POWER_CONTROL_RPM):
        """Cycle power to this host by turning it OFF, then ON.

        @param power_method Specifies which method of power control to
                            use. By default "RPM" will be used. Valid values
                            are the strings "RPM", "manual", "servoj10".

        """
        if power_method in (self.POWER_CONTROL_SERVO,
                            self.POWER_CONTROL_MANUAL):
            self.power_off(power_method=power_method)
            time.sleep(self._POWER_CYCLE_TIMEOUT)
            self.power_on(power_method=power_method)
        else:
            rpm_client.set_power(self.hostname, 'CYCLE')


    def get_platform(self):
        """Determine the correct platform label for this host.

        @returns a string representing this host's platform.
        """
        crossystem = utils.Crossystem(self)
        crossystem.init()
        # Extract fwid value and use the leading part as the platform id.
        # fwid generally follow the format of {platform}.{firmware version}
        # Example: Alex.X.YYY.Z or Google_Alex.X.YYY.Z
        platform = crossystem.fwid().split('.')[0].lower()
        # Newer platforms start with 'Google_' while the older ones do not.
        return platform.replace('google_', '')


    def get_architecture(self):
        """Determine the correct architecture label for this host.

        @returns a string representing this host's architecture.
        """
        crossystem = utils.Crossystem(self)
        crossystem.init()
        return crossystem.arch()


    def get_chrome_version(self):
        """Gets the Chrome version number and milestone as strings.

        Invokes "chrome --version" to get the version number and milestone.

        @return A tuple (chrome_ver, milestone) where "chrome_ver" is the
            current Chrome version number as a string (in the form "W.X.Y.Z")
            and "milestone" is the first component of the version number
            (the "W" from "W.X.Y.Z").  If the version number cannot be parsed
            in the "W.X.Y.Z" format, the "chrome_ver" will be the full output
            of "chrome --version" and the milestone will be the empty string.

        """
        version_string = self.run(client_constants.CHROME_VERSION_COMMAND).stdout
        return utils.parse_chrome_version(version_string)


    @label_decorator()
    def get_board(self):
        """Determine the correct board label for this host.

        @returns a string representing this host's board.
        """
        release_info = utils.parse_cmd_output('cat /etc/lsb-release',
                                              run_method=self.run)
        return (ds_constants.BOARD_PREFIX +
                release_info['CHROMEOS_RELEASE_BOARD'])


    @label_decorator('lightsensor')
    def has_lightsensor(self):
        """Determine the correct board label for this host.

        @returns the string 'lightsensor' if this host has a lightsensor or
                 None if it does not.
        """
        search_cmd = "find -L %s -maxdepth 4 | egrep '%s'" % (
            self._LIGHTSENSOR_SEARCH_DIR, '|'.join(self._LIGHTSENSOR_FILES))
        try:
            # Run the search cmd following the symlinks. Stderr_tee is set to
            # None as there can be a symlink loop, but the command will still
            # execute correctly with a few messages printed to stderr.
            self.run(search_cmd, stdout_tee=None, stderr_tee=None)
            return 'lightsensor'
        except error.AutoservRunError:
            # egrep exited with a return code of 1 meaning none of the possible
            # lightsensor files existed.
            return None


    @label_decorator('bluetooth')
    def has_bluetooth(self):
        """Determine the correct board label for this host.

        @returns the string 'bluetooth' if this host has bluetooth or
                 None if it does not.
        """
        try:
            self.run('test -d /sys/class/bluetooth/hci0')
            # test exited with a return code of 0.
            return 'bluetooth'
        except error.AutoservRunError:
            # test exited with a return code 1 meaning the directory did not
            # exist.
            return None


    @label_decorator('ec')
    def get_ec(self):
        """
        Determine the type of EC on this host.

        @returns a string representing this host's embedded controller type.
        At present, it only returns "ec:cros", for Chrome OS ECs. Other types
        of EC (or none) don't return any strings, since no tests depend on
        those.
        """
        cmd = 'mosys ec info'
        # The output should look like these, so that the last field should
        # match our EC version scheme:
        #
        #   stm | stm32f100 | snow_v1.3.139-375eb9f
        #   ti | Unknown-10de | peppy_v1.5.114-5d52788
        #
        # Non-Chrome OS ECs will look like these:
        #
        #   ENE | KB932 | 00BE107A00
        #   ite | it8518 | 3.08
        #
        # And some systems don't have ECs at all (Lumpy, for example).
        regexp = r'^.*\|\s*(\S+_v\d+\.\d+\.\d+-[0-9a-f]+)\s*$'

        ecinfo = self.run(command=cmd, ignore_status=True)
        if ecinfo.exit_status == 0:
            res = re.search(regexp, ecinfo.stdout)
            if res:
                logging.info("EC version is %s", res.groups()[0])
                return 'ec:cros'
            logging.info("%s got: %s", cmd, ecinfo.stdout)
            # Has an EC, but it's not a Chrome OS EC
            return None
        logging.info("%s exited with status %d", cmd, ecinfo.exit_status)
        # No EC present
        return None


    @label_decorator('accels')
    def get_accels(self):
        """
        Determine the type of accelerometers on this host.

        @returns a string representing this host's accelerometer type.
        At present, it only returns "accel:cros-ec", for accelerometers
        attached to a Chrome OS EC, or none, if no accelerometers.
        """
        # Check to make sure we have ectool
        rv = self.run('which ectool', ignore_status=True)
        if rv.exit_status:
            logging.info("No ectool cmd found, assuming no EC accelerometers")
            return None

        # Check that the EC supports the motionsense command
        rv = self.run('ectool motionsense', ignore_status=True)
        if rv.exit_status:
            logging.info("EC does not support motionsense command "
                         "assuming no EC accelerometers")
            return None

        # Check that EC motion sensors are active
        active = self.run('ectool motionsense active').stdout.split('\n')
        if active[0] == "0":
            logging.info("Motion sense inactive, assuming no EC accelerometers")
            return None

        logging.info("EC accelerometers found")
        return 'accel:cros-ec'


    @label_decorator('chameleon')
    def has_chameleon(self):
        """Determine if a Chameleon connected to this host.

        @returns a list containing two strings ('chameleon' and
                 'chameleon:' + label, e.g. 'chameleon:hdmi') if this host
                 has a Chameleon or None if it has not.
        """
        if self._chameleon_host:
            return ['chameleon', 'chameleon:' + self.chameleon.get_label()]
        else:
            return None


    @label_decorator('audio_loopback_dongle')
    def has_loopback_dongle(self):
        """Determine if an audio loopback dongle is plugged to this host.

        @returns 'audio_loopback_dongle' when there is an audio loopback dongle
                                         plugged to this host.
                 None                    when there is no audio loopback dongle
                                         plugged to this host.
        """
        nodes_info = self.run(command=cras_utils.get_cras_nodes_cmd(),
                              ignore_status=True).stdout
        if (cras_utils.node_type_is_plugged('HEADPHONE', nodes_info) and
            cras_utils.node_type_is_plugged('MIC', nodes_info)):
                return 'audio_loopback_dongle'
        else:
                return None


    @label_decorator('power_supply')
    def get_power_supply(self):
        """
        Determine what type of power supply the host has

        @returns a string representing this host's power supply.
                 'power:battery' when the device has a battery intended for
                        extended use
                 'power:AC_primary' when the device has a battery not intended
                        for extended use (for moving the machine, etc)
                 'power:AC_only' when the device has no battery at all.
        """
        psu = self.run(command='mosys psu type', ignore_status=True)
        if psu.exit_status:
            # The psu command for mosys is not included for all platforms. The
            # assumption is that the device will have a battery if the command
            # is not found.
            return 'power:battery'

        psu_str = psu.stdout.strip()
        if psu_str == 'unknown':
            return None

        return 'power:%s' % psu_str


    @label_decorator('storage')
    def get_storage(self):
        """
        Determine the type of boot device for this host.

        Determine if the internal device is SCSI or dw_mmc device.
        Then check that it is SSD or HDD or eMMC or something else.

        @returns a string representing this host's internal device type.
                 'storage:ssd' when internal device is solid state drive
                 'storage:hdd' when internal device is hard disk drive
                 'storage:mmc' when internal device is mmc drive
                 None          When internal device is something else or
                               when we are unable to determine the type
        """
        # The output should be /dev/mmcblk* for SD/eMMC or /dev/sd* for scsi
        rootdev_cmd = ' '.join(['. /usr/sbin/write_gpt.sh;',
                                '. /usr/share/misc/chromeos-common.sh;',
                                'load_base_vars;',
                                'get_fixed_dst_drive'])
        rootdev = self.run(command=rootdev_cmd, ignore_status=True)
        if rootdev.exit_status:
            logging.info("Fail to run %s", rootdev_cmd)
            return None
        rootdev_str = rootdev.stdout.strip()

        if not rootdev_str:
            return None

        rootdev_base = os.path.basename(rootdev_str)

        mmc_pattern = '/dev/mmcblk[0-9]'
        if re.match(mmc_pattern, rootdev_str):
            # Use type to determine if the internal device is eMMC or somthing
            # else. We can assume that MMC is always an internal device.
            type_cmd = 'cat /sys/block/%s/device/type' % rootdev_base
            type = self.run(command=type_cmd, ignore_status=True)
            if type.exit_status:
                logging.info("Fail to run %s", type_cmd)
                return None
            type_str = type.stdout.strip()

            if type_str == 'MMC':
                return 'storage:mmc'

        scsi_pattern = '/dev/sd[a-z]+'
        if re.match(scsi_pattern, rootdev.stdout):
            # Read symlink for /sys/block/sd* to determine if the internal
            # device is connected via ata or usb.
            link_cmd = 'readlink /sys/block/%s' % rootdev_base
            link = self.run(command=link_cmd, ignore_status=True)
            if link.exit_status:
                logging.info("Fail to run %s", link_cmd)
                return None
            link_str = link.stdout.strip()
            if 'usb' in link_str:
                return None

            # Read rotation to determine if the internal device is ssd or hdd.
            rotate_cmd = str('cat /sys/block/%s/queue/rotational'
                              % rootdev_base)
            rotate = self.run(command=rotate_cmd, ignore_status=True)
            if rotate.exit_status:
                logging.info("Fail to run %s", rotate_cmd)
                return None
            rotate_str = rotate.stdout.strip()

            rotate_dict = {'0':'storage:ssd', '1':'storage:hdd'}
            return rotate_dict.get(rotate_str)

        # All other internal device / error case will always fall here
        return None


    @label_decorator('servo')
    def get_servo(self):
        """Determine if the host has a servo attached.

        If the host has a working servo attached, it should have a servo label.

        @return: string 'servo' if the host has servo attached. Otherwise,
                 returns None.
        """
        return 'servo' if self._servo_host else None


    @label_decorator('video_labels')
    def get_video_labels(self):
        """Run /usr/local/bin/avtest_label_detect to get a list of video labels.

        Sample output of avtest_label_detect:
        Detected label: hw_video_acc_vp8
        Detected label: webcam

        @return: A list of labels detected by tool avtest_label_detect.
        """
        try:
            result = self.run('/usr/local/bin/avtest_label_detect').stdout
            return re.findall('^Detected label: (\w+)$', result, re.M)
        except error.AutoservRunError:
            # The tool is not installed.
            return []


    @label_decorator('video_glitch_detection')
    def is_video_glitch_detection_supported(self):
        """ Determine if a board under test is supported for video glitch
        detection tests.

        @return: 'video_glitch_detection' if board is supported, None otherwise.
        """
        board = self.get_board().replace(ds_constants.BOARD_PREFIX, '')

        if board in video_test_constants.SUPPORTED_BOARDS:
            return 'video_glitch_detection'

        return None


    @label_decorator('touch_labels')
    def get_touch(self):
        """
        Determine whether board under test has a touchpad or touchscreen.

        @return: A list of some combination of 'touchscreen' and 'touchpad',
            depending on what is present on the device.

        """
        labels = []
        looking_for = ['touchpad', 'touchscreen']
        player = input_playback.InputPlayback()
        input_events = self.run('ls /dev/input/event*').stdout.strip().split()
        filename = '/tmp/touch_labels'
        for event in input_events:
            self.run('evtest %s > %s' % (event, filename), timeout=1,
                     ignore_timeout=True)
            properties = self.run('cat %s' % filename).stdout
            input_type = player._determine_input_type(properties)
            if input_type in looking_for:
                labels.append(input_type)
                looking_for.remove(input_type)
            if len(looking_for) == 0:
                break
        self.run('rm %s' % filename)

        return labels


    @label_decorator('internal_display')
    def has_internal_display(self):
        """Determine if the device under test is equipped with an internal
        display.

        @return: 'internal_display' if one is present; None otherwise.
        """
        from autotest_lib.client.cros.graphics import graphics_utils
        from autotest_lib.client.common_lib import utils as common_utils

        def __system_output(cmd):
            return self.run(cmd).stdout

        def __read_file(remote_path):
            return self.run('cat %s' % remote_path).stdout

        # Hijack the necessary client functions so that we can take advantage
        # of the client lib here.
        # FIXME: find a less hacky way than this
        original_system_output = utils.system_output
        original_read_file = common_utils.read_file
        utils.system_output = __system_output
        common_utils.read_file = __read_file
        try:
            return ('internal_display' if graphics_utils.has_internal_display()
                                   else None)
        finally:
            utils.system_output = original_system_output
            common_utils.read_file = original_read_file


    @label_decorator('lucidsleep')
    def has_lucid_sleep_support(self):
        """Determine if the device under test has support for lucid sleep.

        @return 'lucidsleep' if this board supports lucid sleep; None otherwise
        """
        board = self.get_board().replace(ds_constants.BOARD_PREFIX, '')
        return 'lucidsleep' if board in LUCID_SLEEP_BOARDS else None


    def is_boot_from_usb(self):
        """Check if DUT is boot from USB.

        @return: True if DUT is boot from usb.
        """
        device = self.run('rootdev -s -d').stdout.strip()
        removable = int(self.run('cat /sys/block/%s/removable' %
                                 os.path.basename(device)).stdout.strip())
        return removable == 1


    def read_from_meminfo(self, key):
        """Return the memory info from /proc/meminfo

        @param key: meminfo requested

        @return the memory value as a string

        """
        meminfo = self.run('grep %s /proc/meminfo' % key).stdout.strip()
        logging.debug('%s', meminfo)
        return int(re.search(r'\d+', meminfo).group(0))


    def get_board_type(self):
        """
        Get the DUT's device type from /etc/lsb-release.
        DEVICETYPE can be one of CHROMEBOX, CHROMEBASE, CHROMEBOOK or more.

        @return value of DEVICETYPE param from lsb-release.
        """
        device_type = self.run('grep DEVICETYPE /etc/lsb-release',
                               ignore_status=True).stdout
        if device_type:
            return device_type.split('=')[-1].strip()
        return ''


    def get_os_type(self):
        return 'cros'


    def enable_adb_testing(self):
        """Mark this host as an adb tester."""
        self.run('touch %s' % constants.ANDROID_TESTER_FILEFLAG)
