# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os

from autotest_lib.client.bin import test, utils
from autotest_lib.client.common_lib import error

class platform_StackProtector(test.test):
    version = 3

    def load_whitelist(self):
        wfile = open(os.path.join(self.bindir, 'whitelist'))
        whitelist = wfile.read().splitlines()
        wfile.close()
        return set(whitelist)

    # http://build.chromium.org/mirror/chromiumos/mirror/distfiles/
    # binutils-2.19.1.tar.bz2
    def setup(self, tarball="binutils-2.19.1.tar.bz2"):
        if os.path.exists(self.srcdir):
            utils.system("rm -rf %s" % self.srcdir)

        tarball = utils.unmap_url(self.bindir, tarball, self.tmpdir)
        utils.extract_tarball_to_dir(tarball, self.srcdir)

        os.chdir(self.srcdir)
        utils.system("patch -p1 < ../binutils-2.19-arm.patch");
        utils.configure()
        utils.make()


    def run_once(self, rootdir="/"):
        """
        Do a find for all files on the system
        For each one, run objdump on them. We'll get either:
        * output containing stack_chk (good)
        * stderr containing 'not recognized' on e.g. shell scripts (ok)
        For those, the egrep -q exit(0)'s and there's no output.
        But, for files compiled without stack protector, the egrep will
        exit(1) and we'll note the name of those files.

        Check all current/future partitions unless known harmless (e.g. proc).
        Skip files < 512 bytes due to objdump false positive and test speed.
        """
        libc_glob = "/lib/libc-[0-9]*"
        os.chdir(self.srcdir)
        cmd = ("find '%s' -wholename %s -prune -o "
               " -wholename /proc -prune -o "
               " -wholename /dev -prune -o "
               " -wholename /sys -prune -o "
               " -wholename /mnt/stateful_partition -prune -o "
               " -wholename /usr/local -prune -o "
               # There are files in /home/chronos that cause false positives,
               # and since that's noexec anyways, it should be skipped.
               " -wholename '/home/chronos' -prune -o "
               # libc needs to be checked differently, skip here:
               " -wholename '%s' -prune -o "
               # The various gconv locale .so's don't count:
               " -wholename '/usr/lib/gconv/*' -prune -o"
               " -type f -size +511c -exec "
               "sh -c 'binutils/objdump -CR {} 2>&1 | "
               "egrep -q \"(stack_chk|Invalid|not recognized)\" || echo {}' ';'"
               )
        badfiles = utils.system_output(cmd % (rootdir, self.autodir, libc_glob))

        # Subtract any files that were on the whitelist.
        seen = set(badfiles.splitlines())
        diff = seen.difference(self.load_whitelist())

        # Special case check for libc, needs different objdump flags.
        cmd = "binutils/objdump -D %s | egrep -q stack_chk || echo %s"
        libc_stack_chk = utils.system_output(cmd % (libc_glob, libc_glob))

        if diff or libc_stack_chk:
            diff.add(libc_stack_chk)
            raise error.TestFail("Missing -fstack-protector:\n"
                                 + "\n".join(diff))
