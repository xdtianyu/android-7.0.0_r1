# Copyright (c) 2011-2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import collections, logging, os

from autotest_lib.client.bin import test, utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.cros import rtc, sys_power

# TODO(tbroch) WOL:
# - Should we test any of the other modes?  I chose magic as it meant that only
#   the target device should be awaken.

class network_EthCaps(test.test):
    """Base class of EthCaps test.

    Verify Capabilities advertised by an ethernet device work.
    We can't verify much in reality though. But we can verify
    WOL for built-in devices which is expected to work.

    @param test.test: test instance
    """
    version = 1

    # If WOL setting changed during test then restore to original during cleanup
    _restore_wol = False


    def _is_usb(self):
        """Determine if device is USB (or not)

        Add-on USB devices won't report the same 'Supports Wake-on' value
        as built-in (ie PCI) ethernet devices.
        """
        if not self._bus_info:
            cmd = "ethtool -i %s | awk '/bus-info/ {print $2}'" % self._ethname
            self._bus_info = utils.system_output(cmd)
            logging.debug("bus_info is %s", self._bus_info)
            if not self._bus_info:
                logging.error("ethtool -i %s has no bus-info", self._ethname)

        # Two bus_info formats are reported by different device drivers:
        # 1) "usb-0000:00:1d.0-1.2"
        #    "0000:00:1d.0" is the "platform" info of the USB host controller
        #    But it's obvious it's USB since that's the prefix. :)
        if self._bus_info.startswith('usb-'):
            return True

        # 2) "2-1.2" where "2-" is USB host controller instance
        return os.path.exists("/sys/bus/usb/devices/%s" % self._bus_info)

    def _parse_ethtool_caps(self):
        """Retrieve ethernet capabilities.

        Executes ethtool command and parses various capabilities into a
        dictionary.
        """
        caps = collections.defaultdict(list)

        cmd = "ethtool %s" % self._ethname
        prev_keyname = None
        for ln in utils.system_output(cmd).splitlines():
            cap_str = ln.strip()
            try:
                (keyname, value) = cap_str.split(': ')
                caps[keyname].extend(value.split())
                prev_keyname = keyname
            except ValueError:
                # keyname from previous line, add there
                if prev_keyname:
                    caps[prev_keyname].extend(cap_str.split())

        for keyname in caps:
            logging.debug("cap['%s'] = %s", keyname, caps[keyname])

        self._caps = caps


    def _check_eth_caps(self):
        """Check necessary LAN capabilities are present.

        Hardware and driver should support the following functionality:
          1000baseT, 100baseT, 10baseT, half-duplex, full-duplex, auto-neg, WOL

        Raises:
          error.TestError if above LAN capabilities are NOT supported.
        """
        default_eth_caps = {
            'Supported link modes': ['10baseT/Half', '100baseT/Half',
                                      '1000baseT/Half', '10baseT/Full',
                                      '100baseT/Full', '1000baseT/Full'],
            'Supports auto-negotiation': ['Yes'],
            # TODO(tbroch): Other WOL caps: 'a': arp and 's': magicsecure are
            # they important?  Are any of these undesirable/security holes?
            'Supports Wake-on': ['pumbg']
            }
        errors = 0

        for keyname in default_eth_caps:
            if keyname not in self._caps:
                logging.error("\'%s\' not a capability of %s", keyname,
                              self._ethname)
                errors += 1
                continue

            for value in default_eth_caps[keyname]:
                if value not in self._caps[keyname]:
                    # WOL not required for USB Ethernet plug-in devices
                    # But all USB Ethernet devices to date report "pg".
                    # Enforce that.
                    # RTL8153 can report 'pumbag'.
                    # AX88178 can report 'pumbg'.
                    if self._is_usb() and keyname == 'Supports Wake-on':
                        if (self._caps[keyname][0].find('p') >= 0) and \
                            (self._caps[keyname][0].find('g') >= 0):
                            continue

                    logging.error("\'%s\' not a supported mode in \'%s\' of %s",
                                  value, keyname, self._ethname)
                    errors += 1

        if errors:
            raise error.TestError("Eth capability checks.  See errors")


    def _test_wol_magic_packet(self):
        """Check the Wake-on-LAN (WOL) magic packet capabilities of a device.

        Raises:
          error.TestError if WOL functionality fails
        """
        # Magic number WOL supported
        capname = 'Supports Wake-on'
        if self._caps[capname][0].find('g') != -1:
            logging.info("%s support magic number WOL", self._ethname)
        else:
            raise error.TestError('%s should support magic number WOL' %
                            self._ethname)

        # Check that WOL works
        if self._caps['Wake-on'][0] != 'g':
            utils.system_output("ethtool -s %s wol g" % self._ethname)
            self._restore_wol = True

        # Set RTC as backup to WOL
        before_secs = rtc.get_seconds()
        alarm_secs =  before_secs + self._suspend_secs + self._threshold_secs
        rtc.set_wake_alarm(alarm_secs)

        sys_power.do_suspend(self._suspend_secs)

        after_secs = rtc.get_seconds()
        # flush RTC as it may not work subsequently if wake was not RTC
        rtc.set_wake_alarm(0)

        suspended_secs = after_secs - before_secs
        if suspended_secs >= (self._suspend_secs + self._threshold_secs):
            raise error.TestError("Device woke due to RTC not WOL")


    def _verify_wol_magic(self):
        """If possible identify wake source was caused by WOL.

        The bits identifying the wake source may be cleared by the time
        userspace gets a chance to query the kernel.  However, firmware
        might have a log and expose the wake source.  Attempt to interrogate
        the wake source details if they are present on the system.

        Returns:
          True if verified or unable to verify due to system limitations
          False otherwise
        """
        fw_log = "/sys/firmware/log"
        if not os.path.isfile(fw_log):
            logging.warning("Unable to verify wake in s/w due to missing log %s",
                         fw_log)
            return True

        log_info_str = utils.system_output("egrep '(SMI|PM1|GPE0)_STS:' %s" %
                                           fw_log)
        status_dict = {}
        for ln in log_info_str.splitlines():
            logging.debug("f/w line = %s", ln)
            try:
                (status_reg, status_values) = ln.strip().split(":")
                status_dict[status_reg] = status_values.split()
            except ValueError:
                # no bits asserted ... empty list
                status_dict[status_reg] = list()

        for status_reg in status_dict:
            logging.debug("status_dict[%s] = %s", status_reg,
                          status_dict[status_reg])

        return ('PM1' in status_dict['SMI_STS']) and \
            ('WAK' in status_dict['PM1_STS']) and \
            ('PCIEXPWAK' in status_dict['PM1_STS']) and \
            len(status_dict['GPE0_STS']) == 0


    def cleanup(self):
        if self._restore_wol:
            utils.system_output("ethtool -s %s wol %s" %
                                (self._ethname, self._caps['Wake-on'][0]))


    def run_once(self, ethname=None, suspend_secs=5, threshold_secs=10):
        """Run the test.

        Args:
          ethname: string of ethernet device under test
          threshold_secs: integer of seconds to determine whether wake occurred
            due to WOL versus RTC
        """
        if not ethname:
            raise error.TestError("Name of ethernet device must be declared")

        self._ethname = ethname
        self._threshold_secs = threshold_secs
        self._suspend_secs = suspend_secs
        self._bus_info = None

        self._parse_ethtool_caps()
        self._check_eth_caps()

        # ChromeOS does not require WOL support for any USB Ethernet Adapters.
        # In fact, WoL only known to work for PCIe Ethernet devices.
        # We know _some_ platforms power off all USB ports when suspended.
        # USB adapters with "pg" capabilities _might_ WoL on _some_ platforms.
        # White list/black listing of platforms will be required to test
        # WoL against USB dongles in the future.
        if self._is_usb():
            logging.debug("Skipping WOL test on USB Ethernet device.")
            return

        self._test_wol_magic_packet()
        # TODO(tbroch) There is evidence in the filesystem of the wake source
        # for coreboot but its still being flushed out.  For now only produce a
        # warning for this check.
        if not self._verify_wol_magic():
            logging.warning("Unable to see evidence of WOL wake in filesystem")
