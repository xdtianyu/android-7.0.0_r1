#!/usr/bin/env python

# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import at_transceiver

import logging
import mox
import os
import unittest

import at_channel
import modem_configuration
import task_loop
import wardmodem_exceptions as wme

class ATTransceiverTestCase(unittest.TestCase):
    """
    Base test fixture for ATTransceiver class.

    """
    class TestMachine(object):
        """ Stub test machine used by tests below. """
        def test_function(self, _):
            """
            A stub StateMachine API function.

            wardmodem calls will be placed to this function.

            @param _: Ignored.

            """
            pass


        # Needed in a test machine.
        def get_well_known_name(self):
            """ Get the well known name of this machine as str. """
            return "test_machine"


    def setUp(self):
        self._mox = mox.Mox()

        # Create a temporary pty pair for the ATTransceiver constructor
        master, slave = os.openpty()

        self._modem_conf = modem_configuration.ModemConfiguration()
        self._at_transceiver = at_transceiver.ATTransceiver(slave,
                                                            self._modem_conf,
                                                            slave)

        # Now replace internal objects in _at_transceiver with mocks
        self._at_transceiver._modem_response_timeout_milliseconds = 0
        self._mock_modem_channel = self._mox.CreateMock(at_channel.ATChannel)
        self._at_transceiver._modem_channel = self._mock_modem_channel
        self._mock_mm_channel = self._mox.CreateMock(at_channel.ATChannel)
        self._at_transceiver._mm_channel = self._mock_mm_channel
        self._mock_task_loop = self._mox.CreateMock(task_loop.TaskLoop)
        self._at_transceiver._task_loop = self._mock_task_loop

        # Also empty out the internal maps, so that actual loaded configuration
        # does not interfere with the test.
        self._at_transceiver._at_to_wm_action_map = {}
        self._at_transceiver._wm_response_to_at_map = {}


