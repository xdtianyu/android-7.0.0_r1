# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging

import pm_errors
import register_machine

from autotest_lib.client.cros.cellular import mm1_constants

class RegisterCdmaMachine(register_machine.RegisterMachine):
    """
    RegisterCdmaMachine handles the CDMA specific state transitions involved in
    bringing the modem to the REGISTERED state.

    """
    def Cancel(self):
        """
        Cancel the current machine.

        Overwritten from parent class.
        """
        logging.info('RegisterCdmaMachine: Canceling register.')
        super(RegisterCdmaMachine, self).Cancel()
        state = self._modem.Get(mm1_constants.I_MODEM, 'State')
        reason = mm1_constants.MM_MODEM_STATE_CHANGE_REASON_USER_REQUESTED
        if state == mm1_constants.MM_MODEM_STATE_SEARCHING:
            logging.info('RegisterCdmaMachine: Setting state to ENABLED.')
            self._modem.ChangeState(mm1_constants.MM_MODEM_STATE_ENABLED,
                                    reason)
            self._modem.SetRegistrationState(
                mm1_constants.MM_MODEM_CDMA_REGISTRATION_STATE_UNKNOWN)
        self._modem.register_step = None
        if self._raise_cb:
            self._raise_cb(
                    pm_errors.MMCoreError(pm_errors.MMCoreError.CANCELLED,
                                          'Cancelled'))


    def _GetModemStateFunctionMap(self):
        return {
            mm1_constants.MM_MODEM_STATE_ENABLED:
                    RegisterCdmaMachine._HandleEnabledState,
            mm1_constants.MM_MODEM_STATE_SEARCHING:
                    RegisterCdmaMachine._HandleSearchingState
        }


    def _HandleEnabledState(self):
        logging.info('RegisterCdmaMachine: Modem is ENABLED.')
        logging.info('RegisterCdmaMachine: Setting state to SEARCHING.')
        self._modem.ChangeState(
                mm1_constants.MM_MODEM_STATE_SEARCHING,
                mm1_constants.MM_MODEM_STATE_CHANGE_REASON_USER_REQUESTED)
        return True


    def _HandleSearchingState(self):
        logging.info('RegisterCdmaMachine: Modem is SEARCHING.')
        network = self._modem.GetHomeNetwork()
        if not network:
            logging.info('RegisterCdmaMachine: No network available.')
            logging.info('RegisterCdmaMachine: Setting state to ENABLED.')
            self._modem.ChangeState(mm1_constants.MM_MODEM_STATE_ENABLED,
                mm1_constants.MM_MODEM_STATE_CHANGE_REASON_UNKNOWN)
            if self._raise_cb:
                self._raise_cb(
                        pm_errors.MMMobileEquipmentError(
                                pm_errors.MMMobileEquipmentError.NO_NETWORK,
                                'No networks were found to register.'))
            self._modem.register_step = None
            return False

        logging.info(
            'RegisterMachineCdma: Registering to network: ' + str(network))
        logging.info('RegisterMachineCdma: Setting state to REGISTERED.')
        self._modem.SetRegistered(network)
        self._modem.ChangeState(
                mm1_constants.MM_MODEM_STATE_REGISTERED,
                mm1_constants.MM_MODEM_STATE_CHANGE_REASON_USER_REQUESTED)
        self._modem.register_step = None
        if self._return_cb:
            self._return_cb()
        return False
