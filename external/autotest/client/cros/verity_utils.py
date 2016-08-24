# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging, mmap, os, time

import common
from autotest_lib.client.bin import os_dep, test
from autotest_lib.client.common_lib import error, logging_manager, utils

""" a wrapper for using verity/dm-verity with a test backing store """

# enum for the 3 possible values of the module parameter.
ERROR_BEHAVIOR_ERROR = 'eio'
ERROR_BEHAVIOR_REBOOT = 'panic'
ERROR_BEHAVIOR_IGNORE = 'none'
ERROR_BEHAVIOR_NOTIFIER = 'notify'  # for platform specific behavior.

# Default configuration for verity_image
DEFAULT_TARGET_NAME = 'verity_image'
DEFAULT_ALG = 'sha1'
DEFAULT_IMAGE_SIZE_IN_BLOCKS = 100
DEFAULT_ERROR_BEHAVIOR = ERROR_BEHAVIOR_ERROR
# TODO(wad) make this configurable when dm-verity doesn't hard-code 4096.
BLOCK_SIZE = 4096

def system(command, timeout=None):
    """Delegate to utils.system to run |command|, logs stderr only on fail.

    Runs |command|, captures stdout and stderr.  Logs stdout to the DEBUG
    log no matter what, logs stderr only if the command actually fails.
    Will time the command out after |timeout|.
    """
    utils.run(command, timeout=timeout, ignore_status=False,
              stdout_tee=utils.TEE_TO_LOGS, stderr_tee=utils.TEE_TO_LOGS,
              stderr_is_expected=True)

