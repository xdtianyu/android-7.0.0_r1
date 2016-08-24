# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging

import pm_errors
import state_machine

from autotest_lib.client.cros.cellular import mm1_constants

class DisableMachine(state_machine.StateMachine):
    """
    DisableMachine handles the state transitions involved in bringing the modem
    to the DISABLED state.

    """
    def __init__(self, modem, return_cb, raise_cb):
        super(DisableMachine, self).__init__(modem)
        self.return_cb = return_cb
        self.raise_cb = raise_cb


    def _HandleConnectedState(self):
        logging.info('DisableMachine: Modem is CONNECTED.')
        assert self._modem.connect_step is None
        # TODO(armansito): Pass a different raise_cb here to handle
        # disconnect failure
        logging.info('DisableMachine: Starting Disconnect.')
        self._modem.Disconnect(mm1_constants.ROOT_PATH, DisableMachine.Step,
                               DisableMachine.Step, self)
        return True


    def _HandleConnectingState(self):
        logging.info('DisableMachine: Modem is CONNECTING.')
        assert self._modem.connect_step
        logging.info('DisableMachine: Canceling connect.')
        self._modem.connect_step.Cancel()
        return True


    def _HandleDisconnectingState(self):
        logging.info('DisableMachine: Modem is DISCONNECTING.')
        assert self._modem.disconnect_step
        logging.info('DisableMachine: Waiting for disconnect.')
        # wait until disconnect ends
        return True


    def _HandleRegisteredState(self):
        logging.info('DisableMachine: Modem is REGISTERED.')
        assert not self._modem.IsPendingRegister()
        assert not self._modem.IsPendingEnable()
        assert not self._modem.IsPendingConnect()
        assert not self._modem.IsPendingDisconnect()
        self._modem.UnregisterWithNetwork()
        logging.info('DisableMachine: Setting state to DISABLING.')
        reason = mm1_constants.MM_MODEM_STATE_CHANGE_REASON_USER_REQUESTED
        self._modem.ChangeState(mm1_constants.MM_MODEM_STATE_DISABLING, reason)
        return True


    def _HandleSearchingState(self):
        logging.info('DisableMachine: Modem is SEARCHING.')
        assert self._modem.register_step
        assert not self._modem.IsPendingEnable()
        assert not self._modem.IsPendingConnect()
        logging.info('DisableMachine: Canceling register.')
        self._modem.register_step.Cancel()
        return True


    def _HandleEnabledState(self):
        logging.info('DisableMachine: Modem is ENABLED.')
        assert not self._modem.IsPendingRegister()
        assert not self._modem.IsPendingEnable()
        assert not self._modem.IsPendingConnect()
        logging.info('DisableMachine: Setting state to DISABLING.')
        reason = mm1_constants.MM_MODEM_STATE_CHANGE_REASON_USER_REQUESTED
        self._modem.ChangeState(mm1_constants.MM_MODEM_STATE_DISABLING, reason)
        return True


    def _HandleDisablingState(self):
        logging.info('DisableMachine: Modem is DISABLING.')
        assert not self._modem.IsPendingRegister()
        assert not self._modem.IsPendingEnable()
        assert not self._modem.IsPendingConnect()
        assert not self._modem.IsPendingDisconnect()
        logging.info('DisableMachine: Setting state to DISABLED.')
        reason = mm1_constants.MM_MODEM_STATE_CHANGE_REASON_USER_REQUESTED
        self._modem.ChangeState(mm1_constants.MM_MODEM_STATE_DISABLED, reason)
        self._modem.disable_step = None
        if self.return_cb:
            self.return_cb()
        return False


    def _GetModemStateFunctionMap(self):
        return {
            mm1_constants.MM_MODEM_STATE_CONNECTED:
                    DisableMachine._HandleConnectedState,
            mm1_constants.MM_MODEM_STATE_CONNECTING:
                    DisableMachine._HandleConnectingState,
            mm1_constants.MM_MODEM_STATE_DISCONNECTING:
                    DisableMachine._HandleDisconnectingState,
            mm1_constants.MM_MODEM_STATE_REGISTERED:
                    DisableMachine._HandleRegisteredState,
            mm1_constants.MM_MODEM_STATE_SEARCHING:
                    DisableMachine._HandleSearchingState,
            mm1_constants.MM_MODEM_STATE_ENABLED:
                    DisableMachine._HandleEnabledState,
            mm1_constants.MM_MODEM_STATE_DISABLING:
                    DisableMachine._HandleDisablingState
        }


    def _ShouldStartStateMachine(self):
        if self._modem.disable_step and self._modem.disable_step != self:
            # There is already a disable operation in progress.
            message = 'Modem disable already in progress.'
            logging.info(message)
            raise pm_errors.MMCoreError(pm_errors.MMCoreError.IN_PROGRESS,
                                        message)
        elif self._modem.disable_step is None:
            # There is no disable operation going in, cancelled or otherwise.
            state = self._modem.Get(mm1_constants.I_MODEM, 'State')
            if state == mm1_constants.MM_MODEM_STATE_DISABLED:
                # The reason we're not raising an error here is that
                # shill will make multiple successive calls to disable
                # but WON'T check for raised errors, which causes
                # problems. Treat this particular case as success.
                logging.info('Already in a disabled state. Ignoring.')
                if self.return_cb:
                    self.return_cb()
                return False

            invalid_states = [
                mm1_constants.MM_MODEM_STATE_FAILED,
                mm1_constants.MM_MODEM_STATE_UNKNOWN,
                mm1_constants.MM_MODEM_STATE_INITIALIZING,
                mm1_constants.MM_MODEM_STATE_LOCKED
            ]
            if state in invalid_states:
                raise pm_errors.MMCoreError(
                        pm_errors.MMCoreError.WRONG_STATE,
                        ('Modem disable cannot be initiated while in state'
                         ' %u.') % state)
            if self._modem.connect_step:
                logging.info('There is an ongoing Connect, canceling it.')
                self._modem.connect_step.Cancel()
            if self._modem.register_step:
                logging.info('There is an ongoing Register, canceling it.')
                self._modem.register_step.Cancel()
            if self._modem.enable_step:
                # This needs to be done here, because the case where an enable
                # cycle has been initiated but it hasn't triggered any state
                # transitions yet would not be detected in a state handler.
                logging.info('There is an ongoing Enable, canceling it.')
                logging.info('This should bring the modem to a disabled state.'
                             ' DisableMachine will not start.')
                self._modem.enable_step.Cancel()
                assert self._modem.Get(mm1_constants.I_MODEM, 'State') == \
                    mm1_constants.MM_MODEM_STATE_DISABLED
            if self._modem.Get(mm1_constants.I_MODEM, 'State') == \
                    mm1_constants.MM_MODEM_STATE_DISABLED:
                if self.return_cb:
                    self.return_cb()
                return False

            logging.info('Starting Disable.')
            self._modem.disable_step = self
        return True
