# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Setup wardmodem package root and other autotest paths.
import common

import state_machine

class NetworkRegistrationMachine(state_machine.StateMachine):
    """
    This state machine controls registration with the selected network operator.

    """
    REGISTRATION_STATUS_CODE = {
            'NOT_REGISTERED': '0',
            'HOME': '1',
            'SEARCHING': '2',
            'DENIED': '3',
            'UNKNOWN': '4',
            'ROAMING': '5',
            'SMS_ONLY_HOME': '6',
            'SMS_ONLY_ROAMING': '7',
            'EMERGENCY': '8',
            'NO_CSFB_HOME': '9',
            'NO_CSFB_ROAMING': '10',
    }

    def __init__(self, state, transceiver, modem_conf):
        """
        @param state: The GlobalState object shared by all state machines.

        @param transceiver: The ATTransceiver object to interact with.

        @param modem_conf: A ModemConfiguration object containing the
                configuration data for the current modem.

        """
        super(NetworkRegistrationMachine, self).__init__(state, transceiver,
                                                         modem_conf)

        # Register all responses used by this machine.
        self._add_response_function(
                'wm_response_network_registration_status_not_registered')
        self._add_response_function(
                'wm_response_network_registration_status_0')
        self._add_response_function(
                'wm_response_network_registration_status_1')
        self._add_response_function(
                'wm_response_network_registration_status_2')

        # Initialize state
        self._state['registration_change_message_verbosity'] = 0
        self.register()

    def get_well_known_name(self):
        """ Returns the well known name for this machine. """
        return 'network_registration_machine'


    # ##########################################################################
    # State machine API functions.
    def set_registration_change_message_verbosity(self, verbosity_code):
        """
        This sets the verbosity level of the messages sent when registration
        state changes, or when mm explicitly requests registration status.

        @param verbosity_code: A verbosity level in ['0', '1', '2']

        """
        try:
            verbosity = int(verbosity_code)
        except (TypeError, ValueError) as e:
            self._raise_runtime_error(self._tag_with_name(
                'Illegal verbosity code: |%s|' % verbosity_code))

        if verbosity < 0 or verbosity > 2:
            self._raise_runtime_error(self._tag_with_name(
                'Verbosity code must be in the range [0, 2]. Obtained: %d' %
                verbosity))

        self._update_state({'registration_change_message_verbosity': verbosity})
        self._respond_ok()


    def get_current_registration_status(self):
        """ Respond to queries about the current registration status. """
        registration_status = self._state['registration_status']
        access_technology = self._state['access_technology']
        verbosity = self._state['registration_change_message_verbosity']
        technology = self._state['access_technology']

        if registration_status == 'NOT_REGISTERED':
            self._respond(
                    self.wm_response_network_registration_status_not_registered,
                    0, verbosity)
        else:
            registration_status_code = self.REGISTRATION_STATUS_CODE[
                    registration_status]
            if verbosity == 0:
                self._respond(self.wm_response_network_registration_status_0,
                              0,
                              registration_status_code)
            elif verbosity == 1:
                self._respond(self.wm_response_network_registration_status_1,
                              0,
                              registration_status_code)
            else:
                assert verbosity == 2
                technology_code = \
                        self._operator_machine().get_current_technology_code()
                self._respond(self.wm_response_network_registration_status_2,
                              0,
                              registration_status_code,
                              self._get_tracking_area_code(),
                              self._get_cell_id(),
                              technology_code)
        self._respond_ok()


    # ##########################################################################
    # API methods for other state machines.
    def register(self):
        """
        Register to the currently selected network operator.

        This method is currently a stub.

        """
        self._update_state({'registration_status': 'HOME'})


    def deregister(self):
        """
        Deregister from the currently selected network operator.

        This method is currently a stub.

        """
        self._update_state({'registration_status': 'NOT_REGISTERED'})


    # ##########################################################################
    # Helper functions.
    def _get_tracking_area_code(self):
        # This is a 4 character hex string.
        # We currently return a fixed string. We might want to change this in
        # the future to simulate a moving device.
        return '1F00'


    def _get_cell_id(self):
        # This is a 8 character string corresponding to the cell id.
        # We currently return a fixed string. We might want to change this in
        # the future to simulate a moving device.
        return '79D803'


    def _operator_machine(self):
        # This machine may not have been created when __init__ is executed.
        # Obtain a fresh handle everytime we want to use it.
        return self._transceiver.get_state_machine('network_operator_machine')