class verity_image(object):
    """ a helper for creating dm-verity targets for testing.

        To use,
          vi = verity_image()
          vi.initialize(self.tmpdir, "dmveritytesta")
          # Create a 409600 byte image with /bin/ls on it
          # The size in bytes is returned.
          backing_path = vi.create_backing_image(100, copy_files=['/bin/ls'])
          # Performs hashing of the backing_path and sets up a device.
          loop_dev = vi.prepare_backing_device()
          # Sets up the mapped device and returns the path:
          # E.g., /dev/mapper/autotest_dmveritytesta
          dev = vi.create_verity_device()
          # Access the mapped device using the returned string.

       TODO(wad) add direct verified and backing store access functions
                 to make writing modifiers easier (e.g., mmap).
    """
    # Define the command template constants.
    verity_cmd = \
        'verity mode=create alg=%s payload=%s payload_blocks=%d hashtree=%s'
    dd_cmd = 'dd if=/dev/zero of=%s bs=4096 count=0 seek=%d'
    mkfs_cmd = 'mkfs.ext3 -b 4096 -F %s'
    dmsetup_cmd = "dmsetup -r create autotest_%s --table '%s'"

    def _device_release(self, cmd, device):
        if utils.system(cmd, ignore_status=True) == 0:
            return
        logging.warning("Could not release %s. Retrying..." % (device))
        # Other things (like cros-disks) may have the device open briefly,
        # so if we initially fail, try again and attempt to gather details
        # on who else is using the device.
        fuser = utils.system_output('fuser -v %s' % (device),
                                    retain_output=True)
        lsblk = utils.system_output('lsblk %s' % (device),
                                    retain_output=True)
        time.sleep(1)
        if utils.system(cmd, ignore_status=True) == 0:
            return
        raise error.TestFail('"%s" failed: %s\n%s' % (cmd, fuser, lsblk))

    def reset(self):
        """Idempotent call which will free any claimed system resources"""
        # Pre-initialize these values to None
        for attr in ['mountpoint', 'device', 'loop', 'file', 'hash_file']:
            if not hasattr(self, attr):
                setattr(self, attr, None)
        logging.info("verity_image is being reset")

        if self.mountpoint is not None:
            system('umount %s' % self.mountpoint)
            self.mountpoint = None

        if self.device is not None:
            self._device_release('dmsetup remove %s' % (self.device),
                                 self.device)
            self.device = None

        if self.loop is not None:
            self._device_release('losetup -d %s' % (self.loop), self.loop)
            self.loop = None

        if self.file is not None:
            os.remove(self.file)
            self.file = None

        if self.hash_file is not None:
            os.remove(self.hash_file)
            self.hash_file = None

        self.alg = DEFAULT_ALG
        self.error_behavior = DEFAULT_ERROR_BEHAVIOR
        self.blocks = DEFAULT_IMAGE_SIZE_IN_BLOCKS
        self.file = None
        self.has_fs = False
        self.hash_file = None
        self.table = None
        self.target_name = DEFAULT_TARGET_NAME

        self.__initialized = False

    def __init__(self):
        """Sets up the defaults for the object and then calls reset()
        """
        self.reset()

    def __del__(self):
        # Release any and all system resources.
        self.reset()

    def _create_image(self):
        """Creates a dummy file."""
        # TODO(wad) replace with python
        utils.system_output(self.dd_cmd % (self.file, self.blocks))

    def _create_fs(self, copy_files):
        """sets up ext3 on the image"""
        self.has_fs = True
        system(self.mkfs_cmd % self.file)
        if type(copy_files) is list:
          for file in copy_files:
              pass  # TODO(wad)

    def _hash_image(self):
        """runs verity over the image and saves the device mapper table"""
        self.table = utils.system_output(self.verity_cmd % (self.alg,
                                                            self.file,
                                                            self.blocks,
                                                            self.hash_file))
        # The verity tool doesn't include a templated error value.
        # For now, we add one.
        self.table += " error_behavior=ERROR_BEHAVIOR"
        logging.info("table is %s" % self.table)

    def _append_hash(self):
        f = open(self.file, 'ab')
        f.write(utils.read_file(self.hash_file))
        f.close()

    def _setup_loop(self):
        # Setup a loop device
        self.loop = utils.system_output('losetup -f --show %s' % (self.file))

    def _setup_target(self):
        # Update the table with the loop dev
        self.table = self.table.replace('HASH_DEV', self.loop)
        self.table = self.table.replace('ROOT_DEV', self.loop)
        self.table = self.table.replace('ERROR_BEHAVIOR', self.error_behavior)

        system(self.dmsetup_cmd % (self.target_name, self.table))
        self.device = "/dev/mapper/autotest_%s" % self.target_name

    def initialize(self,
                   tmpdir,
                   target_name,
                   alg=DEFAULT_ALG,
                   size_in_blocks=DEFAULT_IMAGE_SIZE_IN_BLOCKS,
                   error_behavior=DEFAULT_ERROR_BEHAVIOR):
        """Performs any required system-level initialization before use.
        """
        try:
            os_dep.commands('losetup', 'mkfs.ext3', 'dmsetup', 'verity', 'dd',
                            'dumpe2fs')
        except ValueError, e:
            logging.error('verity_image cannot be used without: %s' % e)
            return False

        # Used for the mapper device name and the tmpfile names.
        self.target_name = target_name

        # Reserve some files to use.
        self.file = os.tempnam(tmpdir, '%s.img.' % self.target_name)
        self.hash_file = os.tempnam(tmpdir, '%s.hash.' % self.target_name)

        # Set up the configurable bits.
        self.alg = alg
        self.error_behavior = error_behavior
        self.blocks = size_in_blocks

        self.__initialized = True
        return True

    def create_backing_image(self, size_in_blocks, with_fs=True,
                             copy_files=None):
        """Creates an image file of the given number of blocks and if specified
           will create a filesystem and copy any files in a copy_files list to
           the fs.
        """
        self.blocks = size_in_blocks
        self._create_image()

        if with_fs is True:
            self._create_fs(copy_files)
        else:
            if type(copy_files) is list and len(copy_files) != 0:
                logging.warning("verity_image.initialize called with " \
                             "files to copy but no fs")

        return self.file

    def prepare_backing_device(self):
        """Hashes the backing image, appends it to the backing image, points
           a loop device at it and returns the path to the loop."""
        self._hash_image()
        self._append_hash()
        self._setup_loop()
        return self.loop

    def create_verity_device(self):
        """Sets up the device mapper node and returns its path"""
        self._setup_target()
        return self.device

    def verifiable(self):
        """Returns True if the dm-verity device does not throw any errors
           when being walked completely or False if it does."""
        try:
            if self.has_fs is True:
                system('dumpe2fs %s' % self.device)
            # TODO(wad) replace with mmap.mmap-based access
            system('dd if=%s of=/dev/null bs=4096' % self.device)
            return True
        except error.CmdError, e:
            return False


class VerityImageTest(test.test):
    """VerityImageTest provides a base class for verity_image tests
       to be derived from.  It sets up a verity_image object for use
       and provides the function mod_and_test() to wrap simple test
       cases for verity_images.

       See platform_DMVerityCorruption as an example usage.
    """
    version = 1
    image_blocks = DEFAULT_IMAGE_SIZE_IN_BLOCKS

    def initialize(self):
        """Overrides test.initialize() to setup a verity_image"""
        self.verity = verity_image()

    # Example callback for mod_and_test that does nothing
    def mod_nothing(self, run_count, backing_path, block_size, block_count):
        pass

    def mod_and_test(self, modifier, count, expected):
        """Takes in a callback |modifier| and runs it |count| times over
           the verified image checking for |expected| out of verity.verifiable()
        """
        tries = 0
        while tries < count:
            # Start fresh then modify each block in the image.
            self.verity.reset()
            self.verity.initialize(self.tmpdir, self.__class__.__name__)
            backing_path = self.verity.create_backing_image(self.image_blocks)
            loop_dev = self.verity.prepare_backing_device()

            modifier(tries,
                     backing_path,
                     BLOCK_SIZE,
                     self.image_blocks)

            mapped_dev = self.verity.create_verity_device()

            # Now check for failure.
            if self.verity.verifiable() is not expected:
                raise error.TestFail(
                    '%s: verity.verifiable() not as expected (%s)' %
                    (modifier.__name__, expected))
            tries += 1
