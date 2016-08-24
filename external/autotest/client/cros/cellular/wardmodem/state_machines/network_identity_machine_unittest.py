#!/usr/bin/env python

# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import common
import network_identity_machine

import logging
import mox
import unittest

import at_transceiver
import global_state
import modem_configuration
import task_loop

class NetworkIdentityMachineUnittest(unittest.TestCase):
    """ Test fixture for NetworkIdentityMachine class. """

    def setUp(self):
        self._mox = mox.Mox()
        self._modem_conf = modem_configuration.ModemConfiguration()
        self._task_loop = self._mox.CreateMock(task_loop.TaskLoop)
        self._transceiver = self._mox.CreateMock(at_transceiver.ATTransceiver)

        self._machine = network_identity_machine.NetworkIdentityMachine(
                global_state.GlobalState(),
                self._transceiver,
                self._modem_conf)

        # Mock out the task_loop
        self._machine._task_loop = self._task_loop

    def test_read_sim_imsi(self):
        """ Test the imsi encoding in read_sim_imsi. """

        self._machine._mcc = '123'
        self._machine._mnc = '456'
        self._machine._misn = '123456789'
        expected_response = '081932541632547698'

        self._task_loop.post_task_after_delay(
                self._transceiver.process_wardmodem_response, 0,
                'wm_response_sim_info_success', expected_response)
        self._task_loop.post_task_after_delay(
                self._transceiver.process_wardmodem_response, 0,
                'wm_response_ok')

        self._mox.ReplayAll()
        self._machine.read_sim_imsi(9)
        self._mox.VerifyAll()

    def test_check_length_and_respond_successful(self):
        """ Test _check_length_and_respond. """
        response1 = 'blahblah'
        response2_clipped = 'moomoo'
        response2 = response2_clipped + '2'
        self._task_loop.post_task_after_delay(
                self._transceiver.process_wardmodem_response, 0,
                'wm_response_sim_info_success', response1)
        self._task_loop.post_task_after_delay(
                self._transceiver.process_wardmodem_response, 0,
                'wm_response_ok')
        self._task_loop.post_task_after_delay(
                self._transceiver.process_wardmodem_response, 0,
                'wm_response_sim_info_success', response2_clipped)
        self._task_loop.post_task_after_delay(
                self._transceiver.process_wardmodem_response, 0,
                'wm_response_ok')

        self._mox.ReplayAll()
        self._machine._check_length_and_respond(response1, '4')
        self._machine._check_length_and_respond(response2, '3')
        self._mox.VerifyAll()


    def test_check_length_and_respond_failure(self):
        """ Test _check_length_and_respond. """
        response = 'BOOoooo..'

        self._task_loop.post_task_after_delay(
                self._transceiver.process_wardmodem_response, 0,
                'wm_response_sim_info_error_too_long')
        self._task_loop.post_task_after_delay(
                self._transceiver.process_wardmodem_response, 0,
                'wm_response_ok')

        self._mox.ReplayAll()
        self._machine._check_length_and_respond(response, '24')
        self._mox.VerifyAll()


if __name__ == '__main__':
    logging.basicConfig(level=logging.DEBUG)
    unittest.main()
