# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging

import pm_errors
import state_machine

from autotest_lib.client.cros.cellular import mm1_constants

class RegisterMachine(state_machine.StateMachine):
    """
    RegisterMachine handles the state transitions involved in bringing the
    modem to the REGISTERED state.

    """
    def __init__(self, modem, operator_code="", return_cb=None, raise_cb=None):
        super(RegisterMachine, self).__init__(modem)
        self._networks = None
        self._operator_code = operator_code
        self._return_cb = return_cb
        self._raise_cb = raise_cb


    def Cancel(self):
        """ Overriden from superclass. """
        logging.info('RegisterMachine: Canceling register.')
        super(RegisterMachine, self).Cancel()
        state = self._modem.Get(mm1_constants.I_MODEM, 'State')
        reason = mm1_constants.MM_MODEM_STATE_CHANGE_REASON_USER_REQUESTED
        if state == mm1_constants.MM_MODEM_STATE_SEARCHING:
            logging.info('RegisterMachine: Setting state to ENABLED.')
            self._modem.ChangeState(mm1_constants.MM_MODEM_STATE_ENABLED,
                                    reason)
            self._modem.SetRegistrationState(
                mm1_constants.MM_MODEM_3GPP_REGISTRATION_STATE_IDLE)
        self._modem.register_step = None
        if self._raise_cb:
            self._raise_cb(
                    pm_errors.MMCoreError(pm_errors.MMCoreError.CANCELLED,
                                          'Cancelled'))


    def _HandleEnabledState(self):
        logging.info('RegisterMachine: Modem is ENABLED.')
        logging.info('RegisterMachine: Setting registration state '
                     'to SEARCHING.')
        self._modem.SetRegistrationState(
            mm1_constants.MM_MODEM_3GPP_REGISTRATION_STATE_SEARCHING)
        logging.info('RegisterMachine: Setting state to SEARCHING.')
        reason = mm1_constants.MM_MODEM_STATE_CHANGE_REASON_USER_REQUESTED
        self._modem.ChangeState(mm1_constants.MM_MODEM_STATE_SEARCHING, reason)
        logging.info('RegisterMachine: Starting network scan.')
        try:
            self._networks = self._modem.SyncScan()
        except pm_errors.MMError as e:
            self._modem.register_step = None
            logging.error('An error occurred during network scan: ' + str(e))
            self._modem.ChangeState(
                    mm1_constants.MM_MODEM_STATE_ENABLED,
                    mm1_constants.MODEM_STATE_CHANGE_REASON_UNKNOWN)
            self._modem.SetRegistrationState(
                    mm1_constants.MM_MODEM_3GPP_REGISTRATION_STATE_IDLE)
            if self._raise_cb:
                self._raise_cb(e)
            return False
        logging.info('RegisterMachine: Found networks: ' + str(self._networks))
        return True


    def _HandleSearchingState(self):
        logging.info('RegisterMachine: Modem is SEARCHING.')
        if not self._networks:
            logging.info('RegisterMachine: Scan returned no networks.')
            logging.info('RegisterMachine: Setting state to ENABLED.')
            self._modem.ChangeState(
                    mm1_constants.MM_MODEM_STATE_ENABLED,
                    mm1_constants.MM_MODEM_STATE_CHANGE_REASON_UNKNOWN)
            # TODO(armansito): Figure out the correct registration
            # state to transition to when no network is present.
            logging.info('RegisterMachine: Setting registration state '
                         'to IDLE.')
            self._modem.SetRegistrationState(
                    mm1_constants.MM_MODEM_3GPP_REGISTRATION_STATE_IDLE)
            self._modem.register_step = None
            if self._raise_cb:
                self._raise_cb(pm_errors.MMMobileEquipmentError(
                        pm_errors.MMMobileEquipmentError.NO_NETWORK,
                        'No networks were found to register.'))
            return False

        # Pick the last network in the list. Roaming networks will come before
        # the home network which makes the last item in the list the home
        # network.
        if self._operator_code:
            if not self._operator_code in self._modem.scanned_networks:
                if self._raise_cb:
                    self._raise_cb(pm_errors.MMCoreError(
                            pm_errors.MMCoreError.FAILED,
                            "Unknown network: " + self._operator_code))
                return False
            network = self._modem.scanned_networks[self._operator_code]
        else:
            network = self._networks[-1]
        logging.info(
            'RegisterMachine: Registering to network: ' + str(network))
        self._modem.SetRegistered(
                network['operator-code'],
                network['operator-long'])

        # The previous call should have set the state to REGISTERED.
        self._modem.register_step = None

        if self._return_cb:
            self._return_cb()
        return False


    def _GetModemStateFunctionMap(self):
        return {
            mm1_constants.MM_MODEM_STATE_ENABLED:
                    RegisterMachine._HandleEnabledState,
            mm1_constants.MM_MODEM_STATE_SEARCHING:
                    RegisterMachine._HandleSearchingState
        }


    def _ShouldStartStateMachine(self):
        if self._modem.register_step and self._modem.register_step != self:
            # There is already an ongoing register operation.
            message = 'Register operation already in progress.'
            logging.info(message)
            error = pm_errors.MMCoreError(pm_errors.MMCoreError.IN_PROGRESS,
                                          message)
            if self._raise_cb:
                self._raise_cb(error)
            else:
                raise error
        elif self._modem.register_step is None:
            # There is no register operation going on, canceled or otherwise.
            state = self._modem.Get(mm1_constants.I_MODEM, 'State')
            if state != mm1_constants.MM_MODEM_STATE_ENABLED:
                message = 'Cannot initiate register while in state %d, ' \
                          'state needs to be ENABLED.' % state
                error = pm_errors.MMCoreError(pm_errors.MMCoreError.WRONG_STATE,
                                              message)
                if self._raise_cb:
                    self._raise_cb(error)
                else:
                    raise error

            logging.info('Starting Register.')
            self._modem.register_step = self
        return True
