# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging, time

from autotest_lib.server import autotest, test
from autotest_lib.client.common_lib import error

_SUSPEND_TIME = 60
_SUSPEND_TIMEOUT = 30

class power_USBHotplugInSuspend(test.test):
    version = 1

    def _switch_usbkey_power(self, on):
        """
        Turn on/off the power to the USB key.

        @param on True to turn on, false otherwise.
        """
        if on:
            self._host.servo.set('prtctl4_pwren', 'on')
        else:
            self._host.servo.set('prtctl4_pwren', 'off')
        time.sleep(self._host.servo.USB_POWEROFF_DELAY)

    def _get_usb_devices(self):
        """
        Get the USB device attached to the client.

        Parses output from lsusb and returns the set of device IDs.
        """
        try:
            lines = self._host.run('lsusb').stdout.strip().split('\n')
        except:
            raise error.TestError('Failed to get list of USB devices.')
        devices = set(line.split()[5] for line in lines)
        logging.info('USB Devices: %s' % (",".join(devices)))
        return devices

    def _suspend_client(self):
        """
        Start the client test power_KernelSuspend to suspend the client and
        do not wait for it to finish.
        """
        client_at = autotest.Autotest(self._host)
        # TODO(scottz): Add server side support to sys_power: crosbug.com/38115
        client_at.run_test('power_KernelSuspend', background=True,
                           seconds=_SUSPEND_TIME)

    def _suspend_and_hotplug(self, insert):
        """
        Suspend the client and add/remove the USB key.  This assumes that a
        USB key is plugged into the servo and is facing the DUT.

        @param insert True to test insertion during suspend, False to test
                      removal.
        """
        # Initialize the USB key and get the set of USB devices before
        # suspending.
        self._switch_usbkey_power(not insert)
        before_suspend = self._get_usb_devices()

        # Suspend the client and wait for it to go down before powering on/off
        # the usb key.
        self._suspend_client()
        if not self._host.ping_wait_down(_SUSPEND_TIMEOUT):
            raise error.TestError('Client failed to suspend.')
        self._switch_usbkey_power(insert)

        # Wait for the client to come back up (suspend time + some slack time).
        # TODO(beeps): Combine the two timeouts in wait_up after
        # crbug.com/221785 is resolved.
        time.sleep(_SUSPEND_TIME)
        if not self._host.wait_up(self._host.RESUME_TIMEOUT):
            raise error.TestError('Client failed to resume.')

        # Get the set of devices plugged in and make sure the change was
        # detected.
        after_suspend = self._get_usb_devices()
        diff = after_suspend ^ before_suspend
        if not diff:
            raise error.TestFail('No USB changes detected after resuming.')

        # Finally, make sure hotplug still works after resuming by switching
        # the USB key's power once more.
        self._switch_usbkey_power(not insert)
        after_hotplug = self._get_usb_devices()
        diff = after_hotplug ^ after_suspend
        if not diff:
            raise error.TestFail('No USB changes detected after hotplugging.')

    def cleanup(self):
        """
        Reset the USB key to its initial state.
        """
        self._host.servo.switch_usbkey(self._init_usbkey_direction)
        self._switch_usbkey_power(self._init_usbkey_power == 'on')
        super(power_USBHotplugInSuspend, self).cleanup()

    def run_once(self, host):
        """
        Tests adding and removing a USB device while the client is suspended.
        """
        self._host = host
        self._init_usbkey_power = self._host.servo.get('prtctl4_pwren')
        self._init_usbkey_direction = self._host.servo.get_usbkey_direction()

        # Make sure the USB key is facing the DUT and is actually present.
        self._host.servo.switch_usbkey('dut')
        self._switch_usbkey_power(False)
        before_insert = self._get_usb_devices()
        self._switch_usbkey_power(True)
        after_insert = self._get_usb_devices()
        diff = after_insert - before_insert
        logging.info('Inserted USB device(s): %s' % (",".join(diff)))
        if not diff:
            raise error.TestError('No new USB devices detected. Is a USB key '
                                  'plugged into the servo?')

        logging.info('Testing insertion during suspend.')
        self._suspend_and_hotplug(True)
        logging.info('Testing removal during suspend.')
        self._suspend_and_hotplug(False)
