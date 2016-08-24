# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import state_machine

class NetworkOperatorMachine(state_machine.StateMachine):
    """
    This state machine controls the network operator that the modem connects to,
    and also the technology used to associate with the network.

    """

    OPERATOR_FORMAT_CODE = {
            'LONG_ALPHANUMERIC': '0',
            'SHORT_ALPHANUMERIC': '1',
            'NUMERIC': '2',
    }

    OPERATOR_FORMAT_STATE = {
            '0': 'LONG_ALPHANUMERIC',
            '1': 'SHORT_ALPHANUMERIC',
            '2': 'NUMERIC',
    }

    TECHNOLOGY_CODE = {
            'GSM': '0',
            'GSM_COMPAT' : '1',
            'UTRAN': '2',
            'GSM_EGPRS': '3',
            'UTRAN_HSDPA': '4',
            'UTRAN_HSUPA': '5',
            'UTRAN_HSDPA_HSUPA': '6',
            'E_UTRAN': '7',
    }

    def __init__(self, state, transceiver, modem_conf):
        """
        @param state: The GlobalState object shared by all state machines.

        @param transceiver: The ATTransceiver object to interact with.

        @param modem_conf: A ModemConfiguration object containing the
                configuration data for the current modem.

        """
        super(NetworkOperatorMachine, self).__init__(state, transceiver,
                                                     modem_conf)

        # Register all responses.
        self._add_response_function('wm_response_operator_name')
        self._add_response_function('wm_response_operator_name_none')


        # Load configuration
        self._operators = modem_conf.network_operators
        self._default_operator_index = modem_conf.network_operator_default_index

        # Initialize state
        self._state['operator_index'] = self._default_operator_index
        self._state['automatic_registration'] = 'TRUE'
        self._state['access_technology'] = self._extract_technology(
                self._default_operator_index)

    def get_well_known_name(self):
        """ Returns the well known name for this machine. """
        return 'network_operator_machine'


    # ##########################################################################
    # State machine API functions.
    def set_operator_format(self, format_code):
        """
        Sets the operator name reporting format.

        @param format_code: The format in which operator is reported.
                Type: str.  Valid arguments are ['0', '1', '2'], as specified by
                the AT specification.

        """
        operator_format = self.OPERATOR_FORMAT_STATE.get(format_code)
        if not operator_format:
            self._raise_runtime_error(self._tag_with_name(
                'Unknown operator format code |%s|' % format_code))

        self._update_state({'operator_format': operator_format})
        self._respond_ok()


    def set_operator_autoselect(self):
        """
        Enable automatic selection and registration to the default operator.

        """
        self._update_state({'automatic_registration': 'TRUE'})

        technology = self._extract_technology(self._default_operator_index)
        self._update_state({'operator_index': self._default_operator_index,
                            'access_technology': technology})

        self._task_loop.post_task_after_delay(
                self._registration_machine().register,
                0)

        self._respond_ok()


    def get_operator_name(self):
        """ Report the current operator selected. """
        operator_format = self._state['operator_format']
        format_code = self.OPERATOR_FORMAT_CODE[operator_format]
        operator_index = self._state['operator_index']

        if operator_index == self._state.INVALID_VALUE:
            # No operator has been selected.
            self._respond(self.wm_response_operator_name_none, 0,
                          format_code)
        else:
            self._respond(self.wm_response_operator_name, 0,
                          format_code,
                          self._extract_operator_name(operator_index,
                                                      operator_format))
        self._respond_ok()

    # ##########################################################################
    # API methods for other state machines.
    def get_current_technology_code(self):
        """
        Return the currently selected technology encoded correctly for the
        +COPS? response.

        """
        return self.TECHNOLOGY_CODE[self._state['access_technology']]


    # ##########################################################################
    # Helper methods.
    def _extract_operator_name(self, index, operator_format):
        try:
            operator = self._operators[index]
        except IndexError as e:
            self._raise_runtime_error(
                    'Not sufficient names in operator name list. '
                    'Requested index: %s. Operator list: %s' %
                    (index, str(self._operators)))
        return operator[operator_format]


    def _extract_technology(self, index):
        try:
            operator = self._operators[index]
        except IndexError as e:
            self._raise_runtime_error(
                    'Not sufficient names in operator name list. '
                    'Requested index: %d. Operator list: %s' %
                    (index, str(self._operators)))
        return operator['TECHNOLOGY']


    def _registration_machine(self):
        # This machine may not have been created when __init__ is executed.
        # Obtain a fresh handle everytime we want to use it.
        return self._transceiver.get_state_machine(
                'network_registration_machine')
