# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import random

# Setup wardmodem package root and other autotest paths.
import common

from state_machines import call_machine

class CallMachineE362(call_machine.CallMachine):
    """
    E362 specific extension to the call machine.

    """
    def __init__(self, state, transceiver, modem_conf):
        """
        @param state: The GlobalState object shared by all state machines.

        @param transceiver: The ATTransceiver object to interact with.

        @param modem_conf: A modem configuration object that contains
                configuration data for different state machines.

        @raises: SetupException if we attempt to create an instance of a machine
                that has not been completely specified (see
                get_well_known_name).

        """
        super(CallMachineE362, self).__init__(state, transceiver, modem_conf)

        # Add all wardmodem response functions used by this machine.
        self._add_response_function('wm_response_qmi_call_result_success')
        self._add_response_function('wm_response_qmi_call_state_connected')
        self._add_response_function('wm_response_qmi_call_state_disconnected')
        self._add_response_function('wm_response_qmi_call_end_reason')
        self._add_response_function('wm_response_qmi_call_duration')

        random.seed()
        self._call_duration = 0


    # ##########################################################################
    # State machine API functions.
    def connect_call(self):
        """
        Connect a call with the registered network.

        Overrides CallMachine.connect_call

        """
        super(CallMachineE362, self).connect_call()
        self._update_state({'call_end_reason': 0})
        self._call_duration = 0


    def disconnect_call(self):
        """
        Disconnect an active call with the registered network.

        Overrides CallMachine.disconnect_call

        """
        super(CallMachineE362, self).disconnect_call()
        self._call_duration = 0


    def get_qmi_call_status(self):
        """
        Get the current call status as returned by the QMI call to E362.

        """
        if self._state['call_status'] == 'CONNECTED':
            # We randomly increment the call duration every time a status check
            # is made in a continuing call.
            self._call_duration += random.randint(0, 20)

            self._respond(self.wm_response_qmi_call_result_success)
            self._respond(self.wm_response_qmi_call_state_connected)
            self._respond(self.wm_response_qmi_call_end_reason, 0,
                          str(self._state['call_end_reason']))
            self._respond(self.wm_response_qmi_call_duration, 0,
                          self._call_duration)
            self._respond(self.wm_response_qmi_call_result_success)
            self._respond(self.wm_response_qmi_call_state_connected)
            self._respond(self.wm_response_qmi_call_end_reason, 0, '0')
            self._respond(self.wm_response_qmi_call_duration, 0,
                          self._call_duration)
            self._respond_ok()
        else:
            self._respond(self.wm_response_qmi_call_result_success)
            self._respond(self.wm_response_qmi_call_state_disconnected)
            self._respond(self.wm_response_qmi_call_end_reason, 0,
                          str(self._state['call_end_reason']))
            self._respond(self.wm_response_qmi_call_duration, 0,
                          self._call_duration)
            self._respond(self.wm_response_qmi_call_result_success)
            self._respond(self.wm_response_qmi_call_state_disconnected)
            self._respond(self.wm_response_qmi_call_end_reason, 0, '0')
            self._respond(self.wm_response_qmi_call_duration, 0,
                          self._call_duration)
            self._respond_ok()
