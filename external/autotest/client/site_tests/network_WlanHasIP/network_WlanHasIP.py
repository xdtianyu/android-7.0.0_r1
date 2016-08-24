#!/usr/bin/python
#
# Copyright (c) 2010 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

__author__ = 'kdlucas@chromium.org (Kelly Lucas)'

import fcntl, socket, struct

from autotest_lib.client.bin import test
from autotest_lib.client.common_lib import error


class network_WlanHasIP(test.test):
    """
    Ensure wlan0 has a valid IP address.
    """
    version = 1

    def get_ip(self, device):
        """
        Get the ip address of device. If no IP address is found it will return
        None, since socket.inet_ntoa will fail with IOError.

        Args:
            device: string, should be a valid network device name.
        Returns:
            string, represents the IP address.
        """

        s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        try:
            ipaddress = socket.inet_ntoa(fcntl.ioctl(
                                         s.fileno(),
                                         0x8915, # SIOCGIFADDR
                                         struct.pack('256s', device[:15])
                                         )[20:24])
        except IOError:
            ipaddress = None

        return ipaddress


    def run_once(self):
        WDEV = 'wlan0'
        wlanip = self.get_ip(WDEV)

        if not wlanip:
            raise error.TestFail('%s does not have an assigned IP!' % WDEV)
