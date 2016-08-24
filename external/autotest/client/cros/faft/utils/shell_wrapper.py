# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""A module to abstract the shell execution environment on DUT."""

import subprocess
import tempfile


class ShellError(Exception):
    """Shell specific exception."""
    pass


class LocalShell(object):
    """An object to wrap the local shell environment."""

    def init(self, os_if):
        self._os_if = os_if

    def _run_command(self, cmd, block=True):
        """Helper function of run_command() methods.

        Return the subprocess.Popen() instance to provide access to console
        output in case command succeeded.  If block=False, will not wait for
        process to return before returning.
        """
        self._os_if.log('Executing %s' % cmd)
        process = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE,
                             stderr=subprocess.PIPE)
        if block:
            process.wait()
        return process

    def run_command(self, cmd, block=True):
        """Run a shell command.

        In case of the command returning an error print its stdout and stderr
        outputs on the console and dump them into the log. Otherwise suppress
        all output.

        In case of command error raise an ShellError exception.
        """
        process = self._run_command(cmd, block)
        if process.returncode:
            err = ['Failed running: %s' % cmd]
            err.append('stdout:')
            err.append(process.stdout.read())
            err.append('stderr:')
            err.append(process.stderr.read())
            text = '\n'.join(err)
            self._os_if.log(text)
            raise ShellError('command %s failed (code: %d)' %
                             (cmd, process.returncode))

    def run_command_get_status(self, cmd):
        """Run a shell command and return its return code.

        The return code of the command is returned, in case of any error.
        """
        process = self._run_command(cmd)
        return process.returncode

    def run_command_get_output(self, cmd):
        """Run shell command and return its console output to the caller.

        The output is returned as a list of strings stripped of the newline
        characters.
        """
        process = self._run_command(cmd)
        return [x.rstrip() for x in process.stdout.readlines()]

    def read_file(self, path):
        """Read the content of the file."""
        with open(path) as f:
            return f.read()

    def write_file(self, path, data):
        """Write the data to the file."""
        with open(path, 'w') as f:
            f.write(data)

    def append_file(self, path, data):
        """Append the data to the file."""
        with open(path, 'a') as f:
            f.write(data)


class AdbShell(object):
    """An object to wrap the ADB shell environment.

    DUT is connected to the host in a 1:1 basis. The command is executed
    via "adb shell".
    """

    def init(self, os_if):
        self._os_if = os_if
        self._host_shell = LocalShell()
        self._host_shell.init(os_if)
        self._root_granted = False

    def _run_command(self, cmd):
        """Helper function of run_command() methods.

        Return the subprocess.Popen() instance to provide access to console
        output in case command succeeded.
        """
        if not self._root_granted:
            if (self._host_shell.run_command_get_output('adb shell whoami')[0]
                != 'root'):
                # Get the root access first as some commands need it.
                self._host_shell.run_command('adb root')
            self._root_granted = True
        cmd = "adb shell '%s'" % cmd.replace("'", "\\'")
        return self._host_shell._run_command(cmd)

    def run_command(self, cmd):
        """Run a shell command.

        In case of the command returning an error print its stdout and stderr
        outputs on the console and dump them into the log. Otherwise suppress
        all output.

        In case of command error raise an ShellError exception.
        """
        process = self._run_command(cmd)
        if process.returncode:
            err = ['Failed running: %s' % cmd]
            err.append('stdout:')
            err.append(process.stdout.read())
            err.append('stderr:')
            err.append(process.stderr.read())
            text = '\n'.join(err)
            self._os_if.log(text)
            raise ShellError('command %s failed (code: %d)' %
                             (cmd, process.returncode))

    def run_command_get_status(self, cmd):
        """Run a shell command and return its return code.

        The return code of the command is returned, in case of any error.
        """
        # Executing command via adb shell always returns 0.
        cmd = '(%s); echo $?' % cmd
        lines = self.run_command_get_output(cmd)
        if len(lines) == 0:
            raise ShellError('Somthing wrong on getting status: %r' % lines)
        return int(lines[-1])

    def run_command_get_output(self, cmd):
        """Run shell command and return its console output to the caller.

        The output is returned as a list of strings stripped of the newline
        characters.
        """
        # stderr is merged into stdout through adb shell.
        cmd = '(%s) 2>/dev/null' % cmd
        process = self._run_command(cmd)
        return [x.rstrip() for x in process.stdout.readlines()]

    def read_file(self, path):
        """Read the content of the file."""
        with tempfile.NamedTemporaryFile() as f:
            cmd = 'adb pull %s %s' % (path, f.name)
            self._host_shell.run_command(cmd)
            return self._host_shell.read_file(f.name)

    def write_file(self, path, data):
        """Write the data to the file."""
        with tempfile.NamedTemporaryFile() as f:
            self._host_shell.write_file(f.name, data)
            cmd = 'adb push %s %s' % (f.name, path)
            self._host_shell.run_command(cmd)

    def append_file(self, path, data):
        """Append the data to the file."""
        with tempfile.NamedTemporaryFile() as f:
            cmd = 'adb pull %s %s' % (path, f.name)
            self._host_shell.run_command(cmd)
            self._host_shell.append_file(f.name, data)
            cmd = 'adb push %s %s' % (f.name, path)
            self._host_shell.run_command(cmd)

    def wait_for_device(self, timeout):
        """Wait for an Android device connected."""
        cmd = 'timeout %s adb wait-for-device' % timeout
        return self._host_shell.run_command_get_status(cmd) == 0

    def wait_for_no_device(self, timeout):
        """Wait for no Android connected (offline)."""
        cmd = ('for i in $(seq 0 %d); do adb shell sleep 1 || false; done' %
               timeout)
        return self._host_shell.run_command_get_status(cmd) != 0
