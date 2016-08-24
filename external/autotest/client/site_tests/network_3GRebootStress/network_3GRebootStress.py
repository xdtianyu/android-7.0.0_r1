# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging, os, time

from autotest_lib.client.bin import test, utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.cros.cellular import mm
from autotest_lib.client.cros.networking import shill_proxy


class ResetAuthorizedContext(object):
    def __init__(self, test):
        self.test = test

    def __enter__(self):
        pass

    def __exit__(self, exception, value, traceback):
        if exception:
            self.test.SetAuthorized(1)
        return False


class network_3GRebootStress(test.test):
    version = 1

    def IsCromo(self, modem_manager):
        path = modem_manager.path
        return path.startswith('/org/chromium')

    def CountModems(self):
        count = len(mm.EnumerateDevices(''))
        logging.debug('Modem count is %d' % count)
        return count

    def EnsureModemAbsent(self):
        utils.poll_for_condition(
            lambda: self.CountModems() == 0,
            error.TestFail('Modem failed to disappear'),
            timeout=shill_proxy.ShillProxy.DEVICE_ENABLE_DISABLE_TIMEOUT)

    def EnsureModemPresent(self):
        utils.poll_for_condition(
            lambda: self.CountModems() == 1,
            error.TestFail('Modem failed to reappear'),
            timeout=shill_proxy.ShillProxy.DEVICE_ENABLE_DISABLE_TIMEOUT)

    def FindUsbDevicePath(self, modem_manager, modem_path):
        logging.info('Modem path: %s' % modem_path)

        modem_obj = modem_manager.GetModem(modem_path)
        props = modem_obj.GetModemProperties()
        net_device = props['Device']
        logging.info('Network device: %s' % net_device)
        if self.IsCromo(modem_manager):
            usb_interface_path, _ = os.path.realpath(
                    '/sys/class/net/%s/device' % net_device).rsplit('/', 1)
        else:
            usb_interface_path = os.path.realpath(net_device)
        self.usb_device_path = usb_interface_path
        logging.info('USB device path: %s' % self.usb_device_path)

    def SetAuthorized(self, authorized):
        logging.debug('Setting authorized to %d' % authorized)
        authorized_path = '%s/authorized' % self.usb_device_path
        authorized_file = open(authorized_path, 'w')
        authorized_file.write('%d' % authorized)
        authorized_file.close()

    def ShouldContinue(self):
        should_continue = True
        message = 'Starting loop %d' % (self.loops_done + 1)

        if self.max_loops != None:
            if self.loops_done >= self.max_loops:
                return False
            loops_left = self.max_loops - self.loops_done
            message += '; %d loops left' % loops_left

        if self.max_seconds != None:
            seconds_done = time.time() - self.start_time
            if seconds_done >= self.max_seconds:
                return False
            seconds_left = self.max_seconds - seconds_done
            message += '; %d seconds left' % seconds_left

        message += '.'
        logging.info(message)

        return True


    # It takes a Gobi about 1.5 seconds to cycle away and back, on average.
    # Assume 2, to have a margin of error.  (If we run out of time, it's
    # fine, but we'd like to run the same number of loops every time.)
    def run_once(self, test_env, max_loops=300, max_seconds=600):
        with test_env:
            self.FindUsbDevicePath(test_env.modem_manager, test_env.modem_path)

            self.max_loops = max_loops
            self.max_seconds = max_seconds

            self.loops_done = 0
            self.start_time = time.time()

            # Use a context to ensure that we don't leave the modem
            # unauthorized if we die with an exception.
            with ResetAuthorizedContext(self):
                while self.ShouldContinue():
                    self.SetAuthorized(0)
                    self.EnsureModemAbsent()
                    self.SetAuthorized(1)
                    self.EnsureModemPresent()
                    self.loops_done += 1

            if self.loops_done < self.max_loops:
                logging.warning('Only got %d loops done in %d seconds. :(',
                                self.loops_done, max_seconds)
