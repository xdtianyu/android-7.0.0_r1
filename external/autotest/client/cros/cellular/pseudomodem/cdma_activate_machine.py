# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import gobject
import logging

import pm_errors
import state_machine

from autotest_lib.client.cros.cellular import mm1_constants

class CdmaActivateMachine(state_machine.StateMachine):
    """
    CdmaActivationMachine implements the asynchronous state updates for a fake
    OTASP "automatic activation".

    """
    def __init__(self, modem, return_cb, raise_cb):
        super(CdmaActivateMachine, self).__init__(modem)
        self._return_cb = return_cb
        self._raise_cb = raise_cb
        self._step_delay = 1


    def Cancel(self, message='Activation canceled.'):
        """ Cancel the CdmaActivateMachine. """
        logging.info('CdmaActivateMachine: Canceling activate.')
        super(CdmaActivateMachine, self).Cancel()
        state = self._modem.Get(mm1_constants.I_MODEM_CDMA, 'ActivationState')

        # If activated, return success.
        if state == mm1_constants.MM_MODEM_CDMA_ACTIVATION_STATE_ACTIVATED:
            logging.info('CdmaActivateMachine: Already activated. '
                         'Returning success.')
            if self._return_cb:
                self._return_cb()
            return

        self._modem.ChangeActivationState(
            mm1_constants.MM_MODEM_CDMA_ACTIVATION_STATE_NOT_ACTIVATED,
            pm_errors.MMCdmaActivationError.UNKNOWN)

        self._modem.cdma_activate_step = None

        if self._raise_cb:
            self._raise_cb(
                pm_errors.MMCoreError(pm_errors.MMCoreError.CANCELLED, message))


    def _GetDefaultHandler(self):
        return CdmaActivateMachine._HandleInvalidState


    def _ScheduleNextStep(self):
        def _DelayedStep():
            self.Step()
            return False
        gobject.timeout_add(self._step_delay * 1000, _DelayedStep)

    def _HandleInvalidState(self):
        state = self._modem.Get(mm1_constants.I_MODEM, 'State')
        message = 'Modem transitioned to invalid state: ' + \
            mm1_constants.ModemStateToString(state)
        logging.info('CdmaActivateMachine: ' + message)
        self.Cancel(message)
        return False


    def _StepFunction(self):
        state = self._modem.Get(mm1_constants.I_MODEM_CDMA, 'ActivationState')
        if state == mm1_constants.MM_MODEM_CDMA_ACTIVATION_STATE_NOT_ACTIVATED:
            return self._HandleNotActivated()
        if state == mm1_constants.MM_MODEM_CDMA_ACTIVATION_STATE_ACTIVATING:
            return self._HandleActivating()
        message = 'Modem is in invalid activation state: ' + state
        logging.error(message)
        self.Cancel(message)
        return False


    def _HandleNotActivated(self):
        logging.info('CdmaActivationMachine: Modem is NOT_ACTIVATED.')
        logging.info('CdmaActivationMachine: Setting state to ACTIVATING')
        self._modem.ChangeActivationState(
            mm1_constants.MM_MODEM_CDMA_ACTIVATION_STATE_ACTIVATING,
            pm_errors.MMCdmaActivationError.NONE)

        # Make the modem reset after 5 seconds.
        self._step_delay = 5
        return True


    def _HandleActivating(self):
        logging.info('CdmaActivationMachine: Modem is ACTIVATING.')
        logging.info('CdmaActivationMachine: Resetting modem.')
        self._modem.ChangeActivationState(
            mm1_constants.MM_MODEM_CDMA_ACTIVATION_STATE_ACTIVATED,
            pm_errors.MMCdmaActivationError.NONE)
        self._modem.Reset()
        self._modem.cdma_activate_step = None
        if self._return_cb:
            self._return_cb()
        return False


    def _GetModemStateFunctionMap(self):
        return {
            mm1_constants.MM_MODEM_STATE_REGISTERED:
                CdmaActivateMachine._StepFunction
        }


    def _ShouldStartStateMachine(self):
        if self._modem.cdma_activate_step and \
            self._modem.cdma_activate_step != self:
            # There is already an activate operation in progress.
            logging.error('There is already an ongoing activate operation.')
            raise pm_errors.MMCoreError(pm_errors.MMCoreError.IN_PROGRESS,
                                        "Activation already in progress.")


        if self._modem.cdma_activate_step is None:
            # There is no activate operation going on, cancelled or otherwise.
            state = self._modem.Get(mm1_constants.I_MODEM_CDMA,
                                    'ActivationState')
            if (state !=
                mm1_constants.MM_MODEM_CDMA_ACTIVATION_STATE_NOT_ACTIVATED):
                message = "Modem is not in state 'NOT_ACTIVATED'."
                logging.error(message)
                raise pm_errors.MMCoreError(pm_errors.MMCoreError.WRONG_STATE,
                                            message)

            state = self._modem.Get(mm1_constants.I_MODEM, 'State')
            if state != mm1_constants.MM_MODEM_STATE_REGISTERED:
                message = 'Modem cannot be activated if not in the ' \
                          'REGISTERED state.'
                logging.error(message)
                raise pm_errors.MMCoreError(pm_errors.MMCoreError.WRONG_STATE,
                                            message)

            self._modem.cdma_activate_step = self
        return True
