#!/usr/bin/env python

# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import common
import request_response

import logging
import mox
import unittest

import at_transceiver
import global_state
import modem_configuration
import task_loop


class RequestResponseTestCase(unittest.TestCase):
    """ Test fixture for RequestResponse class. """

    def setUp(self):
        self._mox = mox.Mox()

        # Prepare modem configuration to suit our needs.
        self._modem_conf = modem_configuration.ModemConfiguration()
        self._modem_conf.base_wm_request_response_map = {
                'AT1': 'AT1RESPONSE',
                'AT2': ('AT2OK', 'AT2ERROR'),
                'AT3': 'AT3RESPONSE'
        }
        self._modem_conf.plugin_wm_request_response_map = {
                'AT3': 'AT3RESPONSE_OVERRIDEN',
                'AT4': 'AT4RESPONSE',
                'AT5': ['AT5RESPONSE1', 'AT5RESPONSE2']
        }

        self._task_loop = self._mox.CreateMock(task_loop.TaskLoop)
        self._transceiver = self._mox.CreateMock(at_transceiver.ATTransceiver)
        self._machine = request_response.RequestResponse(
                global_state.GlobalState(),
                self._transceiver,
                self._modem_conf)

        # Mock out the task_loop
        self._machine._task_loop = self._task_loop

    def test_enabled_responses(self):
        """
        Responses for different configurations when machine is enabled.

        """
        self._machine.enable_machine()
        self._task_loop.post_task_after_delay(mox.IgnoreArg(),
                                              0,
                                              'wm_response_text_only',
                                              'AT1RESPONSE')
        self._task_loop.post_task_after_delay(mox.IgnoreArg(),
                                              0,
                                              'wm_response_text_only',
                                              'AT2OK')
        self._task_loop.post_task_after_delay(mox.IgnoreArg(),
                                              0,
                                              'wm_response_text_only',
                                              'AT3RESPONSE_OVERRIDEN')
        self._task_loop.post_task_after_delay(mox.IgnoreArg(),
                                              0,
                                              'wm_response_text_only',
                                              'AT4RESPONSE')
        self._task_loop.post_task_after_delay(mox.IgnoreArg(),
                                              0,
                                              'wm_response_text_only',
                                              'AT5RESPONSE1')
        self._task_loop.post_task_after_delay(mox.IgnoreArg(),
                                              0,
                                              'wm_response_text_only',
                                              'AT5RESPONSE2')
        self._task_loop.post_task_after_delay(mox.IgnoreArg(),
                                              0,
                                              'wm_response_ok')
        self._task_loop.post_task_after_delay(mox.IgnoreArg(),
                                              0,
                                              'wm_response_error')

        self._mox.ReplayAll()
        self._machine.act_on('AT1')
        self._machine.act_on('AT2')
        self._machine.act_on('AT3')
        self._machine.act_on('AT4')
        self._machine.act_on('AT5')
        self._machine.act_on('AT6')
        self._mox.VerifyAll()


    def test_disabled_responses(self):
        """
        Responses for different configurations when machine is enabled.

        """
        self._machine.disable_machine()

        self._task_loop.post_task_after_delay(mox.IgnoreArg(),
                                              0,
                                              'wm_response_error')
        self._task_loop.post_task_after_delay(mox.IgnoreArg(),
                                              0,
                                              'wm_response_text_only',
                                              'AT2ERROR')
        self._task_loop.post_task_after_delay(mox.IgnoreArg(),
                                              0,
                                              'wm_response_error')
        self._task_loop.post_task_after_delay(mox.IgnoreArg(),
                                              0,
                                              'wm_response_error')
        self._task_loop.post_task_after_delay(mox.IgnoreArg(),
                                              0,
                                              'wm_response_error')
        self._task_loop.post_task_after_delay(mox.IgnoreArg(),
                                              0,
                                              'wm_response_error')

        self._mox.ReplayAll()
        self._machine.act_on('AT1')
        self._machine.act_on('AT2')
        self._machine.act_on('AT3')
        self._machine.act_on('AT4')
        self._machine.act_on('AT5')
        self._machine.act_on('AT6')
        self._mox.VerifyAll()


if __name__ == '__main__':
    logging.basicConfig(level=logging.DEBUG)
    unittest.main()
