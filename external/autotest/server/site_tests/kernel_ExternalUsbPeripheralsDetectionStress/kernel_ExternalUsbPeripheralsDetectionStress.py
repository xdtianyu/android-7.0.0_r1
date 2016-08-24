# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging, os, re, time, random

from autotest_lib.client.bin import utils
from autotest_lib.server import test
from autotest_lib.client.common_lib import error
from autotest_lib.server.cros.usb_mux_controller import USBMuxController

_WAIT_DELAY = 5
_USB_DIR = '/sys/bus/usb/devices'
MAX_PORTS = 8
TMP_FAILED_TEST_LIST = list()
PORT_BEING_TESTED = list()

class kernel_ExternalUsbPeripheralsDetectionStress(test.test):
    """Uses USB multiplexer to repeatedly connect and disconnect USB devices."""
    version = 1


    def set_hub_power(self, on=True):
        """Setting USB hub power status

        @param on: To power on the servo-usb hub or not.

        """
        reset = 'off'
        if not on:
            reset = 'on'
        self.host.servo.set('dut_hub1_rst1', reset)
        time.sleep(_WAIT_DELAY)


    def check_manufacturer_and_product_info(
            self, vId, pId, manufacturer, pName):
        """Check manufacturer and product info from lsusb against dict values.

        @param vId: Vendor id of the connected USB device.
        @param pId: Product id of the connected USB device.
        @param manufacturer: Manufacturer name of the connected USB device.
        @param pName: Product name of the connected USB device
        @param result: To track test result.
        @return result value

        """
        result = True
        manu_cmd = ('lsusb -v -d ' + vId + ':' +  pId + ' | grep iManufacturer')
        prod_cmd = ('lsusb -v -d ' + vId + ':' +  pId + ' | grep iProduct')

        manu_cmd_output = (self.host.run(manu_cmd, ignore_status=True).
                           stdout.strip())
        prod_cmd_output = (self.host.run(prod_cmd, ignore_status=True).
                           stdout.strip())

        manu_verify = 'iManufacturer.*' + manufacturer
        prod_verify = 'iProduct.*' + pName

        match_result_manu = re.search(manu_verify, manu_cmd_output) != None
        match_result_prod = re.search(prod_verify, prod_cmd_output) != None

        if not match_result_manu or not match_result_prod:
            logging.debug('Manufacturer or productName do not match.')
            result = False

        return result


    def check_driver_symlink_and_dir(self, devicePath, productName):
        """Check driver symlink and dir against devicePath value from dict.

        @param devicePath: Device driver path.
        @param productName: Product name of the connected USB device.
        @param result: To track test result.
        @return result value

        """
        result = True
        tmp_list = [device_dir for device_dir in
                    self.host.run('ls -1 %s' % devicePath,
                    ignore_status=True).stdout.split('\n')
                    if re.match(r'\d-\d.*:\d\.\d', device_dir)]

        if not tmp_list:
            logging.debug('No driver created/loaded for %s', productName)
            result = False

        flag = False
        for device_dir in tmp_list:
            driver_path = os.path.join(devicePath,
                                       '%s/driver' % device_dir)
            if self._exists_on(driver_path):
                flag = True
                link = (self.host.run('ls -l %s | grep ^l'
                                      '| grep driver'
                                      % driver_path, ignore_status=True)
                                      .stdout.strip())
                logging.info('%s', link)

        if not flag:
            logging.debug('Device driver not found')
            result = False

        return result


    def check_usb_peripherals_details(self, connected_device_dict):
        """Checks USB peripheral details against the values in dictionary.

        @param connected_device_dict: Dictionary of device attributes.

        """
        result = True
        usbPort = connected_device_dict['usb_port']
        PORT_BEING_TESTED.append(usbPort)
        self.usb_mux.enable_port(usbPort)

        vendorId = connected_device_dict['deviceInfo']['vendorId']
        productId = connected_device_dict['deviceInfo']['productId']
        manufacturer = connected_device_dict['deviceInfo']['manufacturer']
        productName = connected_device_dict['deviceInfo']['productName']
        lsusbOutput = connected_device_dict['deviceInfo']['lsusb']
        devicePath = connected_device_dict['deviceInfo']['devicePath']

        try:
            utils.poll_for_condition(
                    lambda: self.host.path_exists(devicePath),
                    exception=utils.TimeoutError('Trouble finding USB device '
                    'on port %d' % usbPort), timeout=15, sleep_interval=1)
        except utils.TimeoutError:
            logging.debug('Trouble finding USB device on port %d', usbPort)
            result = False
            pass

        if not ((vendorId + ':' + productId in lsusbOutput) and
                self.check_manufacturer_and_product_info(
                vendorId, productId, manufacturer, productName) and
                self.check_driver_symlink_and_dir(devicePath, productName)):
            result = False

        self.usb_mux.disable_all_ports()
        try:
            utils.poll_for_condition(
                    lambda: not self.host.path_exists(devicePath),
                    exception=utils.TimeoutError('Device driver path does not '
                    'disappear after device is disconnected.'), timeout=15,
                    sleep_interval=1)
        except utils.TimeoutError:
            logging.debug('Device driver path is still present after device is '
                         'disconnected from the DUT.')
            result = False
            pass

        if result is False:
            logging.debug('Test failed on port %s.', usbPort)
            TMP_FAILED_TEST_LIST.append(usbPort)


    def get_usb_device_dirs(self):
        """Gets the usb device dirs from _USB_DIR path.

        @returns list with number of device dirs else None

        """
        usb_dir_list = list()
        cmd = 'ls -1 %s' % _USB_DIR
        cmd_output = self.host.run(cmd).stdout.strip().split('\n')
        for d in cmd_output:
            usb_dir_list.append(os.path.join(_USB_DIR, d))
        return usb_dir_list


    def parse_device_dir_for_info(self, dir_list, usb_device_dict):
        """Uses vendorId/device path and to get other device attributes.

        @param dir_list: Complete path of directories.
        @param usb_device_dict: Dictionary to store device attributes.
        @returns usb_device_dict with device attributes

        """
        for d_path in dir_list:
            file_name = os.path.join(d_path, 'idVendor')
            if self._exists_on(file_name):
                vendor_id = self.host.run('cat %s' % file_name).stdout.strip()
                if vendor_id:
                    usb_device_dict['deviceInfo']['vendorId'] = vendor_id
                    usb_device_dict['deviceInfo']['devicePath'] = d_path
                    usb_device_dict['deviceInfo']['productId'] = (
                            self.get_product_info(d_path, 'idProduct'))
                    usb_device_dict['deviceInfo']['productName'] = (
                            self.get_product_info(d_path, 'product'))
                    usb_device_dict['deviceInfo']['manufacturer'] = (
                            self.get_product_info(d_path, 'manufacturer'))
        return usb_device_dict


    def get_product_info(self, directory, prod_string):
        """Gets the product id, name and manufacturer info from device path.

        @param directory: Driver path for the USB device.
        @param prod_string: Device attribute string.
        returns the output of the cat command

        """
        product_file_name = os.path.join(directory, prod_string)
        if self._exists_on(product_file_name):
            return self.host.run('cat %s' % product_file_name).stdout.strip()
        return None


    def _exists_on(self, path):
        """Checks if file exists on host or not.

        @returns True or False
        """
        return self.host.run('ls %s' % path,
                             ignore_status=True).exit_status == 0


    def check_lsusb_diff(self, original_lsusb_output):
        """Compare LSUSB output for before and after connecting each device.

        @param original_lsusb_output: lsusb output prior to connecting device.
        @returns the difference between new and old lsusb outputs

        """
        lsusb_output = (self.host.run('lsusb', ignore_status=True).
                        stdout.strip().split('\n'))
        lsusb_diff = (list(set(lsusb_output).
                      difference(set(original_lsusb_output))))
        return lsusb_diff


    def run_once(self, host, loop_count):
        """Main function to run the autotest.

        @param host: Host object representing the DUT.
        @param loop_count: Number of iteration cycles.
        @raise error.TestFail if one or more USB devices are not detected

        """
        self.host = host
        self.usb_mux = USBMuxController(self.host)

        # Make sure all USB ports are disabled prior to starting the test.
        self.usb_mux.disable_all_ports()

        self.host.servo.switch_usbkey('dut')
        self.host.servo.set('usb_mux_sel3', 'dut_sees_usbkey')

        self.set_hub_power(False)
        # Collect the USB devices directories before switching on hub
        usb_list_dir_off = self.get_usb_device_dirs()

        self.set_hub_power(True)
        # Collect the USB devices directories after switching on hub
        usb_list_dir_on = self.get_usb_device_dirs()

        lsusb_original_out = (self.host.run('lsusb', ignore_status=True).
                                 stdout.strip().split('\n'))
        list_of_usb_device_dictionaries = list()
        usb_port = 0

        # Extract connected USB device information and store it in a dict.
        while usb_port < MAX_PORTS:
            usb_device_dict = {'usb_port':None,'deviceInfo':
                    {'devicePath':None,'vendorId':None,'productId':None,
                    'productName':None,'manufacturer':None,'lsusb':None}}
            usb_device_dir_list = list()
            self.usb_mux.enable_port(usb_port)
            try:
                utils.poll_for_condition(
                        lambda: self.check_lsusb_diff(lsusb_original_out),
                        exception=utils.TimeoutError('No USB device on port '
                        '%d' % usb_port), timeout=_WAIT_DELAY, sleep_interval=1)
            except utils.TimeoutError:
                logging.debug('No USB device found on port %d', usb_port)
                pass

            # Maintain list of associated dirs for each connected USB device
            for device in self.get_usb_device_dirs():
                if device not in usb_list_dir_on:
                    usb_device_dir_list.append(device)

            usb_device_dict = self.parse_device_dir_for_info(
                    usb_device_dir_list, usb_device_dict)

            lsusb_diff = self.check_lsusb_diff(lsusb_original_out)
            if lsusb_diff:
                usb_device_dict['usb_port'] = usb_port
                usb_device_dict['deviceInfo']['lsusb'] = lsusb_diff[0]
                list_of_usb_device_dictionaries.append(usb_device_dict)

            self.usb_mux.disable_all_ports()
            try:
                utils.poll_for_condition(
                        lambda: not self.check_lsusb_diff(lsusb_original_out),
                        exception=utils.TimeoutError('Timed out waiting for '
                        'USB device to disappear.'), timeout=_WAIT_DELAY,
                        sleep_interval=1)
            except utils.TimeoutError:
                logging.debug('Timed out waiting for USB device to disappear.')
                pass
            logging.info('%s', usb_device_dict)
            usb_port += 1

        if len(list_of_usb_device_dictionaries) == 0:
            # Fails if no devices detected
            raise error.TestError('No connected devices were detected. Make '
                                  'sure the devices are connected to USB_KEY '
                                  'and DUT_HUB1_USB on the servo board.')
        logging.info('Connected devices list: %s',
                      list_of_usb_device_dictionaries)

        # loop_count defines the number of times the USB peripheral details
        # should be checked and random.choice is used to randomly select one of
        # the elements from the usb device list specifying the device whose
        # details should be checked.
        for i in xrange(loop_count):
            self.check_usb_peripherals_details(
                    random.choice(list_of_usb_device_dictionaries))

        logging.info('Sequence of ports tested with random picker: %s',
                      ', '.join(map(str, PORT_BEING_TESTED)))

        if TMP_FAILED_TEST_LIST:
            logging.info('Failed to verify devices on following ports: %s',
                         ', '.join(map(str, TMP_FAILED_TEST_LIST)))
            raise error.TestFail('Failed to do full device verification on '
                                 'some ports.')
