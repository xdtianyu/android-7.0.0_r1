# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from autotest_lib.client.common_lib import error
from autotest_lib.server import autotest, test


class power_BrightnessResetAfterReboot(test.test):
    """Tests for default brightness level after rebooting the device.
    """
    version = 1

    GET_BRIGHTNESS_FLAG = '--get_brightness_percent'
    SET_BRIGHTNESS_FLAG = '--set_brightness_percent'

    def run_once(self, host, client_autotest):
        """This test verify that user should get default brightness
        after rebooting the device with maximum brigntness level.
        """
        if host.has_internal_display() is None:
            raise error.TestNAError('Test can not processed on '
                                    'devices without internal display')
        self.is_freon_build(host)
        autotest_client = autotest.Autotest(host)
        host.reboot()
        autotest_client.run_test(client_autotest,
                                 exit_without_logout=True)

        initial_bright = self.backlight_control(self.GET_BRIGHTNESS_FLAG,
                                                host)

        if initial_bright < 10.0 or initial_bright > 90.0:
            raise error.TestFail('Default brightness level is out '
                                 'of scope(10% - 90%): %f'
                                 %(initial_bright))

        self.backlight_control('%s=0' % (self.SET_BRIGHTNESS_FLAG), host)
        if not self.backlight_control(self.GET_BRIGHTNESS_FLAG, host) == 0:
            raise error.TestFail('Not able to change the brightness '
                                 'to minum(0%) level')
        self.backlight_control('%s=100' % (self.SET_BRIGHTNESS_FLAG), host)
        if not self.backlight_control(self.GET_BRIGHTNESS_FLAG, host) == 100:
            raise error.TestFail('Not able to change the brightness '
                                 'to maximum(100%) level')

        host.reboot()
        autotest_client.run_test(client_autotest,
                                 exit_without_logout=True)
        bright_after_reboot = self.backlight_control(self.GET_BRIGHTNESS_FLAG,
                                                     host)
        if not initial_bright == bright_after_reboot:
            raise error.TestFail('Not able to reset default brightness\n'
                                 'Previous boot default brightness: %f\n'
                                 'Current boot default brightness: %f'
                                  % (initial_bright, bright_after_reboot))


    def backlight_control(self, arg_str, host):
        """Executes backlight_tool with arguments passed through arg_str.
           Two functions are used:
               - sets the brightness percentage to a given level
               - gets the brightness percentage and returns a float value
           @param arg_str: Command argument to set/get the backlight.
           @param host: host object representing the DUT.
        """
        cmd = 'backlight_tool %s' % (arg_str)
        try:
            result = host.run(cmd).stdout.rstrip()
        except error.CmdError:
            raise error.TestFail(cmd)
        if result:
            return float(result)


    def is_freon_build(self, host):
        """ Checks for the freon builds.
            @param host: host object representing the DUT
        """
        if not host.run('modetest', ignore_status=True ).exit_status == 0:
            raise error.TestNAError('Test can not processed on non-freon build')

