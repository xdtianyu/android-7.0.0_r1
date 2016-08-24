# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging, os
from autotest_lib.client.bin import test, utils
from autotest_lib.client.common_lib import error
import tempfile

class security_ModuleLocking(test.test):
    """
    Handle examining the system for specific module loading capabilities.
    """
    version = 1

    def _passed(self, msg):
        logging.info('ok: %s', msg)

    def _failed(self, msg):
        logging.error('FAIL: %s', msg)
        self._failures.append(msg)

    def _fatal(self, msg):
        logging.error('FATAL: %s', msg)
        raise error.TestError(msg)

    def check(self, boolean, msg, fatal=False):
        """
        Check boolean state and report condition to log.

        @param boolean: condition to examine
        @param msg: what the condition is testing
        @param fatal: should the test full abort on the condition failing
        """
        if boolean == True:
            self._passed(msg)
        else:
            msg = "could not satisfy '%s'" % (msg)
            if fatal:
                self._fatal(msg)
            else:
                self._failed(msg)

    def module_loaded(self, module):
        """
        Detect if the given module is already loaded in the kernel.

        @param module: name of module to check
        """
        module = module.replace('-', '_')
        match = "%s " % (module)
        for line in open("/proc/modules"):
            if line.startswith(match):
                return True
        return False

    def rmmod(self, module):
        """
        Unload a module if it is already loaded in the kernel.

        @param module: name of module to unload
        """
        if self.module_loaded(module):
            utils.system("rmmod %s" % (module))

    def modprobe(self, module):
        """
        If a module is not already loaded in the kernel, load it via modprobe.

        @param module: name of module to load
        """
        if not self.module_loaded(module):
            utils.system("modprobe %s" % (module))

    def _module_path(self, module):
        """
        Locate a kernel module's full filesystem path.

        @param module: name of module to locate
        """
        ko = utils.system_output("find /lib/modules -name '%s.ko'" % (module))
        return ko.splitlines()[0]

    def module_loads_outside_rootfs(self, module):
        """
        Copies the given module into /tmp and tries to load it from there
        using insmod directly.

        @param module: name of module to test
        """
        # Start from a clean slate.
        self.rmmod(module)

        # Make sure we can load with standard mechanisms.
        self.modprobe(module)
        self.rmmod(module)

        # Load module directly with insmod from root filesystem.
        ko = self._module_path(module)
        utils.system("insmod %s" % (ko))
        self.rmmod(module)

        # Load module directly with insmod from /tmp.
        tmp = "/tmp/%s.ko" % (module)
        utils.system("cp %s %s" % (ko, tmp))
        rc = utils.system("insmod %s" % (tmp), ignore_status=True)

        # Clean up.
        self.rmmod(module)
        utils.system("rm %s" % (tmp))

        if rc == 0:
            return True
        return False

    def module_loads_old_api(self, module):
        """
        Loads a module using the old blob-style kernel syscall. With
        kmod, this requires compressing the module first to trigger
        in-memory decompression and loading.

        @param module: name of module to test
        """
        # Start from a clean slate.
        self.rmmod(module)

        # Compress module to trigger the old API.
        tmp = "/tmp/%s.ko.gz" % (module)
        ko = self._module_path(module)
        utils.system("gzip -c %s > %s" % (ko, tmp))
        rc = utils.system("insmod %s" % (tmp), ignore_status=True)

        # Clean up.
        self.rmmod(module)
        utils.system("rm %s" % (tmp))

        if rc == 0:
            return True
        return False

    def module_loads_after_bind_umount(self, module):
        """
        Makes sure modules can still load after a bind mount of the
        filesystem is umounted.

        @param module: name of module to test
        """

        # Start from a clean slate.
        self.rmmod(module)

        # Make sure we can load with standard mechanisms.
        self.modprobe(module)
        self.rmmod(module)

        # Create and umount a bind mount of the root filesystem.
        bind = tempfile.mkdtemp(prefix=module)
        rc = utils.system("mount -o bind / %s && umount %s" % (bind, bind))
        utils.system("rmdir %s" % (bind))

        # Attempt to load again.
        self.modprobe(module)
        self.rmmod(module)

        if rc == 0:
            return True
        return False

    def run_once(self):
        """
        Check that the fd-based module loading syscall is enforcing the
        module fd origin to the root filesystem, and that it can be
        disabled and will allow the old syscall API as well.
        TODO(keescook): add production test to make sure that on a verified
        boot, "/proc/sys/kernel/chromiumos/module_locking" does not exist.
        """
        # Empty failure list means test passes.
        self._failures = []

        # Check that the sysctl is either missing or set to 1.
        sysctl = "/proc/sys/kernel/chromiumos/module_locking"
        if os.path.exists(sysctl):
            self.check(open(sysctl).read() == '1\n', "%s enabled" % (sysctl))

        # Check the enforced state is to deny non-rootfs module loads.
        module = "test_module"
        loaded = self.module_loads_outside_rootfs(module)
        self.check(loaded == False, "cannot load %s from /tmp" % (module))

        # Check old API fails when enforcement enabled.
        loaded = self.module_loads_old_api(module)
        self.check(loaded == False, "cannot load %s with old API" % (module))

        # Make sure the bind umount bug is not present.
        loaded = self.module_loads_after_bind_umount(module)
        self.check(loaded == True, "can load %s after bind umount" % (module))

        # If the sysctl exists, verify that it will disable the restriction.
        if os.path.exists(sysctl):
            # Disable restriction.
            open(sysctl, "w").write("0\n")
            self.check(open(sysctl).read() == '0\n', "%s disabled" % (sysctl))

            # Check enforcement is disabled.
            loaded = self.module_loads_outside_rootfs(module)
            self.check(loaded == True, "can load %s from /tmp" % (module))

            # Check old API works when enforcement disabled.
            loaded = self.module_loads_old_api(module)
            self.check(loaded == True, "can load %s with old API" % (module))

            # Clean up.
            open(sysctl, "w").write("1\n")
            self.check(open(sysctl).read() == '1\n', "%s enabled" % (sysctl))

        # Raise a failure if anything unexpected was seen.
        if len(self._failures):
            raise error.TestFail((", ".join(self._failures)))
