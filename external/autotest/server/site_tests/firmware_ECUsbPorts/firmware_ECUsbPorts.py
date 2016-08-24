# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging

from autotest_lib.client.common_lib import error
from autotest_lib.server.cros.faft.firmware_test import FirmwareTest


class firmware_ECUsbPorts(FirmwareTest):
    """
    Servo based EC USB port control test.
    """
    version = 1


    # Delay for remote shell command call to return
    RPC_DELAY = 1

    # Delay between turning off and on USB ports
    REBOOT_DELAY = 6

    # Timeout range for waiting system to shutdown
    SHUTDOWN_TIMEOUT = 10

    # USB charge modes, copied from ec/include/usb_charge.h
    USB_CHARGE_MODE_DISABLED       = 0
    USB_CHARGE_MODE_SDP2           = 1
    USB_CHARGE_MODE_CDP            = 2
    USB_CHARGE_MODE_DCP_SHORT      = 3
    USB_CHARGE_MODE_ENABLED        = 4

    def initialize(self, host, cmdline_args):
        super(firmware_ECUsbPorts, self).initialize(host, cmdline_args)
        # Only run in normal mode
        self.switcher.setup_mode('normal')
        self.ec.send_command("chan 0")


    def cleanup(self):
        self.ec.send_command("chan 0xffffffff")
        super(firmware_ECUsbPorts, self).cleanup()


    def fake_reboot_by_usb_mode_change(self):
        """
        Turn off USB ports and also kill FAFT client so that this acts like a
        reboot. If USB ports cannot be turned off or on, reboot step would
        fail.
        """
        for_all_ports_cmd = ('id=0; while [ $id -lt %d ];' +
                             'do ectool usbchargemode "$id" %d;' +
                             'id=$((id+1)); sleep 0.5; done')
        # Port disable - same for smart and dumb ports.
        ports_off_cmd = for_all_ports_cmd % (self._port_count,
                                             self.USB_CHARGE_MODE_DISABLED)
        # Port enable - different command based on smart/dumb port.
        port_enable_param = (self.USB_CHARGE_MODE_SDP2
            if self._smart_usb_charge else self.USB_CHARGE_MODE_ENABLED)
        ports_on_cmd = for_all_ports_cmd % (self._port_count, port_enable_param)
        cmd = ("(sleep %d; %s; sleep %d; %s)&" %
                (self.RPC_DELAY, ports_off_cmd,
                 self.REBOOT_DELAY,
                 ports_on_cmd))
        self.faft_client.system.run_shell_command(cmd)
        self.faft_client.disconnect()


    def get_port_count(self):
        """
        Get the number of USB ports by checking the number of GPIO named
        USB*_ENABLE.
        """
        cnt = 0
        limit = 10
        while limit > 0:
            try:
                gpio_name = "USB%d_ENABLE" % (cnt + 1)
                self.ec.send_command_get_output(
                        "gpioget %s" % gpio_name,
                        ["[01].\s*%s" % gpio_name])
                cnt = cnt + 1
                limit = limit - 1
            except error.TestFail:
                logging.info("Found %d USB ports", cnt)
                return cnt

        # Limit reached. Probably something went wrong.
        raise error.TestFail("Unexpected error while trying to determine " +
                             "number of USB ports")


    def wait_port_disabled(self, port_count, timeout):
        """
        Wait for all USB ports to be disabled.

        Args:
          @param port_count: Number of USB ports.
          @param timeout: Timeout range.
        """
        logging.info('Waiting for %d USB ports to be disabled.', port_count)
        while timeout > 0:
            try:
                timeout = timeout - 1
                for idx in xrange(1, port_count+1):
                    gpio_name = "USB%d_ENABLE" % idx
                    self.ec.send_command_get_output(
                            "gpioget %s" % gpio_name,
                            ["0.\s*%s" % gpio_name])
                return True
            except error.TestFail:
                # USB ports not disabled. Retry.
                pass
        return False


    def check_power_off_mode(self):
        """Shutdown the system and check USB ports are disabled."""
        self._failed = False
        self.faft_client.system.run_shell_command("shutdown -P now")
        self.switcher.wait_for_client_offline()
        if not self.wait_port_disabled(self._port_count, self.SHUTDOWN_TIMEOUT):
            logging.info("Fails to wait for USB port disabled")
            self._failed = True
        self.servo.power_short_press()


    def check_failure(self):
        """Returns true if failure has been encountered."""
        return not self._failed


    def run_once(self):
        if not self.check_ec_capability(['usb']):
            raise error.TestNAError("Nothing needs to be tested on this device")

        self._smart_usb_charge = (
            'smart_usb_charge' in self.faft_config.ec_capability)
        self._port_count = self.get_port_count()

        logging.info("Turn off all USB ports and then turn them on again.")
        self.switcher.mode_aware_reboot(
                'custom', self.fake_reboot_by_usb_mode_change)

        logging.info("Check USB ports are disabled when powered off.")
        self.switcher.mode_aware_reboot('custom', self.check_power_off_mode)

        logging.info("Check if failure occurred.")
        self.check_state(self.check_failure)
