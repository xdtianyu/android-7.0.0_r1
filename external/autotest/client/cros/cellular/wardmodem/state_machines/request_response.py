# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import pprint

# Setup wardmodem package root and other autotest paths.
import common

import state_machine


class RequestResponse(state_machine.StateMachine):
    """
    The trivial state machine that implements all request-response interaction.

    A lot of interaction with the modem is simple request-response.
    There is a |request_response| GlobalState component. If it is |ENABLED|,
    this machine sends the expected responses. If it is |DISABLED|, the machine
    always responds with the appropriate error.

    """

    def __init__(self, state, transceiver, modem_conf):
        """
        @param state: The GlobalState object shared by all state machines.

        @param transceiver: The ATTransceiver object to interact with.

        @param modem_conf: A ModemConfiguration object containing the
                configuration data for the current modem.

        """
        super(RequestResponse, self).__init__(state, transceiver, modem_conf)

        self._load_request_response_map(modem_conf)

        # Start off enabled.
        self.enable_machine()


    def get_well_known_name(self):
        """ Returns the well known name for this machine. """
        return 'request_response'


    # ##########################################################################
    # API that could be used by other state machines.
    def enable_machine(self):
        """ Enable the machine so that it responds to queries. """
        self._state['request_response_enabled'] = 'TRUE'


    def disable_machine(self):
        """ Disable machine so that it will only respond with error. """
        self._state['request_response_enabled'] = 'FALSE'


    # ##########################################################################
    # State machine API functions.
    def act_on(self, atcom):
        """
        Reply to the AT command |atcom| by following the request_response map.

        This is the implementation of state-less responses given by the modem.
        There is only one macro level handle to turn off the whole state
        machine. No other state is referenced / maintained.

        @param atcom: The AT command in query.

        """
        response = self._responses.get(atcom, None)
        if not response:
            self._respond_error()
            return

        # If |response| is a tuple, it is of the form |(response_ok ,
        # response_error)|. Otherwise, it is of the form |response_ok|.
        #
        # |response_ok| is either a list of str, or str
        # Let's say |response_ok| is ['response1', 'response2'], then we must
        # respond with ['response1', 'response2', 'OK']
        # Let's say |response_ok| is 'send_this'. Then we must respond with
        # 'send_this' (Without the trailing 'OK')
        #
        # |response_error| is str.
        #
        # Having such a flexible specification for response allows a very
        # natural definition of responses (@see base.conf). But we must be
        # careful with type checking, which we do next.
        if type(response) is tuple:
            assert len(response) == 2
            response_ok = response[0]
            response_error = response[1]
        else:
            response_ok = response
            response_error = None

        assert type(response_ok) is list or type(response_ok) is str
        if type(response_ok) is list:
            for part in response_ok:
                assert type(part) is str

        if response_error:
            assert type(response_error) is str

        # Now construct the actual response.
        if self._is_enabled():
            if type(response_ok) is str:
                self._respond_with_text(response_ok)
            else:
                for part in response_ok:
                    self._respond_with_text(part)
                self._respond_ok()
        else:
            if response_error:
                self._respond_with_text(response_error)
            else:
                self._respond_error()


    # #########################################################################
    # Helper functions.
    def _is_enabled(self):
        return self._state['request_response_enabled'] == 'TRUE'


    def _load_request_response_map(self, modem_conf):
        self._responses = modem_conf.base_wm_request_response_map
        # Now update specific entries with those overriden by the plugin.
        for key, value in modem_conf.plugin_wm_request_response_map.items():
            self._responses[key] = value
        self._logger.info('Loaded request-response map.')
        self._logger.debug(pprint.pformat(self._responses))
