# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import os

import common
from autotest_lib.client.common_lib import error

"""
Functions to query and control debugd dev tools.

This file provides a set of functions to check the general state of the
debugd dev tools, and a set of classes to interface to the individual
tools.

Current tool classes are:
    RootfsVerificationTool
    BootFromUsbTool
    SshServerTool
    SystemPasswordTool
These classes have functions to check the state and enable/disable the
tool. Some tools may not be able to disable themselves, in which case
an exception will be thrown (for example, RootfsVerificationTool cannot
be disabled).

General usage will look something like this:

# Make sure tools are accessible on the system.
if debugd_dev_tools.are_dev_tools_available(host):
    # Create the tool(s) you want to interact with.
    tools = [debugd_dev_tools.SshServerTool(), ...]
    for tool in tools:
        # Initialize tools and save current state.
        tool.initialize(host, save_initial_state=True)
        # Perform required action with tools.
        tool.enable()
        # Restore initial tool state.
        tool.restore_state()
    # Clean up temporary files.
    debugd_dev_tools.remove_temp_files()
"""


# Defined in system_api/dbus/service_constants.h.
DEV_FEATURES_DISABLED = 1 << 0
DEV_FEATURE_ROOTFS_VERIFICATION_REMOVED = 1 << 1
DEV_FEATURE_BOOT_FROM_USB_ENABLED = 1 << 2
DEV_FEATURE_SSH_SERVER_CONFIGURED = 1 << 3
DEV_FEATURE_DEV_MODE_ROOT_PASSWORD_SET = 1 << 4
DEV_FEATURE_SYSTEM_ROOT_PASSWORD_SET = 1 << 5


# Location to save temporary files to store and load state. This folder should
# be persistent through a power cycle so we can't use /tmp.
_TEMP_DIR = '/usr/local/autotest/tmp/debugd_dev_tools'


class AccessError(error.CmdError):
    """Raised when debugd D-Bus access fails."""
    pass


class FeatureUnavailableError(error.TestNAError):
    """Raised when a feature cannot be enabled or disabled."""
    pass


def query_dev_tools_state(host):
    """
    Queries debugd for the current dev features state.

    @param host: Host device.

    @return: Integer debugd query return value.

    @raise AccessError: Can't talk to debugd on the host.
    """
    result = _send_debugd_command(host, 'QueryDevFeatures')
    state = int(result.stdout)
    logging.debug('query_dev_tools_state = %d (0x%04X)', state, state)
    return state


def are_dev_tools_available(host):
    """
    Check if dev tools are available on the host.

    @param host: Host device.

    @return: True if tools are available, False otherwise.
    """
    try:
        return query_dev_tools_state(host) != DEV_FEATURES_DISABLED
    except AccessError:
        return False


def remove_temp_files(host):
    """
    Removes all DevTools temporary files and directories.

    Any test using dev tools should try to call this just before
    exiting to erase any temporary files that may have been saved.

    @param host: Host device.
    """
    host.run('rm -rf "%s"' % _TEMP_DIR)


def expect_access_failure(host, tools):
    """
    Verifies that access is denied to all provided tools.

    Will check are_dev_tools_available() first to try to avoid changing
    device state in case access is allowed. Otherwise, the function
    will try to enable each tool in the list and throw an exception if
    any succeeds.

    @param host: Host device.
    @param tools: List of tools to checks.

    @raise TestFail: are_dev_tools_available() returned True or
                     a tool successfully enabled.
    """
    if are_dev_tools_available(host):
        raise error.TestFail('Unexpected dev tool access success')
    for tool in tools:
        try:
            tool.enable()
        except AccessError:
            # We want an exception, otherwise the tool succeeded.
            pass
        else:
            raise error.TestFail('Unexpected %s enable success.' % tool)


def _send_debugd_command(host, name, args=()):
    """
    Sends a debugd command.

    @param host: Host to run the command on.
    @param name: String debugd D-Bus function name.
    @param args: List of string arguments to pass to dbus-send.

    @return: The dbus-send CmdResult object.

    @raise AccessError: debugd call returned an error.
    """
    command = ('dbus-send --system --fixed --print-reply '
               '--dest=org.chromium.debugd /org/chromium/debugd '
               '"org.chromium.debugd.%s"' % name)
    for arg in args:
        command += ' %s' % arg
    try:
        return host.run(command)
    except error.CmdError as e:
        raise AccessError(e.command, e.result_obj, e.additional_text)


