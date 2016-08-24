# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import state_machine

import logging
import mox
import unittest

import at_transceiver
import global_state
import modem_configuration
import task_loop
import wardmodem_exceptions as wme

class StateMachineBadTestCase(unittest.TestCase):
    """
    Test that an abstract machine can not be instantiated.

    """

    def test_failed_instantiation(self):
        """
        Only subclasses of StateMachine that implement the get_well_known_name
        can be instantiated. Test that a direct instantiation of StateMachine
        fails.

        """
        self._mox = mox.Mox()
        self._transceiver = self._mox.CreateMock(at_transceiver.ATTransceiver)
        self._state = self._mox.CreateMock(global_state.GlobalState)
        self._task_loop = self._mox.CreateMock(task_loop.TaskLoop)
        self._modem_conf = self._mox.CreateMock(
                modem_configuration.ModemConfiguration)

        self.assertRaises(wme.WardModemSetupException,
                          state_machine.StateMachine, self._state,
                          self._transceiver, self._modem_conf)

class StateMachineTestCase(unittest.TestCase):
    """
    Test fixture for StateMachine class.

    """

    class TestStateMachine(state_machine.StateMachine):
        #pylint: disable=C0111
        """
        A simple test machine that can be instantiated.
        """

        def get_well_known_name(self):
            return 'TestStateMachine'


    def setUp(self):
        self._mox = mox.Mox()
        self._transceiver = self._mox.CreateMock(at_transceiver.ATTransceiver)
        self._state = self._mox.CreateMock(global_state.GlobalState)
        self._task_loop = self._mox.CreateMock(task_loop.TaskLoop)
        self._modem_conf = self._mox.CreateMock(
                modem_configuration.ModemConfiguration)

        self._state_machine = StateMachineTestCase.TestStateMachine(
                self._state, self._transceiver, self._modem_conf)
        # Replace some internal objects with mocks.
        self._state_machine._task_loop = self._task_loop

    def _add_response_functions(self):
        self._state_machine._add_response_function('wm_response_1')
        self._state_machine._add_response_function('wm_response_2')
        self._state_machine._add_response_function('wm_response_3')


    def test_add_response_function(self):
        """
        Tests that only valid response functions can be added.

        """
        self.assertRaises(
                wme.WardModemSetupException,
                self._state_machine._add_response_function,
                'no spaces')
        self.assertRaises(
                wme.WardModemSetupException,
                self._state_machine._add_response_function,
                'MUST_BE_LOWER_CASE')
        self.assertRaises(
                wme.WardModemSetupException,
                self._state_machine._add_response_function,
                'must_begin_with_wm_response_')

        self._state_machine._add_response_function('wm_response_something')


    def test_dispatch_functions(self):
        """
        Basic test for the _respond, _update_state and _update_state_and_respond
        dispatch functions.

        """
        self._add_response_functions()
        response_delay_ms  = 30
        response = self._state_machine.wm_response_1
        response_arg1 = 1
        response_arg2 = 'blah'
        state_update = {'comp1': 'VAL1', 'comp2': 'VAL2'}
        state_update_delay_ms = 20

        self._task_loop.post_task_after_delay(
                self._transceiver.process_wardmodem_response, response_delay_ms,
                response, response_arg1, response_arg2)
        self._task_loop.post_task_after_delay(
                self._state_machine._update_state_callback,
                state_update_delay_ms, state_update, mox.IgnoreArg())

        self._mox.ReplayAll()
        self._state_machine._respond(response, response_delay_ms, response_arg1,
                                     response_arg2)
        self._state_machine._update_state(state_update, state_update_delay_ms)
        self._mox.VerifyAll()


if __name__ == '__main__':
    logging.basicConfig(level=logging.DEBUG)
    unittest.main()
