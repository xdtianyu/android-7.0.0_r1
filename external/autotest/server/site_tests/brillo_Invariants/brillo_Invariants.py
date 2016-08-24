# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import common
from autotest_lib.client.common_lib import error
from autotest_lib.server import test


class brillo_Invariants(test.test):
    """Check for things that should look the same on all Brillo devices."""
    version = 1


    def assert_path_test(self, path, test, negative=False):
        """Performs a test against a path.

        See the man page for test(1) for valid tests (e.g. -e, -b, -d).

        @param path: the path to check.
        @param test: the test to perform, without leading dash.
        @param negative: if True, test for the negative.
        """
        self.dut.run('test %s -%s %s' % ('!' if negative else '', test, path))


    def assert_selinux_context(self, path, ctx):
        """Checks the selinux context of a path.

        @param path: the path to check.
        @param ctx: the selinux context to check for.

        @raises error.TestFail
        """
        # Example output of 'ls -Z /dev/block/by-name/misc' is:
        # u:object_r:misc_block_device:s0 /dev/block/by-name/misc
        tokens = self.dut.run_output('ls -Z %s' % path).split()
        path_ctx = tokens[0]
        if not ctx in path_ctx:
            raise error.TestFail('Context "%s" for path "%s" does not '
                                 'contain "%s"' % (path_ctx, path, ctx))


    def check_fstab_name(self):
        """Checks that the fstab file has the name /fstab.<ro.hardware>.
        """
        hardware = self.dut.run_output('getprop ro.hardware')
        self.assert_path_test('/fstab.%s' % hardware, 'e')
        self.assert_path_test('/fstab.device', 'e', negative=True)


    def check_block_devices(self):
        """Checks required devices in /dev/block/by-name/ and their context.
        """
        ctx = 'boot_block_device'
        self.assert_selinux_context('/dev/block/by-name/boot_a', ctx)
        self.assert_selinux_context('/dev/block/by-name/boot_b', ctx)
        ctx = 'system_block_device'
        self.assert_selinux_context('/dev/block/by-name/system_a', ctx)
        self.assert_selinux_context('/dev/block/by-name/system_b', ctx)
        ctx = 'misc_block_device'
        self.assert_selinux_context('/dev/block/by-name/misc', ctx)
        self.assert_path_test('/dev/block/by-name/userdata', 'b')


    def run_once(self, dut=None):
        """Check for things that should look the same on all Brillo devices.

        @param dut: host object representing the device under test.
        """
        self.dut = dut
        self.check_fstab_name()
        self.check_block_devices()