class DevTool(object):
    """
    Parent tool class.

    Each dev tool has its own child class that handles the details
    of disabling, enabling, and querying the functionality. This class
    provides some common functionality needed by multiple tools.

    Child classes should implement the following:
      - is_enabled(): use debugd to query whether the tool is enabled.
      - enable(): use debugd to enable the tool.
      - disable(): manually disable the tool.
      - save_state(): record the current tool state on the host.
      - restore_state(): restore the saved tool state.

    If a child class cannot perform the required action (for
    example the rootfs tool can't currently restore its initial
    state), leave the function unimplemented so it will throw an
    exception if a test attempts to use it.
    """


    def initialize(self, host, save_initial_state=False):
        """
        Sets up the initial tool state. This must be called on
        every tool before use.

        @param host: Device host the test is running on.
        @param save_initial_state: True to save the device state.
        """
        self._host = host
        if save_initial_state:
            self.save_state()


    def is_enabled(self):
        """
        Each tool should override this to query itself using debugd.
        Normally this can be done by using the provided
        _check_enabled() function.
        """
        self._unimplemented_function_error('is_enabled')


    def enable(self):
        """
        Each tool should override this to enable itself using debugd.
        """
        self._unimplemented_function_error('enable')


    def disable(self):
        """
        Each tool should override this to disable itself.
        """
        self._unimplemented_function_error('disable')


    def save_state(self):
        """
        Save the initial tool state. Should be overridden by child
        tool classes.
        """
        self._unimplemented_function_error('_save_state')


    def restore_state(self):
        """
        Restore the initial tool state. Should be overridden by child
        tool classes.
        """
        self._unimplemented_function_error('_restore_state')


    def _check_enabled(self, bits):
        """
        Checks if the given feature is currently enabled according to
        the debugd status query function.

        @param bits: Integer status bits corresponding to the features.

        @return: True if the status query is enabled and the
                 indicated bits are all set, False otherwise.
        """
        state = query_dev_tools_state(self._host)
        enabled = bool((state != DEV_FEATURES_DISABLED) and
                       (state & bits == bits))
        logging.debug('%s _check_enabled = %s (0x%04X / 0x%04X)',
                      self, enabled, state, bits)
        return enabled


    def _get_temp_path(self, source_path):
        """
        Get temporary storage path for a file or directory.

        Temporary path is based on the tool class name and the
        source directory to keep tool files isolated and prevent
        name conflicts within tools.

        The function returns a full temporary path corresponding to
        |source_path|.

        For example, _get_temp_path('/foo/bar.txt') would return
        '/path/to/temp/folder/debugd_dev_tools/FooTool/foo/bar.txt'.

        @param source_path: String path to the file or directory.

        @return: Temp path string.
        """
        return '%s/%s/%s' % (_TEMP_DIR, self, source_path)


    def _save_files(self, paths):
        """
        Saves a set of files to a temporary location.

        This can be used to save specific files so that a tool can
        save its current state before starting a test.

        See _restore_files() for restoring the saved files.

        @param paths: List of string paths to save.
        """
        for path in paths:
            temp_path = self._get_temp_path(path)
            self._host.run('mkdir -p "%s"' % os.path.dirname(temp_path))
            self._host.run('cp -r "%s" "%s"' % (path, temp_path),
                           ignore_status=True)


    def _restore_files(self, paths):
        """
        Restores saved files to their original location.

        Used to restore files that have previously been saved by
        _save_files(), usually to return the device to its initial
        state.

        This function does not erase the saved files, so it can
        be used multiple times if needed.

        @param paths: List of string paths to restore.
        """
        for path in paths:
            self._host.run('rm -rf "%s"' % path)
            self._host.run('cp -r "%s" "%s"' % (self._get_temp_path(path),
                                                path),
                           ignore_status=True)


    def _unimplemented_function_error(self, function_name):
        """
        Throws an exception if a required tool function hasn't been
        implemented.
        """
        raise FeatureUnavailableError('%s has not implemented %s()' %
                                      (self, function_name))


    def __str__(self):
        """
        Tool name accessor for temporary files and logging.

        Based on class rather than unique instance naming since all
        instances of the same tool have identical functionality.
        """
        return type(self).__name__


