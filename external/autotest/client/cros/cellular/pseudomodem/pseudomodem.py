#!/usr/bin/env python
# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# This module is the entry point for pseudomodem. Though honestly, I can't think
# of any case when you want to use this module directly. Instead, use the
# |pseudomodem_context| module that provides a way to launch pseudomodem in a
# child process.

import argparse
import dbus
import dbus.mainloop.glib
import gobject
import imp
import json
import logging
import os
import os.path
import signal
import sys
import testing
import traceback

import logging_setup
import modem_cdma
import modem_3gpp
import modemmanager
import sim
import state_machine_factory as smf

import common
from autotest_lib.client.cros.cellular import mm1_constants

# Flags used by pseudomodem modules only that are defined below in
# ParserArguments.
CLI_FLAG = '--cli'
EXIT_ERROR_FILE_FLAG = '--exit-error-file'

class PseudoModemManager(object):
    """
    The main class to be used to launch the pseudomodem.

    There should be only one instance of this class that orchestrates
    pseudomodem.

    """

    def Setup(self, opts):
        """
        Call |Setup| to prepare pseudomodem to be launched.

        @param opts: The options accepted by pseudomodem. See top level function
                |ParseArguments| for details.

        """
        self._opts = opts

        self._in_exit_sequence = False
        self._manager = None
        self._modem = None
        self._state_machine_factory = None
        self._sim = None
        self._mainloop = None

        self._dbus_loop = dbus.mainloop.glib.DBusGMainLoop()
        self._bus = dbus.SystemBus(private=True, mainloop=self._dbus_loop)
        self._bus_name = dbus.service.BusName(mm1_constants.I_MODEM_MANAGER,
                                              self._bus)
        logging.info('Exported dbus service with well known name: |%s|',
                     self._bus_name.get_name())

        self._SetupPseudomodemParts()
        logging.info('Pseudomodem setup completed!')


    def StartBlocking(self):
        """
        Start pseudomodem operation.

        This call blocks untill |GracefulExit| is called from some other
        context.

        """
        self._mainloop = gobject.MainLoop()
        self._mainloop.run()


    def GracefulExit(self):
        """ Stop pseudomodem operation and clean up. """
        if self._in_exit_sequence:
            logging.debug('Already exiting.')
            return

        self._in_exit_sequence = True
        logging.info('pseudomodem shutdown sequence initiated...')
        # Guard each step by its own try...catch, because we want to attempt
        # each step irrespective of whether the earlier ones succeeded.
        try:
            if self._manager:
                self._manager.Remove(self._modem)
        except Exception as e:
            logging.warning('Error while exiting: %s', repr(e))
        try:
            if self._mainloop:
                self._mainloop.quit()
        except Exception as e:
            logging.warning('Error while exiting: %s', repr(e))

        logging.info('pseudomodem: Bye! Bye!')


    def _SetupPseudomodemParts(self):
        """
        Contructs all pseudomodem objects, but does not start operation.

        Three main objects are created: the |Modem|, the |Sim|, and the
        |StateMachineFactory|. This objects may be instantiations of the default
        classes, or of user provided classes, depending on options provided.

        """
        self._ReadCustomParts()

        use_3gpp = (self._opts.family == '3GPP')

        if not self._modem and not self._state_machine_factory:
            self._state_machine_factory = smf.StateMachineFactory()
            logging.info('Created default state machine factory.')

        if use_3gpp and not self._sim:
            self._sim = sim.SIM(sim.SIM.Carrier('test'),
                                mm1_constants.MM_MODEM_ACCESS_TECHNOLOGY_GSM,
                                locked=self._opts.locked)
            logging.info('Created default 3GPP SIM.')

        # Store this constant here because the variable name is too long.
        network_available = dbus.types.UInt32(
                mm1_constants.MM_MODEM_3GPP_NETWORK_AVAILABILITY_AVAILABLE)
        if not self._modem:
            if use_3gpp:
                technology_gsm = dbus.types.UInt32(
                        mm1_constants.MM_MODEM_ACCESS_TECHNOLOGY_GSM)
                networks = [modem_3gpp.Modem3gpp.GsmNetwork(
                        'Roaming Network Long ' + str(i),
                        'Roaming Network Short ' + str(i),
                        '00100' + str(i + 1),
                        network_available,
                        technology_gsm)
                        for i in xrange(self._opts.roaming_networks)]
                # TODO(armansito): Support "not activated" initialization option
                # for 3GPP carriers.
                self._modem = modem_3gpp.Modem3gpp(
                        self._state_machine_factory,
                        roaming_networks=networks)
                logging.info('Created default 3GPP modem.')
            else:
                self._modem = modem_cdma.ModemCdma(
                        self._state_machine_factory,
                        modem_cdma.ModemCdma.CdmaNetwork(
                                activated=self._opts.activated))
                logging.info('Created default CDMA modem.')

        # Everyone gets the |_bus|, woohoo!
        self._manager = modemmanager.ModemManager(self._bus)
        self._modem.SetBus(self._bus)  # Also sets it on StateMachineFactory.
        self._manager.Add(self._modem)

        # Unfortunately, setting the SIM has to be deferred until everyone has
        # their BUS set. |self._sim| exists if the user provided one, or if the
        # modem family is |3GPP|.
        if self._sim:
            self._modem.SetSIM(self._sim)

        # The testing interface can be brought up now that we have the bus.
        self._testing_object = testing.Testing(self._modem, self._bus)


    def _ReadCustomParts(self):
        """
        Loads user provided implementations of pseudomodem objects.

        The user can provide their own implementations of the |Modem|, |Sim| or
        |StateMachineFactory| classes.

        """
        if not self._opts.test_module:
            return

        test_module = self._LoadCustomPartsModule(self._opts.test_module)

        if self._opts.test_modem_class:
            self._modem = self._CreateCustomObject(test_module,
                                                   self._opts.test_modem_class,
                                                   self._opts.test_modem_arg)

        if self._opts.test_sim_class:
            self._sim = self._CreateCustomObject(test_module,
                                                 self._opts.test_sim_class,
                                                 self._opts.test_sim_arg)

        if self._opts.test_state_machine_factory_class:
            if self._opts.test_modem_class:
                logging.warning(
                        'User provided a |Modem| implementation as well as a '
                        '|StateMachineFactory|. Ignoring the latter.')
            else:
                self._state_machine_factory = self._CreateCustomObject(
                        test_module,
                        self._opts.test_state_machine_factory_class,
                        self._opts.test_state_machine_factory_arg)


    def _CreateCustomObject(self, test_module, class_name, arg_file_name):
        """
        Create the custom object specified by test.

        @param test_module: The loaded module that implemets the custom object.
        @param class_name: Name of the class implementing the custom object.
        @param arg_file_name: Absolute path to file containing list of arguments
                taken by |test_module|.|class_name| constructor in json.
        @returns: A brand new object of the custom type.
        @raises: AttributeError if the class definition is not found;
                ValueError if |arg_file| does not contain valid json
                representaiton of a python list.
                Other errors may be raised during object creation.

        """
        arg = None
        if arg_file_name:
            arg_file = open(arg_file_name, 'rb')
            try:
                arg = json.load(arg_file)
            finally:
                arg_file.close()
            if not isinstance(arg, list):
                raise ValueError('Argument must be a python list.')

        class_def = getattr(test_module, class_name)
        try:
            if arg:
                logging.debug('Loading test class %s%s',
                              class_name, str(arg))
                return class_def(*arg)
            else:
                logging.debug('Loading test class %s', class_def)
                return class_def()
        except Exception as e:
            logging.error('Exception raised when instantiating class %s: %s',
                          class_name, str(e))
            raise


    def _LoadCustomPartsModule(self, module_abs_path):
        """
        Loads the given file as a python module.

        The loaded module *is* added to |sys.modules|.

        @param module_abs_path: Absolute path to the file to be loaded.
        @returns: The loaded module.
        @raises: ImportError if the module can not be loaded, or if another
                 module with the same name is already loaded.

        """
        path, name = os.path.split(module_abs_path)
        name, _ = os.path.splitext(name)

        if name in sys.modules:
            raise ImportError('A module named |%s| is already loaded.' %
                              name)

        logging.debug('Loading module %s from %s', name, path)
        module_file, filepath, data = imp.find_module(name, [path])
        try:
            module = imp.load_module(name, module_file, filepath, data)
        except Exception as e:
            logging.error(
                    'Exception raised when loading test module from %s: %s',
                    module_abs_path, str(e))
            raise
        finally:
            module_file.close()
        return module


