# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""A module to provide interface to OS services."""

import datetime
import os
import re
import struct

import shell_wrapper


class OSInterfaceError(Exception):
    """OS interface specific exception."""
    pass

class Crossystem(object):
    """A wrapper for the crossystem utility."""

    # Code dedicated for user triggering recovery mode through crossystem.
    USER_RECOVERY_REQUEST_CODE = '193'

    def init(self, os_if):
        """Init the instance. If running on Mario - adjust the map."""
        self.os_if = os_if

    def __getattr__(self, name):
        """
        Retrieve a crosssystem attribute.

        Attempt to access crossystemobject.name will invoke `crossystem name'
        and return the stdout as the value.
        """
        return self.os_if.run_shell_command_get_output(
            'crossystem %s' % name)[0]

    def __setattr__(self, name, value):
        if name in ('os_if',):
            self.__dict__[name] = value
        else:
            self.os_if.run_shell_command('crossystem "%s=%s"' % (name, value))

    def request_recovery(self):
        """Request recovery mode next time the target reboots."""

        self.__setattr__('recovery_request', self.USER_RECOVERY_REQUEST_CODE)


class OSInterface(object):
    """An object to encapsulate OS services functions."""

    ANDROID_TESTER_FILE = '/mnt/stateful_partition/.android_faft_tester'

    def __init__(self):
        """Object construction time initialization."""
        self.state_dir = None
        self.log_file = None
        self.cs = Crossystem()
        self.is_android = os.path.isfile(self.ANDROID_TESTER_FILE)
        if self.is_android:
            self.shell = shell_wrapper.AdbShell()
            self.host_shell = shell_wrapper.LocalShell()
        else:
            self.shell = shell_wrapper.LocalShell()
            self.host_shell = None


    def init(self, state_dir=None, log_file=None):
        """Initialize the OS interface object.

        Args:
          state_dir - a string, the name of the directory (as defined by the
                      caller). The contents of this directory persist over
                      system restarts and power cycles.
          log_file - a string, the name of the log file kept in the state
                     directory.

        Default argument values support unit testing.
        """
        self.cs.init(self)
        self.state_dir = state_dir

        if self.state_dir:
            if not os.path.exists(self.state_dir):
                try:
                    os.mkdir(self.state_dir)
                except OSError, err:
                    raise OSInterfaceError(err)
            if log_file:
                if log_file[0] == '/':
                    self.log_file = log_file
                else:
                    self.log_file = os.path.join(state_dir, log_file)

        # Initialize the shell. Should be after creating the log file.
        self.shell.init(self)
        if self.host_shell:
            self.host_shell.init(self)

    def has_host(self):
        """Return True if a host is connected to DUT."""
        return self.is_android

    def run_shell_command(self, cmd):
        """Run a shell command."""
        self.shell.run_command(cmd)

    def run_shell_command_get_status(self, cmd):
        """Run shell command and return its return code."""
        return self.shell.run_command_get_status(cmd)

    def run_shell_command_get_output(self, cmd):
        """Run shell command and return its console output."""
        return self.shell.run_command_get_output(cmd)

    def run_host_shell_command(self, cmd, block=True):
        """Run a shell command on the host."""
        if self.host_shell:
            self.host_shell.run_command(cmd, block)
        else:
            raise OSInterfaceError('There is no host for DUT.')

    def run_host_shell_command_get_status(self, cmd):
        """Run shell command and return its return code on the host."""
        if self.host_shell:
            return self.host_shell.run_command_get_status(cmd)
        else:
            raise OSInterfaceError('There is no host for DUT.')

    def run_host_shell_command_get_output(self, cmd):
        """Run shell command and return its console output."""
        if self.host_shell:
            return self.host_shell.run_command_get_output(cmd)
        else:
            raise OSInterfaceError('There is no host for DUT.')

    def read_file(self, path):
        """Read the content of the file."""
        return self.shell.read_file(path)

    def write_file(self, path, data):
        """Write the data to the file."""
        self.shell.write_file(path, data)

    def append_file(self, path, data):
        """Append the data to the file."""
        self.shell.append_file(path, data)

    def path_exists(self, path):
        """Return True if the path exists on DUT."""
        cmd = 'test -e %s' % path
        return self.run_shell_command_get_status(cmd) == 0

    def is_dir(self, path):
        """Return True if the path is a directory."""
        cmd = 'test -d %s' % path
        return self.run_shell_command_get_status(cmd) == 0

    def create_dir(self, path):
        """Create a new directory."""
        cmd = 'mkdir -p %s' % path
        return self.run_shell_command(cmd)

    def create_temp_file(self, prefix):
        """Create a temporary file with a prefix."""
        if self.is_android:
            tmp_path = '/data/local/tmp'
        else:
            tmp_path = '/tmp'
        cmd = 'mktemp -p %s %sXXXXXX' % (tmp_path, prefix)
        return self.run_shell_command_get_output(cmd)[0]

    def copy_file(self, from_path, to_path):
        """Copy the file."""
        cmd = 'cp -f %s %s' % (from_path, to_path)
        return self.run_shell_command(cmd)

    def copy_dir(self, from_path, to_path):
        """Copy the directory."""
        cmd = 'cp -rf %s %s' % (from_path, to_path)
        return self.run_shell_command(cmd)

    def remove_file(self, path):
        """Remove the file."""
        cmd = 'rm -f %s' % path
        return self.run_shell_command(cmd)

    def remove_dir(self, path):
        """Remove the directory."""
        cmd = 'rm -rf %s' % path
        return self.run_shell_command(cmd)

    def get_file_size(self, path):
        """Get the size of the file."""
        cmd = 'stat -c %%s %s' % path
        return int(self.run_shell_command_get_output(cmd)[0])

    def target_hosted(self):
        """Return True if running on DUT."""
        if self.is_android:
            return True
        signature = open('/etc/lsb-release', 'r').readlines()[0]
        return re.search(r'chrom(ium|e)os', signature, re.IGNORECASE) != None

    def state_dir_file(self, file_name):
        """Get a full path of a file in the state directory."""
        return os.path.join(self.state_dir, file_name)

    def wait_for_device(self, timeout):
        """Wait for an Android device to be connected."""
        return self.shell.wait_for_device(timeout)

    def wait_for_no_device(self, timeout):
        """Wait for no Android device to be connected (offline)."""
        return self.shell.wait_for_no_device(timeout)

    def log(self, text):
        """Write text to the log file and print it on the screen, if enabled.

        The entire log (maintained across reboots) can be found in
        self.log_file.
        """
        if not self.log_file or not os.path.exists(self.state_dir):
            # Called before environment was initialized, ignore.
            return

        timestamp = datetime.datetime.strftime(
            datetime.datetime.now(), '%I:%M:%S %p:')

        with open(self.log_file, 'a') as log_f:
            log_f.write('%s %s\n' % (timestamp, text))
            log_f.flush()
            os.fdatasync(log_f)

    def is_removable_device(self, device):
        """Check if a certain storage device is removable.

        device - a string, file name of a storage device or a device partition
                 (as in /dev/sda[0-9] or /dev/mmcblk0p[0-9]).

        Returns True if the device is removable, False if not.
        """
        if self.is_android:
            return False

        if not self.target_hosted():
            return False

        # Drop trailing digit(s) and letter(s) (if any)
        base_dev = self.strip_part(device.split('/')[2])
        removable = int(self.read_file('/sys/block/%s/removable' % base_dev))

        return removable == 1

    def get_internal_disk(self, device):
        """Get the internal disk by given the current disk.

        If device is removable device, internal disk is decided by which kind
        of divice (arm or x86). Otherwise, return device itself.

        device - a string, file name of a storage device or a device partition
                 (as in /dev/sda[0-9] or /dev/mmcblk0p[0-9]).

        Return internal kernel disk.
        """
        if self.is_removable_device(device):
            if self.path_exists('/dev/mmcblk0'):
                return '/dev/mmcblk0'
            else:
                return '/dev/sda'
        else:
            return self.strip_part(device)

    def get_root_part(self):
        """Return a string, the name of root device with partition number"""
        # FIXME(waihong): Android doesn't support dual kernel/root and misses
        # the related tools. Just return something that not break the existing
        # code.
        if self.is_android:
            return '/dev/mmcblk0p3'
        else:
            return self.run_shell_command_get_output('rootdev -s')[0]

    def get_root_dev(self):
        """Return a string, the name of root device without partition number"""
        return self.strip_part(self.get_root_part())

    def join_part(self, dev, part):
        """Return a concatenated string of device and partition number"""
        if 'mmcblk' in dev:
            return dev + 'p' + part
        else:
            return dev + part

    def strip_part(self, dev_with_part):
        """Return a stripped string without partition number"""
        dev_name_stripper = re.compile('p?[0-9]+$')
        return dev_name_stripper.sub('', dev_with_part)

    def retrieve_body_version(self, blob):
        """Given a blob, retrieve body version.

        Currently works for both, firmware and kernel blobs. Returns '-1' in
        case the version can not be retrieved reliably.
        """
        header_format = '<8s8sQ'
        preamble_format = '<40sQ'
        magic, _, kb_size = struct.unpack_from(header_format, blob)

        if magic != 'CHROMEOS':
            return -1  # This could be a corrupted version case.

        _, version = struct.unpack_from(preamble_format, blob, kb_size)
        return version

    def retrieve_datakey_version(self, blob):
        """Given a blob, retrieve firmware data key version.

        Currently works for both, firmware and kernel blobs. Returns '-1' in
        case the version can not be retrieved reliably.
        """
        header_format = '<8s96sQ'
        magic, _, version = struct.unpack_from(header_format, blob)
        if magic != 'CHROMEOS':
            return -1 # This could be a corrupted version case.
        return version

    def retrieve_kernel_subkey_version(self, blob):
        """Given a blob, retrieve kernel subkey version.

        It is in firmware vblock's preamble.
        """

        header_format = '<8s8sQ'
        preamble_format = '<72sQ'
        magic, _, kb_size = struct.unpack_from(header_format, blob)

        if magic != 'CHROMEOS':
            return -1

        _, version = struct.unpack_from(preamble_format, blob, kb_size)
        return version

    def retrieve_preamble_flags(self, blob):
        """Given a blob, retrieve preamble flags if available.

        It only works for firmware. If the version of preamble header is less
        than 2.1, no preamble flags supported, just returns 0.
        """
        header_format = '<8s8sQ'
        preamble_format = '<32sII64sI'
        magic, _, kb_size = struct.unpack_from(header_format, blob)

        if magic != 'CHROMEOS':
            return -1  # This could be a corrupted version case.

        _, ver, subver, _, flags = struct.unpack_from(preamble_format, blob,
                                                      kb_size)

        if ver > 2 or (ver == 2 and subver >= 1):
            return flags
        else:
            return 0  # Returns 0 if preamble flags not available.

    def read_partition(self, partition, size):
        """Read the requested partition, up to size bytes."""
        tmp_file = self.state_dir_file('part.tmp')
        self.run_shell_command('dd if=%s of=%s bs=1 count=%d' % (
                partition, tmp_file, size))
        data = self.read_file(tmp_file)
        self.remove_file(tmp_file)
        return data