class RootfsVerificationTool(DevTool):
    """
    Rootfs verification removal tool.

    This tool is currently unable to transition from non-verified back
    to verified rootfs; it may potentially require re-flashing an OS.
    Since devices in the test lab run in verified mode, this tool is
    unsuitable for automated testing until this capability is
    implemented.
    """


    def is_enabled(self):
        return self._check_enabled(DEV_FEATURE_ROOTFS_VERIFICATION_REMOVED)


    def enable(self):
        _send_debugd_command(self._host, 'RemoveRootfsVerification')
        self._host.reboot()


    def disable(self):
        raise FeatureUnavailableError('Cannot re-enable rootfs verification')


class BootFromUsbTool(DevTool):
    """
    USB boot configuration tool.

    Certain boards have restrictions with USB booting. Mario can't
    boot from USB at all, and Alex/ZGB can't disable USB booting
    once it's been enabled. Any attempts to perform these operation
    will raise a FeatureUnavailableError exception.
    """


    # Lists of which platforms can't enable or disable USB booting.
    ENABLE_UNAVAILABLE_PLATFORMS = ('mario',)
    DISABLE_UNAVAILABLE_PLATFORMS = ('mario', 'alex', 'zgb')


    def is_enabled(self):
        return self._check_enabled(DEV_FEATURE_BOOT_FROM_USB_ENABLED)


    def enable(self):
        platform = self._host.get_platform().lower()
        if any(p in platform for p in self.ENABLE_UNAVAILABLE_PLATFORMS):
            raise FeatureUnavailableError('USB boot unavilable on %s' %
                                          platform)
        _send_debugd_command(self._host, 'EnableBootFromUsb')


    def disable(self):
        platform = self._host.get_platform().lower()
        if any(p in platform for p in self.DISABLE_UNAVAILABLE_PLATFORMS):
            raise FeatureUnavailableError("Can't disable USB boot on %s" %
                                          platform)
        self._host.run('crossystem dev_boot_usb=0')


    def save_state(self):
        self.initial_state = self.is_enabled()


    def restore_state(self):
        if self.initial_state:
            self.enable()
        else:
            self.disable()


class SshServerTool(DevTool):
    """
    SSH server tool.

    SSH configuration has two components, the init file and the test
    keys. Since a system could potentially have none, just the init
    file, or all files, we want to be sure to restore just the files
    that existed before the test started.
    """


    PATHS = ('/etc/init/openssh-server.conf',
             '/root/.ssh/authorized_keys',
             '/root/.ssh/id_rsa',
             '/root/.ssh/id_rsa.pub')


    def is_enabled(self):
        return self._check_enabled(DEV_FEATURE_SSH_SERVER_CONFIGURED)


    def enable(self):
        _send_debugd_command(self._host, 'ConfigureSshServer')


    def disable(self):
        for path in self.PATHS:
            self._host.run('rm -f %s' % path)


    def save_state(self):
        self._save_files(self.PATHS)


    def restore_state(self):
        self._restore_files(self.PATHS)


class SystemPasswordTool(DevTool):
    """
    System password configuration tool.

    This tool just affects the system password (/etc/shadow). We could
    add a devmode password tool if we want to explicitly test that as
    well.
    """


    SYSTEM_PATHS = ('/etc/shadow',)
    DEV_PATHS = ('/mnt/stateful_partition/etc/devmode.passwd',)


    def is_enabled(self):
        return self._check_enabled(DEV_FEATURE_SYSTEM_ROOT_PASSWORD_SET)


    def enable(self):
        # Save the devmode.passwd file to avoid affecting it.
        self._save_files(self.DEV_PATHS)
        try:
            _send_debugd_command(self._host, 'SetUserPassword',
                                 ('string:root', 'string:test0000'))
        finally:
            # Restore devmode.passwd
            self._restore_files(self.DEV_PATHS)


    def disable(self):
        self._host.run('passwd -d root')


    def save_state(self):
        self._save_files(self.SYSTEM_PATHS)


    def restore_state(self):
        self._restore_files(self.SYSTEM_PATHS)
