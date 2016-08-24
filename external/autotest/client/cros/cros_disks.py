# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import dbus, gobject, logging, os, stat
from dbus.mainloop.glib import DBusGMainLoop

import common
from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib import autotemp, error
from mainloop import ExceptionForward
from mainloop import GenericTesterMainLoop


"""This module contains several helper classes for writing tests to verify the
CrosDisks DBus interface. In particular, the CrosDisksTester class can be used
to derive functional tests that interact with the CrosDisks server over DBus.
"""


class ExceptionSuppressor(object):
    """A context manager class for suppressing certain types of exception.

    An instance of this class is expected to be used with the with statement
    and takes a set of exception classes at instantiation, which are types of
    exception to be suppressed (and logged) in the code block under the with
    statement.

    Example:

        with ExceptionSuppressor(OSError, IOError):
            # An exception, which is a sub-class of OSError or IOError, is
            # suppressed in the block code under the with statement.
    """
    def __init__(self, *args):
        self.__suppressed_exc_types = (args)

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_value, traceback):
        if exc_type and issubclass(exc_type, self.__suppressed_exc_types):
            try:
                logging.exception('Suppressed exception: %s(%s)',
                                  exc_type, exc_value)
            except Exception:
                pass
            return True
        return False


class DBusClient(object):
    """ A base class of a DBus proxy client to test a DBus server.

    This class is expected to be used along with a GLib main loop and provides
    some convenient functions for testing the DBus API exposed by a DBus server.
    """
    def __init__(self, main_loop, bus, bus_name, object_path):
        """Initializes the instance.

        Args:
            main_loop: The GLib main loop.
            bus: The bus where the DBus server is connected to.
            bus_name: The bus name owned by the DBus server.
            object_path: The object path of the DBus server.
        """
        self.__signal_content = {}
        self.main_loop = main_loop
        self.signal_timeout_in_seconds = 10
        logging.debug('Getting D-Bus proxy object on bus "%s" and path "%s"',
                      bus_name, object_path)
        self.proxy_object = bus.get_object(bus_name, object_path)

    def clear_signal_content(self, signal_name):
        """Clears the content of the signal.

        Args:
            signal_name: The name of the signal.
        """
        if signal_name in self.__signal_content:
            self.__signal_content[signal_name] = None

    def get_signal_content(self, signal_name):
        """Gets the content of a signal.

        Args:
            signal_name: The name of the signal.

        Returns:
            The content of a signal or None if the signal is not being handled.
        """
        return self.__signal_content.get(signal_name)

    def handle_signal(self, interface, signal_name, argument_names=()):
        """Registers a signal handler to handle a given signal.

        Args:
            interface: The DBus interface of the signal.
            signal_name: The name of the signal.
            argument_names: A list of argument names that the signal contains.
        """
        if signal_name in self.__signal_content:
            return

        self.__signal_content[signal_name] = None

        def signal_handler(*args):
            self.__signal_content[signal_name] = dict(zip(argument_names, args))

        logging.debug('Handling D-Bus signal "%s(%s)" on interface "%s"',
                      signal_name, ', '.join(argument_names), interface)
        self.proxy_object.connect_to_signal(signal_name, signal_handler,
                                            interface)

    def wait_for_signal(self, signal_name):
        """Waits for the reception of a signal.

        Args:
            signal_name: The name of the signal to wait for.

        Returns:
            The content of the signal.
        """
        if signal_name not in self.__signal_content:
            return None

        def check_signal_content():
            context = self.main_loop.get_context()
            while context.iteration(False):
                pass
            return self.__signal_content[signal_name] is not None

        logging.debug('Waiting for D-Bus signal "%s"', signal_name)
        utils.poll_for_condition(condition=check_signal_content,
                                 desc='%s signal' % signal_name,
                                 timeout=self.signal_timeout_in_seconds)
        content = self.__signal_content[signal_name]
        logging.debug('Received D-Bus signal "%s(%s)"', signal_name, content)
        self.__signal_content[signal_name] = None
        return content

    def expect_signal(self, signal_name, expected_content):
        """Waits the the reception of a signal and verifies its content.

        Args:
            signal_name: The name of the signal to wait for.
            expected_content: The expected content of the signal, which can be
                              partially specified. Only specified fields are
                              compared between the actual and expected content.

        Returns:
            The actual content of the signal.

        Raises:
            error.TestFail: A test failure when there is a mismatch between the
                            actual and expected content of the signal.
        """
        actual_content = self.wait_for_signal(signal_name)
        logging.debug("%s signal: expected=%s actual=%s",
                      signal_name, expected_content, actual_content)
        for argument, expected_value in expected_content.iteritems():
            if argument not in actual_content:
                raise error.TestFail(
                    ('%s signal missing "%s": expected=%s, actual=%s') %
                    (signal_name, argument, expected_content, actual_content))

            if actual_content[argument] != expected_value:
                raise error.TestFail(
                    ('%s signal not matched on "%s": expected=%s, actual=%s') %
                    (signal_name, argument, expected_content, actual_content))
        return actual_content


