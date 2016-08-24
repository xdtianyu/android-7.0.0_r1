# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import cdma_activate_machine
import connect_cdma_machine
import connect_machine
import disable_machine
import disconnect_machine
import enable_machine
import pm_constants
import register_cdma_machine
import register_machine

class StateMachineFactory(object):
    """
    State machines are created by the |Modem| objects by calling methods from
    an object of type StateMachineFactory.

    To supply your own state machines, simply pass in your own subclass of
    |StateMachineFactory| that provides your implementations of the
    state machine.

    This default implementation allows tailoring the different state machines to
    be interactive as needed.

    """
    def __init__(self):
        self._bus = None
        self._interactive = set()


    def SetBus(self, bus):
        """
        Set the default dbus bus.

        @param bus: The dbus bus.

        """
        self._bus = bus


    def SetInteractiveAll(self):
        """
        Set all machines to be launched in interactive mode.

        All core pseudomodem machines should appear here. If you add a state
        machine to pseudomodem, please add it here so that tests can easily run
        it in interactive mode.

        """
        self.SetInteractive(pm_constants.STATE_MACHINE_CDMA_ACTIVATE)
        self.SetInteractive(pm_constants.STATE_MACHINE_CONNECT)
        self.SetInteractive(pm_constants.STATE_MACHINE_CONNECT_CDMA)
        self.SetInteractive(pm_constants.STATE_MACHINE_DISABLE)
        self.SetInteractive(pm_constants.STATE_MACHINE_DISCONNECT)
        self.SetInteractive(pm_constants.STATE_MACHINE_ENABLE)
        self.SetInteractive(pm_constants.STATE_MACHINE_REGISTER)
        self.SetInteractive(pm_constants.STATE_MACHINE_REGISTER_CDMA)


    def SetInteractive(self, machine_name):
        """
        Set the given machine to be launched in interative mode.

        @param machine_name: The name of the machine to be launched in
                interactive mode.

        """
        self._interactive.add(machine_name)


    def CreateMachine(self, machine_name, *args, **kwargs):
        """
        Create an instance of the given machine.

        @param machine_name: The name of the machine to be created. All
                supported machine names are exported as constants in the
                |pm_constants| module.
        @param *args, **kwargs: Arguments to pass to the machine constructor.
        @returns: A new instance of the deseried machine

        """
        if machine_name == pm_constants.STATE_MACHINE_CDMA_ACTIVATE:
            machine = cdma_activate_machine.CdmaActivateMachine(*args, **kwargs)
        elif machine_name == pm_constants.STATE_MACHINE_CONNECT:
            machine = connect_machine.ConnectMachine(*args, **kwargs)
        elif machine_name == pm_constants.STATE_MACHINE_CONNECT_CDMA:
            machine = connect_cdma_machine.ConnectCdmaMachine(*args, **kwargs)
        elif machine_name == pm_constants.STATE_MACHINE_DISABLE:
            machine = disable_machine.DisableMachine(*args, **kwargs)
        elif machine_name == pm_constants.STATE_MACHINE_DISCONNECT:
            machine = disconnect_machine.DisconnectMachine(*args, **kwargs)
        elif machine_name == pm_constants.STATE_MACHINE_ENABLE:
            machine = enable_machine.EnableMachine(*args, **kwargs)
        elif machine_name == pm_constants.STATE_MACHINE_REGISTER:
            machine = register_machine.RegisterMachine(*args, **kwargs)
        elif machine_name == pm_constants.STATE_MACHINE_REGISTER_CDMA:
            machine = register_cdma_machine.RegisterCdmaMachine(*args, **kwargs)
        else:
            # Reaching here is a non recoverable programming error.
            assert False

        if machine_name in self._interactive:
            machine.EnterInteractiveMode(self._bus)
        return machine
