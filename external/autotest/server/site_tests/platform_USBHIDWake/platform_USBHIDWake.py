# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging, os

from autotest_lib.client.common_lib import error, autotemp, site_utils
from autotest_lib.server import autotest, hosts
from autotest_lib.server import test, utils

bored_now = """
[  _____             ____  _               _  ]
[ |_   _|__   ___   / ___|| | _____      _| | ]
[  | |/ _ \ / _ \  \___ \| |/ _ \ \ /\ / /| | ]
[  | | (_) | (_) |  ___) | | (_) \ V  V / |_| ]
[  |_|\___/ \___/  |____/|_|\___/ \_/\_/  (_) ]
[                                             ]
[ The device didn't wake up - either the HID device isn't working or  ]
[ the chromebook just didn't wake up: Either way, wake the chromebook ]
[ so we can finish the test, or we'll be sitting here for a while...  ]
"""

press_button_banner = """
[     _   _   _             _   _              ]
[    / \ | |_| |_ ___ _ __ | |_(_) ___  _ __   ]
[   / _ \| __| __/ _ \ '_ \| __| |/ _ \| '_ \  ]
[  / ___ \ |_| ||  __/ | | | |_| | (_) | | | | ]
[ /_/   \_\__|\__\___|_| |_|\__|_|\___/|_| |_| ]
[                                              ]
[ Press the power, sleep or other suitable button on your USB HID Device ]
[ NOTE: NOT on the Chromebook itself - on the USB Keyboard/Remote/etc    ]
[ Then press Return or Enter here so we can proceed with the test        ]
"""


class platform_USBHIDWake(test.test):
    version = 1

    def suspend(self):
        self._client.run("(echo mem > /sys/power/state &)")


    def check_dependencies(self):
        if not utils.system('which openvt', ignore_status=True) == 0:
            raise error.TestError('openvt missing (see control file)')
        if not utils.system('sudo true', ignore_status=True) == 0:
            raise error.TestError('Insufficient privileges: cannot sudo')


    def prompt(self, banner=">>>>>>>>>>> Achtung! <<<<<<<<<<<"):
        """prompt the user with the supplied banner,
        then wait for them to press enter

        @param banner: A [possibly multi-line] banner prompt to display
        """
        temp = autotemp.tempfile(unique_id='vtprompt', text=True)
        os.write(temp.fd, banner)
        pcmd = ("sudo openvt -s -w -- " +
                "sh -c 'clear && cat %s && read -p \"READY> \" REPLY &&" +
                " echo $REPLY'") % temp.name
        utils.system(pcmd)
        temp.clean()


    def wait_for_host(self, host=None, timeout=30):
        '''Wait for the DUT to come back up, with a timeout

        @param host: ip address or hostname of DUT
        @param timeout: maximum time in seconds to wait

        Returns True if the host comes up in time, False otherwise'''
        return site_utils.ping(host, deadline=timeout) == 0


    def have_hid_device(self):
        """Return True is a USB HID device is present, False otherwise"""
        cmd = 'grep "^03$" /sys/bus/usb/devices/[0-9]*/[0-9]*/bInterfaceClass'
        rval = self._client.run(cmd, ignore_status=True)
        return rval.exit_status == 0


    def run_once(self, client_ip):
        """Check to see if a DUT at the given address wakes from suspend
        on USB HID events

        @param client_ip: ip address (string) at which the DUT may be found"""
        self.check_dependencies()
        if not client_ip:
            raise error.TestError('Must have test client IP address')
        self._client = hosts.create_host(client_ip)
        if not self.have_hid_device():
            raise error.TestError('No HID devices found, please attach one')
        self.suspend()
        self.prompt(banner=press_button_banner)
        if not self.wait_for_host(host=client_ip, timeout=10):
            self.prompt(banner=bored_now)
            raise error.TestFail('DUT did not wake up on HID event')
