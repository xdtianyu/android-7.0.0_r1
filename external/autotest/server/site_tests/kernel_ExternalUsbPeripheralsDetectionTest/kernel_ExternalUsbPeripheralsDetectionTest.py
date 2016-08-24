# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging, os, re, time

from autotest_lib.server import test
from autotest_lib.client.common_lib import error

_WAIT_DELAY = 25
_USB_DIR = '/sys/bus/usb/devices'

class kernel_ExternalUsbPeripheralsDetectionTest(test.test):
    """Uses servo to repeatedly connect/remove USB devices during boot."""
    version = 1


    def set_hub_power(self, on=True):
        """Setting USB hub power status

        @param: on To power on the servo-usb hub or not

        """
        reset = 'off'
        if not on:
            reset = 'on'
        self.host.servo.set('dut_hub1_rst1', reset)
        self.pluged_status = on
        time.sleep(_WAIT_DELAY)


    def check_usb_peripherals_details(self):
        """Checks the effect from plugged in USB peripherals.

        @returns True if command line output is matched successfuly; Else False
        """
        failed = list()
        for cmd in self.usb_checks.keys():
            out_match_list = self.usb_checks.get(cmd)
            logging.info('Running %s',  cmd)

            # Run the usb check command
            cmd_out_lines = (self.host.run(cmd, ignore_status=True).
                             stdout.strip().split('\n'))
            for out_match in out_match_list:
                match_result = False
                for cmd_out_line in cmd_out_lines:
                    match_result = (match_result or
                        re.search(out_match, cmd_out_line) != None)
                if not match_result:
                    failed.append((cmd,out_match))
        return failed


    def get_usb_device_dirs(self):
        """Gets the usb device dirs from _USB_DIR path.

        @returns list with number of device dirs else None
        """
        usb_dir_list = []
        cmd = 'ls -1 %s' % _USB_DIR
        tmp = self.host.run(cmd).stdout.strip().split('\n')
        for d in tmp:
            usb_dir_list.append(os.path.join(_USB_DIR, d))
        return usb_dir_list


    def get_vendor_id_dict_from_dut(self, dir_list):
        """Finds the vendor id from provided dir list.

        @param dir_list: full path of directories
        @returns dict of all vendor ids vs file path
        """
        vendor_id_dict = dict()
        for d in dir_list:
            file_name = os.path.join(d, 'idVendor')
            if self._exists_on(file_name):
                vendor_id = self.host.run('cat %s' % file_name).stdout.strip()
                if vendor_id:
                    vendor_id_dict[vendor_id] = d
        logging.info('%s', vendor_id_dict)
        return vendor_id_dict


    def _exists_on(self, path):
        """Checks if file exists on host or not.

        @returns True or False
        """
        return self.host.run('ls %s' % path,
                             ignore_status=True).exit_status == 0



    def run_once(self, host, usb_checks=None,
                 vendor_id_dict_control_file=None):
        """Main function to run the autotest.

        @param host: name of the host
        @param usb_checks: dictionary defined in control file
        @param vendor_id_list: dictionary defined in control file
        """
        self.host = host
        self.usb_checks = usb_checks

        self.host.servo.switch_usbkey('dut')
        self.host.servo.set('usb_mux_sel3', 'dut_sees_usbkey')
        time.sleep(_WAIT_DELAY)

        self.set_hub_power(False)
        # Collect the USB devices directories before switching on hub
        usb_list_dir_off = self.get_usb_device_dirs()

        self.set_hub_power(True)
        # Collect the USB devices directories after switching on hub
        usb_list_dir_on = self.get_usb_device_dirs()

        diff_list = list(set(usb_list_dir_on).difference(set(usb_list_dir_off)))
        if len(diff_list) == 0:
            # Fail if no devices detected after
            raise error.TestError('No connected devices were detected. Make '
                                  'sure the devices are connected to USB_KEY '
                                  'and DUT_HUB1_USB on the servo board.')
        logging.debug('Connected devices list: %s', diff_list)

        # Test 1: check USB peripherals info in detail
        failed = self.check_usb_peripherals_details()
        if len(failed)> 0:
            raise error.TestError('USB device not detected %s', str(failed))

        # Test 2: check USB device dir under /sys/bus/usb/devices
        vendor_ids = {}
        # Gets a dict idVendor: dir_path
        vendor_ids = self.get_vendor_id_dict_from_dut(diff_list)
        for vid in vendor_id_dict_control_file.keys():
            if vid not in vendor_ids.keys():
                raise error.TestFail('%s is not detected at %s dir'
                                     % (vendor_id_dict_control_file[vid],
                                     _USB_DIR))
            else:
            # Test 3: check driver symlink and dir for each USB device
                tmp_list = [device_dir for device_dir in
                            self.host.run('ls -1 %s' % vendor_ids[vid],
                            ignore_status=True).stdout.split('\n')
                            if re.match(r'\d-\d.*:\d\.\d', device_dir)]
                if not tmp_list:
                    raise error.TestFail('No driver created/loaded for %s'
                                         % vendor_id_dict_control_file[vid])
                logging.info('---- Drivers for %s ----',
                             vendor_id_dict_control_file[vid])
                flag = False
                for device_dir in tmp_list:
                    driver_path = os.path.join(vendor_ids[vid],
                                               '%s/driver' % device_dir)
                    if self._exists_on(driver_path):
                        flag = True
                        link = (self.host.run('ls -l %s | grep ^l'
                                              '| grep driver'
                                              % driver_path, ignore_status=True)
                                              .stdout.strip())
                        logging.info('%s', link)
                if not flag:
                    raise error.TestFail('Drivers are not loaded - %s', driver_path)
