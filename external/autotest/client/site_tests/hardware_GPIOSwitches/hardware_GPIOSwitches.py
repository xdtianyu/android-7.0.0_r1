# -*- coding: utf-8 -*-
#
# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

__author__ = 'nsanders@chromium.org (Nick Sanders)'

import logging, os

from autotest_lib.client.bin import test, utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.cros import gpio

class hardware_GPIOSwitches(test.test):
    version = 4

    def initialize(self):
        self._gpio = gpio.Gpio(error.TestError)

    def gpio_read(self, name):
        try:
            return self._gpio.read(name)
        except:
            raise error.TestError(
                    'Unable to read gpio value "%s"\n'
                    '測試程式無法讀取 gpio 數值 "%s"' % (name, name))

    def run_once(self):
        self._gpio.setup()
        keyvals = {}
        keyvals['level_recovery'] = self.gpio_read('recovery_button')
        keyvals['level_developer'] = self.gpio_read('developer_switch')
        keyvals['level_firmware_writeprotect'] = self.gpio_read('write_protect')

        self.write_perf_keyval(keyvals)
