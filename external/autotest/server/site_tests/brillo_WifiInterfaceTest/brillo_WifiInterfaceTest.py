# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging

import common
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros.network import iw_runner
from autotest_lib.server import test


class brillo_WifiInterfaceTest(test.test):
    """Verify that a Brillo device has its wifi properly configured."""
    version = 1

    def get_ifconfig_dict(self, ifconfig_output):
        """Convert output of ifconfig into a dictionary.

        @param ifconfig_output: List of ifconfig output lines.

        @return Dictionary mapping interface names (e.g. 'wlan0') to their list
                of stripped output lines.
        """
        curr_iface = None
        ifconfig_dict = {}
        for line in ifconfig_output:
            if curr_iface is None:
                curr_iface, line = line.split(None, 1)
                ifconfig_dict[curr_iface] = []

            line = line.strip()
            if line:
                ifconfig_dict[curr_iface].append(line)
            else:
                curr_iface = None

        return ifconfig_dict


    def run_once(self, host=None, wifi_iface=None, wifi_ssid=None):
        """Check that a given wifi interface is properly configured.

        @param host: a host object representing the DUT.
        @param wifi_iface: Name of the wifi interface to test; None means we'll
                           try to detect at least one that works.
        @param wifi_ssid: Name of the SSID we want the interface to be
                          connected to; None means any.

        @raise TestFail: The test failed.
        """
        err_iface = ('No interface is' if wifi_iface is None
                      else 'Interface %s is not' % wifi_iface)

        # First check link status and SSID.
        iw = iw_runner.IwRunner(remote_host=host)
        active_ifaces = []
        try:
            iw_ifaces = [iface_tuple.if_name
                         for iface_tuple in iw.list_interfaces()]
            if wifi_iface is not None:
                if wifi_iface not in iw_ifaces:
                    raise error.TestFail(
                            'Interface %s not listed by iw' % wifi_iface)
                test_ifaces = [wifi_iface]
            else:
                test_ifaces = iw_ifaces

            for iface in test_ifaces:
                iface_ssid = iw.get_link_value(iface, 'SSID')
                if (iface_ssid is not None and
                    (wifi_ssid is None or iface_ssid == wifi_ssid)):
                    active_ifaces.append(iface)
        except error.AutoservRunError:
            raise error.TestFail('Failed to run iw')

        if not active_ifaces:
            err_ssid = 'any SSID' if wifi_ssid is None else 'SSID ' + wifi_ssid
            raise error.TestFail('%s connected to %s' % (err_iface, err_ssid))

        logging.info('Active wifi interfaces: %s', ', '.join(active_ifaces))

        # Then check IPv4 connectivity.
        try:
            ifconfig_output = host.run_output('ifconfig').splitlines()
        except error.AutoservRunError:
            raise error.TestFail('Failed to run ifconfig')

        ifconfig_dict = self.get_ifconfig_dict(ifconfig_output)
        connected_ifaces = [iface for iface in active_ifaces
                            if any(['inet addr:' in line
                                    for line in ifconfig_dict.get(iface, [])])]
        if not connected_ifaces:
            raise error.TestFail('%s IPv4 connected' % err_iface)

        logging.info('IPv4 connected wifi interfaces: %s',
                     ', '.join(connected_ifaces))
