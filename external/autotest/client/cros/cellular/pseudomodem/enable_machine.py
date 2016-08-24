# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging

import pm_errors
import state_machine

from autotest_lib.client.cros.cellular import mm1_constants

class EnableMachine(state_machine.StateMachine):
    """
    EnableMachine handles the state transitions involved in bringing the modem
    to the ENABLED state.

    """
    def __init__(self, modem, return_cb, raise_cb):
        super(EnableMachine, self).__init__(modem)
        self.return_cb = return_cb
        self.raise_cb = raise_cb


    def Cancel(self):
        """ Overriden from superclass. """
        logging.info('EnableMachine: Canceling enable.')
        super(EnableMachine, self).Cancel()
        state = self._modem.Get(mm1_constants.I_MODEM, 'State')
        reason = mm1_constants.MM_MODEM_STATE_CHANGE_REASON_USER_REQUESTED
        if state == mm1_constants.MM_MODEM_STATE_ENABLING:
            logging.info('EnableMachine: Setting state to DISABLED.')
            self._modem.ChangeState(mm1_constants.MM_MODEM_STATE_DISABLED,
                                    reason)
        self._modem.enable_step = None
        if self.raise_cb:
            self.raise_cb(pm_errors.MMCoreError(
                    pm_errors.MMCoreError.CANCELLED, 'Operation cancelled'))


    def _HandleDisabledState(self):
        assert self._modem.disable_step is None
        assert self._modem.disconnect_step is None
        logging.info('EnableMachine: Setting power state to ON')
        self._modem.SetUInt32(mm1_constants.I_MODEM, 'PowerState',
                              mm1_constants.MM_MODEM_POWER_STATE_ON)
        logging.info('EnableMachine: Setting state to ENABLING')
        reason = mm1_constants.MM_MODEM_STATE_CHANGE_REASON_USER_REQUESTED
        self._modem.ChangeState(mm1_constants.MM_MODEM_STATE_ENABLING, reason)
        return True


    def _HandleEnablingState(self):
        assert self._modem.disable_step is None
        assert self._modem.disconnect_step is None
        logging.info('EnableMachine: Setting state to ENABLED.')
        reason = mm1_constants.MM_MODEM_STATE_CHANGE_REASON_USER_REQUESTED
        self._modem.ChangeState(mm1_constants.MM_MODEM_STATE_ENABLED, reason)
        return True


    def _HandleEnabledState(self):
        assert self._modem.disable_step is None
        assert self._modem.disconnect_step is None
        logging.info('EnableMachine: Searching for networks.')
        self._modem.enable_step = None
        if self.return_cb:
            self.return_cb()
        self._modem.RegisterWithNetwork()
        return False


    def _GetModemStateFunctionMap(self):
        return {
            mm1_constants.MM_MODEM_STATE_DISABLED:
                    EnableMachine._HandleDisabledState,
            mm1_constants.MM_MODEM_STATE_ENABLING:
                    EnableMachine._HandleEnablingState,
            mm1_constants.MM_MODEM_STATE_ENABLED:
                    EnableMachine._HandleEnabledState
        }


    def _ShouldStartStateMachine(self):
        state = self._modem.Get(mm1_constants.I_MODEM, 'State')
        # Return success if already enabled.
        if state >= mm1_constants.MM_MODEM_STATE_ENABLED:
            logging.info('Modem is already enabled. Nothing to do.')
            if self.return_cb:
                self.return_cb()
            return False
        if self._modem.enable_step and self._modem.enable_step != self:
            # There is already an enable operation in progress.
            # Note: ModemManager currently returns "WrongState" for this case.
            # The API suggests that "InProgress" should be returned, so that's
            # what we do here.
            logging.error('There is already an ongoing enable operation')
            if state == mm1_constants.MM_MODEM_STATE_ENABLING:
                message = 'Modem enable already in progress.'
            else:
                message = 'Modem enable has already been initiated' \
                          ', ignoring.'
            raise pm_errors.MMCoreError(pm_errors.MMCoreError.IN_PROGRESS,
                                        message)
        elif self._modem.enable_step is None:
            # There is no enable operation going on, cancelled or otherwise.
            if state != mm1_constants.MM_MODEM_STATE_DISABLED:
                message = 'Modem cannot be enabled if not in the DISABLED' \
                          ' state.'
                logging.error(message)
                raise pm_errors.MMCoreError(pm_errors.MMCoreError.WRONG_STATE,
                                            message)
            logging.info('Starting Enable')
            self._modem.enable_step = self
        return True
