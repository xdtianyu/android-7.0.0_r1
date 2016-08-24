# Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""
Factory install tests.

FactoryInstallTest is an abstract superclass; factory_InstallVM and
factory_InstallServo are two concrete implementations.

Subclasses of FactoryInstallTest supports the following flags:

    factory_install_image: (required) path to factory install shim
    factory_test_image: (required) path to factory test image
    test_image: (required) path to ChromeOS test image
    miniomaha_port: port for miniomaha
    debug_make_factory_package: whether to re-make the factory package before
        running tests (defaults to true; may be set to false for debugging
        only)
"""

import glob, logging, os, re, shutil, socket, sys, thread, time, traceback
from abc import abstractmethod
from StringIO import StringIO

from autotest_lib.client.bin import utils as client_utils
from autotest_lib.client.common_lib import error
from autotest_lib.server import test, utils


# How long to wait for the mini-Omaha server to come up.
_MINIOMAHA_TIMEOUT_SEC = 50

# Path to make_factory_package.sh within the source root.
_MAKE_FACTORY_PACKAGE_PATH = \
    "platform/factory-utils/factory_setup/make_factory_package.sh"

# Path to miniomaha.py within the source root.
_MINIOMAHA_PATH = "platform/factory-utils/factory_setup/miniomaha.py"

# Sleep interval for nontrivial operations (like rsyncing).
_POLL_SLEEP_INTERVAL_SEC = 2

# The hwid_updater script (run in the factory install shim).  This is a format
# string with a single argument (the name of the HWID cfg).
_HWID_UPDATER_SH_TEMPLATE = """
echo Running hwid_updater "$@" >&2
set -ex
MOUNT_DIR=$(mktemp -d --tmpdir)
mount "$1" "$MOUNT_DIR"
ls -l "$MOUNT_DIR"
mkdir -p "$MOUNT_DIR/dev_image/share/chromeos-hwid"
echo %s > "$MOUNT_DIR/dev_image/share/chromeos-hwid/cfg"
umount "$MOUNT_DIR"
"""


class FactoryInstallTest(test.test):
    """
    Factory install VM tests.

    See file-level docstring for details.
    """

    version = 1

    # How long to wait for the factory tests to install.
    FACTORY_INSTALL_TIMEOUT_SEC = 1800

    # How long to wait for the factory test image to come up.
    WAIT_UP_TIMEOUT_SEC = 30

    # How long to wait for the factory tests to run.
    FACTORY_TEST_TIMEOUT_SEC = 240

    # How long to wait for the ChromeOS image to run.
    FIRST_BOOT_TIMEOUT_SEC = 480

    #
    # Abstract functions that must be overridden by subclasses.
    #

    @abstractmethod
    def get_hwid_cfg(self):
        """
        Returns the HWID cfg, used to select a test list.
        """
        pass

    @abstractmethod
    def run_factory_install(self, shim_image):
        """
        Performs the factory install and starts the factory tests.

        When this returns, the DUT should be starting up (or have already
        started up) in factory test mode.
        """
        pass

    @abstractmethod
    def get_dut_client(self):
        """
        Returns a client (subclass of CrosHost) to control the DUT.
        """
        pass

    @abstractmethod
    def reboot_for_wipe(self):
        """
        Reboots the machine after preparing to wipe the hard drive.
        """
        pass

    #
    # Utility methods that may be used by subclasses.
    #

    def src_root(self):
        """
        Returns the CrOS source root.
        """
        return os.path.join(os.environ["CROS_WORKON_SRCROOT"], "src")

    def parse_boolean(self, val):
        """
        Parses a string as a Boolean value.
        """
        # Insist on True or False, because (e.g.) bool('false') == True.
        if str(val) not in ["True", "False"]:
            raise error.TestError("Not a boolean: '%s'" % val)
        return str(val) == "True"

    #
    # Private utility methods.
    #

    def _modify_file(self, path, func):
        """
        Modifies a file as the root user.

        @param path: The path to the file to modify.
        @param func: A function that will be invoked with a single argument
            (the current contents of the file, or None if the file does not
            exist) and which should return the new contents.
        """
        if os.path.exists(path):
            contents = utils.system_output("sudo cat %s" % path)
        else:
            contents = func(None)

        utils.run("sudo dd of=%s" % path, stdin=func(contents))

    def _mount_partition(self, image, index):
        """
        Mounts a partition of an image temporarily using loopback.

        The partition will be automatically unmounted when the test exits.

        @param image: The image to mount.
        @param index: The partition number to mount.
        @return: The mount point.
        """
        mount_point = os.path.join(self.tmpdir,
                                   "%s_%d" % (image, index))
        if not os.path.exists(mount_point):
            os.makedirs(mount_point)
        common_args = "cgpt show -i %d %s" % (index, image)
        offset = int(utils.system_output(common_args + " -b")) * 512
        size = int(utils.system_output(common_args + " -s")) * 512
        utils.run("sudo mount -o rw,loop,offset=%d,sizelimit=%d %s %s" % (
                offset, size, image, mount_point))
        self.cleanup_tasks.append(lambda: self._umount_partition(mount_point))
        return mount_point

    def _umount_partition(self, mount_point):
        """
        Unmounts the mount at the given mount point.

        Also deletes the mount point directory.  Does not raise an
        exception if the mount point does not exist or the mount fails.
        """
        if os.path.exists(mount_point):
            utils.run("sudo umount -d %s" % mount_point)
            os.rmdir(mount_point)

    def _make_factory_package(self, factory_test_image, test_image):
        """
        Makes the factory package.
        """
        # Create a pseudo-HWID-updater that merely sets the HWID to "vm" or
        # "servo" so that the appropriate test list will run.  (This gets run by
        # the factory install shim.)
        hwid_updater = os.path.join(self.tmpdir, "hwid_updater.sh")
        with open(hwid_updater, "w") as f:
            f.write(_HWID_UPDATER_SH_TEMPLATE % self.get_hwid_cfg())

        utils.run("%s --factory=%s --release=%s "
                  "--firmware_updater=none --hwid_updater=%s " %
                  (os.path.join(self.src_root(), _MAKE_FACTORY_PACKAGE_PATH),
                   factory_test_image, test_image, hwid_updater))

    def _start_miniomaha(self):
        """
        Starts a mini-Omaha server and drains its log output.
        """
        def is_miniomaha_up():
            try:
                utils.urlopen(
                    "http://localhost:%d" % self.miniomaha_port).read()
                return True
            except:
                return False

        assert not is_miniomaha_up()

        self.miniomaha_output = os.path.join(self.outputdir, "miniomaha.out")

        # TODO(jsalz): Add cwd to BgJob rather than including the 'cd' in the
        # command.
        bg_job = utils.BgJob(
            "cd %s; exec ./%s --port=%d --factory_config=miniomaha.conf"
            % (os.path.join(self.src_root(),
                            os.path.dirname(_MINIOMAHA_PATH)),
               os.path.basename(_MINIOMAHA_PATH),
               self.miniomaha_port), verbose=True,
            stdout_tee=utils.TEE_TO_LOGS,
            stderr_tee=open(self.miniomaha_output, "w"))
        self.cleanup_tasks.append(lambda: utils.nuke_subprocess(bg_job.sp))
        thread.start_new_thread(utils.join_bg_jobs, ([bg_job],))

        client_utils.poll_for_condition(is_miniomaha_up,
                                        timeout=_MINIOMAHA_TIMEOUT_SEC,
                                        desc="Miniomaha server")

    def _prepare_factory_install_shim(self, factory_install_image):
        # Make a copy of the factory install shim image (to use as hdb).
        modified_image = os.path.join(self.tmpdir, "shim.bin")
        logging.info("Creating factory install image: %s", modified_image)
        shutil.copyfile(factory_install_image, modified_image)

        # Mount partition 1 of the modified_image and set the mini-Omaha server.
        mount = self._mount_partition(modified_image, 1)
        self._modify_file(
            os.path.join(mount, "dev_image/etc/lsb-factory"),
            lambda contents: re.sub(
                r"^(CHROMEOS_(AU|DEV)SERVER)=.+",
                r"\1=http://%s:%d/update" % (
                    socket.gethostname(), self.miniomaha_port),
                contents,
                re.MULTILINE))
        self._umount_partition(mount)

        return modified_image

    def _run_factory_tests_and_prepare_wipe(self):
        """
        Runs the factory tests and prepares the machine for wiping.
        """
        dut_client = self.get_dut_client()
        if not dut_client.wait_up(FactoryInstallTest.WAIT_UP_TIMEOUT_SEC):
            raise error.TestFail("DUT never came up to run factory tests")

        # Poll the factory log, and wait for the factory_Review test to become
        # active.
        local_factory_log = os.path.join(self.outputdir, "factory.log")
        remote_factory_log = "/var/log/factory.log"

        # Wait for factory.log file to exist
        dut_client.run(
            "while ! [ -e %s ]; do sleep 1; done" % remote_factory_log,
            timeout=FactoryInstallTest.FACTORY_TEST_TIMEOUT_SEC)

        status_map = {}

        def wait_for_factory_logs():
            dut_client.get_file(remote_factory_log, local_factory_log)
            data = open(local_factory_log).read()
            new_status_map = dict(
                re.findall(r"status change for (\S+) : \S+ -> (\S+)", data))
            if status_map != new_status_map:
                logging.info("Test statuses: %s", status_map)
                # Can't assign directly since it's in a context outside
                # this function.
                status_map.clear()
                status_map.update(new_status_map)
            return status_map.get("factory_Review.z") == "ACTIVE"

        client_utils.poll_for_condition(
            wait_for_factory_logs,
            timeout=FactoryInstallTest.FACTORY_TEST_TIMEOUT_SEC,
            sleep_interval=_POLL_SLEEP_INTERVAL_SEC,
            desc="Factory logs")

        # All other statuses should be "PASS".
        expected_status_map = {
            "memoryrunin": "PASS",
            "factory_Review.z": "ACTIVE",
            "factory_Start.e": "PASS",
            "hardware_SAT.memoryrunin_s1": "PASS",
        }
        if status_map != expected_status_map:
            raise error.TestFail("Expected statuses of %s but found %s" % (
                    expected_status_map, status_map))

        dut_client.run("cd /usr/local/factory/bin; "
                       "./gooftool --prepare_wipe --verbose")

    def _complete_install(self):
        """
        Completes the install, resulting in a full ChromeOS image.
        """
        # Restart the SSH client: with a new OS, some configuration
        # properties (e.g., availability of rsync) may have changed.
        dut_client = self.get_dut_client()

        if not dut_client.wait_up(FactoryInstallTest.FIRST_BOOT_TIMEOUT_SEC):
            raise error.TestFail("DUT never came up after install")

        # Check lsb-release to make sure we have a real live ChromeOS image
        # (it should be the test build).
        lsb_release = os.path.join(self.tmpdir, "lsb-release")
        dut_client.get_file("/etc/lsb-release", lsb_release)
        expected_re = r"^CHROMEOS_RELEASE_DESCRIPTION=.*Test Build"
        data = open(lsb_release).read()
        assert re.search(
            "^CHROMEOS_RELEASE_DESCRIPTION=.*Test Build", data, re.MULTILINE), (
            "Didn't find expected regular expression %s in lsb-release: " % (
                expected_re, data))
        logging.info("Install succeeded!  lsb-release is:\n%s", data)

        dut_client.halt()
        if not dut_client.wait_down(
            timeout=FactoryInstallTest.WAIT_UP_TIMEOUT_SEC):
            raise error.TestFail("Client never went down after ChromeOS boot")

    #
    # Autotest methods.
    #

    def setup(self):
        self.cleanup_tasks = []
        self.ssh_tunnel_port = utils.get_unused_port()

    def run_once(self, factory_install_image, factory_test_image, test_image,
                 miniomaha_port=None, debug_make_factory_package=True,
                 **args):
        """
        Runs the test once.

        See the file-level comments for an explanation of the test arguments.

        @param args: Must be empty (present as a check against misspelled
            arguments on the command line)
        """
        assert not args, "Unexpected arguments %s" % args

        self.miniomaha_port = (
            int(miniomaha_port) if miniomaha_port else utils.get_unused_port())

        if self.parse_boolean(debug_make_factory_package):
            self._make_factory_package(factory_test_image, test_image)
        self._start_miniomaha()
        shim_image = self._prepare_factory_install_shim(factory_install_image)
        self.run_factory_install(shim_image)
        self._run_factory_tests_and_prepare_wipe()
        self.reboot_for_wipe()
        self._complete_install()

    def cleanup(self):
        for task in self.cleanup_tasks:
            try:
                task()
            except:
                logging.info("Exception in cleanup task:")
                traceback.print_exc(file=sys.stdout)
