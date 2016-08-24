#!/usr/bin/python
#
# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging, os
from autotest_lib.client.bin import utils, test
from autotest_lib.client.common_lib import error

class kernel_CrosECSysfs(test.test):
    '''Make sure the EC sysfs interface provides meaningful output'''
    version = 1

    cros_ec = '/dev/cros_ec'
    sysfs_path = '/sys/devices/virtual/chromeos/cros_ec'
    kernel_ver = os.uname()[2]
    if utils.compare_versions(kernel_ver, "3.14") >= 0:
        sysfs_path = '/sys/class/chromeos/cros_ec'

    def _read_file(self, filename):
        """
        Return the contents of the given file or fail.

        @param filename Full path to the file to be read
        """
        try:
            content = utils.read_file(filename)
        except Exception as err:
            raise error.TestFail('sysfs file problem: %s' % err)
        return content

    def _read_sysfs(self, filename):
        """
        Read the contents of the given sysfs file or fail

        @param filename Name of the file within the sysfs interface directory
        """
        fullpath = os.path.join(self.sysfs_path, filename)
        return self._read_file(fullpath)

    def _read_field(self, filename, field):
        """
        Return the given field from the sysfs file or fail

        @param filename Name of the file within the sysfs interface directory
        @param field Name of field to match in the file content
        """
        fullpath = os.path.join(self.sysfs_path, filename)
        content = self._read_file(fullpath)
        match = utils.get_field(content, 0, field)
        if match is None:
            raise error.TestFail("no '%s' field in %s" % (field, fullpath))
        return match

    def run_once(self):
        """
        Quick check for the existence of the basic sysfs files
        """
        # If /dev/cros_ec isn't present, then the MFD_CROS_EC_DEV driver isn't
        # present, so there's no point to looking for the sysfs interface to it.
        if not os.path.exists(self.cros_ec):
            raise error.TestFail("%s not found. No driver?" % self.cros_ec)

        flashsize = self._read_field('flashinfo', 'FlashSize')
        logging.info("flashsize is %s", flashsize)

        build = self._read_field('version', 'Build info:')
        logging.info("build is %s", build)

        reboot = self._read_sysfs('reboot')
        if reboot.find("ro") < 0:
            raise error.TestFail('reboot help is weird: %s' % reboot)
        logging.info("reboot is %s", reboot)
