# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import ast
import ctypes
import logging
import os
import pprint
import re
import time
import uuid

from threading import Timer
from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib import error
from autotest_lib.server import test
from autotest_lib.server.cros import vboot_constants as vboot
from autotest_lib.server.cros.faft.config.config import Config as FAFTConfig
from autotest_lib.server.cros.faft.rpc_proxy import RPCProxy
from autotest_lib.server.cros.faft.utils import mode_switcher
from autotest_lib.server.cros.faft.utils.faft_checkers import FAFTCheckers
from autotest_lib.server.cros.servo import chrome_ec
from autotest_lib.server.cros.servo import chrome_usbpd

ConnectionError = mode_switcher.ConnectionError


class FAFTBase(test.test):
    """The base class of FAFT classes.

    It launches the FAFTClient on DUT, such that the test can access its
    firmware functions and interfaces. It also provides some methods to
    handle the reboot mechanism, in order to ensure FAFTClient is still
    connected after reboot.
    """
    def initialize(self, host):
        """Create a FAFTClient object and install the dependency."""
        self.servo = host.servo
        self.servo.initialize_dut()
        self._client = host
        self.faft_client = RPCProxy(host)
        self.lockfile = '/var/tmp/faft/lock'


class FirmwareTest(FAFTBase):
    """
    Base class that sets up helper objects/functions for firmware tests.

    TODO: add documentaion as the FAFT rework progresses.
    """
    version = 1

    # Mapping of partition number of kernel and rootfs.
    KERNEL_MAP = {'a':'2', 'b':'4', '2':'2', '4':'4', '3':'2', '5':'4'}
    ROOTFS_MAP = {'a':'3', 'b':'5', '2':'3', '4':'5', '3':'3', '5':'5'}
    OTHER_KERNEL_MAP = {'a':'4', 'b':'2', '2':'4', '4':'2', '3':'4', '5':'2'}
    OTHER_ROOTFS_MAP = {'a':'5', 'b':'3', '2':'5', '4':'3', '3':'5', '5':'3'}

    CHROMEOS_MAGIC = "CHROMEOS"
    CORRUPTED_MAGIC = "CORRUPTD"

    # Delay for waiting client to return before EC suspend
    EC_SUSPEND_DELAY = 5

    # Delay between EC suspend and wake
    WAKE_DELAY = 10

    # Delay between closing and opening lid
    LID_DELAY = 1

    _SERVOD_LOG = '/var/log/servod.log'

    _ROOTFS_PARTITION_NUMBER = 3

    _backup_firmware_sha = ()
    _backup_kernel_sha = dict()
    _backup_cgpt_attr = dict()
    _backup_gbb_flags = None
    _backup_dev_mode = None

    # Class level variable, keep track the states of one time setup.
    # This variable is preserved across tests which inherit this class.
    _global_setup_done = {
        'gbb_flags': False,
        'reimage': False,
        'usb_check': False,
    }

    @classmethod
    def check_setup_done(cls, label):
        """Check if the given setup is done.

        @param label: The label of the setup.
        """
        return cls._global_setup_done[label]

    @classmethod
    def mark_setup_done(cls, label):
        """Mark the given setup done.

        @param label: The label of the setup.
        """
        cls._global_setup_done[label] = True

    @classmethod
    def unmark_setup_done(cls, label):
        """Mark the given setup not done.

        @param label: The label of the setup.
        """
        cls._global_setup_done[label] = False

    def initialize(self, host, cmdline_args, ec_wp=None):
        super(FirmwareTest, self).initialize(host)
        self.run_id = str(uuid.uuid4())
        logging.info('FirmwareTest initialize begin (id=%s)', self.run_id)
        # Parse arguments from command line
        args = {}
        self.power_control = host.POWER_CONTROL_RPM
        for arg in cmdline_args:
            match = re.search("^(\w+)=(.+)", arg)
            if match:
                args[match.group(1)] = match.group(2)
        if 'power_control' in args:
            self.power_control = args['power_control']
            if self.power_control not in host.POWER_CONTROL_VALID_ARGS:
                raise error.TestError('Valid values for --args=power_control '
                                      'are %s. But you entered wrong argument '
                                      'as "%s".'
                                       % (host.POWER_CONTROL_VALID_ARGS,
                                       self.power_control))

        self.faft_config = FAFTConfig(
                self.faft_client.system.get_platform_name())
        self.checkers = FAFTCheckers(self)
        self.switcher = mode_switcher.create_mode_switcher(self)

        if self.faft_config.chrome_ec:
            self.ec = chrome_ec.ChromeEC(self.servo)
        # Check for presence of a USBPD console
        if self.faft_config.chrome_usbpd:
            self.usbpd = chrome_usbpd.ChromeUSBPD(self.servo)
        elif self.faft_config.chrome_ec:
            # If no separate USBPD console, then PD exists on EC console
            self.usbpd = self.ec
        # Get plankton console
        self.plankton = host.plankton
        self.plankton_host = host._plankton_host

        self._setup_uart_capture()
        self._setup_servo_log()
        self._record_system_info()
        self.fw_vboot2 = self.faft_client.system.get_fw_vboot2()
        logging.info('vboot version: %d', 2 if self.fw_vboot2 else 1)
        if self.fw_vboot2:
            self.faft_client.system.set_fw_try_next('A')
            if self.faft_client.system.get_crossystem_value('mainfw_act') == 'B':
                logging.info('mainfw_act is B. rebooting to set it A')
                self.switcher.mode_aware_reboot()
        self._setup_gbb_flags()
        self._stop_service('update-engine')
        self._create_faft_lockfile()
        self._setup_ec_write_protect(ec_wp)
        # See chromium:239034 regarding needing this sync.
        self.blocking_sync()
        logging.info('FirmwareTest initialize done (id=%s)', self.run_id)

    def cleanup(self):
        """Autotest cleanup function."""
        # Unset state checker in case it's set by subclass
        logging.info('FirmwareTest cleaning up (id=%s)', self.run_id)
        try:
            self.faft_client.system.is_available()
        except:
            # Remote is not responding. Revive DUT so that subsequent tests
            # don't fail.
            self._restore_routine_from_timeout()
        self.switcher.restore_mode()
        self._restore_ec_write_protect()
        self._restore_gbb_flags()
        self._start_service('update-engine')
        self._remove_faft_lockfile()
        self._record_servo_log()
        self._record_faft_client_log()
        self._cleanup_uart_capture()
        super(FirmwareTest, self).cleanup()
        logging.info('FirmwareTest cleanup done (id=%s)', self.run_id)

    def _record_system_info(self):
        """Record some critical system info to the attr keyval.

        This info is used by generate_test_report later.
        """
        system_info = {
            'hwid': self.faft_client.system.get_crossystem_value('hwid'),
            'ec_version': self.faft_client.ec.get_version(),
            'ro_fwid': self.faft_client.system.get_crossystem_value('ro_fwid'),
            'rw_fwid': self.faft_client.system.get_crossystem_value('fwid'),
            'servod_version': self._client._servo_host.run(
                'servod --version').stdout.strip(),
        }
        logging.info('System info:\n' + pprint.pformat(system_info))
        self.write_attr_keyval(system_info)

    def invalidate_firmware_setup(self):
        """Invalidate all firmware related setup state.

        This method is called when the firmware is re-flashed. It resets all
        firmware related setup states so that the next test setup properly
        again.
        """
        self.unmark_setup_done('gbb_flags')

    def _retrieve_recovery_reason_from_trap(self):
        """Try to retrieve the recovery reason from a trapped recovery screen.

        @return: The recovery_reason, 0 if any error.
        """
        recovery_reason = 0
        logging.info('Try to retrieve recovery reason...')
        if self.servo.get_usbkey_direction() == 'dut':
            self.switcher.bypass_rec_mode()
        else:
            self.servo.switch_usbkey('dut')

        try:
            self.switcher.wait_for_client()
            lines = self.faft_client.system.run_shell_command_get_output(
                        'crossystem recovery_reason')
            recovery_reason = int(lines[0])
            logging.info('Got the recovery reason %d.', recovery_reason)
        except ConnectionError:
            logging.error('Failed to get the recovery reason due to connection '
                          'error.')
        return recovery_reason

    def _reset_client(self):
        """Reset client to a workable state.

        This method is called when the client is not responsive. It may be
        caused by the following cases:
          - halt on a firmware screen without timeout, e.g. REC_INSERT screen;
          - corrupted firmware;
          - corrutped OS image.
        """
        # DUT may halt on a firmware screen. Try cold reboot.
        logging.info('Try cold reboot...')
        self.switcher.mode_aware_reboot(reboot_type='cold',
                                        sync_before_boot=False,
                                        wait_for_dut_up=False)
        self.switcher.wait_for_client_offline()
        self.switcher.bypass_dev_mode()
        try:
            self.switcher.wait_for_client()
            return
        except ConnectionError:
            logging.warn('Cold reboot doesn\'t help, still connection error.')

        # DUT may be broken by a corrupted firmware. Restore firmware.
        # We assume the recovery boot still works fine. Since the recovery
        # code is in RO region and all FAFT tests don't change the RO region
        # except GBB.
        if self.is_firmware_saved():
            self._ensure_client_in_recovery()
            logging.info('Try restore the original firmware...')
            if self.is_firmware_changed():
                try:
                    self.restore_firmware()
                    return
                except ConnectionError:
                    logging.warn('Restoring firmware doesn\'t help, still '
                                 'connection error.')

        # Perhaps it's kernel that's broken. Let's try restoring it.
        if self.is_kernel_saved():
            self._ensure_client_in_recovery()
            logging.info('Try restore the original kernel...')
            if self.is_kernel_changed():
                try:
                    self.restore_kernel()
                    return
                except ConnectionError:
                    logging.warn('Restoring kernel doesn\'t help, still '
                                 'connection error.')

        # DUT may be broken by a corrupted OS image. Restore OS image.
        self._ensure_client_in_recovery()
        logging.info('Try restore the OS image...')
        self.faft_client.system.run_shell_command('chromeos-install --yes')
        self.switcher.mode_aware_reboot(wait_for_dut_up=False)
        self.switcher.wait_for_client_offline()
        self.switcher.bypass_dev_mode()
        try:
            self.switcher.wait_for_client()
            logging.info('Successfully restore OS image.')
            return
        except ConnectionError:
            logging.warn('Restoring OS image doesn\'t help, still connection '
                         'error.')

    def _ensure_client_in_recovery(self):
        """Ensure client in recovery boot; reboot into it if necessary.

        @raise TestError: if failed to boot the USB image.
        """
        logging.info('Try boot into USB image...')
        self.switcher.reboot_to_mode(to_mode='rec', sync_before_boot=False,
                                     wait_for_dut_up=False)
        self.servo.switch_usbkey('host')
        self.switcher.bypass_rec_mode()
        try:
            self.switcher.wait_for_client()
        except ConnectionError:
            raise error.TestError('Failed to boot the USB image.')

    def _restore_routine_from_timeout(self):
        """A routine to try to restore the system from a timeout error.

        This method is called when FAFT failed to connect DUT after reboot.

        @raise TestFail: This exception is already raised, with a decription
                         why it failed.
        """
        # DUT is disconnected. Capture the UART output for debug.
        self._record_uart_capture()

        # TODO(waihong@chromium.org): Implement replugging the Ethernet to
        # identify if it is a network flaky.

        recovery_reason = self._retrieve_recovery_reason_from_trap()

        # Reset client to a workable state.
        self._reset_client()

        # Raise the proper TestFail exception.
        if recovery_reason:
            raise error.TestFail('Trapped in the recovery screen (reason: %d) '
                                 'and timed out' % recovery_reason)
        else:
            raise error.TestFail('Timed out waiting for DUT reboot')

    def assert_test_image_in_usb_disk(self, usb_dev=None):
        """Assert an USB disk plugged-in on servo and a test image inside.

        @param usb_dev: A string of USB stick path on the host, like '/dev/sdc'.
                        If None, it is detected automatically.
        @raise TestError: if USB disk not detected or not a test image.
        """
        if self.check_setup_done('usb_check'):
            return
        if usb_dev:
            assert self.servo.get_usbkey_direction() == 'host'
        else:
            self.servo.switch_usbkey('host')
            usb_dev = self.servo.probe_host_usb_dev()
            if not usb_dev:
                raise error.TestError(
                        'An USB disk should be plugged in the servo board.')

        rootfs = '%s%s' % (usb_dev, self._ROOTFS_PARTITION_NUMBER)
        logging.info('usb dev is %s', usb_dev)
        tmpd = self.servo.system_output('mktemp -d -t usbcheck.XXXX')
        self.servo.system('mount -o ro %s %s' % (rootfs, tmpd))

        try:
            usb_lsb = self.servo.system_output('cat %s' %
                os.path.join(tmpd, 'etc/lsb-release'))
            logging.debug('Dumping lsb-release on USB stick:\n%s', usb_lsb)
            dut_lsb = '\n'.join(self.faft_client.system.
                run_shell_command_get_output('cat /etc/lsb-release'))
            logging.debug('Dumping lsb-release on DUT:\n%s', dut_lsb)
            if not re.search(r'RELEASE_TRACK=.*test', usb_lsb):
                raise error.TestError('USB stick in servo is no test image')
            usb_board = re.search(r'BOARD=(.*)', usb_lsb).group(1)
            dut_board = re.search(r'BOARD=(.*)', dut_lsb).group(1)
            if usb_board != dut_board:
                raise error.TestError('USB stick in servo contains a %s '
                    'image, but DUT is a %s' % (usb_board, dut_board))
        finally:
            for cmd in ('umount %s' % rootfs, 'sync', 'rm -rf %s' % tmpd):
                self.servo.system(cmd)

        self.mark_setup_done('usb_check')

    def setup_usbkey(self, usbkey, host=None):
        """Setup the USB disk for the test.

        It checks the setup of USB disk and a valid ChromeOS test image inside.
        It also muxes the USB disk to either the host or DUT by request.

        @param usbkey: True if the USB disk is required for the test, False if
                       not required.
        @param host: Optional, True to mux the USB disk to host, False to mux it
                    to DUT, default to do nothing.
        """
        if usbkey:
            self.assert_test_image_in_usb_disk()
        elif host is None:
            # USB disk is not required for the test. Better to mux it to host.
            host = True

        if host is True:
            self.servo.switch_usbkey('host')
        elif host is False:
            self.servo.switch_usbkey('dut')

    def get_usbdisk_path_on_dut(self):
        """Get the path of the USB disk device plugged-in the servo on DUT.

        Returns:
          A string representing USB disk path, like '/dev/sdb', or None if
          no USB disk is found.
        """
        cmd = 'ls -d /dev/s*[a-z]'
        original_value = self.servo.get_usbkey_direction()

        # Make the dut unable to see the USB disk.
        self.servo.switch_usbkey('off')
        no_usb_set = set(
            self.faft_client.system.run_shell_command_get_output(cmd))

        # Make the dut able to see the USB disk.
        self.servo.switch_usbkey('dut')
        time.sleep(self.faft_config.usb_plug)
        has_usb_set = set(
            self.faft_client.system.run_shell_command_get_output(cmd))

        # Back to its original value.
        if original_value != self.servo.get_usbkey_direction():
            self.servo.switch_usbkey(original_value)

        diff_set = has_usb_set - no_usb_set
        if len(diff_set) == 1:
            return diff_set.pop()
        else:
            return None

    def _create_faft_lockfile(self):
        """Creates the FAFT lockfile."""
        logging.info('Creating FAFT lockfile...')
        command = 'touch %s' % (self.lockfile)
        self.faft_client.system.run_shell_command(command)

    def _remove_faft_lockfile(self):
        """Removes the FAFT lockfile."""
        logging.info('Removing FAFT lockfile...')
        command = 'rm -f %s' % (self.lockfile)
        self.faft_client.system.run_shell_command(command)

    def _stop_service(self, service):
        """Stops a upstart service on the client.

        @param service: The name of the upstart service.
        """
        logging.info('Stopping %s...', service)
        command = 'status %s | grep stop || stop %s' % (service, service)
        self.faft_client.system.run_shell_command(command)

    def _start_service(self, service):
        """Starts a upstart service on the client.

        @param service: The name of the upstart service.
        """
        logging.info('Starting %s...', service)
        command = 'status %s | grep start || start %s' % (service, service)
        self.faft_client.system.run_shell_command(command)

    def clear_set_gbb_flags(self, clear_mask, set_mask):
        """Clear and set the GBB flags in the current flashrom.

        @param clear_mask: A mask of flags to be cleared.
        @param set_mask: A mask of flags to be set.
        """
        gbb_flags = self.faft_client.bios.get_gbb_flags()
        new_flags = gbb_flags & ctypes.c_uint32(~clear_mask).value | set_mask
        if new_flags != gbb_flags:
            self._backup_gbb_flags = gbb_flags
            logging.info('Changing GBB flags from 0x%x to 0x%x.',
                         gbb_flags, new_flags)
            self.faft_client.bios.set_gbb_flags(new_flags)
            # If changing FORCE_DEV_SWITCH_ON flag, reboot to get a clear state
            if ((gbb_flags ^ new_flags) & vboot.GBB_FLAG_FORCE_DEV_SWITCH_ON):
                self.switcher.mode_aware_reboot()
        else:
            logging.info('Current GBB flags look good for test: 0x%x.',
                         gbb_flags)

    def check_ec_capability(self, required_cap=None, suppress_warning=False):
        """Check if current platform has required EC capabilities.

        @param required_cap: A list containing required EC capabilities. Pass in
                             None to only check for presence of Chrome EC.
        @param suppress_warning: True to suppress any warning messages.
        @return: True if requirements are met. Otherwise, False.
        """
        if not self.faft_config.chrome_ec:
            if not suppress_warning:
                logging.warn('Requires Chrome EC to run this test.')
            return False

        if not required_cap:
            return True

        for cap in required_cap:
            if cap not in self.faft_config.ec_capability:
                if not suppress_warning:
                    logging.warn('Requires EC capability "%s" to run this '
                                 'test.', cap)
                return False

        return True

    def check_root_part_on_non_recovery(self, part):
        """Check the partition number of root device and on normal/dev boot.

        @param part: A string of partition number, e.g.'3'.
        @return: True if the root device matched and on normal/dev boot;
                 otherwise, False.
        """
        return self.checkers.root_part_checker(part) and \
                self.checkers.crossystem_checker({
                    'mainfw_type': ('normal', 'developer'),
                })

    def _join_part(self, dev, part):
        """Return a concatenated string of device and partition number.

        @param dev: A string of device, e.g.'/dev/sda'.
        @param part: A string of partition number, e.g.'3'.
        @return: A concatenated string of device and partition number,
                 e.g.'/dev/sda3'.

        >>> seq = FirmwareTest()
        >>> seq._join_part('/dev/sda', '3')
        '/dev/sda3'
        >>> seq._join_part('/dev/mmcblk0', '2')
        '/dev/mmcblk0p2'
        """
        if 'mmcblk' in dev:
            return dev + 'p' + part
        else:
            return dev + part

    def copy_kernel_and_rootfs(self, from_part, to_part):
        """Copy kernel and rootfs from from_part to to_part.

        @param from_part: A string of partition number to be copied from.
        @param to_part: A string of partition number to be copied to.
        """
        root_dev = self.faft_client.system.get_root_dev()
        logging.info('Copying kernel from %s to %s. Please wait...',
                     from_part, to_part)
        self.faft_client.system.run_shell_command('dd if=%s of=%s bs=4M' %
                (self._join_part(root_dev, self.KERNEL_MAP[from_part]),
                 self._join_part(root_dev, self.KERNEL_MAP[to_part])))
        logging.info('Copying rootfs from %s to %s. Please wait...',
                     from_part, to_part)
        self.faft_client.system.run_shell_command('dd if=%s of=%s bs=4M' %
                (self._join_part(root_dev, self.ROOTFS_MAP[from_part]),
                 self._join_part(root_dev, self.ROOTFS_MAP[to_part])))

    def ensure_kernel_boot(self, part):
        """Ensure the request kernel boot.

        If not, it duplicates the current kernel to the requested kernel
        and sets the requested higher priority to ensure it boot.

        @param part: A string of kernel partition number or 'a'/'b'.
        """
        if not self.checkers.root_part_checker(part):
            if self.faft_client.kernel.diff_a_b():
                self.copy_kernel_and_rootfs(
                        from_part=self.OTHER_KERNEL_MAP[part],
                        to_part=part)
            self.reset_and_prioritize_kernel(part)
            self.switcher.mode_aware_reboot()

    def set_hardware_write_protect(self, enable):
        """Set hardware write protect pin.

        @param enable: True if asserting write protect pin. Otherwise, False.
        """
        self.servo.set('fw_wp_vref', self.faft_config.wp_voltage)
        self.servo.set('fw_wp_en', 'on')
        self.servo.set('fw_wp', 'on' if enable else 'off')

    def set_ec_write_protect_and_reboot(self, enable):
        """Set EC write protect status and reboot to take effect.

        The write protect state is only activated if both hardware write
        protect pin is asserted and software write protect flag is set.
        This method asserts/deasserts hardware write protect pin first, and
        set corresponding EC software write protect flag.

        If the device uses non-Chrome EC, set the software write protect via
        flashrom.

        If the device uses Chrome EC, a reboot is required for write protect
        to take effect. Since the software write protect flag cannot be unset
        if hardware write protect pin is asserted, we need to deasserted the
        pin first if we are deactivating write protect. Similarly, a reboot
        is required before we can modify the software flag.

        @param enable: True if activating EC write protect. Otherwise, False.
        """
        self.set_hardware_write_protect(enable)
        if self.faft_config.chrome_ec:
            self.set_chrome_ec_write_protect_and_reboot(enable)
        else:
            self.faft_client.ec.set_write_protect(enable)
            self.switcher.mode_aware_reboot()

    def set_chrome_ec_write_protect_and_reboot(self, enable):
        """Set Chrome EC write protect status and reboot to take effect.

        @param enable: True if activating EC write protect. Otherwise, False.
        """
        if enable:
            # Set write protect flag and reboot to take effect.
            self.ec.set_flash_write_protect(enable)
            self.sync_and_ec_reboot()
        else:
            # Reboot after deasserting hardware write protect pin to deactivate
            # write protect. And then remove software write protect flag.
            self.sync_and_ec_reboot()
            self.ec.set_flash_write_protect(enable)

    def _setup_ec_write_protect(self, ec_wp):
        """Setup for EC write-protection.

        It makes sure the EC in the requested write-protection state. If not, it
        flips the state. Flipping the write-protection requires DUT reboot.

        @param ec_wp: True to request EC write-protected; False to request EC
                      not write-protected; None to do nothing.
        """
        if ec_wp is None:
            self._old_ec_wp = None
            return
        self._old_ec_wp = self.checkers.crossystem_checker({'wpsw_boot': '1'})
        if ec_wp != self._old_ec_wp:
            logging.info('The test required EC is %swrite-protected. Reboot '
                         'and flip the state.', '' if ec_wp else 'not ')
            self.switcher.mode_aware_reboot(
                    'custom',
                     lambda:self.set_ec_write_protect_and_reboot(ec_wp))

    def _restore_ec_write_protect(self):
        """Restore the original EC write-protection."""
        if (not hasattr(self, '_old_ec_wp')) or (self._old_ec_wp is None):
            return
        if not self.checkers.crossystem_checker(
                {'wpsw_boot': '1' if self._old_ec_wp else '0'}):
            logging.info('Restore original EC write protection and reboot.')
            self.switcher.mode_aware_reboot(
                    'custom',
                    lambda:self.set_ec_write_protect_and_reboot(
                            self._old_ec_wp))

    def _setup_uart_capture(self):
        """Setup the CPU/EC/PD UART capture."""
        self.cpu_uart_file = os.path.join(self.resultsdir, 'cpu_uart.txt')
        self.servo.set('cpu_uart_capture', 'on')
        self.ec_uart_file = None
        self.usbpd_uart_file = None
        if self.faft_config.chrome_ec:
            try:
                self.servo.set('ec_uart_capture', 'on')
                self.ec_uart_file = os.path.join(self.resultsdir, 'ec_uart.txt')
            except error.TestFail as e:
                if 'No control named' in str(e):
                    logging.warn('The servod is too old that ec_uart_capture '
                                 'not supported.')
            # Log separate PD console if supported
            if self.check_ec_capability(['usbpd_uart'], suppress_warning=True):
                try:
                    self.servo.set('usbpd_uart_capture', 'on')
                    self.usbpd_uart_file = os.path.join(self.resultsdir,
                                                        'usbpd_uart.txt')
                except error.TestFail as e:
                    if 'No control named' in str(e):
                        logging.warn('The servod is too old that '
                                     'usbpd_uart_capture is not supported.')
        else:
            logging.info('Not a Google EC, cannot capture ec console output.')

    def _record_uart_capture(self):
        """Record the CPU/EC/PD UART output stream to files."""
        if self.cpu_uart_file:
            with open(self.cpu_uart_file, 'a') as f:
                f.write(ast.literal_eval(self.servo.get('cpu_uart_stream')))
        if self.ec_uart_file and self.faft_config.chrome_ec:
            with open(self.ec_uart_file, 'a') as f:
                f.write(ast.literal_eval(self.servo.get('ec_uart_stream')))
        if (self.usbpd_uart_file and self.faft_config.chrome_ec and
            self.check_ec_capability(['usbpd_uart'], suppress_warning=True)):
            with open(self.usbpd_uart_file, 'a') as f:
                f.write(ast.literal_eval(self.servo.get('usbpd_uart_stream')))

    def _cleanup_uart_capture(self):
        """Cleanup the CPU/EC/PD UART capture."""
        # Flush the remaining UART output.
        self._record_uart_capture()
        self.servo.set('cpu_uart_capture', 'off')
        if self.ec_uart_file and self.faft_config.chrome_ec:
            self.servo.set('ec_uart_capture', 'off')
        if (self.usbpd_uart_file and self.faft_config.chrome_ec and
            self.check_ec_capability(['usbpd_uart'], suppress_warning=True)):
            self.servo.set('usbpd_uart_capture', 'off')

    def _get_power_state(self, power_state):
        """
        Return the current power state of the AP
        """
        return self.ec.send_command_get_output("powerinfo", [power_state])

    def wait_power_state(self, power_state, retries):
        """
        Wait for certain power state.

        @param power_state: power state you are expecting
        @param retries: retries.  This is necessary if AP is powering down
        and transitioning through different states.
        """
        logging.info('Checking power state "%s" maximum %d times.',
                     power_state, retries)
        while retries > 0:
            logging.info("try count: %d" % retries)
            try:
                retries = retries - 1
                ret = self._get_power_state(power_state)
                return True
            except error.TestFail:
                pass
        return False

    def delayed(seconds):
        def decorator(f):
            def wrapper(*args, **kargs):
                t = Timer(seconds, f, args, kargs)
                t.start()
            return wrapper
        return decorator

    @delayed(WAKE_DELAY)
    def wake_by_power_button(self):
        """Delay by WAKE_DELAY seconds and then wake DUT with power button."""
        self.servo.power_normal_press()

    @delayed(WAKE_DELAY)
    def wake_by_lid_switch(self):
        """Delay by WAKE_DELAY seconds and then wake DUT with lid switch."""
        self.servo.set('lid_open', 'no')
        time.sleep(self.LID_DELAY)
        self.servo.set('lid_open', 'yes')

    def suspend_as_reboot(self, wake_func):
        """
        Suspend DUT and also kill FAFT client so that this acts like a reboot.

        Args:
          wake_func: A function that is called to wake DUT. Note that this
            function must delay itself so that we don't wake DUT before
            suspend_as_reboot returns.
        """
        cmd = '(sleep %d; powerd_dbus_suspend) &' % self.EC_SUSPEND_DELAY
        self.faft_client.system.run_shell_command(cmd)
        self.faft_client.disconnect()
        time.sleep(self.EC_SUSPEND_DELAY)
        logging.info("wake function disabled")
        wake_func()

    def _fetch_servo_log(self):
        """Fetch the servo log."""
        cmd = '[ -e %s ] && cat %s || echo NOTFOUND' % ((self._SERVOD_LOG,) * 2)
        servo_log = self.servo.system_output(cmd)
        return None if servo_log == 'NOTFOUND' else servo_log

    def _setup_servo_log(self):
        """Setup the servo log capturing."""
        self.servo_log_original_len = -1
        if self.servo.is_localhost():
            # No servo log recorded when servod runs locally.
            return

        servo_log = self._fetch_servo_log()
        if servo_log:
            self.servo_log_original_len = len(servo_log)
        else:
            logging.warn('Servo log file not found.')

    def _record_servo_log(self):
        """Record the servo log to the results directory."""
        if self.servo_log_original_len != -1:
            servo_log = self._fetch_servo_log()
            servo_log_file = os.path.join(self.resultsdir, 'servod.log')
            with open(servo_log_file, 'a') as f:
                f.write(servo_log[self.servo_log_original_len:])

    def _record_faft_client_log(self):
        """Record the faft client log to the results directory."""
        client_log = self.faft_client.system.dump_log(True)
        client_log_file = os.path.join(self.resultsdir, 'faft_client.log')
        with open(client_log_file, 'w') as f:
            f.write(client_log)

    def _setup_gbb_flags(self):
        """Setup the GBB flags for FAFT test."""
        if self.faft_config.gbb_version < 1.1:
            logging.info('Skip modifying GBB on versions older than 1.1.')
            return

        if self.check_setup_done('gbb_flags'):
            return

        logging.info('Set proper GBB flags for test.')
        self.clear_set_gbb_flags(vboot.GBB_FLAG_DEV_SCREEN_SHORT_DELAY |
                                 vboot.GBB_FLAG_FORCE_DEV_SWITCH_ON |
                                 vboot.GBB_FLAG_FORCE_DEV_BOOT_USB |
                                 vboot.GBB_FLAG_DISABLE_FW_ROLLBACK_CHECK |
                                 vboot.GBB_FLAG_FORCE_DEV_BOOT_FASTBOOT_FULL_CAP,
                                 vboot.GBB_FLAG_ENTER_TRIGGERS_TONORM |
                                 vboot.GBB_FLAG_FAFT_KEY_OVERIDE)
        self.mark_setup_done('gbb_flags')

    def drop_backup_gbb_flags(self):
        """Drops the backup GBB flags.

        This can be used when a test intends to permanently change GBB flags.
        """
        self._backup_gbb_flags = None

    def _restore_gbb_flags(self):
        """Restore GBB flags to their original state."""
        if self._backup_gbb_flags is None:
            return
        # Setting up and restoring the GBB flags take a lot of time. For
        # speed-up purpose, don't restore it.
        logging.info('***')
        logging.info('*** Please manually restore the original GBB flags to: '
                     '0x%x ***', self._backup_gbb_flags)
        logging.info('***')
        self.unmark_setup_done('gbb_flags')

    def setup_tried_fwb(self, tried_fwb):
        """Setup for fw B tried state.

        It makes sure the system in the requested fw B tried state. If not, it
        tries to do so.

        @param tried_fwb: True if requested in tried_fwb=1;
                          False if tried_fwb=0.
        """
        if tried_fwb:
            if not self.checkers.crossystem_checker({'tried_fwb': '1'}):
                logging.info(
                    'Firmware is not booted with tried_fwb. Reboot into it.')
                self.faft_client.system.set_try_fw_b()
        else:
            if not self.checkers.crossystem_checker({'tried_fwb': '0'}):
                logging.info(
                    'Firmware is booted with tried_fwb. Reboot to clear.')

    def power_on(self):
        """Switch DUT AC power on."""
        self._client.power_on(self.power_control)

    def power_off(self):
        """Switch DUT AC power off."""
        self._client.power_off(self.power_control)

    def power_cycle(self):
        """Power cycle DUT AC power."""
        self._client.power_cycle(self.power_control)

    def setup_rw_boot(self, section='a'):
        """Make sure firmware is in RW-boot mode.

        If the given firmware section is in RO-boot mode, turn off the RO-boot
        flag and reboot DUT into RW-boot mode.

        @param section: A firmware section, either 'a' or 'b'.
        """
        flags = self.faft_client.bios.get_preamble_flags(section)
        if flags & vboot.PREAMBLE_USE_RO_NORMAL:
            flags = flags ^ vboot.PREAMBLE_USE_RO_NORMAL
            self.faft_client.bios.set_preamble_flags(section, flags)
            self.switcher.mode_aware_reboot()

    def setup_kernel(self, part):
        """Setup for kernel test.

        It makes sure both kernel A and B bootable and the current boot is
        the requested kernel part.

        @param part: A string of kernel partition number or 'a'/'b'.
        """
        self.ensure_kernel_boot(part)
        logging.info('Checking the integrity of kernel B and rootfs B...')
        if (self.faft_client.kernel.diff_a_b() or
                not self.faft_client.rootfs.verify_rootfs('B')):
            logging.info('Copying kernel and rootfs from A to B...')
            self.copy_kernel_and_rootfs(from_part=part,
                                        to_part=self.OTHER_KERNEL_MAP[part])
        self.reset_and_prioritize_kernel(part)

    def reset_and_prioritize_kernel(self, part):
        """Make the requested partition highest priority.

        This function also reset kerenl A and B to bootable.

        @param part: A string of partition number to be prioritized.
        """
        root_dev = self.faft_client.system.get_root_dev()
        # Reset kernel A and B to bootable.
        self.faft_client.system.run_shell_command(
            'cgpt add -i%s -P1 -S1 -T0 %s' % (self.KERNEL_MAP['a'], root_dev))
        self.faft_client.system.run_shell_command(
            'cgpt add -i%s -P1 -S1 -T0 %s' % (self.KERNEL_MAP['b'], root_dev))
        # Set kernel part highest priority.
        self.faft_client.system.run_shell_command('cgpt prioritize -i%s %s' %
                (self.KERNEL_MAP[part], root_dev))

    def blocking_sync(self):
        """Run a blocking sync command."""
        # The double calls to sync fakes a blocking call
        # since the first call returns before the flush
        # is complete, but the second will wait for the
        # first to finish.
        self.faft_client.system.run_shell_command('sync')
        self.faft_client.system.run_shell_command('sync')

        # sync only sends SYNCHRONIZE_CACHE but doesn't
        # check the status. For mmc devices, use `mmc
        # status get` command to send an empty command to
        # wait for the disk to be available again.  For
        # other devices, hdparm sends TUR to check if
        # a device is ready for transfer operation.
        root_dev = self.faft_client.system.get_root_dev()
        if 'mmcblk' in root_dev:
            self.faft_client.system.run_shell_command('mmc status get %s' %
                                                      root_dev)
        else:
            self.faft_client.system.run_shell_command('hdparm -f %s' % root_dev)

    def sync_and_ec_reboot(self, flags=''):
        """Request the client sync and do a EC triggered reboot.

        @param flags: Optional, a space-separated string of flags passed to EC
                      reboot command, including:
                          default: EC soft reboot;
                          'hard': EC cold/hard reboot.
        """
        self.blocking_sync()
        self.ec.reboot(flags)
        time.sleep(self.faft_config.ec_boot_to_console)
        self.check_lid_and_power_on()

    def reboot_and_reset_tpm(self):
        """Reboot into recovery mode, reset TPM, then reboot back to disk."""
        self.switcher.reboot_to_mode(to_mode='rec')
        self.faft_client.system.run_shell_command('chromeos-tpm-recovery')
        self.switcher.mode_aware_reboot()

    def full_power_off_and_on(self):
        """Shutdown the device by pressing power button and power on again."""
        boot_id = self.get_bootid()
        # Press power button to trigger Chrome OS normal shutdown process.
        # We use a customized delay since the normal-press 1.2s is not enough.
        self.servo.power_key(self.faft_config.hold_pwr_button_poweroff)
        # device can take 44-51 seconds to restart,
        # add buffer from the default timeout of 60 seconds.
        self.switcher.wait_for_client_offline(timeout=100, orig_boot_id=boot_id)
        time.sleep(self.faft_config.shutdown)
        # Short press power button to boot DUT again.
        self.servo.power_key(self.faft_config.hold_pwr_button_poweron)

    def check_lid_and_power_on(self):
        """
        On devices with EC software sync, system powers on after EC reboots if
        lid is open. Otherwise, the EC shuts down CPU after about 3 seconds.
        This method checks lid switch state and presses power button if
        necessary.
        """
        if self.servo.get("lid_open") == "no":
            time.sleep(self.faft_config.software_sync)
            self.servo.power_short_press()

    def _modify_usb_kernel(self, usb_dev, from_magic, to_magic):
        """Modify the kernel header magic in USB stick.

        The kernel header magic is the first 8-byte of kernel partition.
        We modify it to make it fail on kernel verification check.

        @param usb_dev: A string of USB stick path on the host, like '/dev/sdc'.
        @param from_magic: A string of magic which we change it from.
        @param to_magic: A string of magic which we change it to.
        @raise TestError: if failed to change magic.
        """
        assert len(from_magic) == 8
        assert len(to_magic) == 8
        # USB image only contains one kernel.
        kernel_part = self._join_part(usb_dev, self.KERNEL_MAP['a'])
        read_cmd = "sudo dd if=%s bs=8 count=1 2>/dev/null" % kernel_part
        current_magic = self.servo.system_output(read_cmd)
        if current_magic == to_magic:
            logging.info("The kernel magic is already %s.", current_magic)
            return
        if current_magic != from_magic:
            raise error.TestError("Invalid kernel image on USB: wrong magic.")

        logging.info('Modify the kernel magic in USB, from %s to %s.',
                     from_magic, to_magic)
        write_cmd = ("echo -n '%s' | sudo dd of=%s oflag=sync conv=notrunc "
                     " 2>/dev/null" % (to_magic, kernel_part))
        self.servo.system(write_cmd)

        if self.servo.system_output(read_cmd) != to_magic:
            raise error.TestError("Failed to write new magic.")

    def corrupt_usb_kernel(self, usb_dev):
        """Corrupt USB kernel by modifying its magic from CHROMEOS to CORRUPTD.

        @param usb_dev: A string of USB stick path on the host, like '/dev/sdc'.
        """
        self._modify_usb_kernel(usb_dev, self.CHROMEOS_MAGIC,
                                self.CORRUPTED_MAGIC)

    def restore_usb_kernel(self, usb_dev):
        """Restore USB kernel by modifying its magic from CORRUPTD to CHROMEOS.

        @param usb_dev: A string of USB stick path on the host, like '/dev/sdc'.
        """
        self._modify_usb_kernel(usb_dev, self.CORRUPTED_MAGIC,
                                self.CHROMEOS_MAGIC)

    def _call_action(self, action_tuple, check_status=False):
        """Call the action function with/without arguments.

        @param action_tuple: A function, or a tuple (function, args, error_msg),
                             in which, args and error_msg are optional. args is
                             either a value or a tuple if multiple arguments.
                             This can also be a list containing multiple
                             function or tuple. In this case, these actions are
                             called in sequence.
        @param check_status: Check the return value of action function. If not
                             succeed, raises a TestFail exception.
        @return: The result value of the action function.
        @raise TestError: An error when the action function is not callable.
        @raise TestFail: When check_status=True, action function not succeed.
        """
        if isinstance(action_tuple, list):
            return all([self._call_action(action, check_status=check_status)
                        for action in action_tuple])

        action = action_tuple
        args = ()
        error_msg = 'Not succeed'
        if isinstance(action_tuple, tuple):
            action = action_tuple[0]
            if len(action_tuple) >= 2:
                args = action_tuple[1]
                if not isinstance(args, tuple):
                    args = (args,)
            if len(action_tuple) >= 3:
                error_msg = action_tuple[2]

        if action is None:
            return

        if not callable(action):
            raise error.TestError('action is not callable!')

        info_msg = 'calling %s' % str(action)
        if args:
            info_msg += ' with args %s' % str(args)
        logging.info(info_msg)
        ret = action(*args)

        if check_status and not ret:
            raise error.TestFail('%s: %s returning %s' %
                                 (error_msg, info_msg, str(ret)))
        return ret

    def run_shutdown_process(self, shutdown_action, pre_power_action=None,
            run_power_action=True, post_power_action=None, shutdown_timeout=None):
        """Run shutdown_action(), which makes DUT shutdown, and power it on.

        @param shutdown_action: function which makes DUT shutdown, like
                                pressing power key.
        @param pre_power_action: function which is called before next power on.
        @param power_action: power_key press by default, set to None to skip.
        @param post_power_action: function which is called after next power on.
        @param shutdown_timeout: a timeout to confirm DUT shutdown.
        @raise TestFail: if the shutdown_action() failed to turn DUT off.
        """
        self._call_action(shutdown_action)
        logging.info('Wait to ensure DUT shut down...')
        try:
            if shutdown_timeout is None:
                shutdown_timeout = self.faft_config.shutdown_timeout
            self.switcher.wait_for_client(timeout=shutdown_timeout)
            raise error.TestFail(
                    'Should shut the device down after calling %s.' %
                    str(shutdown_action))
        except ConnectionError:
            logging.info(
                'DUT is surely shutdown. We are going to power it on again...')

        if pre_power_action:
            self._call_action(pre_power_action)
        if run_power_action:
            self.servo.power_key(self.faft_config.hold_pwr_button_poweron)
        if post_power_action:
            self._call_action(post_power_action)

    def get_bootid(self, retry=3):
        """
        Return the bootid.
        """
        boot_id = None
        while retry:
            try:
                boot_id = self._client.get_boot_id()
                break
            except error.AutoservRunError:
                retry -= 1
                if retry:
                    logging.info('Retry to get boot_id...')
                else:
                    logging.warning('Failed to get boot_id.')
        logging.info('boot_id: %s', boot_id)
        return boot_id

    def check_state(self, func):
        """
        Wrapper around _call_action with check_status set to True. This is a
        helper function to be used by tests and is currently implemented by
        calling _call_action with check_status=True.

        TODO: This function's arguments need to be made more stringent. And
        its functionality should be moved over to check functions directly in
        the future.

        @param func: A function, or a tuple (function, args, error_msg),
                             in which, args and error_msg are optional. args is
                             either a value or a tuple if multiple arguments.
                             This can also be a list containing multiple
                             function or tuple. In this case, these actions are
                             called in sequence.
        @return: The result value of the action function.
        @raise TestFail: If the function does notsucceed.
        """
        logging.info("-[FAFT]-[ start stepstate_checker ]----------")
        self._call_action(func, check_status=True)
        logging.info("-[FAFT]-[ end state_checker ]----------------")

    def get_current_firmware_sha(self):
        """Get current firmware sha of body and vblock.

        @return: Current firmware sha follows the order (
                 vblock_a_sha, body_a_sha, vblock_b_sha, body_b_sha)
        """
        current_firmware_sha = (self.faft_client.bios.get_sig_sha('a'),
                                self.faft_client.bios.get_body_sha('a'),
                                self.faft_client.bios.get_sig_sha('b'),
                                self.faft_client.bios.get_body_sha('b'))
        if not all(current_firmware_sha):
            raise error.TestError('Failed to get firmware sha.')
        return current_firmware_sha

    def is_firmware_changed(self):
        """Check if the current firmware changed, by comparing its SHA.

        @return: True if it is changed, otherwise Flase.
        """
        # Device may not be rebooted after test.
        self.faft_client.bios.reload()

        current_sha = self.get_current_firmware_sha()

        if current_sha == self._backup_firmware_sha:
            return False
        else:
            corrupt_VBOOTA = (current_sha[0] != self._backup_firmware_sha[0])
            corrupt_FVMAIN = (current_sha[1] != self._backup_firmware_sha[1])
            corrupt_VBOOTB = (current_sha[2] != self._backup_firmware_sha[2])
            corrupt_FVMAINB = (current_sha[3] != self._backup_firmware_sha[3])
            logging.info("Firmware changed:")
            logging.info('VBOOTA is changed: %s', corrupt_VBOOTA)
            logging.info('VBOOTB is changed: %s', corrupt_VBOOTB)
            logging.info('FVMAIN is changed: %s', corrupt_FVMAIN)
            logging.info('FVMAINB is changed: %s', corrupt_FVMAINB)
            return True

    def backup_firmware(self, suffix='.original'):
        """Backup firmware to file, and then send it to host.

        @param suffix: a string appended to backup file name
        """
        remote_temp_dir = self.faft_client.system.create_temp_dir()
        remote_bios_path = os.path.join(remote_temp_dir, 'bios')
        self.faft_client.bios.dump_whole(remote_bios_path)
        self._client.get_file(remote_bios_path,
                              os.path.join(self.resultsdir, 'bios' + suffix))
        self._client.run('rm -rf %s' % remote_temp_dir)
        logging.info('Backup firmware stored in %s with suffix %s',
            self.resultsdir, suffix)

        self._backup_firmware_sha = self.get_current_firmware_sha()

    def is_firmware_saved(self):
        """Check if a firmware saved (called backup_firmware before).

        @return: True if the firmware is backuped; otherwise False.
        """
        return self._backup_firmware_sha != ()

    def clear_saved_firmware(self):
        """Clear the firmware saved by the method backup_firmware."""
        self._backup_firmware_sha = ()

    def restore_firmware(self, suffix='.original'):
        """Restore firmware from host in resultsdir.

        @param suffix: a string appended to backup file name
        """
        if not self.is_firmware_changed():
            return

        # Backup current corrupted firmware.
        self.backup_firmware(suffix='.corrupt')

        # Restore firmware.
        remote_temp_dir = self.faft_client.system.create_temp_dir()
        self._client.send_file(os.path.join(self.resultsdir, 'bios' + suffix),
                               os.path.join(remote_temp_dir, 'bios'))

        self.faft_client.bios.write_whole(
            os.path.join(remote_temp_dir, 'bios'))
        self.switcher.mode_aware_reboot()
        logging.info('Successfully restore firmware.')

    def setup_firmwareupdate_shellball(self, shellball=None):
        """Deside a shellball to use in firmware update test.

        Check if there is a given shellball, and it is a shell script. Then,
        send it to the remote host. Otherwise, use
        /usr/sbin/chromeos-firmwareupdate.

        @param shellball: path of a shellball or default to None.

        @return: Path of shellball in remote host. If use default shellball,
                 reutrn None.
        """
        updater_path = None
        if shellball:
            # Determine the firmware file is a shellball or a raw binary.
            is_shellball = (utils.system_output("file %s" % shellball).find(
                    "shell script") != -1)
            if is_shellball:
                logging.info('Device will update firmware with shellball %s',
                             shellball)
                temp_dir = self.faft_client.system.create_temp_dir(
                            'shellball_')
                temp_shellball = os.path.join(temp_dir, 'updater.sh')
                self._client.send_file(shellball, temp_shellball)
                updater_path = temp_shellball
            else:
                raise error.TestFail(
                    'The given shellball is not a shell script.')
            return updater_path

    def is_kernel_changed(self):
        """Check if the current kernel is changed, by comparing its SHA1 hash.

        @return: True if it is changed; otherwise, False.
        """
        changed = False
        for p in ('A', 'B'):
            backup_sha = self._backup_kernel_sha.get(p, None)
            current_sha = self.faft_client.kernel.get_sha(p)
            if backup_sha != current_sha:
                changed = True
                logging.info('Kernel %s is changed', p)
        return changed

    def backup_kernel(self, suffix='.original'):
        """Backup kernel to files, and the send them to host.

        @param suffix: a string appended to backup file name.
        """
        remote_temp_dir = self.faft_client.system.create_temp_dir()
        for p in ('A', 'B'):
            remote_path = os.path.join(remote_temp_dir, 'kernel_%s' % p)
            self.faft_client.kernel.dump(p, remote_path)
            self._client.get_file(
                    remote_path,
                    os.path.join(self.resultsdir, 'kernel_%s%s' % (p, suffix)))
            self._backup_kernel_sha[p] = self.faft_client.kernel.get_sha(p)
        logging.info('Backup kernel stored in %s with suffix %s',
            self.resultsdir, suffix)

    def is_kernel_saved(self):
        """Check if kernel images are saved (backup_kernel called before).

        @return: True if the kernel is saved; otherwise, False.
        """
        return len(self._backup_kernel_sha) != 0

    def clear_saved_kernel(self):
        """Clear the kernel saved by backup_kernel()."""
        self._backup_kernel_sha = dict()

    def restore_kernel(self, suffix='.original'):
        """Restore kernel from host in resultsdir.

        @param suffix: a string appended to backup file name.
        """
        if not self.is_kernel_changed():
            return

        # Backup current corrupted kernel.
        self.backup_kernel(suffix='.corrupt')

        # Restore kernel.
        remote_temp_dir = self.faft_client.system.create_temp_dir()
        for p in ('A', 'B'):
            remote_path = os.path.join(remote_temp_dir, 'kernel_%s' % p)
            self._client.send_file(
                    os.path.join(self.resultsdir, 'kernel_%s%s' % (p, suffix)),
                    remote_path)
            self.faft_client.kernel.write(p, remote_path)

        self.switcher.mode_aware_reboot()
        logging.info('Successfully restored kernel.')

    def backup_cgpt_attributes(self):
        """Backup CGPT partition table attributes."""
        self._backup_cgpt_attr = self.faft_client.cgpt.get_attributes()

    def restore_cgpt_attributes(self):
        """Restore CGPT partition table attributes."""
        current_table = self.faft_client.cgpt.get_attributes()
        if current_table == self._backup_cgpt_attr:
            return
        logging.info('CGPT table is changed. Original: %r. Current: %r.',
                     self._backup_cgpt_attr,
                     current_table)
        self.faft_client.cgpt.set_attributes(self._backup_cgpt_attr)

        self.switcher.mode_aware_reboot()
        logging.info('Successfully restored CGPT table.')

    def try_fwb(self, count=0):
        """set to try booting FWB count # times

        Wrapper to set fwb_tries for vboot1 and fw_try_count,fw_try_next for
        vboot2

        @param count: an integer specifying value to program into
                      fwb_tries(vb1)/fw_try_next(vb2)
        """
        if self.fw_vboot2:
            self.faft_client.system.set_fw_try_next('B', count)
        else:
            # vboot1: we need to boot into fwb at least once
            if not count:
                count = count + 1
            self.faft_client.system.set_try_fw_b(count)

