# Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""
Factory install servo tests.

This test supports the flags documented in FactoryInstallTest, plus:

    servo_host: the host running the servod (defaults to localhost)
    servo_port: the port on which to run servod (defaults to an unused
        port)
    debug_image_usb: whether to image the USB disk in servo mode (default to
        true, may be set to false for debugging only if the USB disk is
        already imaged)
"""


import glob, logging, os, re, time

from autotest_lib.client.bin import utils as client_utils
from autotest_lib.client.common_lib import error
from autotest_lib.server import hosts
from autotest_lib.server import utils
from autotest_lib.server.cros.factory_install_test import FactoryInstallTest
from autotest_lib.server.cros.servo import servo


class factory_InstallServo(FactoryInstallTest):
    """
    Factory install VM tests.

    See file-level docstring for more information.
    """

    def _create_servo(self, servo_host, servo_port):
        self.servo = servo.Servo(
                hosts.ServoHost(servo_host=servo_host, servo_port=servo_port))
        def kill_servo():
            del self.servo
        self.cleanup_tasks.append(kill_servo)
        self.servo.initialize_dut(cold_reset=True)

        self.servo.enable_usb_hub()
        self.servo_usb_disk = self.servo.probe_host_usb_dev()
        if not self.servo_usb_disk:
            raise error.TestError("Unable to find USB disk")
        logging.info("Servo USB device detected at %s", self.servo_usb_disk)

    def get_hwid_cfg(self):
        """
        Overridden from superclass.
        """
        return "servo"

    def get_dut_client(self):
        """
        Overridden from superclass.
        """
        return hosts.SSHHost(self.dut_ip)

    def run_factory_install(self, shim_image):
        """
        Overridden from superclass.
        """
        self.servo.install_recovery_image(image_path=shim_image)

        # Wait for the IP address of the DUT to appear in the Miniohama
        # server logs.
        def get_dut_ip():
            match = re.search(r"(\d+\.\d+\.\d+\.\d+) - -.*htpdate",
                              open(self.miniomaha_output).read())
            return match.group(1) if match else None

        self.dut_ip = client_utils.poll_for_condition(
            get_dut_ip, timeout=FactoryInstallTest.FACTORY_INSTALL_TIMEOUT_SEC,
            desc="Get DUT IP")

        logging.debug("DUT IP is %s", self.dut_ip)

        if not self.get_dut_client().wait_up(
            FactoryInstallTest.FACTORY_INSTALL_TIMEOUT_SEC):
            raise error.TestFail("DUT never came up at %s" % self.dut_ip)

    def reboot_for_wipe(self):
        """
        Overridden from superclass.
        """
        self.get_dut_client().reboot(
            timeout=FactoryInstallTest.FIRST_BOOT_TIMEOUT_SEC)

    def run_once(self, servo_host="localhost", servo_port=None,
                 debug_image_usb=True,
                 **args):
        self.image_usb = self.parse_boolean(debug_image_usb)
        self._create_servo(
            servo_host,
            int(servo_port) if servo_port else utils.get_unused_port())
        super(factory_InstallServo, self).run_once(**args)
