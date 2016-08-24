# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging, re, time

from autotest_lib.server import test
from autotest_lib.server.cros import stress
from autotest_lib.server.cros.servo import servo
from autotest_lib.client.common_lib import error

_WAIT_DELAY = 5

class platform_ExternalUSBBootStress(test.test):
    """Uses servo to repeatedly connect/remove USB devices during boot."""
    version = 1

    def run_once(self, host, reboots):
        reboots = int(reboots)
        self.client = host
        # The servo hubs come up as diffs in connected components.  These
        # should be ignored for this test.  It is a list so when servo next
        # is available it may have a differnet hub which can be appended.
        servo_hardware_list = ['Standard Microsystems Corp.']


        def strip_lsusb_output(lsusb_output):
            items = lsusb_output.split('\n')
            named_list = []
            unnamed_device_count = 0
            for item in items:
                columns = item.split(' ')
                if len(columns) == 6 or len(' '.join(columns[6:]).strip()) == 0:
                    logging.info('Unnamed device located, adding generic name.')
                    name = 'Unnamed device %d' % unnamed_device_count
                    unnamed_device_count += 1
                else:
                    name = ' '.join(columns[6:]).strip()
                if name not in servo_hardware_list:
                    named_list.append(name)
            return named_list


        def set_hub_power(on=True, check_host_detection=False):
            reset = 'off'
            if not on:
                reset = 'on'
            host.servo.set('dut_hub1_rst1', reset)
            if check_host_detection:
                time.sleep(_WAIT_DELAY)
                return strip_lsusb_output(host.run('lsusb').stdout.strip())


        def stress_hotplug():
            # Devices need some time to come up and to be recognized.  However
            # this is a stress test so we want to move reasonably fast.
            time.sleep(2)
            removed = set_hub_power(False)
            time.sleep(1)
            connected = set_hub_power()


        host.servo.switch_usbkey('dut')
        host.servo.set('usb_mux_sel3', 'dut_sees_usbkey')

        # There are some mice that need the data and power connection to both
        # be removed, otherwise they won't come back up.  This means that the
        # external devices should only use the usb connections labeled:
        # USB_KEY and DUT_HUB1_USB.
        connected = set_hub_power(check_host_detection=True)
        off_list = set_hub_power(on=False, check_host_detection=True)
        diff_list = set(connected).difference(set(off_list))
        if len(diff_list) == 0:
            raise error.TestError('No connected devices were detected.  Make '
                                  'sure the devices are connected to USB_KEY '
                                  'and DUT_HUB1_USB on the servo board.')
        logging.info('Connected devices list: %s' % diff_list)
        set_hub_power(True)

        lsb_release = host.run('cat /etc/lsb-release').stdout.split('\n')
        unsupported_gbb_boards = ['x86-mario', 'x86-alex', 'x86-zgb']
        skip_gbb = False
        for line in lsb_release:
            m = re.match(r'^CHROMEOS_RELEASE_BOARD=(.+)$', line)
            if m and m.group(1) in unsupported_gbb_boards:
                skip_gbb = True
                break

        logging.info('Rebooting the device %d time(s)' % reboots)
        for i in xrange(reboots):
            # We want fast boot past the dev screen
            if not skip_gbb:
                host.run('/usr/share/vboot/bin/set_gbb_flags.sh 0x01')
            stressor = stress.ControlledStressor(stress_hotplug)
            logging.info('Reboot iteration %d of %d' % (i + 1, reboots))
            if skip_gbb:
                # For devices that do not support gbb we have servo
                # accelerate booting through dev mode.
                host.servo.get_power_state_controller().reset()
                host.servo.power_short_press()
                time.sleep(servo.Servo.BOOT_DELAY)
                host.servo.ctrl_d()
                stressor.start()
                host.wait_up(timeout=120)
            else:
                stressor.start()
                self.client.reboot()
            logging.info('Reboot complete, shutting down stressor.')
            stressor.stop()
            connected_now = set_hub_power(check_host_detection=True)
            diff_now = set(connected_now).difference(set(off_list))
            if diff_list != diff_now:
                raise error.TestFail('The list of connected items does not '
                                      'match the master list.\nMaster: %s\n'
                                      'Current: %s' %
                                      (diff_list, diff_now))
            logging.info('Connected devices for iteration %d: %s' %
                         (i, diff_now))