class ATTransceiverCommonTestCase(ATTransceiverTestCase):
    """
    Tests common to all three modes of ATTransceiver.

    """

    def test_successful_mode_selection(self):
        """
        Test that all modes can be selected, when both channels are provided.

        """
        self._at_transceiver.mode = at_transceiver.ATTransceiverMode.WARDMODEM
        self.assertEqual(self._at_transceiver.mode,
                         at_transceiver.ATTransceiverMode.WARDMODEM)
        self._at_transceiver.mode = (
                at_transceiver.ATTransceiverMode.PASS_THROUGH)
        self.assertEqual(self._at_transceiver.mode,
                         at_transceiver.ATTransceiverMode.PASS_THROUGH)
        self._at_transceiver.mode = (
               at_transceiver.ATTransceiverMode.SPLIT_VERIFY)
        self.assertEqual(self._at_transceiver.mode,
                         at_transceiver.ATTransceiverMode.SPLIT_VERIFY)

    def test_unsuccessful_mode_selection(self):
        """
        Test that only WARDMODEM mode can be selected if the modem channel is
        missing.

        """
        self._at_transceiver._modem_channel = None
        self._at_transceiver.mode = at_transceiver.ATTransceiverMode.WARDMODEM
        self.assertEqual(self._at_transceiver.mode,
                         at_transceiver.ATTransceiverMode.WARDMODEM)
        self._at_transceiver.mode = (
                at_transceiver.ATTransceiverMode.PASS_THROUGH)
        self.assertEqual(self._at_transceiver.mode,
                         at_transceiver.ATTransceiverMode.WARDMODEM)
        self._at_transceiver.mode = (
               at_transceiver.ATTransceiverMode.SPLIT_VERIFY)
        self.assertEqual(self._at_transceiver.mode,
                         at_transceiver.ATTransceiverMode.WARDMODEM)


    def test_update_at_to_wm_action_map(self):
        """
        Test that _at_to_wm_action_map is updated correctly under different
        scenarios.

        """
        # The diffs if this test fails can be rather long.
        self.maxDiff = None
        self._at_transceiver._at_to_wm_action_map = {}

        # Test initialization
        raw_map = {'AT1=': ('STATE_MACHINE1', 'function1'),
                   'AT2=1,2': ('STATE_MACHINE2', 'function2'),
                   'AT3=*,care,do': ('STATE_MACHINE3', 'function3', (0, 1)),
                   'AT4?': ('STATE_MACHINE4', 'function4'),
                   'AT5=': ('STATE_MACHINE5', 'function5', ()),
                   'AT5=*': ('STATE_MACHINE6', 'function6')}
        parsed_map = {'AT1=': {(): ('STATE_MACHINE1', 'function1', ())},
                      'AT2=': {('1','2'): ('STATE_MACHINE2', 'function2', ())},
                      'AT3=': {('*','care','do'): ('STATE_MACHINE3',
                                                   'function3', (0, 1))},
                      'AT4?': {(): ('STATE_MACHINE4', 'function4', ())},
                      'AT5=': {(): ('STATE_MACHINE5', 'function5', ()),
                               ('*',): ('STATE_MACHINE6', 'function6', ())}}

        self._at_transceiver._update_at_to_wm_action_map(raw_map)
        self.assertEqual(parsed_map, self._at_transceiver._at_to_wm_action_map)

        # Test update
        raw_good_update = {'AT1=': ('STATE_MACHINE7', 'function7'),
                           'AT5=2': ('STATE_MACHINE8', 'function8', 0),
                           'AT6?': ('STATE_MACHINE9', 'function9')}
        parsed_map = {'AT1=': {(): ('STATE_MACHINE7', 'function7', ())},
                      'AT2=': {('1','2'): ('STATE_MACHINE2', 'function2', ())},
                      'AT3=': {('*','care','do'): ('STATE_MACHINE3',
                                                   'function3', (0, 1))},
                      'AT4?': {(): ('STATE_MACHINE4', 'function4', ())},
                      'AT5=': {(): ('STATE_MACHINE5', 'function5', ()),
                               ('*',): ('STATE_MACHINE6', 'function6', ()),
                               ('2',): ('STATE_MACHINE8', 'function8', (0,))},
                      'AT6?': {(): ('STATE_MACHINE9', 'function9', ())}}
        self._at_transceiver._update_at_to_wm_action_map(raw_good_update)
        self.assertEqual(parsed_map, self._at_transceiver._at_to_wm_action_map)


    def test_find_wardmodem_action_for_at(self):
        """
        Setup _at_to_wm_action_map in the test and then test whether we can find
        actions for AT commands off of that map.

        """
        raw_map = {'AT1=': ('STATE_MACHINE1', 'function1'),
                   'AT2=1,2': ('STATE_MACHINE2', 'function2'),
                   'AT3=*,b,c': ('STATE_MACHINE3', 'function3', (0, 1)),
                   'AT4?': ('STATE_MACHINE4', 'function4'),
                   'AT5=': ('STATE_MACHINE5', 'function5', ()),
                   'AT5=*': ('STATE_MACHINE6', 'function6')}
        self._at_transceiver._update_at_to_wm_action_map(raw_map)

        self.assertEqual(
                ('STATE_MACHINE1', 'function1', ()),
                self._at_transceiver._find_wardmodem_action_for_at('AT1='))
        self.assertEqual(
                ('STATE_MACHINE2', 'function2', ()),
                self._at_transceiver._find_wardmodem_action_for_at('AT2=1,2'))
        self.assertEqual(
                ('STATE_MACHINE3', 'function3', ('a','b')),
                self._at_transceiver._find_wardmodem_action_for_at('AT3=a,b,c'))
        self.assertEqual(
                ('STATE_MACHINE3', 'function3', ('','b')),
                self._at_transceiver._find_wardmodem_action_for_at('AT3=,b,c'))
        self.assertEqual(
                ('STATE_MACHINE5', 'function5', ()),
                self._at_transceiver._find_wardmodem_action_for_at('AT5='))
        self.assertEqual(
                ('STATE_MACHINE6', 'function6', ()),
                self._at_transceiver._find_wardmodem_action_for_at('AT5=s'))
        # Unsuccessful cases
        self.assertRaises(
                wme.ATTransceiverException,
                self._at_transceiver._find_wardmodem_action_for_at,
                'DOESNOTEXIST')


    def test_find_wardmodem_action_for_at_returns_fallback(self):
        """
        Test that when a fallback machine is setup, and unmatched AT command is
        forwarded to this machine.

        """
        mock_test_machine = self._mox.CreateMock(self.TestMachine)
        mock_test_machine.get_well_known_name().MultipleTimes().AndReturn(
                'FALLBACK_MACHINE')
        self._mox.ReplayAll()
        self._at_transceiver.register_state_machine(mock_test_machine)
        self._at_transceiver.register_fallback_state_machine(
                mock_test_machine.get_well_known_name(),
                'act_on')
        self.assertEqual(
                ('FALLBACK_MACHINE', 'act_on', ('DOESNOTEXIST',)),
                self._at_transceiver._find_wardmodem_action_for_at(
                        'DOESNOTEXIST'))
        self._mox.VerifyAll()


    def test_post_wardmodem_request(self):
        """
        Test that a wardmodem request can be posted successfully end-to-end.

        """
        raw_map = {'AT=*': ('TestMachine', 'test_function', 0)}
        arg = 'fake_arg'
        command = 'AT=' + arg
        mock_test_machine = self._mox.CreateMock(self.TestMachine)
        self._at_transceiver._update_at_to_wm_action_map(raw_map)
        mock_test_machine.get_well_known_name().AndReturn('TestMachine')
        self._mock_task_loop.post_task(
                self._at_transceiver._execute_state_machine_function,
                command, mox.IgnoreArg(), mock_test_machine.test_function,
                arg)

        self._mox.ReplayAll()
        self._at_transceiver.register_state_machine(mock_test_machine)
        self._at_transceiver._post_wardmodem_request(command)
        self._mox.VerifyAll()


    def test_update_wm_response_to_at_map(self):
        """
        Test that the wm_response_to_at_map is correctly updated.

        """
        raw_map = {'some_function': 'AT=some_function',
                   'some_other_function': 'AT=some_other_function'}
        self._at_transceiver._update_wm_response_to_at_map(raw_map)
        self.assertEqual(raw_map,
                         self._at_transceiver._wm_response_to_at_map)

        raw_map = {'some_other_function': 'AT=overwritten_function',
                   'some_new_function': 'AT=this_is_new_too'}
        updated_map = {'some_function': 'AT=some_function',
                       'some_other_function': 'AT=overwritten_function',
                       'some_new_function': 'AT=this_is_new_too'}
        self._at_transceiver._update_wm_response_to_at_map(raw_map)
        self.assertEqual(updated_map,
                         self._at_transceiver._wm_response_to_at_map)


    def test_construct_at_response(self):
        """
        Test that construct_at_response correctly replaces by actual arguments.

        """
        self.assertEqual(
                'AT=arg1,some,arg2',
                self._at_transceiver._construct_at_response(
                        'AT=*,some,*', 'arg1','arg2'))
        self.assertEqual(
                'AT=1,some,thing',
                self._at_transceiver._construct_at_response(
                        'AT=*,some,thing', 1))
        self.assertEqual(
                'AT=some,other,thing',
                self._at_transceiver._construct_at_response(
                        'AT=some,other,thing'))
        self.assertEqual(
                'AT=needsnone',
                self._at_transceiver._construct_at_response(
                        'AT=needsnone', 'butonegiven'))
        # Unsuccessful cases
        self.assertRaises(
                wme.ATTransceiverException,
                self._at_transceiver._construct_at_response,
                'AT=*,needstwo,*', 'onlyonegiven')


    def test_process_wardmodem_response(self):
        """
        A basic test for process_wardmodem_response.

        """
        self._mox.StubOutWithMock(self._at_transceiver,
                             '_process_wardmodem_at_command')
        raw_map = {'func1': 'AT=*,given,*',
                   'func2': 'AT=nothing,needed'}
        self._at_transceiver._update_wm_response_to_at_map(raw_map)

        self._at_transceiver._process_wardmodem_at_command('AT=a,given,2')
        self._at_transceiver._process_wardmodem_at_command('AT=nothing,needed')

        self._mox.ReplayAll()
        self._at_transceiver.process_wardmodem_response('func1','a',2)
        self._at_transceiver.process_wardmodem_response('func2')
        self._mox.UnsetStubs()
        self._mox.VerifyAll()