class CrosDisksClient(DBusClient):
    """A DBus proxy client for testing the CrosDisks DBus server.
    """

    CROS_DISKS_BUS_NAME = 'org.chromium.CrosDisks'
    CROS_DISKS_INTERFACE = 'org.chromium.CrosDisks'
    CROS_DISKS_OBJECT_PATH = '/org/chromium/CrosDisks'
    DBUS_PROPERTIES_INTERFACE = 'org.freedesktop.DBus.Properties'
    FORMAT_COMPLETED_SIGNAL = 'FormatCompleted'
    FORMAT_COMPLETED_SIGNAL_ARGUMENTS = (
        'status', 'path'
    )
    MOUNT_COMPLETED_SIGNAL = 'MountCompleted'
    MOUNT_COMPLETED_SIGNAL_ARGUMENTS = (
        'status', 'source_path', 'source_type', 'mount_path'
    )

    def __init__(self, main_loop, bus):
        """Initializes the instance.

        Args:
            main_loop: The GLib main loop.
            bus: The bus where the DBus server is connected to.
        """
        super(CrosDisksClient, self).__init__(main_loop, bus,
                                              self.CROS_DISKS_BUS_NAME,
                                              self.CROS_DISKS_OBJECT_PATH)
        self.interface = dbus.Interface(self.proxy_object,
                                        self.CROS_DISKS_INTERFACE)
        self.properties = dbus.Interface(self.proxy_object,
                                         self.DBUS_PROPERTIES_INTERFACE)
        self.handle_signal(self.CROS_DISKS_INTERFACE,
                           self.FORMAT_COMPLETED_SIGNAL,
                           self.FORMAT_COMPLETED_SIGNAL_ARGUMENTS)
        self.handle_signal(self.CROS_DISKS_INTERFACE,
                           self.MOUNT_COMPLETED_SIGNAL,
                           self.MOUNT_COMPLETED_SIGNAL_ARGUMENTS)

    def is_alive(self):
        """Invokes the CrosDisks IsAlive method.

        Returns:
            True if the CrosDisks server is alive or False otherwise.
        """
        return self.interface.IsAlive()

    def enumerate_auto_mountable_devices(self):
        """Invokes the CrosDisks EnumerateAutoMountableDevices method.

        Returns:
            A list of sysfs paths of devices that are auto-mountable by
            CrosDisks.
        """
        return self.interface.EnumerateAutoMountableDevices()

    def enumerate_devices(self):
        """Invokes the CrosDisks EnumerateMountableDevices method.

        Returns:
            A list of sysfs paths of devices that are recognized by
            CrosDisks.
        """
        return self.interface.EnumerateDevices()

    def get_device_properties(self, path):
        """Invokes the CrosDisks GetDeviceProperties method.

        Args:
            path: The device path.

        Returns:
            The properties of the device in a dictionary.
        """
        return self.interface.GetDeviceProperties(path)

    def format(self, path, filesystem_type=None, options=None):
        """Invokes the CrosDisks Format method.

        Args:
            path: The device path to format.
            filesystem_type: The filesystem type used for formatting the device.
            options: A list of options used for formatting the device.
        """
        if filesystem_type is None:
            filesystem_type = ''
        if options is None:
            options = []
        self.clear_signal_content(self.FORMAT_COMPLETED_SIGNAL)
        self.interface.Format(path, filesystem_type, options)

    def wait_for_format_completion(self):
        """Waits for the CrosDisks FormatCompleted signal.

        Returns:
            The content of the FormatCompleted signal.
        """
        return self.wait_for_signal(self.FORMAT_COMPLETED_SIGNAL)

    def expect_format_completion(self, expected_content):
        """Waits and verifies for the CrosDisks FormatCompleted signal.

        Args:
            expected_content: The expected content of the FormatCompleted
                              signal, which can be partially specified.
                              Only specified fields are compared between the
                              actual and expected content.

        Returns:
            The actual content of the FormatCompleted signal.

        Raises:
            error.TestFail: A test failure when there is a mismatch between the
                            actual and expected content of the FormatCompleted
                            signal.
        """
        return self.expect_signal(self.FORMAT_COMPLETED_SIGNAL,
                                  expected_content)

    def mount(self, path, filesystem_type=None, options=None):
        """Invokes the CrosDisks Mount method.

        Args:
            path: The device path to mount.
            filesystem_type: The filesystem type used for mounting the device.
            options: A list of options used for mounting the device.
        """
        if filesystem_type is None:
            filesystem_type = ''
        if options is None:
            options = []
        self.clear_signal_content(self.MOUNT_COMPLETED_SIGNAL)
        self.interface.Mount(path, filesystem_type, options)

    def unmount(self, path, options=None):
        """Invokes the CrosDisks Unmount method.

        Args:
            path: The device or mount path to unmount.
            options: A list of options used for unmounting the path.
        """
        if options is None:
            options = []
        self.interface.Unmount(path, options)

    def wait_for_mount_completion(self):
        """Waits for the CrosDisks MountCompleted signal.

        Returns:
            The content of the MountCompleted signal.
        """
        return self.wait_for_signal(self.MOUNT_COMPLETED_SIGNAL)

    def expect_mount_completion(self, expected_content):
        """Waits and verifies for the CrosDisks MountCompleted signal.

        Args:
            expected_content: The expected content of the MountCompleted
                              signal, which can be partially specified.
                              Only specified fields are compared between the
                              actual and expected content.

        Returns:
            The actual content of the MountCompleted signal.

        Raises:
            error.TestFail: A test failure when there is a mismatch between the
                            actual and expected content of the MountCompleted
                            signal.
        """
        return self.expect_signal(self.MOUNT_COMPLETED_SIGNAL,
                                  expected_content)


