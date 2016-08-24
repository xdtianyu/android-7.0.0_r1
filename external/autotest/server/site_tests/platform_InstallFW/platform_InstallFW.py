# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
from autotest_lib.server import test, utils

class platform_InstallFW(test.test):
    """Test to install FW on DUT"""
    version = 1

    def run_once(self, host=None, fw_type=None, fw_path=None, fw_name=None):
        """Run test to install firmware.

        @param host: host to run on
        @param fw_type: must be either "bios" or "ec"
        @param fw_path: path to fw binary or set to "local"
        @param fw_name: (optional) name of binary file
        """

        if fw_path == "local":
            fw_dst = "/usr/sbin/chromeos-firmwareupdate"
            is_shellball = True
        else:
            fw_src = "%s/%s" % (fw_path, fw_name)
            # Determine the firmware file is a shellball or a raw binary.
            is_shellball = (utils.system_output("file %s" % fw_src).find(
                    "shell script") != -1)
            fw_dst = "/tmp/%s" % fw_name
            # Copy binary from server to client.
            host.send_file(fw_src, fw_dst)

        # Install bios/ec on a client.
        if fw_type == "bios":
            if is_shellball:
                host.run("sudo /bin/sh %s --mode recovery --update_main "
                         "--noupdate_ec" % fw_dst)
            else:
                host.run("sudo /usr/sbin/flashrom -p host -w %s"
                         % fw_dst)
        if fw_type == "ec":
            if is_shellball:
                host.run("sudo /bin/sh %s --mode recovery --update_ec "
                         "--noupdate_main" % fw_dst)
            else:
                host.run("sudo /usr/sbin/flashrom -p ec -w %s"
                         % fw_dst)
        # Reboot client after installing the binary.
        host.reboot()
        # Get the versions of BIOS and EC binaries.
        bios_info = host.run("crossystem fwid")
        logging.info("BIOS version info:\n %s", bios_info)
        ec_info = host.run("sudo mosys -k ec info")
        logging.info("EC version info:\n %s", ec_info)
