#!/usr/bin/env python

# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import argparse
import dbus
import logging
import os
import signal
import sys
import traceback

import at_transceiver
import global_state
import modem_configuration
import task_loop
import wardmodem_exceptions as wme

import common
from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.cros.cellular import net_interface

STATE_MACHINE_DIR_NAME = 'state_machines'

class WardModem(object):
    """
    The main wardmodem object that replaces a physical modem.

    What it does:
        - Loads configuration data.
        - Accepts custom state machines from the test.
        - Builds objects and ties them together.
        - Exposes objects for further customization

    What it does not do:
        - Tweak the different knobs provided by internal objects that it exposes
          for further customization.
          That is the responsibility of the WardModemContext.
        - Care about setting up / tearing down environment.
          Again, see WardModemContext.
    """

    def __init__(self,
                 replaced_modem = None,
                 state_machines = None,
                 modem_at_port_dev_name = None):
        """
        @param replaced_modem: Name of the modem being emulated. If left None,
                the base modem will be emulated. A list of valid modems can be
                found in the module modem_configuration

        @param state_machines: Objects of subtypes of StateMachine that override
                any state machine defined in the configuration files for the
                same well-known-name.

        @param modem_at_port_dev_name: The full path to the primary AT port of
                the physical modem. This is needed only if we're running in a
                mode where we pass on modemmanager commands to the modem. This
                should be a string of the form '/dev/XXX'

        """
        self._logger = logging.getLogger(__name__)

        if not state_machines:
            state_machines = []
        if modem_at_port_dev_name and (
                type(modem_at_port_dev_name) is not str or
                modem_at_port_dev_name[0:5] != '/dev/'):
            raise wme.WardModemSetupException(
                    'Modem device name must be of the form "/dev/XXX", '
                    'where XXX is the udev device.')

        # The modem that wardmodem is intended to replace.
        self._replaced_modem = replaced_modem

        # Pseudo net interface exported to shill.
        self._net_interface = net_interface.PseudoNetInterface()

        # The internal task loop object. Readable through a property.
        self._task_loop = task_loop.TaskLoop()

        # The global state object shared by all state machines.
        self._state = global_state.GlobalState()

        # The configuration object for the replaced modem.
        self._modem_conf = modem_configuration.ModemConfiguration(
                replaced_modem)

        self._create_transceiver(modem_at_port_dev_name)
        self._setup_state_machines(state_machines)

        self._started = False


    def start(self):
        """
        Turns on the wardmodem.

        This call is blocking. It will return after |stop| is called.

        """
        self._logger.info('Starting wardmodem...')
        self._net_interface.Setup()
        self._task_loop.start()


    def stop(self):
        """
        Stops wardmodem and cleanup.

        """
        # We need to delete a bunch of stuff *before* the task_loop can be
        # stopped.
        self._logger.info('Stopping wardmodem.')
        self._net_interface.Teardown()
        del self._transceiver
        os.close(self._wm_at_port)
        os.close(self._mm_at_port)
        if self._modem_at_port:
            os.close(self._modem_at_port)
        self.task_loop.stop()


    @property
    def modem(self):
        """
        The physical modem being replaced [read-only].

        @return string representing the replaced modem.

        """
        return self._replaced_modem


    @property
    def modem_conf(self):
        """
        The ModemConfiguration object loaded for the replaced modem [read-only].

        @return A ModemConfiguration object.

        """
        return self._modem_conf

    @property
    def transceiver(self):
        """
        The ATTransceiver that will orchestrate communication [read-only].

        @return ATTransceiver object.

        """
        return self._transceiver


    @property
    def task_loop(self):
        """
        The main loop for asynchronous operations [read-only].

        @return TaskLoop object.

        """
        return self._task_loop


    @property
    def state(self):
        """
        The global state object that must by shared by all state machines.

        @return GlobalState object.

        """
        return self._state


    @property
    def mm_at_port_pts_name(self):
        """
        Name of the pty terminal to be used by modemmanager.

        @return A string of the form 'pts/X' where X is the pty number.

        """
        fullname = os.ttyname(self._mm_at_port)
        # fullname is of the form /dev/pts/X where X is a pts number.
        # We want to return just the pts/X part.
        assert fullname[0:5] == '/dev/'
        return fullname[5:]


    def _create_transceiver(self, modem_at_port_dev_name):
        """
        Opens a pty pair and initialize ATTransceiver.

        @param modem_at_port_dev_name: The device name of the primary port.

        """
        self._modem_at_port = None
        if modem_at_port_dev_name:
            try:
                self._modem_at_port = os.open(modem_at_port_dev_name,
                                              os.O_RDWR)
            except (TypeError, OSError) as e:
                logging.warning('Could not open modem_port |%s|\nError:\n%s',
                                modem_at_port_dev_name, e)

        self._wm_at_port, self._mm_at_port = os.openpty()
        self._transceiver = at_transceiver.ATTransceiver(self._wm_at_port,
                                                         self._modem_conf,
                                                         self._modem_at_port)

    def _setup_state_machines(self, client_machines):
        """
        Creates the state machines looking at sources in the right order.

        @param client_machines: The client provided state machine objects.

        """
        # A local list of state machines created
        state_machines = []

        # Create the state machines comprising the wardmodem.
        # Highest priority is given to the client provided state machines. The
        # remaining will be instantiated based on |replaced_modem|.
        for sm in client_machines:
            if sm.get_well_known_name() in state_machines:
                raise wme.SetupError('Multiple state machines provided with '
                                     'well-known-name |%s|' %
                                     sm.get_well_known_name)
            state_machines.append(sm.get_well_known_name())
            self._transceiver.register_state_machine(sm)
            self._logger.debug('Added client specified machine {%s --> %s}',
                               sm.get_well_known_name(),
                               sm.__class__.__name__)
        # Now instantiate modem specific state machines.
        for sm_module in self._modem_conf.plugin_state_machines:
            sm = self._create_state_machine(sm_module)
            if sm.get_well_known_name() not in state_machines:
                state_machines.append(sm.get_well_known_name())
                self._transceiver.register_state_machine(sm)
                self._logger.debug(
                        'Added modem specific machine {%s --> %s}',
                        sm.get_well_known_name(),
                        sm.__class__.__name__)
        # Finally instantiate generic state machines.
        for sm_module in self._modem_conf.base_state_machines:
            sm = self._create_state_machine(sm_module)
            if sm.get_well_known_name() not in state_machines:
                state_machines.append(sm.get_well_known_name())
                self._transceiver.register_state_machine(sm)
                self._logger.debug('Added default machine {%s --> %s}',
                                   sm.get_well_known_name(),
                                   sm.__class__.__name__)
        self._logger.info('Loaded state machines: %s', str(state_machines))

        # Also setup the fallback state machine
        self._transceiver.register_fallback_state_machine(
                self._modem_conf.fallback_machine,
                self._modem_conf.fallback_function)


    def _create_state_machine(self, module_name):
        """
        Creates a state machine object given the |module_name|.

        There is a specific naming convention for these state machine
        definitions. If |module_name| is new_and_shiny_machine, the state
        machine class must be named NewAndShinyMachine.

        @param module_name: The name of the module from which the state machine
                should be created.

        @returns An object of type new_and_shiny_machine.NewAndShinyMachine, if
                it exists.

        @raises WardModemSetupError if |module_name| is malformed or the object
                creation fails.

        """
        # Obtain the name of the state machine class from module_name.
        # viz, convert my_module_name --> MyModuleName
        parts = module_name.split('_')
        parts = [x.title() for x in parts]
        class_name = ''.join(parts)

        self._import_state_machine_module_as_sm(module_name)
        return getattr(sm, class_name)(
                self._state,
                self._transceiver,
                self._modem_conf)


    def _import_state_machine_module_as_sm(self, module_name):
        global sm
        if module_name == 'call_machine':
            from state_machines import call_machine as sm
        elif module_name == 'call_machine_e362':
            from state_machines import call_machine_e362 as sm
        elif module_name == 'level_indicators_machine':
            from state_machines import level_indicators_machine as sm
        elif module_name == 'modem_power_level_machine':
            from state_machines import modem_power_level_machine as sm
        elif module_name == 'network_identity_machine':
            from state_machines import network_identity_machine as sm
        elif module_name == 'network_operator_machine':
            from state_machines import network_operator_machine as sm
        elif module_name == 'network_registration_machine':
            from state_machines import network_registration_machine as sm
        elif module_name == 'request_response':
            from state_machines import request_response as sm
        else:
            raise wme.WardModemSetupException('Unknown state machine module: '
                                              '%s' % module_name)


