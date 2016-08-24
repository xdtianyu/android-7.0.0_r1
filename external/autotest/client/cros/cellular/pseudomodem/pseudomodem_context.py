# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# This module helps launch pseudomodem as a subprocess. It helps with the
# initial setup of pseudomodem, as well as ensures proper cleanup.
# For details about the options accepted by pseudomodem, please check the
# |pseudomodem| module.
# This module also doubles as the python entry point to run pseudomodem from the
# command line. To avoid confusion, please use the shell script run_pseudomodem
# to run pseudomodem from command line.

import dbus
import json
import logging
import os
import pwd
import signal
import stat
import sys
import subprocess
import tempfile

import common
from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.cros import service_stopper
from autotest_lib.client.cros.cellular import mm1_constants
from autotest_lib.client.cros.cellular import net_interface

import pm_constants
import pseudomodem

# TODO(pprabhu) Move this to the right utils file.
# pprabhu: I haven't yet figured out which of the myriad utils files I should
# update. There is an implementation of |nuke_subprocess| that does not take
# timeout_hint_seconds in common_lib/base_utils.py, but |poll_for_condition|
# is not available there.
def nuke_subprocess(subproc, timeout_hint_seconds=0):
    """
    Attempt to kill the given subprocess via an escalating series of signals.

    Between each attempt, the process is given |timeout_hint_seconds| to clean
    up. So, the function may take up to 3 * |timeout_hint_seconds| time to
    finish.

    @param subproc: The python subprocess to nuke.
    @param timeout_hint_seconds: The time to wait between successive attempts.
    @returns: The result from the subprocess, None if we failed to kill it.

    """
    # check if the subprocess is still alive, first
    if subproc.poll() is not None:
        return subproc.poll()

    signal_queue = [signal.SIGINT, signal.SIGTERM, signal.SIGKILL]
    for sig in signal_queue:
        logging.info('Nuking %s with %s', subproc.pid, sig)
        utils.signal_pid(subproc.pid, sig)
        try:
            utils.poll_for_condition(
                    lambda: subproc.poll() is not None,
                    timeout=timeout_hint_seconds)
            return subproc.poll()
        except utils.TimeoutError:
            pass
    return None


class PseudoModemManagerContextException(Exception):
    """ Exception class for exceptions raised by PseudoModemManagerContext. """
    pass