class CrosDisksTester(GenericTesterMainLoop):
    """A base tester class for testing the CrosDisks server.

    A derived class should override the get_tests method to return a list of
    test methods. The perform_one_test method invokes each test method in the
    list to verify some functionalities of CrosDisks server.
    """
    def __init__(self, test):
        bus_loop = DBusGMainLoop(set_as_default=True)
        bus = dbus.SystemBus(mainloop=bus_loop)
        self.main_loop = gobject.MainLoop()
        super(CrosDisksTester, self).__init__(test, self.main_loop)
        self.cros_disks = CrosDisksClient(self.main_loop, bus)

    def get_tests(self):
        """Returns a list of test methods to be invoked by perform_one_test.

        A derived class should override this method.

        Returns:
            A list of test methods.
        """
        return []

    @ExceptionForward
    def perform_one_test(self):
        """Exercises each test method in the list returned by get_tests.
        """
        tests = self.get_tests()
        self.remaining_requirements = set([test.func_name for test in tests])
        for test in tests:
            test()
            self.requirement_completed(test.func_name)


class FilesystemTestObject(object):
    """A base class to represent a filesystem test object.

    A filesystem test object can be a file, directory or symbolic link.
    A derived class should override the _create and _verify method to implement
    how the test object should be created and verified, respectively, on a
    filesystem.
    """
    def __init__(self, path, content, mode):
        """Initializes the instance.

        Args:
            path: The relative path of the test object.
            content: The content of the test object.
            mode: The file permissions given to the test object.
        """
        self._path = path
        self._content = content
        self._mode = mode

    def create(self, base_dir):
        """Creates the test object in a base directory.

        Args:
            base_dir: The base directory where the test object is created.

        Returns:
            True if the test object is created successfully or False otherwise.
        """
        if not self._create(base_dir):
            logging.debug('Failed to create filesystem test object at "%s"',
                          os.path.join(base_dir, self._path))
            return False
        return True

    def verify(self, base_dir):
        """Verifies the test object in a base directory.

        Args:
            base_dir: The base directory where the test object is expected to be
                      found.

        Returns:
            True if the test object is found in the base directory and matches
            the expected content, or False otherwise.
        """
        if not self._verify(base_dir):
            logging.debug('Failed to verify filesystem test object at "%s"',
                          os.path.join(base_dir, self._path))
            return False
        return True

    def _create(self, base_dir):
        return False

    def _verify(self, base_dir):
        return False