class WardModemContext(object):
    """
    Setup wardmodem according to the options provided.

    This context should be used by everyone to interact with WardModem.
    This context will
    (1) Setup wardmodem, setting the correct options on the internals exposed by
        the wardmodem object.
    (2) Manage the modemmanager instance during the context's lifetime.

    """

    MODEMMANAGER_RESTART_TIMEOUT = 60

    def __init__(self, use_wardmodem=True, detach=True, args=None):
        """
        @param use_wardmodem: If False, this context is a no-op. Otherwise, the
                whole wardmodem magic is done.

        @param detach: A bool flag indicating whether wardmodem should be run in
                its own process. If True, |start| will return immediately,
                starting WardModem in its own process; Otherwise, |start| will
                block until |stop| is called.

        @param args: Options to setup WardModem. This is a list of string
                command line arguments accepted by the parser defined in
                |get_option_parser|.
                TODO(pprabhu) Also except a dict of options to ease
                customization in tests.

        """
        self._logger = logging.getLogger(__name__)
        self._logger.info('Initializing wardmodem context.')

        self._use_wardmodem = use_wardmodem
        if not self._use_wardmodem:
            self._logger.info('WardModemContext directed to do nothing. '
                              'All wardmodem actions are no-op.')
            self._logger.debug('........... Welcome to the real world Neo.')
            return

        self._logger.debug('Wardmodem arguments: detach: %s, args: %s',
                           detach, str(args))

        self._started = False
        self._wardmodem = None
        self._was_mm_running = False
        self._detach = detach
        option_parser = self._get_option_parser()

        # XXX:HACK For some reason, parse_args picks up argv when the context is
        # created by an autotest test. Workaround: stash away the argv.
        argv_stash = sys.argv
        sys.argv = ['wardmodem']
        self._options = option_parser.parse_args(args)
        sys.argv = argv_stash


    def __enter__(self):
        self.start()
        return self


    def __exit__(self, type, value, traceback):
        self.stop()
        # Don't supress any exceptions raised in the 'with' block
        return False


    def start(self):
        """
        Start the WardModem, setting up the correct environment.

        If |detach| was True, this call will return immediately, running
        WardModem in its own process; Otherwise, this call will block and only
        return when |stop| is called.

        """
        if not self._use_wardmodem:
            return

        if self._started:
            raise wme.WardModemSetupException(
                    'Attempted to re-enter an already active wardmodem '
                    'context.')

        self._started = True
        self._wardmodem = WardModem(
                self._options.modem,
                modem_at_port_dev_name=self._options.modem_port)
        if not self._prepare_wardmodem(self._options):
            raise wme.WardModemSetupException(
                    'Contradictory wardmodem setup options detected.')

        self._prepare_mm()

        if not self._detach:
            self._wardmodem.start()
            return

        self._logger.debug('Will fork wardmodem process.')
        self._child = os.fork()
        if self._child == 0:
            # Setup a way to stop the child.
            def _exit_child(signum, frame):
                self._logger.info('Signal handler called with signal %s',
                                  signum)
                self._cleanup()
                os._exit(0)
            signal.signal(signal.SIGINT, _exit_child)
            signal.signal(signal.SIGTERM, _exit_child)
            # In detach mode, all uncaught exceptions raised by wardmodem
            # will be thrown here. Since this is a child process, they will
            # be lost.
            # At least log them before throwing them again, so that we know
            # something went wrong in wardmodem.
            try:
                self._wardmodem.start()
            except Exception as e:
                traceback.print_exc()
                raise

        else:
            # Wait here for the child to start before continuing.
            # We will continue once we know that modemmanager process has
            # detected the wardmodem device, and has exported it on its dbus
            # interface.
            utils.poll_for_condition(
                    self._check_for_modem,
                    exception=wme.WardModemSetupException(
                            'Could not cleanly restart modemmanager.'),
                    timeout=self.MODEMMANAGER_RESTART_TIMEOUT,
                    sleep_interval=1)
            self._logger.debug('Continuing the main process outside '
                               'wardmodem.')


    def stop(self):
        """
        Stops WardModem, restore environment to its previous state.

        """
        if not self._use_wardmodem:
            return

        if not self._started:
            self._logger.warning('No wardmodem instance running! '
                                 'Nothing to stop.')
            return

        if self._detach:
            self._logger.debug('Attempting to kill child wardmodem process.')
            if self._child != 0:
                os.kill(self._child, signal.SIGINT)
                os.waitpid(self._child, 0)
                self._child = 0
            self._logger.debug('Finished waiting on child wardmodem process '
                               'to finish.')
        else:
            self._cleanup()
        self._started = False


    def _cleanup(self):
        # Restore mm before turning off wardmodem.
        self._restore_mm()
        self._wardmodem.stop()
        self._logger.info('Bye, Bye!')


    def _prepare_wardmodem(self, options):
        """
        Tweaks the internals exposed by WardModem post-creation according to the
        options provided.

        @param options: is an object returned by argparse.

        """
        if options.modem:
            if options.pass_through_mode:
                self._logger.warning('Ignoring "modem" in pass-through-mode.')

        if options.at_terminator:
            self._wardmodem.transceiver.at_terminator = options.at_terminator

        if options.pass_through_mode:
            self._wardmodem.transceiver.mode = \
                    at_transceiver.ATTransceiverMode.PASS_THROUGH

        if options.bridge_mode:
            self._wardmodem.transceiver.mode = \
                    at_transceiver.ATTransceiverMode.SPLIT_VERIFY

        if options.modem_port:
            if not options.pass_through_mode and not options.bridge_mode:
                self._logger.warning('Ignoring "modem-port" in normal mode.')
        else:
            if options.pass_through_mode or options.bridge_mode:
                self._logger.error('"modem-port" needed in %s mode.' %
                              'bridge-mode' if options.bridge_mode else
                              'pass-through-mode')
                return False

        if options.fast:
            if options.pass_through_mode:
                self._logger.warning('Ignoring "fast" in pass-through-mode')
            else:
                self._wardmodem.task_loop.ignore_delays = True

        if options.randomize_delays:
            if options.fast:
                self._logger.warning('Ignoring option "randomize-delays" '
                                '"fast" overrides "randomize-delays".')
            if options.pass_through_mode:
                self._logger.warning('Ignoring "randomize-delays" in '
                                     'pass-through-mode')
            if not options.fast and not options.pass_through_mode:
                self._wardmodem.task_loop.random_delays = True

        if options.max_randomized_delay:
            if (options.fast or not options.randomize_delays or
                options.pass_through_mode):
                self._logger.warning('Ignoring "max_randomized_delays"')
            else:
                self._wardmodem.task_loop.max_random_delay_ms = \
                        options.max_randomized_delay

        return True


    def _prepare_mm(self):
        """
        Starts modemmanager in test mode listening to the WardModem specified
        pty end-point.

        """
        self._was_mm_running = False
        try:
            result = utils.run('pgrep ModemManager')
            if result.stdout:
                self._was_mm_running = True
        except error.CmdError:
            pass
        try:
            utils.run('initctl stop modemmanager')
        except error.CmdError:
            pass

        mm_opts = ''
        mm_opts += '--log-level=DEBUG '
        mm_opts += '--timestamps '
        mm_opts += '--test '
        mm_opts += '--debug '
        mm_opts += '--test-plugin=' + self._wardmodem.modem_conf.mm_plugin + ' '
        mm_opts += '--test-at-port="' + self._wardmodem.mm_at_port_pts_name + \
                   '" '
        mm_opts += '--test-net-port=' + \
                   net_interface.PseudoNetInterface.IFACE_NAME + ' '
        result = utils.run('ModemManager %s &' % mm_opts)
        self._logger.debug('ModemManager stderr:\n%s', result.stderr)


    def _check_for_modem(self):
        bus = dbus.SystemBus()
        try:
            manager = bus.get_object('org.freedesktop.ModemManager1',
                                      '/org/freedesktop/ModemManager1')
            imanager = dbus.Interface(manager,
                                      'org.freedesktop.DBus.ObjectManager')
            modems = imanager.GetManagedObjects()
        except dbus.exceptions.DBusException as e:
            # The ObjectManager interface on modemmanager is not up yet.
            return False
        # Check if a modem with the test at port has been exported
        if self._wardmodem.mm_at_port_pts_name in str(modems):
            return True
        else:
            return False


    def _restore_mm(self):
        """
        Stops the test instance of modemmanager and restore it to previous
        state.

        """
        result = None
        try:
            result = utils.run('pgrep ModemManager')
            self._logger.warning('ModemManager in test mode still running! '
                                 'Killing it ourselves.')
            try:
                utils.run('pkill -9 ModemManager')
            except error.CmdError:
                self._logger.warning('Failed to kill test ModemManager.')
        except error.CmdError:
            self._logger.debug('As expected: ModemManager in test mode killed.')
        if self._was_mm_running:
            try:
                utils.run('initctl start modemmanager')
            except error.CmdError:
                self._logger.warning('Failed to restart modemmanager service.')


    def _get_option_parser(self):
        """
        Build an argparse parser to accept options from the user/test to tweak
        WardModem post-creation.

        """
        parser = argparse.ArgumentParser(
                description='Run the wardmodem modem emulator.')

        modem_group = parser.add_argument_group(
                'Modem',
                'Description of the modem to emulate.')
        modem_group.add_argument(
                '--modem',
                help='The modem to emulate.')
        modem_group.add_argument('--at-terminator',
                                 help='The string terminator to use.')

        physical_modem_group = parser.add_argument_group(
                'Physical modem',
                'Interaction with the physical modem on-board.')
        physical_modem_group.add_argument(
                '--pass-through-mode',
                default=False,
                nargs='?',
                const=True,
                help='Act as a transparent channel between the modem manager '
                     'and the physical modem. "--modem-port" option required.')
        physical_modem_group.add_argument(
                '--bridge-mode',
                default=False,
                nargs='?',
                const=True,
                help='Should we also setup a bridge with the real modem? Note '
                     'that the responses generated by wardmodem state machines '
                     'take precedence over those received from the physical '
                     'modem. The bridge is used for a soft-verification: A '
                     'warning is generated if the responses do not match. '
                     '"--modem-port" option required.')
        physical_modem_group.add_argument(
                '--modem-port',
                help='The primary port used by the real modem. ')

        behaviour_group = parser.add_argument_group(
                'Behaviour',
                'Tweak the behaviour of running wardmodem.')
        behaviour_group.add_argument(
                '--fast',
                default=False,
                nargs='?',
                const=True,
                help='Run the emulator with minimum delay between operations.')
        behaviour_group.add_argument(
                '--randomize-delays',
                default=False,
                nargs='?',
                const=True,
                help='Run emulator with randomized delays between operations.')
        behaviour_group.add_argument(
                '--max-randomized-delay',
                type=int,
                help='The maximum randomized delay added between operations in '
                     '"randomize-delays" mode.')

        return parser


