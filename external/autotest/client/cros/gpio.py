#!/usr/bin/env python
# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

'''Chrome OS device GPIO library

This module provides a convenient way to detect, setup, and access to GPIO
values on a Chrome OS compatible device.

See help(Gpio) for more information.
'''

import os, shutil, sys, tempfile


class Gpio(object):
    '''
    Utility to access GPIO values.

    Usage:
        gpio = Gpio()
        try:
            gpio.setup()
            print gpio.read(gpio.DEVELOPER_SWITCH_CURRENT)
        except:
            print "gpio failed"
    '''

    # GPIO property names (by "crossystem"):
    DEVELOPER_SWITCH_CURRENT = 'devsw_cur'
    RECOVERY_BUTTON_CURRENT = 'recoverysw_cur'
    WRITE_PROTECT_CURRENT = 'wpsw_cur'

    DEVELOPER_SWITCH_BOOT = 'devsw_boot'
    RECOVERY_BUTTON_BOOT = 'recoverysw_boot'
    WRITE_PROTECT_BOOT = 'wpsw_boot'

    def __init__(self, exception_type=IOError):
        self._exception_type = exception_type

        # list of property conversions, usually str2int.
        self._override_map = {
                self.DEVELOPER_SWITCH_CURRENT: int,
                self.DEVELOPER_SWITCH_BOOT: int,
                self.RECOVERY_BUTTON_CURRENT: int,
                self.RECOVERY_BUTTON_BOOT: int,
                self.WRITE_PROTECT_CURRENT: int,
                self.WRITE_PROTECT_BOOT: int,
        }

        # list of legacy (chromeos_acpi) property names.
        self._legacy_map = {
                'developer_switch': self.DEVELOPER_SWITCH_CURRENT,
                'recovery_button': self.RECOVERY_BUTTON_CURRENT,
                'write_protect': self.WRITE_PROTECT_CURRENT,
        }

    def setup(self):
        '''Configures system for processing GPIO.

        Returns:
            Raises an exception if gpio_setup execution failed.
        '''
        # This is the place to do any configuration / system detection.
        # Currently "crossystem" handles everything so we don't need to do
        # anything now.
        pass

    def read(self, name):
        '''Reads a GPIO property value.
           Check "crossystem" command for the list of available property names.

        Parameters:
            name: the name of property to read.

        Returns: current value, or raise exceptions.
        '''
        debug_title = "Gpio.read('%s'): " % name

        # convert legacy names
        if name in self._legacy_map:
            name = self._legacy_map[name]

        temp_fd, temp_file = tempfile.mkstemp()
        os.close(temp_fd)
        command = "crossystem %s 2>%s" % (name, temp_file)
        pipe = os.popen(command, 'r')
        value = pipe.read()
        exit_status = pipe.close()
        if exit_status:
            with open(temp_file, 'r') as temp_handle:
                debug_info = temp_handle.read()
            value = value.strip()
            debug_info = debug_info.strip()
            if value:
                debug_info = value + '\n' + debug_info
            if debug_info:
                debug_info = '\nInformation: ' + debug_info
            raise self._exception_type(
                    debug_title + "Command failed (%d): %s%s" %
                    (exit_status, command, debug_info))
        # convert values
        if name in self._override_map:
            try:
                value = self._override_map[name](value)
            except:
                raise self._exception_type(debug_title +
                                           'Conversion failed: %s' % value)
        return value


def main():
    gpio = Gpio()
    try:
        gpio.setup()
        print ("developer switch current status: %s" %
               gpio.read(gpio.DEVELOPER_SWITCH_CURRENT))
    except Exception, e:
        print "GPIO failed. %s" % e
        sys.exit(1)

if __name__ == '__main__':
    main()