class FilesystemTestDirectory(FilesystemTestObject):
    """A filesystem test object that represents a directory."""

    def __init__(self, path, content, mode=stat.S_IRWXU|stat.S_IRGRP| \
                 stat.S_IXGRP|stat.S_IROTH|stat.S_IXOTH):
        super(FilesystemTestDirectory, self).__init__(path, content, mode)

    def _create(self, base_dir):
        path = os.path.join(base_dir, self._path) if self._path else base_dir

        if self._path:
            with ExceptionSuppressor(OSError):
                os.makedirs(path)
                os.chmod(path, self._mode)

        if not os.path.isdir(path):
            return False

        for content in self._content:
            if not content.create(path):
                return False
        return True

    def _verify(self, base_dir):
        path = os.path.join(base_dir, self._path) if self._path else base_dir
        if not os.path.isdir(path):
            return False

        for content in self._content:
            if not content.verify(path):
                return False
        return True


class FilesystemTestFile(FilesystemTestObject):
    """A filesystem test object that represents a file."""

    def __init__(self, path, content, mode=stat.S_IRUSR|stat.S_IWUSR| \
                 stat.S_IRGRP|stat.S_IROTH):
        super(FilesystemTestFile, self).__init__(path, content, mode)

    def _create(self, base_dir):
        path = os.path.join(base_dir, self._path)
        with ExceptionSuppressor(IOError):
            with open(path, 'wb+') as f:
                f.write(self._content)
            with ExceptionSuppressor(OSError):
                os.chmod(path, self._mode)
            return True
        return False

    def _verify(self, base_dir):
        path = os.path.join(base_dir, self._path)
        with ExceptionSuppressor(IOError):
            with open(path, 'rb') as f:
                return f.read() == self._content
        return False


class DefaultFilesystemTestContent(FilesystemTestDirectory):
    def __init__(self):
        super(DefaultFilesystemTestContent, self).__init__('', [
            FilesystemTestFile('file1', '0123456789'),
            FilesystemTestDirectory('dir1', [
                FilesystemTestFile('file1', ''),
                FilesystemTestFile('file2', 'abcdefg'),
                FilesystemTestDirectory('dir2', [
                    FilesystemTestFile('file3', 'abcdefg'),
                ]),
            ]),
        ], stat.S_IRWXU|stat.S_IRGRP|stat.S_IXGRP|stat.S_IROTH|stat.S_IXOTH)


