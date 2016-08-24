# Copyright (c) 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import collections
import glob
import logging
import re
import time

from autotest_lib.client.bin import test
from autotest_lib.client.common_lib import error, utils
from autotest_lib.client.cros import ec as cros_ec, cros_logging


class usbpd_GFU(test.test):
    """Integration test for USB-PD Google Firmware Update (GFU).

    Test should:
    - interrogate what firmware's are available for each device and for each:
      1. Use ectool's flashpd to write RW with that to mimic old hw
         - Validate that kernel driver successfully updates to latest RW.
      2. Erase RW and see update as well.

    TODO:
      3. Check that update is checked after S2R.
    """

    version = 1

    FW_PATH = '/lib/firmware/cros-pd'
    # <device>_v<major>.<minor>.<build>-<commit SHA>
    FW_NAME_RE = r'%s/(\w+)_v(\d+)\.(\d+)\.(\d+)-([0-9a-f]+).*' % (FW_PATH)
    GOOGLE_VID = '0x18d1'
    MAX_UPDATE_SECS = 80
    FW_UP_DNAME = 'cros_ec_pd_update'
    # TODO(tbroch) This will be change once cros_ec_pd_update is abstracted from
    # ACPI driver.  Will need to fix this once it happens.
    FW_UP_DISABLE_PATH = '/sys/devices/LNXSYSTM:00/device:00/PNP0A08:00/device:1e/PNP0C09:00/GOOG0003:00/disable'

    # TODO(tbroch) find better way to build this or we'll have to edit test for
    # each new PD peripheral.
    DEV_MAJOR = dict(zinger=1, minimuffin=2, dingdong=3, hoho=4)

    def _index_firmware_avail(self):
        """Index the various USB-PD firmwares in the rootfs.

        TODO(crosbug.com/434522) This method will need reworked after we've come
        up with a better method for firmware release.

        @returns: dictionary of firmwares (key == name, value == list of
          firmware paths)
        """
        fw_dict = collections.defaultdict(list)
        for fw in glob.glob('%s/*_v[1-9].*.bin' % (self.FW_PATH)):
            mat = re.match(self.FW_NAME_RE, fw)
            if not mat:
                continue

            name = mat.group(1)
            fw_dict[name].append(fw)

        return fw_dict

    def _is_gfu(self, port):
        """Is it in GFU?

        @param port: EC_USBPD object for port.

        @returns: True if GFU enterd, False otherwise.
        """
        return port.is_amode_supported(self.GOOGLE_VID)

    def _is_in_rw(self, port):
        """Is PD device in RW firmware?

        @param port: EC_USBPD object for port.

        @returns: True if in RW, False otherwise.
        """
        flash_info = port.get_flash_info()
        logging.debug('flash_info = %s', flash_info)
        return flash_info['image_status'] == 'RW'

    def _set_kernel_fw_update(self, disable=0):
        """Disable the FW update driver.

        @param disable: 1 for disable, 0 for enable.
        """
        utils.write_one_line(self.FW_UP_DISABLE_PATH, disable)
        if not disable:
            # Allow kernel driver time quiesce
            time.sleep(2)

    def _modify_rw(self, port, rw=None, tries=3):
        """Modify RW of USB-PD device in <port>.

        @param port: EC_USBPD object for port.
        @param rw: Path to RW FW to write using ectool.  If None then uses
          /dev/null to invalidate the RW.
        @param tries: Number of tries to update RW via flashpd

        @returns: True if success, False otherwise.
        """
        timeout = self.MAX_UPDATE_SECS

        if not rw:
            rw = '/dev/null'
            tries = 1

        self._set_kernel_fw_update(disable=1)

        while (tries):
            try:
                # Note in flashpd <dev_major> <port> <file> the dev_major is
                # unnecessary in all cases so its just been set to 0
                port.ec_command('flashpd 0 %d %s' % (port.index, rw),
                                ignore_status=True, timeout=timeout)

            except error.CmdTimeoutError:
                # TODO(tbroch) could remove try/except if ec_command used run
                # instead of system_output + ignore_timeout=True
                tries -= 1
                continue

            if rw != '/dev/null' and not self._is_in_rw(port):
                logging.warn('Port%d: not in RW after flashpd ... retrying',
                             port.index)
                tries -= 1
            else:
                break

        self._set_kernel_fw_update()

        msg = self._reader.get_last_msg([r'%s.*is in RO' % port.index,
                                         self.FW_UP_DNAME],
                                        retries=5, sleep_seconds=2)
        if not msg:
            logging.warn('Port%d: Driver does NOT see dev in not in RO',
                         port.index)
            return False
        logging.info('Port%d: Driver sees device in RO', port.index)
        return True

    def _test_update(self, port, rw=None, tries=3):
        """Test RW update.

        Method tests the kernel's RW update process by first modifying the
        existing RW (either invalidating or rolling it back) via ectool.  It
        then querys the syslog to validate kernel sees the need for update and
        is successful.

        @param port: EC_USBPD object for port.
        @param rw: path to RW firmware to write via ectool to test upgrade.
        @param tries: integer number of attempts to write RW.  Necessary as
          update is not robust (design decision).
        """
        if not tries:
            raise error.TestError('Retries must be > 0')

        if not self._is_in_rw(port):
            raise error.TestError('Port%d: Device is not in RW' % port.index)

        fw_up_re = r'%s.*Port%d FW update completed' % (self.FW_UP_DNAME,
                                                        port.index)

        while tries:
            self._reader.set_start_by_current()
            rsp = self._modify_rw(port, rw)

            if not rsp:
                rsp_str = 'Port%d: RW modified with RW=%s failed' % \
                          (port.index, rw)
                if tries:
                    logging.warn('%s ... retrying.', rsp_str)
                    tries -= 1
                else:
                    raise error.TestError(rsp_str)

            self._reader.set_start_by_current()
            msg = self._reader.get_last_msg([fw_up_re],
                                            retries=(self.MAX_UPDATE_SECS / 2),
                                            sleep_seconds=2)

            if not msg:
                rsp_str = 'Port%d: driver did NOT update FW' % port.index
                if tries:
                    logging.warn('%s ... retrying.', rsp_str)
                    tries -= 1
                    continue
                else:
                    raise error.TestError(rsp_str)

            logging.info('Port%d: Driver completed RW update', port.index)

            # Allow adequate reboot time after RW write completes and device is
            # rebooted.
            time.sleep(3)

            if not self._is_in_rw(port):
                rsp_str = 'Port%d: Device is not in RW' % port.index
                if tries:
                    logging.warn('%s ... retrying.', rsp_str)
                    tries -= 1
                    continue
                else:
                    raise error.TestError(rsp_str)

            break # success #

    def _test_rw_rollback(self, port, fw_dict):
        """Test rolling back RW firmware.

        @param port: EC_USBPD object for port.
        @param fw_dict: dictionary of firmwares.
        """
        self._set_kernel_fw_update()

        # test old RW update
        flash_info = port.get_flash_info()
        for dev_name in fw_dict.keys():
            if flash_info['dev_major'] == self.DEV_MAJOR[dev_name]:
                for old_rw in sorted(fw_dict[dev_name], reverse=True)[1:]:
                    logging.info('Port%d: Rollback test %s to %s',
                                 port.index, dev_name, old_rw)
                    self._test_update(port, rw=old_rw)
                break

    def _test_ro_only(self, port, ro_reps):
        """Test FW update on device with RO only.

        @param port: EC_USBPD object for port.
        @param ro_reps: Number of times to repeat test.
        """
        # test update in RO ro_reps times
        for i in xrange(ro_reps):
            logging.info('RO Loop%d', i)
            self._test_update(port)

    def run_once(self, ro_reps=1):

        fw_dict = self._index_firmware_avail()

        self._usbpd = cros_ec.EC_USBPD()
        self._reader = cros_logging.LogReader()

        for port in self._usbpd.ports:
            if not port.is_dfp():
                continue

            logging.info('Port%d: is a DFP', port.index)

            if not self._is_gfu(port):
                continue

            logging.info('Port%d: supports GFU', port.index)

            self._test_rw_rollback(port, fw_dict)
            self._test_ro_only(port, ro_reps)

    def cleanup(self):
        self._set_kernel_fw_update()