class ATTransceiverWardModemTestCase(ATTransceiverTestCase):
    """
    Test ATTransceiver class in the WARDMODEM mode.

    """

    def setUp(self):
        super(ATTransceiverWardModemTestCase, self).setUp()
        self._at_transceiver.mode = at_transceiver.ATTransceiverMode.WARDMODEM


    def test_wardmodem_at_command(self):
        """
        Test the case when AT command is received from wardmodem.

        """
        at_command = 'AT+commmmmmmmmand'
        self._mock_mm_channel.send(at_command)

        self._mox.ReplayAll()
        self._at_transceiver._process_wardmodem_at_command(at_command)
        self._mox.VerifyAll()


    def test_mm_at_command(self):
        """
        Test the case when AT command is received from modem manager.

        """
        at_command = 'AT+commmmmmmmmand'
        self._mox.StubOutWithMock(self._at_transceiver,
                                  '_post_wardmodem_request')

        self._at_transceiver._post_wardmodem_request(at_command)

        self._mox.ReplayAll()
        self._at_transceiver._process_mm_at_command(at_command)
        self._mox.UnsetStubs()
        self._mox.VerifyAll()


class ATTransceiverPassThroughTestCase(ATTransceiverTestCase):
    """
    Test ATTransceiver class in the PASS_THROUGH mode.

    """

    def setUp(self):
        super(ATTransceiverPassThroughTestCase, self).setUp()
        self._at_transceiver.mode = (
                at_transceiver.ATTransceiverMode.PASS_THROUGH)


    def test_modem_at_command(self):
        """
        Test the case when AT command received from physical modem.

        """
        at_command = 'AT+commmmmmmmmand'
        self._mock_mm_channel.send(at_command)

        self._mox.ReplayAll()
        self._at_transceiver._process_modem_at_command(at_command)
        self._mox.VerifyAll()


    def test_mm_at_command(self):
        """
        Test the case when AT command is received from modem manager.

        """
        at_command = 'AT+commmmmmmmmand'
        self._mock_modem_channel.send(at_command)

        self._mox.ReplayAll()
        self._at_transceiver._process_mm_at_command(at_command)
        self._mox.VerifyAll()


