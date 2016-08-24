# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging

import common
from autotest_lib.client.cros.cellular.pseudomodem import modem_3gpp
from autotest_lib.client.cros.cellular.pseudomodem import pm_constants
from autotest_lib.client.cros.cellular.pseudomodem import state_machine
from autotest_lib.client.cros.cellular.pseudomodem import state_machine_factory

class InteractiveStateMachineFactory(state_machine_factory.StateMachineFactory):
    """ Run relevant state machines in interactive mode. """
    def __init__(self):
        super(InteractiveStateMachineFactory, self).__init__()
        self.SetInteractive(pm_constants.STATE_MACHINE_ENABLE)
        self.SetInteractive(pm_constants.STATE_MACHINE_REGISTER)


class ScanMachine(state_machine.StateMachine):
    """
    Handle shill initiated 3GPP scan request.

    A simple machine that allows the test to hook into the Scan asynchronous
    call.

    """
    # State machine states.
    SCAN_STATE = 'Scan'
    DONE_STATE = 'Done'

    def __init__(self, modem):
        super(ScanMachine, self).__init__(modem)
        self._state = ScanMachine.SCAN_STATE


    def _HandleScanState(self):
        """ The only real state in this machine. """
        self._modem.DoScan()
        self._state = ScanMachine.DONE_STATE
        return True


    def _GetCurrentState(self):
        return self._state


    def _GetModemStateFunctionMap(self):
        return {
                ScanMachine.SCAN_STATE: ScanMachine._HandleScanState,
                # ScanMachine.DONE_STATE is the final state. So, no handler.
        }


    def _ShouldStartStateMachine(self):
        return True


class ScanStateMachineFactory(state_machine_factory.StateMachineFactory):
    """ Extend StateMachineFactory to create an interactive ScanMachine. """
    def ScanMachine(self, *args, **kwargs):
        """ Create a ScanMachine when needed in the modem. """
        machine = ScanMachine(*args, **kwargs)
        machine.EnterInteractiveMode(self._bus)
        return machine


class AsyncScanModem(modem_3gpp.Modem3gpp):
    """ 3GPP modem that uses ScanMachine for the Scan call. """
    def __init__(self):
        super(AsyncScanModem, self).__init__(
                state_machine_factory=ScanStateMachineFactory())


    def Scan(self, return_cb, raise_cb):
        """ Overriden from Modem3gpp. """
        # Stash away the scan_ok callback for when the Scan finishes.
        logging.debug('Network scan initiated.')
        self._scan_ok_callback = return_cb
        self._scan_failed_callback = raise_cb
        self._scan_machine = self._state_machine_factory.ScanMachine(self)
        self._scan_machine.Start()


    def DoScan(self):
        """ Defer to Modem3gpp to take the original |SyncScan| action. """
        # We're done scanning, drop |_scan_machine| reference.
        self._scan_machine = None
        try:
            scan_result = super(AsyncScanModem, self).SyncScan()
        except dbus.exceptions.DBusException as e:
            logging.warning('Network scan failed')
            self._scan_failed_callback(e)
            return

        logging.debug('Network scan completed.')
        self._scan_ok_callback(scan_result)