# ##############################################################################
# Public static functions.
def ParseArguments(arg_string=None):
    """
    The main argument parser.

    Pseudomodem is a command line tool.
    Since pseudomodem is a highly customizable tool, the command line arguments
    are expected to be quite complex.
    We use argparse to keep the command line options easy to use.

    @param arg_string: If not None, the string to parse. If none, |sys.argv| is
            used to obtain the argument string.
    @returns: The parsed options object.

    """
    parser = argparse.ArgumentParser(
            description="Run pseudomodem to simulate a modem using the "
                        "modemmanager-next DBus interface.")

    parser.add_argument(
            CLI_FLAG,
            action='store_true',
            default=False,
            help='Launch the command line interface in foreground to interact '
                 'with the launched pseudomodem process. This argument is used '
                 'by |pseudomodem_context|. pseudomodem itself ignores it.')
    parser.add_argument(
            EXIT_ERROR_FILE_FLAG,
            default=None,
            help='If provided, full path to file to which pseudomodem should '
                 'dump the error condition before exiting, in case of a crash. '
                 'The file is not created if it does not already exist.')

    modem_arguments = parser.add_argument_group(
            title='Modem options',
            description='Options to customize the modem exported.')
    modem_arguments.add_argument(
            '--family', '-f',
            choices=['3GPP', 'CDMA'],
            default='3GPP')

    gsm_arguments = parser.add_argument_group(
            title='3GPP options',
            description='Options specific to 3GPP modems. [Only make sense '
                        'when modem family is 3GPP]')

    gsm_arguments.add_argument(
            '--roaming-networks', '-r',
            type=_NonNegInt,
            default=0,
            metavar='<# networks>',
            help='Number of roaming networks available')

    cdma_arguments = parser.add_argument_group(
            title='CDMA options',
            description='Options specific to CDMA modems. [Only make sense '
                        'when modem family is CDMA]')

    sim_arguments = parser.add_argument_group(
            title='SIM options',
            description='Options to customize the SIM in the modem. [Only make '
                        'sense when modem family is 3GPP]')
    sim_arguments.add_argument(
            '--activated',
            type=bool,
            default=True,
            help='Determine whether the SIM is activated')
    sim_arguments.add_argument(
            '--locked', '-l',
            type=bool,
            default=False,
            help='Determine whether the SIM is in locked state')

    testing_arguments = parser.add_argument_group(
            title='Testing interface options',
            description='Options to modify how the tests or user interacts '
                        'with pseudomodem')
    testing_arguments = parser.add_argument(
            '--interactive-state-machines-all',
            type=bool,
            default=False,
            help='Launch all state machines in interactive mode.')
    testing_arguments = parser.add_argument(
            '--interactive-state-machine',
            type=str,
            default=None,
            help='Launch the specified state machine in interactive mode. May '
                 'be repeated to specify multiple machines.')

    customize_arguments = parser.add_argument_group(
            title='Customizable modem options',
            description='Options to customize the emulated modem.')
    customize_arguments.add_argument(
            '--test-module',
            type=str,
            default=None,
            metavar='CUSTOM_MODULE',
            help='Absolute path to the module with custom definitions.')
    customize_arguments.add_argument(
            '--test-modem-class',
            type=str,
            default=None,
            metavar='MODEM_CLASS',
            help='Name of the class in CUSTOM_MODULE that implements the modem '
                 'to load.')
    customize_arguments.add_argument(
            '--test-modem-arg',
            type=str,
            default=None,
            help='Absolute path to the json description of argument list '
                 'taken by MODEM_CLASS.')
    customize_arguments.add_argument(
            '--test-sim-class',
            type=str,
            default=None,
            metavar='SIM_CLASS',
            help='Name of the class in CUSTOM_MODULE that implements the SIM '
                 'to load.')
    customize_arguments.add_argument(
            '--test-sim-arg',
            type=str,
            default=None,
            help='Aboslute path to the json description of argument list '
                 'taken by SIM_CLASS')
    customize_arguments.add_argument(
            '--test-state-machine-factory-class',
            type=str,
            default=None,
            metavar='SMF_CLASS',
            help='Name of the class in CUSTOM_MODULE that impelements the '
                 'state machine factory to load. Only used if MODEM_CLASS is '
                 'not provided.')
    customize_arguments.add_argument(
            '--test-state-machine-factory-arg',
            type=str,
            default=None,
            help='Absolute path to the json description of argument list '
                 'taken by SMF_CLASS')

    opts = parser.parse_args(arg_string)

    # Extra sanity checks.
    if opts.family == 'CDMA' and opts.roaming_networks > 0:
        raise argparse.ArgumentTypeError('CDMA networks do not support '
                                         'roaming networks.')

    test_objects = (opts.test_modem_class or
                    opts.test_sim_class or
                    opts.test_state_machine_factory_class)
    if not opts.test_module and test_objects:
        raise argparse.ArgumentTypeError('test_module is required with any '
                                         'other customization arguments.')

    if opts.test_modem_class and opts.test_state_machine_factory_class:
        logging.warning('test-state-machine-factory-class will be ignored '
                        'because test-modem-class was provided.')

    return opts


