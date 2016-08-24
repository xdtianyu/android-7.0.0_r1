# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import dbus
import dbus.service
import gobject
import logging

import pm_errors
import pm_constants
import utils

from autotest_lib.client.cros.cellular import mm1_constants

class StateMachine(dbus.service.Object):
    """
    StateMachine is the abstract base class for the complex state machines
    that are involved in the pseudo modem manager.

    Every state transition is managed by a function that has been mapped to a
    specific modem state. For example, the method that handles the case where
    the modem is in the ENABLED state would look like:

        def _HandleEnabledState(self):
            # Do stuff.

    The correct method will be dynamically located and executed by the step
    function according to the dictionary returned by the subclass'
    implementation of StateMachine._GetModemStateFunctionMap.

    Using the StateMachine in |interactive| mode:
    In interactive mode, the state machine object exposes a dbus object under
    the object path |pm_constants.TESTING_PATH|/|self._GetIsmObjectName()|,
    where |self._GetIsmObjectName()| returns the dbus object name to be used.

    In this mode, the state machine waits for a dbus method call
    |pm_constants.I_TESTING_ISM|.|Advance| when a state transition is possible
    before actually executing the transition.

    """
    def __init__(self, modem):
        super(StateMachine, self).__init__(None, None)
        self._modem = modem
        self._started = False
        self._done = False
        self._interactive = False
        self._trans_func_map = self._GetModemStateFunctionMap()


    def __exit__(self):
        self.remove_from_connection()


    @property
    def cancelled(self):
        """
        @returns: True, if the state machine has been cancelled or has
                transitioned to a terminal state. False, otherwise.

        """
        return self._done


    def Cancel(self):
        """
        Tells the state machine to stop transitioning to further states.

        """
        self._done = True


    def EnterInteractiveMode(self, bus):
        """
        Run this machine in interactive mode.

        This function must be called before |Start|. In this mode, the machine
        waits for an |Advance| call before each step.

        @param bus: The bus on which the testing interface must be exported.

        """
        if not bus:
            self.warning('Cannot enter interactive mode without a |bus|.')
            return

        self._interactive = True
        self._ism_object_path = '/'.join([pm_constants.TESTING_PATH,
                                          self._GetIsmObjectName()])
        self.add_to_connection(bus, self._ism_object_path)
        self._interactive = True
        self._waiting_for_advance = False
        logging.info('Running state machine in interactive mode')
        logging.info('Exported test object at %s', self._ism_object_path)


    def Start(self):
        """ Start the state machine. """
        self.Step()


    @utils.log_dbus_method()
    @dbus.service.method(pm_constants.I_TESTING_ISM, out_signature='b')
    def Advance(self):
        """
        Advance a step on a state machine running in interactive mode.

        @returns: True if the state machine was advanced. False otherwise.
        @raises: TestError if called on a non-interactive state machine.

        """
        if not self._interactive:
            raise pm_errors.TestError(
                    'Can not advance a non-interactive state machine')

        if not self._waiting_for_advance:
            logging.warning('%s received an unexpected advance request',
                            self._GetIsmObjectName())
            return False
        logging.info('%s state machine advancing', self._GetIsmObjectName())
        self._waiting_for_advance = False
        if not self._next_transition(self):
            self._done = True
        self._ScheduleNextStep()
        return True


    @dbus.service.signal(pm_constants.I_TESTING_ISM)
    def Waiting(self):
        """
        Signal sent out by an interactive machine when it is waiting for remote
        dbus call  on the |Advance| function.

        """
        logging.info('%s state machine waiting', self._GetIsmObjectName())


    @utils.log_dbus_method()
    @dbus.service.method(pm_constants.I_TESTING_ISM, out_signature='b')
    def IsWaiting(self):
        """
        Determine whether the state machine is waiting for user action.

        @returns: True if machine is waiting for |Advance| call.

        """
        return self._waiting_for_advance


    def Step(self):
        """
        Executes the next corresponding state transition based on the modem
        state.

        """
        logging.info('StateMachine: Step')
        if self._done:
            logging.info('StateMachine: Terminating.')
            return

        if not self._started:
            if not self._ShouldStartStateMachine():
                logging.info('StateMachine cannot start.')
                return
            self._started = True

        state = self._GetCurrentState()
        func = self._trans_func_map.get(state, self._GetDefaultHandler())
        if not self._interactive:
            if func and func(self):
                self._ScheduleNextStep()
            else:
                self._done = True
            return

        assert not self._waiting_for_advance
        if func:
            self._next_transition = func
            self._waiting_for_advance = True
            self.Waiting()  # Wait for user to |Advance| the machine.
        else:
            self._done = True


    def _ScheduleNextStep(self):
        """
        Schedules the next state transition to execute on the idle loop.
        subclasses can override this method to implement custom logic, such as
        delays.

        """
        gobject.idle_add(StateMachine.Step, self)


    def _GetIsmObjectName(self):
        """
        The name of the dbus object exposed by this object with |I_TESTING_ISM|
        interface.

        By default, this is the name of the most concrete class of the object.

        """
        return self.__class__.__name__


    def _GetDefaultHandler(self):
        """
        Returns the function to handle a modem state, for which the value
        returned by StateMachine._GetModemStateFunctionMap is None. The
        returned function's signature must match:

            StateMachine -> Boolean

        This function by default returns None. If no function exists to handle
        a modem state, the default behavior is to terminate the state machine.

        """
        return None


    def _GetModemStateFunctionMap(self):
        """
        Returns a mapping from modem states to corresponding transition
        functions to execute. The returned function's signature must match:

            StateMachine -> Boolean

        The first argument to the function is a state machine, which will
        typically be passed a value of |self|. The return value, if True,
        indicates that the state machine should keep executing further state
        transitions. A return value of False indicates that the state machine
        will transition to a terminal state.

        This method must be implemented by a subclass. Subclasses can further
        override this method to provide custom functionality.

        """
        raise NotImplementedError()


    def _ShouldStartStateMachine(self):
        """
        This method will be called when the state machine is in a starting
        state. This method should return True, if the state machine can
        successfully begin its state transitions, False if it should not
        proceed. This method can also raise an exception in the failure case.

        In the success case, this method should also execute any necessary
        initialization steps.

        This method must be implemented by a subclass. Subclasses can
        further override this method to provide custom functionality.

        """
        raise NotImplementedError()


    def _GetCurrentState(self):
        """
        Get the current state of the state machine.

        This method is called to get the current state of the machine when
        deciding what the next transition should be.
        By default, the state machines are tied to the modem state, and this
        function simply returns the modem state.

        Subclasses can override this function to use custom states in the state
        machine.

        @returns: The modem state.

        """
        return self._modem.Get(mm1_constants.I_MODEM, 'State')