# ##############################################################################
# Run WardModem as a script.
# ##############################################################################
_wardmodem_context = None

SIGNAL_TO_NAMES_DICT = \
        dict((getattr(signal, n), n)
             for n in dir(signal) if n.startswith('SIG') and '_' not in n)

def exit_wardmodem_script(signum, frame):
    """
    Signal handler to intercept Keyboard interrupt and stop the WardModem.

    @param signum: The signal that was sent to the script

    @param frame: Current stack frame [ignored].

    """
    global _wardmodem_context
    if signum == signal.SIGINT:
        logging.info('Captured Ctrl-C. Exiting wardmodem.')
        _wardmodem_context.stop()
    else:
        logging.warning('Captured unexpected signal: %s',
                        SIGNAL_TO_NAMES_DICT.get(signum, str(signum)))


def main():
    """
    Entry function to wardmodem script.

    """
    global _wardmodem_context
    # HACK: I should not have logged anything before getting here, but
    # basicConfig wasn't doing anything: So, attempt to clean config.
    root = logging.getLogger()
    if root.handlers:
        for handler in root.handlers:
            root.removeHandler(handler)
    logger_format = ('[%(asctime)-15s][%(filename)s:%(lineno)s:%(levelname)s] '
                     '%(message)s')
    logging.basicConfig(format=logger_format,
                        level=logging.DEBUG)

    _wardmodem_context = WardModemContext(True, False, sys.argv[1:])
    logging.info('\n####################################################\n'
                 'Running wardmodem, hit Ctrl+C to exit.\n'
                 '####################################################\n')

    signal.signal(signal.SIGINT, exit_wardmodem_script)
    _wardmodem_context.start()


if __name__ == '__main__':
    main()