def ExtractExitError(dump_file_path):
    """
    Gets the exit error left behind by a crashed pseudomodem.

    If there is a file at |dump_file_path|, extracts the error and the traceback
    left behind by the child process. This function is intended to be used by
    the launching process to parse the error file left behind by pseudomodem.

    @param dump_file_path: Full path to the file to read.
    @returns: (error_reason, error_traceback)
            error_reason: str. The one line reason for error that should be
                    used to raise exceptions.
            error_traceback: A list of str. This is the traceback left
                    behind by the child process, if any. May be [].

    """
    error_reason = 'No detailed reason found :('
    error_traceback = []
    if dump_file_path:
        try:
            dump_file = open(dump_file_path, 'rb')
            error_reason = dump_file.readline().strip()
            error_traceback = dump_file.readlines()
            dump_file.close()
        except OSError as e:
            logging.error('Could not open dump file %s: %s',
                          dump_file_path, str(e))
    return error_reason, error_traceback


# The single global instance of PseudoModemManager.
_pseudo_modem_manager = None


# ##############################################################################
# Private static functions.
def _NonNegInt(value):
    value = int(value)
    if value < 0:
        raise argparse.ArgumentTypeError('%s is not a non-negative int' % value)
    return value


def _DumpExitError(dump_file_path, exc):
    """
    Dump information about the raised exception in the exit error file.

    Format of file dumped:
    - First line is the reason for the crash.
    - Subsequent lines are the traceback from the exception raised.

    We expect the file to exist, because we want the launching context (that
    will eventually read the error dump) to create and own the file.

    @param dump_file_path: Full path to file to which we should dump.
    @param exc: The exception raised.

    """
    if not dump_file_path:
        return

    if not os.path.isfile(dump_file_path):
        logging.error('File |%s| does not exist. Can not dump exit error.',
                      dump_file_path)
        return

    try:
        dump_file = open(dump_file_path, 'wb')
    except IOError as e:
        logging.error('Could not open file |%s| to dump exit error. '
                      'Exception raised when opening file: %s',
                      dump_file_path, str(e))
        return

    dump_file.write(str(exc) + '\n')
    dump_file.writelines(traceback.format_exc())
    dump_file.close()


def sig_handler(signum, frame):
    """
    Top level signal handler to handle user interrupt.

    @param signum: The signal received.
    @param frame: Ignored.
    """
    global _pseudo_modem_manager
    logging.debug('Signal handler called with signal %d', signum)
    if _pseudo_modem_manager:
        _pseudo_modem_manager.GracefulExit()


def main():
    """
    This is the entry point for raw pseudomodem.

    You should not be running this module as a script. If you're trying to run
    pseudomodem from the command line, see |pseudomodem_context| module.

    """
    global _pseudo_modem_manager

    logging_setup.SetupLogging()

    logging.info('Pseudomodem commandline: [%s]', str(sys.argv))
    opts = ParseArguments()

    signal.signal(signal.SIGINT, sig_handler)
    signal.signal(signal.SIGTERM, sig_handler)

    try:
        _pseudo_modem_manager = PseudoModemManager()
        _pseudo_modem_manager.Setup(opts)
        _pseudo_modem_manager.StartBlocking()
    except Exception as e:
        logging.error('Caught exception at top level: %s', str(e))
        _DumpExitError(opts.exit_error_file, e)
        _pseudo_modem_manager.GracefulExit()
        raise


if __name__ == '__main__':
    main()