class ATTransceiverSplitVerifyTestCase(ATTransceiverTestCase):
    """
    Test ATTransceiver class in the SPLIT_VERIFY mode.

    """

    def setUp(self):
        super(ATTransceiverSplitVerifyTestCase, self).setUp()
        self._at_transceiver.mode = (
                at_transceiver.ATTransceiverMode.SPLIT_VERIFY)


    def test_mm_at_command(self):
        """
        Test that that incoming modem manager command is multiplexed to
        wardmodem and physical modem.

        """
        at_command = 'AT+commmmmmmmmand'
        self._mox.StubOutWithMock(self._at_transceiver,
                                  '_post_wardmodem_request')
        self._mock_modem_channel.send(at_command).InAnyOrder()
        self._at_transceiver._post_wardmodem_request(at_command).InAnyOrder()

        self._mox.ReplayAll()
        self._at_transceiver._process_mm_at_command(at_command)
        self._mox.UnsetStubs()
        self._mox.VerifyAll()


    def test_successful_single_at_response_modem_wardmodem(self):
        """
        Test the case when one AT response is received successfully.
        In this case, physical modem command comes first.

        """
        at_command = 'AT+commmmmmmmmand'
        self._mock_mm_channel.send(at_command)

        self._mox.ReplayAll()
        self._at_transceiver._process_modem_at_command(at_command)
        self._at_transceiver._process_wardmodem_at_command(at_command)
        self._mox.VerifyAll()


    def test_successful_single_at_response_wardmodem_modem(self):
        """
        Test the case when one AT response is received successfully.
        In this case, wardmodem command comes first.

        """
        at_command = 'AT+commmmmmmmmand'
        task_id = 3
        self._mock_task_loop.post_task_after_delay(
                self._at_transceiver._modem_response_timed_out,
                mox.IgnoreArg()).AndReturn(task_id)
        self._mock_task_loop.cancel_posted_task(task_id)
        self._mock_mm_channel.send(at_command)

        self._mox.ReplayAll()
        self._at_transceiver._process_wardmodem_at_command(at_command)
        self._at_transceiver._process_modem_at_command(at_command)
        self._mox.VerifyAll()

    def test_mismatched_at_response(self):
        """
        Test the case when both responses arrive, but are not identical.

        """
        wardmodem_command = 'AT+wardmodem'
        modem_command = 'AT+modem'
        self._mox.StubOutWithMock(self._at_transceiver,
                                  '_report_verification_failure')
        self._at_transceiver._report_verification_failure(
                self._at_transceiver.VERIFICATION_FAILED_MISMATCH,
                modem_command,
                wardmodem_command)
        self._mock_mm_channel.send(wardmodem_command)

        self._mox.ReplayAll()
        self._at_transceiver._process_modem_at_command(modem_command)
        self._at_transceiver._process_wardmodem_at_command(wardmodem_command)
        self._mox.UnsetStubs()
        self._mox.VerifyAll()


    def test_modem_response_times_out(self):
        """
        Test the case when the physical modem fails to respond.

        """
        at_command = 'AT+commmmmmmmmand'
        task_id = 3
        self._mox.StubOutWithMock(self._at_transceiver,
                                  '_report_verification_failure')

        self._mock_task_loop.post_task_after_delay(
                self._at_transceiver._modem_response_timed_out,
                mox.IgnoreArg()).AndReturn(task_id)
        self._at_transceiver._report_verification_failure(
                self._at_transceiver.VERIFICATION_FAILED_TIME_OUT,
                None,
                at_command)
        self._mock_mm_channel.send(at_command)

        self._mox.ReplayAll()
        self._at_transceiver._process_wardmodem_at_command(at_command)
        self._at_transceiver._modem_response_timed_out()
        self._mox.UnsetStubs()
        self._mox.VerifyAll()


    def test_multiple_successful_responses(self):
        """
        Test the case two wardmodem responses are queued, and then two matching
        modem responses are received.

        """
        first_at_command = 'AT+first'
        second_at_command = 'AT+second'
        first_task_id = 3
        second_task_id = 4

        self._mock_task_loop.post_task_after_delay(
                self._at_transceiver._modem_response_timed_out,
                mox.IgnoreArg()).AndReturn(first_task_id)
        self._mock_task_loop.cancel_posted_task(first_task_id)
        self._mock_mm_channel.send(first_at_command)
        self._mock_task_loop.post_task_after_delay(
                self._at_transceiver._modem_response_timed_out,
                mox.IgnoreArg()).AndReturn(second_task_id)
        self._mock_task_loop.cancel_posted_task(second_task_id)
        self._mock_mm_channel.send(second_at_command)

        self._mox.ReplayAll()
        self._at_transceiver._process_wardmodem_at_command(first_at_command)
        self._at_transceiver._process_wardmodem_at_command(second_at_command)
        self._at_transceiver._process_modem_at_command(first_at_command)
        self._at_transceiver._process_modem_at_command(second_at_command)
        self._mox.VerifyAll()


if __name__ == '__main__':
    logging.basicConfig(level=logging.DEBUG)
    unittest.main()
