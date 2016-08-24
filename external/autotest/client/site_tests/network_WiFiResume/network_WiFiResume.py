# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from autotest_lib.client.bin import test, utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.cros import rtc, sys_power

import logging

class network_WiFiResume(test.test):
    version = 1

    def run_once(self, reachable=None, wifi_timeout=5, dev=None):
        '''Check that WiFi is working after a resume

        If test is successful, the performance key "secs_iface_up_delay" is
        reported as seconds the interface needed to be up

        @param reachable: ip address (string) to try to ping as a network test
        @param dev: device (eg 'wlan0') to use for wifi tests.
                    autodetected if unset
        @param wifi_timeout: number of seconds with in which a WiFi association
                        must be (re)established after a suspend/resume cycle
        '''

        if not dev:
            dev = get_wifi_dev()

        if not dev:
            raise error.TestError('No WiFi device found')

        logging.info('checking wifi interface %s', dev)
        check_wifi_dev(dev)

        if network_is_up(reachable=reachable, dev=dev):
            suspend_to_ram()
            start = rtc.get_seconds()
            deadline = start + wifi_timeout
            have_network = network_is_up(reachable=reachable, dev=dev)

            while (not have_network) and (deadline > rtc.get_seconds()):
                have_network = network_is_up(reachable=reachable, dev=dev)

            if have_network:
                delay = rtc.get_seconds() - start
                logging.info('Network came up at %d seconds', delay)
                self.write_perf_keyval({'secs_iface_up_delay': delay})
                return

            delay = rtc.get_seconds() - start
            raise error.TestFail('Network down after %d seconds' % delay)

        raise error.TestFail('Network down at start of test - cannot continue')


def suspend_to_ram(secs_to_suspend=5):
    logging.info('Scheduling wakeup in %d seconds\n', secs_to_suspend)
    sys_power.do_suspend(secs_to_suspend)
    logging.info('Woke up at %d', rtc.get_seconds())


def get_wifi_dev():
    return utils.system_output('iwconfig 2>/dev/null | (read i x; echo $i)')


def check_wifi_dev(dev):
    cmd = 'iwconfig %s 2>/dev/null | grep "^%s"' % (dev, dev)
    ret = utils.system(cmd, ignore_status=True)
    if dev and ret == 0:
        return
    raise error.TestError('"%s" is not a valid WiFi device' % dev)


def get_pingable_address(dev=None):
    if not dev:
        dev = get_wifi_dev()
    cmd = 'ip route show dev %s to match 0/0|if read X X G X; then echo $G; fi'
    return utils.system_output(cmd % dev) or None


def network_is_up(reachable=None, dev=None):
    if not reachable:
        reachable = get_pingable_address(dev=dev)
    if not reachable:
        return False
    if utils.ping(reachable, tries=1) == 0:
        return True
    return False
