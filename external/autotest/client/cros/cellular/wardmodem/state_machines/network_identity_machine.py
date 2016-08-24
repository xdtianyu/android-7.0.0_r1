# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import math

# Setup wardmodem package root and other autotest paths.
import common

import state_machine

class NetworkIdentityMachine(state_machine.StateMachine):
    """
    Various identification numbers are used by the network to identify a
    customer's mobile equipment. These numbers are variously dependent on the
    SIM card, the network, the original subscriber, the geographical location
    etc.

    We serve queries regarding these identification numbers from this state
    machine, so that intuitive handles can be used to fake different
    combinations of the facets listed above.

    """

    def __init__(self, state, transceiver, modem_conf):
        """
        @param state: The GlobalState object shared by all state machines.

        @param transceiver: The ATTransceiver object to interact with.

        @param modem_conf: A ModemConfiguration object containing the
                configuration data for the current modem.

        """
        super(NetworkIdentityMachine, self).__init__(state, transceiver,
                                                     modem_conf)

        # Register all responses.
        self._add_response_function('wm_response_sim_info_success')
        self._add_response_function('wm_response_sim_info_error_too_long')
        self._add_response_function('wm_response_mdn')

        # Load configuration.
        self._mcc = modem_conf.network_identity_default_mcc
        self._mnc = modem_conf.network_identity_default_mnc
        self._msin = modem_conf.network_identity_default_msin
        self._mdn = modem_conf.network_identity_default_mdn


    def get_well_known_name(self):
        """ Returns the well known name for this machine. """
        return 'network_identity_machine'


    # ##########################################################################
    # State machine API functions.
    def read_imsi_from_modem(self):
        """
        Return the IMSI stored in the modem.

        This is currently a stub that returns the same IMSI as returned from
        SIM (@see read_sim_imsi). Note that the format of the returned value is
        different for these two functions. Some modems actually report two
        different IMSIs depending on where it is queried from. Implement this
        function if you want to simulate that behaviour.

        """
        self._respond_text(self._get_sim_imsi())
        self._respond_ok()


    def read_sim_admin_data(self, length_str):
        """
        Return administrative data read from the SIM.

        The administrative data contains, besides the length of the MNC, the
        state of the device -- is it under active development, undergoing some
        qualification process etc.
        We force the mode to be normal, and allow modems to provide different
        length of MNC if desired.

        @param length_str: (Type: str) Length of expected response.

        """
        answer = '0000000'
        answer += str(len(self._mnc))
        self._check_length_and_respond(answer, length_str)


    def read_sim_imsi(self, length_str):
        """
        Return the IMSI.

        The IMSI information is also available from request_response state
        machine. These two IMSI values can actually be different.

        @param length_str: (Type: str) Length of the expected response.

        """
        imsi = self._get_sim_imsi()
        # See ETSI TS 151.11 V14.4.0
        # The format of the returned string is
        # 'l' ['x' '<padded_imsi>']
        #
        # |l| is the number of bytes used by the 'x' and 'imsi' together.
        #
        # |x| encodes some parity checking, and can be '8' or '9' based on
        # parity. We currently set it to 9 (I haven't seen '8' in practice')
        #
        # |padded_imsi| is imsi padded with 'f' on the right to make the whole
        # thing 18 characters
        #
        # Finally the bytes within [] are represented LSB first, so the odd and
        # even characters in that string need to be swapped.
        x = '9'
        padded_imsi = x + imsi
        l = str(int(math.ceil(len(padded_imsi)/2)))
        while len(padded_imsi) < 16:
            padded_imsi += 'F'
        # Encode this number LSB first.
        switched_imsi = []
        for i in range(8):
            switched_imsi.append(padded_imsi[2*i+1])
            switched_imsi.append(padded_imsi[2*i])
        padded_imsi = ''.join(switched_imsi)

        # l should be 2 character long
        if len(l) != 2:
            l = '0' + l
        response = l + padded_imsi

        self._check_length_and_respond(response, length_str)


    def read_service_provider_name(self, length_str):
        """
        Return the name of the service provider encoded as hex string.

        Not Implemented. None of the modems we use return this information
        correctly right now.

        @param length_str: (Type: str) Length of the expected response.

        """
        # 'F' is used to pad unused bits. An all-'F' string means that we do not
        # have this information.
        # (TODO) pprabhu: Figure out the encoding scheme for this information if
        # a test needs it.
        provider_name = 'FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF'
        self._check_length_and_respond(provider_name, length_str)


    def read_mdn(self):
        """
        Return the Mobile Directory Number for the current service account.

        """
        self._respond(self.wm_response_mdn, 0, self._mdn)
        self._respond_ok()


    def _check_length_and_respond(self, response, length_str):
        """
        Checks that the length of the response is not less than the expected
        response. If it is more, the response is clipped.

        @param response: The string to respond with.

        @param length_str: (Type: str)The expected length in bytes. Note that
                the number of bytes in |response| is half the length of the
                string |response|, since each byte encodes two characters.

        """
        try:
            length = int(length_str)
        except ValueError as e:
            dbgstr = self._tag_with_name(
                    'Failed to detect expected length of the response. '
                    'Are you sure this is a number: |%s|' % length_str)
            self._respond_error()
            return

        # We require ceil(len(response)) number of bytes to encode the string
        # |response|, since each byte holds two characters.
        if 2 * length > len(response):
            dbgstr = self._tag_with_name(
                    'Response too short. Requested reponse length: %d, Actual '
                    'response length: %d' % (2 * length, len(response)))
            self._logger.warning(dbgstr)
            self._respond(self.wm_response_sim_info_error_too_long)
        else:
            dbgstr = self._tag_with_name(
                    'Response: |%s|, clipped to length %d: |%s|' %
                    (response, length, response[:2*length]))
            self._logger.debug(dbgstr)
            self._respond(self.wm_response_sim_info_success, 0,
                          response[:2*length])
        self._respond_ok()


    def _get_sim_imsi(self):
        return self._mcc + self._mnc + self._msin