class VirtualFilesystemImage(object):
    def __init__(self, block_size, block_count, filesystem_type,
                 *args, **kwargs):
        """Initializes the instance.

        Args:
            block_size: The number of bytes of each block in the image.
            block_count: The number of blocks in the image.
            filesystem_type: The filesystem type to be given to the mkfs
                             program for formatting the image.

        Keyword Args:
            mount_filesystem_type: The filesystem type to be given to the
                                   mount program for mounting the image.
            mkfs_options: A list of options to be given to the mkfs program.
        """
        self._block_size = block_size
        self._block_count = block_count
        self._filesystem_type = filesystem_type
        self._mount_filesystem_type = kwargs.get('mount_filesystem_type')
        if self._mount_filesystem_type is None:
            self._mount_filesystem_type = filesystem_type
        self._mkfs_options = kwargs.get('mkfs_options')
        if self._mkfs_options is None:
            self._mkfs_options = []
        self._image_file = None
        self._loop_device = None
        self._mount_dir = None

    def __del__(self):
        with ExceptionSuppressor(Exception):
            self.clean()

    def __enter__(self):
        self.create()
        return self

    def __exit__(self, exc_type, exc_value, traceback):
        self.clean()
        return False

    def _remove_temp_path(self, temp_path):
        """Removes a temporary file or directory created using autotemp."""
        if temp_path:
            with ExceptionSuppressor(Exception):
                path = temp_path.name
                temp_path.clean()
                logging.debug('Removed "%s"', path)

    def _remove_image_file(self):
        """Removes the image file if one has been created."""
        self._remove_temp_path(self._image_file)
        self._image_file = None

    def _remove_mount_dir(self):
        """Removes the mount directory if one has been created."""
        self._remove_temp_path(self._mount_dir)
        self._mount_dir = None

    @property
    def image_file(self):
        """Gets the path of the image file.

        Returns:
            The path of the image file or None if no image file has been
            created.
        """
        return self._image_file.name if self._image_file else None

    @property
    def loop_device(self):
        """Gets the loop device where the image file is attached to.

        Returns:
            The path of the loop device where the image file is attached to or
            None if no loop device is attaching the image file.
        """
        return self._loop_device

    @property
    def mount_dir(self):
        """Gets the directory where the image file is mounted to.

        Returns:
            The directory where the image file is mounted to or None if no
            mount directory has been created.
        """
        return self._mount_dir.name if self._mount_dir else None

    def create(self):
        """Creates a zero-filled image file with the specified size.

        The created image file is temporary and removed when clean()
        is called.
        """
        self.clean()
        self._image_file = autotemp.tempfile(unique_id='fsImage')
        try:
            logging.debug('Creating zero-filled image file at "%s"',
                          self._image_file.name)
            utils.run('dd if=/dev/zero of=%s bs=%s count=%s' %
                      (self._image_file.name, self._block_size,
                       self._block_count))
        except error.CmdError as exc:
            self._remove_image_file()
            message = 'Failed to create filesystem image: %s' % exc
            raise RuntimeError(message)

    def clean(self):
        """Removes the image file if one has been created.

        Before removal, the image file is detached from the loop device that
        it is attached to.
        """
        self.detach_from_loop_device()
        self._remove_image_file()

    def attach_to_loop_device(self):
        """Attaches the created image file to a loop device.

        Creates the image file, if one has not been created, by calling
        create().

        Returns:
            The path of the loop device where the image file is attached to.
        """
        if self._loop_device:
            return self._loop_device

        if not self._image_file:
            self.create()

        logging.debug('Attaching image file "%s" to loop device',
                      self._image_file.name)
        utils.run('losetup -f %s' % self._image_file.name)
        output = utils.system_output('losetup -j %s' % self._image_file.name)
        # output should look like: "/dev/loop0: [000d]:6329 (/tmp/test.img)"
        self._loop_device = output.split(':')[0]
        logging.debug('Attached image file "%s" to loop device "%s"',
                      self._image_file.name, self._loop_device)
        return self._loop_device

    def detach_from_loop_device(self):
        """Detaches the image file from the loop device."""
        if not self._loop_device:
            return

        self.unmount()

        logging.debug('Cleaning up remaining mount points of loop device "%s"',
                      self._loop_device)
        utils.run('umount -f %s' % self._loop_device, ignore_status=True)

        logging.debug('Detaching image file "%s" from loop device "%s"',
                      self._image_file.name, self._loop_device)
        utils.run('losetup -d %s' % self._loop_device)
        self._loop_device = None

    def format(self):
        """Formats the image file as the specified filesystem."""
        self.attach_to_loop_device()
        try:
            logging.debug('Formatting image file at "%s" as "%s" filesystem',
                          self._image_file.name, self._filesystem_type)
            utils.run('yes | mkfs -t %s %s %s' %
                      (self._filesystem_type, ' '.join(self._mkfs_options),
                       self._loop_device))
            logging.debug('blkid: %s', utils.system_output(
                'blkid -c /dev/null %s' % self._loop_device,
                ignore_status=True))
        except error.CmdError as exc:
            message = 'Failed to format filesystem image: %s' % exc
            raise RuntimeError(message)

    def mount(self, options=None):
        """Mounts the image file to a directory.

        Args:
            options: An optional list of mount options.
        """
        if self._mount_dir:
            return self._mount_dir.name

        if options is None:
            options = []

        options_arg = ','.join(options)
        if options_arg:
            options_arg = '-o ' + options_arg

        self.attach_to_loop_device()
        self._mount_dir = autotemp.tempdir(unique_id='fsImage')
        try:
            logging.debug('Mounting image file "%s" (%s) to directory "%s"',
                          self._image_file.name, self._loop_device,
                          self._mount_dir.name)
            utils.run('mount -t %s %s %s %s' %
                      (self._mount_filesystem_type, options_arg,
                       self._loop_device, self._mount_dir.name))
        except error.CmdError as exc:
            self._remove_mount_dir()
            message = ('Failed to mount virtual filesystem image "%s": %s' %
                       (self._image_file.name, exc))
            raise RuntimeError(message)
        return self._mount_dir.name

    def unmount(self):
        """Unmounts the image file from the mounted directory."""
        if not self._mount_dir:
            return

        try:
            logging.debug('Unmounting image file "%s" (%s) from directory "%s"',
                          self._image_file.name, self._loop_device,
                          self._mount_dir.name)
            utils.run('umount %s' % self._mount_dir.name)
        except error.CmdError as exc:
            message = ('Failed to unmount virtual filesystem image "%s": %s' %
                       (self._image_file.name, exc))
            raise RuntimeError(message)
        finally:
            self._remove_mount_dir()
