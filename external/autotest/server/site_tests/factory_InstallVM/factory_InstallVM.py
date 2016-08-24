# Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""
Factory install VM tests.

This test supports the flags documented in FactoryInstallTest, plus:

    debug_vnc: whether to run VNC on the KVM (for debugging only)
    debug_save_hda: if specified, path to save the hda.bin to after
        running the factory install shim (for debugging only)
    debug_reuse_hda: if specified, path to an existing hda.bin image
        to reuse (for debugging only)
"""


import os, re

from autotest_lib.client.bin import utils as client_utils
from autotest_lib.server.cros.factory_install_test import FactoryInstallTest
from autotest_lib.server.hosts import ssh_host


# How long to wait after killing KVMs.
_KILL_KVM_WAIT_SEC = 5

# The size of the "SSD" in the KVM.
_HDA_SIZE_MB = 8192

# The name of the image used for hda.  This should be unique to this test,
# since we will kill all stray KVM processes using this disk image.
_HDA_FILENAME = "factory_InstallVM_hda.bin"


class factory_InstallVM(FactoryInstallTest):
    """
    Factory install VM tests.

    See file-level docstring for more information.
    """

    def _get_kvm_command(self, kvm_args=[]):
        """
        Returns the command to run KVM.

        @param kvm_args: A list of extra args to pass to KVM.
        """
        kvm_base_args = [
            "kvm",
            "-m", "2048",
            "-net", "nic,model=virtio",
            "-net", "user,hostfwd=tcp::%d-:22" % self.ssh_tunnel_port,
            "-vga", "vmware",  # Because -vga std is slow
            ]

        if self.vnc:
            # Without nographic, we need to explicitly add "-serial stdio"
            # (or output will go to vc).  Use 127.0.0.1 to ensure that kvm
            # listens with IPv4.
            kvm_base_args.extend(["-serial", "stdio", "-vnc", "127.0.0.1:1"])
        else:
            kvm_base_args.append("-nographic")

        return " ".join(kvm_base_args + kvm_args)

    def _kill_kvm(self):
        """
        Kills the KVM on the client machine.

        This will kill any KVM whose command line contains _HDA_FILENAME
        (which is specific to this test).
        """
        def try_kill_kvm():
            pattern = "^kvm.*%s" % _HDA_FILENAME,
            if (self.client.run("pgrep -f '%s'" % pattern, ignore_status=True)
                .exit_status == 1):
                return True
            self.client.run("pkill -f '%s'" % (pattern))
            return False

        client_utils.poll_for_condition(
            try_kill_kvm, timeout=_KILL_KVM_WAIT_SEC, desc="Kill KVM")

    def get_hwid_cfg(self):
        """
        Overridden from superclass.
        """
        return "vm"

    def get_dut_client(self):
        """
        Overridden from superclass.
        """
        return ssh_host.SSHHost("localhost", port=self.ssh_tunnel_port)

    def run_factory_install(self, local_hdb):
        """
        Overridden from superclass.
        """
        self.hda = os.path.join(self.client.get_tmp_dir(), _HDA_FILENAME)

        if self.reuse_hda is not None:
            self.client.run("cp %s %s" % (self.reuse_hda, self.hda))
        else:
            # Mount partition 12 the image and modify it to enable serial
            # logging.
            mount = self._mount_partition(local_hdb, 12)
            self._modify_file(
                os.path.join(mount, "syslinux/usb.A.cfg"),
                lambda contents: re.sub(r"console=\w+", "console=ttyS0",
                                        contents))
            self._umount_partition(mount)

            # On the client, create a nice big sparse file for hda
            # (a.k.a. the SSD).
            self.client.run("truncate -s %dM %s" % (_HDA_SIZE_MB, self.hda))
            hdb = os.path.join(self.client.get_tmp_dir(), "hdb.bin")
            self.client.send_file(local_hdb, hdb)

            # Fire up the KVM and wait for the factory install to complete.
            self._kill_kvm()  # Just in case
            self.client.run_grep(
                self._get_kvm_command(
                    ["-drive", "file=%s,boot=off" % self.hda,
                     "-drive", "file=%s,boot=on" % hdb,
                     "-no-reboot"]),
                timeout=FactoryInstallTest.FACTORY_INSTALL_TIMEOUT_SEC,
                stdout_ok_regexp="Factory Installer Complete")
            self._kill_kvm()

            if self.save_hda is not None:
                self.client.run("cp %s %s" % (self.hda, self.save_hda))

        # Run KVM again (the factory tests should now come up).
        kvm = self.client.run(self._get_kvm_command([
                    "-hda", self.hda,
                    "-daemonize",
                    "-no-reboot"]))

    def reboot_for_wipe(self):
        """
        Overridden from superclass.
        """
        # Use halt instead of reboot; reboot doesn't work consistently in KVM.
        self.get_dut_client().halt()
        self._kill_kvm()

        # Start KVM again.  The ChromeOS test image should now come up.
        kvm = self.client.run(
            self._get_kvm_command(["-hda", self.hda, "-daemonize"]))

    def run_once(self, host, debug_reuse_hda=None, debug_save_hda=None,
                 debug_vnc=False, **args):
        self.client = host
        self.reuse_hda = debug_reuse_hda
        self.save_hda = debug_save_hda
        self.vnc = self.parse_boolean(debug_vnc)

        self.cleanup_tasks.append(self._kill_kvm)

        super(factory_InstallVM, self).run_once(**args)