class PseudoModemManagerContext(object):
    """
    A context to launch pseudomodem in background.

    Tests should use |PeudoModemManagerContext| to launch pseudomodem. It is
    intended to be used with the |with| clause like so:

    with PseudoModemManagerContext(...):
        # Run test

    pseudomodem will be launch in a subprocess safely when entering the |with|
    block, and cleaned up when exiting.

    """
    SHORT_TIMEOUT_SECONDS = 4
    # Some actions are dependent on hardware cooperating. We need to wait longer
    # for these. Try to minimize using this constant.
    WAIT_FOR_HARDWARE_TIMEOUT_SECONDS = 12
    TEMP_FILE_PREFIX = 'pseudomodem_'
    REAL_MANAGER_SERVICES = ['modemmanager', 'cromo']
    REAL_MANAGER_PROCESSES = ['ModemManager', 'cromo']
    TEST_OBJECT_ARG_FLAGS = ['test-modem-arg',
                             'test-sim-arg',
                             'test-state-machine-factory-arg']

    def __init__(self,
                 use_pseudomodem,
                 flags_map=None,
                 block_output=True,
                 bus=None):
        """
        @param use_pseudomodem: This flag can be used to treat pseudomodem as a
                no-op. When |True|, pseudomodem is launched as expected. When
                |False|, this operation is a no-op, and pseudomodem will not be
                launched.
        @param flags_map: This is a map of pseudomodem arguments. See
                |pseudomodem| module for the list of supported arguments. For
                example, to launch pseudomodem with a modem of family 3GPP, use:
                    with PseudoModemManager(True, flags_map={'family' : '3GPP}):
                        # Do stuff
        @param block_output: If True, output from the pseudomodem process is not
                piped to stdout. This is the default.
        @param bus: A handle to the dbus.SystemBus. If you use dbus in your
                tests, you should obtain a handle to the bus and pass it in
                here. Not doing so can cause incompatible mainloop settings in
                the dbus module.

        """
        self._use_pseudomodem = use_pseudomodem
        self._block_output = block_output

        self._temp_files = []
        self.cmd_line_flags = self._ConvertMapToFlags(flags_map if flags_map
                                                      else {})
        self._service_stopper = service_stopper.ServiceStopper(
                self.REAL_MANAGER_SERVICES)
        self._net_interface = None
        self._null_pipe = None
        self._exit_error_file_path = None
        self._pseudomodem_process = None

        self._bus = bus
        if not self._bus:
            # Currently, the glib mainloop, or a wrapper thereof are the only
            # mainloops we ever use with dbus. So, it's a comparatively safe bet
            # to set that up as the mainloop here.
            # Ideally, if a test wants to use dbus, it should pass us its own
            # bus.
            dbus_loop = dbus.mainloop.glib.DBusGMainLoop()
            self._bus = dbus.SystemBus(private=True, mainloop=dbus_loop)


    @property
    def cmd_line_flags(self):
        """ The command line flags that will be passed to pseudomodem. """
        return self._cmd_line_flags


    @cmd_line_flags.setter
    def cmd_line_flags(self, val):
        """
        Set the command line flags to be passed to pseudomodem.

        @param val: The flags.

        """
        logging.info('Command line flags for pseudomodem set to: |%s|', val)
        self._cmd_line_flags = val


    def __enter__(self):
        return self.Start()


    def __exit__(self, *args):
        return self.Stop(*args)


    def Start(self):
        """ Start the context. This launches pseudomodem. """
        if not self._use_pseudomodem:
            return self

        self._CheckPseudoModemArguments()

        self._service_stopper.stop_services()
        self._WaitForRealModemManagersToDie()

        self._net_interface = net_interface.PseudoNetInterface()
        self._net_interface.Setup()

        toplevel = os.path.dirname(os.path.realpath(__file__))
        cmd = [os.path.join(toplevel, 'pseudomodem.py')]
        cmd = cmd + self.cmd_line_flags

        fd, self._exit_error_file_path = self._CreateTempFile()
        os.close(fd)  # We don't need the fd.
        cmd = cmd + [pseudomodem.EXIT_ERROR_FILE_FLAG,
                     self._exit_error_file_path]

        # Setup health checker for child process.
        signal.signal(signal.SIGCHLD, self._SigchldHandler)

        if self._block_output:
            self._null_pipe = open(os.devnull, 'w')
            self._pseudomodem_process = subprocess.Popen(
                    cmd,
                    preexec_fn=PseudoModemManagerContext._SetUserModem,
                    close_fds=True,
                    stdout=self._null_pipe,
                    stderr=self._null_pipe)
        else:
            self._pseudomodem_process = subprocess.Popen(
                    cmd,
                    preexec_fn=PseudoModemManagerContext._SetUserModem,
                    close_fds=True)
        self._EnsurePseudoModemUp()
        return self


    def Stop(self, *args):
        """ Exit the context. This terminates pseudomodem. """
        if not self._use_pseudomodem:
            return

        # Remove health check on child process.
        signal.signal(signal.SIGCHLD, signal.SIG_DFL)

        if self._pseudomodem_process:
            if self._pseudomodem_process.poll() is None:
                if (nuke_subprocess(self._pseudomodem_process,
                                    self.SHORT_TIMEOUT_SECONDS) is
                    None):
                    logging.warning('Failed to clean up the launched '
                                    'pseudomodem process')
            self._pseudomodem_process = None

        if self._null_pipe:
            self._null_pipe.close()
            self._null_pipe = None

        if self._net_interface:
            self._net_interface.Teardown()
            self._net_interface = None

        self._DeleteTempFiles()
        self._service_stopper.restore_services()


    def _ConvertMapToFlags(self, flags_map):
        """
        Convert the argument map given to the context to flags for pseudomodem.

        @param flags_map: A map of flags. The keys are the names of the flags
                accepted by pseudomodem. The value, if not None, is the value
                for that flag. We do not support |None| as the value for a flag.
        @returns: the list of flags to pass to pseudomodem.

        """
        cmd_line_flags = []
        for key, value in flags_map.iteritems():
            cmd_line_flags.append('--' + key)
            if key in self.TEST_OBJECT_ARG_FLAGS:
                cmd_line_flags.append(self._DumpArgToFile(value))
            elif value:
                cmd_line_flags.append(value)
        return cmd_line_flags


    def _DumpArgToFile(self, arg):
        """
        Dump a given python list to a temp file in json format.

        This is used to pass arguments to custom objects from tests that
        are to be instantiated by pseudomodem. The argument must be a list. When
        running pseudomodem, this list will be unpacked to get the arguments.

        @returns: Absolute path to the tempfile created.

        """
        fd, arg_file_path = self._CreateTempFile()
        arg_file = os.fdopen(fd, 'wb')
        json.dump(arg, arg_file)
        arg_file.close()
        return arg_file_path


    def _WaitForRealModemManagersToDie(self):
        """
        Wait for real modem managers to quit. Die otherwise.

        Sometimes service stopper does not kill ModemManager process, if it is
        launched by something other than upstart. We want to ensure that the
        process is dead before continuing.

        This method can block for up to a minute. Sometimes, ModemManager can
        take up to a 10 seconds to die after service stopper has stopped it. We
        wait for it to clean up before concluding that the process is here to
        stay.

        @raises: PseudoModemManagerContextException if a modem manager process
                does not quit in a reasonable amount of time.
        """
        def _IsProcessRunning(process):
            try:
                utils.run('pgrep -x %s' % process)
                return True
            except error.CmdError:
                return False

        for manager in self.REAL_MANAGER_PROCESSES:
            try:
                utils.poll_for_condition(
                        lambda:not _IsProcessRunning(manager),
                        timeout=self.WAIT_FOR_HARDWARE_TIMEOUT_SECONDS)
            except utils.TimeoutError:
                err_msg = ('%s is still running. '
                           'It may interfere with pseudomodem.' %
                           manager)
                logging.error(err_msg)
                raise PseudoModemManagerContextException(err_msg)


    def _CheckPseudoModemArguments(self):
        """
        Parse the given pseudomodem arguments.

        By parsing the arguments in the context, we can provide early feedback
        about incorrect arguments.

        """
        pseudomodem.ParseArguments(self.cmd_line_flags)


    @staticmethod
    def _SetUserModem():
        """
        Set the unix user of the calling process to |modem|.

        This functions is called by the launched subprocess so that pseudomodem
        can be launched as the |modem| user.
        On encountering an error, this method will terminate the process.

        """
        try:
            pwd_data = pwd.getpwnam(pm_constants.MM1_USER)
        except KeyError as e:
            logging.error('Could not find uid for user %s [%s]',
                          pm_constants.MM1_USER, str(e))
            sys.exit(1)

        logging.debug('Setting UID to %d', pwd_data.pw_uid)
        try:
            os.setuid(pwd_data.pw_uid)
        except OSError as e:
            logging.error('Could not set uid to %d [%s]',
                          pwd_data.pw_uid, str(e))
            sys.exit(1)


    def _EnsurePseudoModemUp(self):
        """ Makes sure that pseudomodem in child process is ready. """
        def _LivenessCheck():
            try:
                testing_object = self._bus.get_object(
                        mm1_constants.I_MODEM_MANAGER,
                        pm_constants.TESTING_PATH)
                return testing_object.IsAlive(
                        dbus_interface=pm_constants.I_TESTING)
            except dbus.DBusException as e:
                logging.debug('LivenessCheck: No luck yet. (%s)', str(e))
                return False

        utils.poll_for_condition(
                _LivenessCheck,
                timeout=self.SHORT_TIMEOUT_SECONDS,
                exception=PseudoModemManagerContextException(
                        'pseudomodem did not initialize properly.'))


    def _CreateTempFile(self):
        """
        Creates a tempfile such that the child process can read/write it.

        The file path is stored in a list so that the file can be deleted later
        using |_DeleteTempFiles|.

        @returns: (fd, arg_file_path)
                 fd: A file descriptor for the created file.
                 arg_file_path: Full path of the created file.

        """
        fd, arg_file_path = tempfile.mkstemp(prefix=self.TEMP_FILE_PREFIX)
        self._temp_files.append(arg_file_path)
        # Set file permissions so that pseudomodem process can read/write it.
        cur_mod = os.stat(arg_file_path).st_mode
        os.chmod(arg_file_path,
                 cur_mod | stat.S_IRGRP | stat.S_IROTH | stat.S_IWGRP |
                 stat.S_IWOTH)
        return fd, arg_file_path


    def _DeleteTempFiles(self):
        """ Deletes all temp files created by this context. """
        for file_path in self._temp_files:
            try:
                os.remove(file_path)
            except OSError as e:
                logging.warning('Failed to delete temp file: %s (error %s)',
                                file_path, str(e))


    def _SigchldHandler(self, signum, frame):
        """
        Signal handler for SIGCHLD.

        This is setup while the pseudomodem subprocess is running. A call to
        this signal handler may signify early termination of the subprocess.

        @param signum: The signal number.
        @param frame: Ignored.

        """
        if not self._pseudomodem_process:
            # We can receive a SIGCHLD even before the setup of the child
            # process is complete.
            return
        if self._pseudomodem_process.poll() is not None:
            # See if child process left detailed error report
            error_reason, error_traceback = pseudomodem.ExtractExitError(
                    self._exit_error_file_path)
            logging.error('pseudomodem child process quit early!')
            logging.error('Reason: %s', error_reason)
            for line in error_traceback:
                logging.error('Traceback: %s', line.strip())
            raise PseudoModemManagerContextException(
                    'pseudomodem quit early! (%s)' %
                    error_reason)
