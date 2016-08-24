# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import subprocess

import pm_errors
import state_machine

from autotest_lib.client.cros.cellular import mm1_constants

class ConnectMachine(state_machine.StateMachine):
    """
    ConnectMachine handles the state transitions involved in bringing the modem
    to the CONNECTED state.

    """
    def __init__(self, modem, properties, return_cb, raise_cb):
        super(ConnectMachine, self).__init__(modem)
        self.connect_props = properties
        self.return_cb = return_cb
        self.raise_cb = raise_cb
        self.enable_initiated = False
        self.register_initiated = False


    def Cancel(self):
        """ Overriden from superclass. """
        logging.info('ConnectMachine: Canceling connect.')
        super(ConnectMachine, self).Cancel()
        state = self._modem.Get(mm1_constants.I_MODEM, 'State')
        reason = mm1_constants.MM_MODEM_STATE_CHANGE_REASON_USER_REQUESTED
        if state == mm1_constants.MM_MODEM_STATE_CONNECTING:
            logging.info('ConnectMachine: Setting state to REGISTERED.')
            self._modem.ChangeState(mm1_constants.MM_MODEM_STATE_REGISTERED,
                                    reason)
        elif self.enable_initiated and self._modem.enable_step:
            self._modem.enable_step.Cancel()
        self._modem.connect_step = None


    def _HandleDisabledState(self):
        logging.info('ConnectMachine: Modem is DISABLED.')
        assert not self._modem.IsPendingEnable()
        if self.enable_initiated:
            message = 'ConnectMachine: Failed to enable modem.'
            logging.error(message)
            self.Cancel()
            self._modem.connect_step = None
            self.raise_cb(pm_errors.MMCoreError(
                    pm_errors.MMCoreError.FAILED, message))
            return False
        else:
            logging.info('ConnectMachine: Initiating Enable.')
            self.enable_initiated = True
            self._modem.Enable(True)

            # state machine will spin until modem gets enabled,
            # or if enable fails
            return True


    def _HandleEnablingState(self):
        logging.info('ConnectMachine: Modem is ENABLING.')
        assert self._modem.IsPendingEnable()
        logging.info('ConnectMachine: Waiting for enable.')
        return True


    def _HandleEnabledState(self):
        logging.info('ConnectMachine: Modem is ENABLED.')

        # Check to see if a register is going on, if not,
        # start register
        if self.register_initiated:
            message = 'ConnectMachine: Failed to register.'
            logging.error(message)
            self.Cancel()
            self._modem.connect_step = None
            self.raise_cb(pm_errors.MMCoreError(pm_errors.MMCoreError.FAILED,
                                                message))
            return False
        else:
            logging.info('ConnectMachine: Waiting for Register.')
            if not self._modem.IsPendingRegister():
                self._modem.RegisterWithNetwork(
                        "", self._return_cb, self._raise_cb)
            self.register_initiated = True
            return True


    def _HandleSearchingState(self):
        logging.info('ConnectMachine: Modem is SEARCHING.')
        logging.info('ConnectMachine: Waiting for modem to register.')
        assert self.register_initiated
        assert self._modem.IsPendingRegister()
        return True


    def _HandleRegisteredState(self):
        logging.info('ConnectMachine: Modem is REGISTERED.')
        assert not self._modem.IsPendingDisconnect()
        assert not self._modem.IsPendingEnable()
        assert not self._modem.IsPendingDisable()
        assert not self._modem.IsPendingRegister()
        logging.info('ConnectMachine: Setting state to CONNECTING.')
        reason = mm1_constants.MM_MODEM_STATE_CHANGE_REASON_USER_REQUESTED
        self._modem.ChangeState(mm1_constants.MM_MODEM_STATE_CONNECTING,
                                reason)
        return True


    def _GetBearerToActivate(self):
        # Import modem here to avoid circular imports.
        import modem
        bearer = None
        bearer_path = None
        bearer_props = {}
        for p, b in self._modem.bearers.iteritems():
            # assemble bearer props
            for key, val in self.connect_props.iteritems():
                if key in modem.ALLOWED_BEARER_PROPERTIES:
                    bearer_props[key] = val
            if (b.bearer_properties == bearer_props):
                logging.info('ConnectMachine: Found matching bearer.')
                bearer = b
                bearer_path = p
                break
        if bearer is None:
            assert bearer_path is None
            logging.info(('ConnectMachine: No matching bearer found, '
                'creating brearer with properties: ' +
                str(self.connect_props)))
            bearer_path = self._modem.CreateBearer(self.connect_props)

        return bearer_path


    def _HandleConnectingState(self):
        logging.info('ConnectMachine: Modem is CONNECTING.')
        assert not self._modem.IsPendingDisconnect()
        assert not self._modem.IsPendingEnable()
        assert not self._modem.IsPendingDisable()
        assert not self._modem.IsPendingRegister()
        try:
            bearer_path = self._GetBearerToActivate()
            self._modem.ActivateBearer(bearer_path)
            logging.info('ConnectMachine: Setting state to CONNECTED.')
            reason = mm1_constants.MM_MODEM_STATE_CHANGE_REASON_USER_REQUESTED
            self._modem.ChangeState(mm1_constants.MM_MODEM_STATE_CONNECTED,
                                    reason)
            self._modem.connect_step = None
            logging.info(
                'ConnectMachine: Returning bearer path: %s', bearer_path)
            self.return_cb(bearer_path)
        except (pm_errors.MMError, subprocess.CalledProcessError) as e:
            logging.error('ConnectMachine: Failed to connect: ' + str(e))
            self.raise_cb(e)
            self._modem.ChangeState(
                    mm1_constants.MM_MODEM_STATE_REGISTERED,
                    mm1_constants.MM_MODEM_STATE_CHANGE_REASON_UNKNOWN)
            self._modem.connect_step = None
        return False


    def _GetModemStateFunctionMap(self):
        return {
            mm1_constants.MM_MODEM_STATE_DISABLED:
                    ConnectMachine._HandleDisabledState,
            mm1_constants.MM_MODEM_STATE_ENABLING:
                    ConnectMachine._HandleEnablingState,
            mm1_constants.MM_MODEM_STATE_ENABLED:
                    ConnectMachine._HandleEnabledState,
            mm1_constants.MM_MODEM_STATE_SEARCHING:
                    ConnectMachine._HandleSearchingState,
            mm1_constants.MM_MODEM_STATE_REGISTERED:
                    ConnectMachine._HandleRegisteredState,
            mm1_constants.MM_MODEM_STATE_CONNECTING:
                    ConnectMachine._HandleConnectingState
        }


    def _ShouldStartStateMachine(self):
        if self._modem.connect_step and self._modem.connect_step != self:
            # There is already a connect operation in progress.
            message = 'There is already an ongoing connect operation.'
            logging.error(message)
            self.raise_cb(pm_errors.MMCoreError(
                    pm_errors.MMCoreError.IN_PROGRESS, message))
            return False
        elif self._modem.connect_step is None:
            # There is no connect operation going on, cancelled or otherwise.
            if self._modem.IsPendingDisable():
                message = 'Modem is currently being disabled. Ignoring ' \
                          'connect.'
                logging.error(message)
                self.raise_cb(
                    pm_errors.MMCoreError(pm_errors.MMCoreError.WRONG_STATE,
                                          message))
                return False
            state = self._modem.Get(mm1_constants.I_MODEM, 'State')
            if state == mm1_constants.MM_MODEM_STATE_CONNECTED:
                message = 'Modem is already connected.'
                logging.error(message)
                self.raise_cb(
                    pm_errors.MMCoreError(pm_errors.MMCoreError.CONNECTED,
                                          message))
                return False
            if state == mm1_constants.MM_MODEM_STATE_DISCONNECTING:
                assert self._modem.IsPendingDisconnect()
                message = 'Cannot connect while disconnecting.'
                logging.error(message)
                self.raise_cb(
                    pm_errors.MMCoreError(pm_errors.MMCoreError.WRONG_STATE,
                                          message))
                return False

            logging.info('Starting Connect.')
            self._modem.connect_step = self
        return True
