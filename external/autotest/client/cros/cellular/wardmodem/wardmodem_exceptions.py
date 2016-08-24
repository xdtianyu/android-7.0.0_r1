# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# This module is meant to keep the Exceptions raised by the modules in the
# wardmodem package together. Note that the modules can raise some system
# defined Exceptions besides these user defined Exceptions.

class WardModemRunTimeException(Exception):
    """
    Exception that indicate failure when the wardmodem is executing
    (accepting / responding to AT commands) should subclass from this.

    """
    pass

class WardModemSetupException(Exception):
    """
    Exception raised during the setup of wardmodem before the actual AT
    command sequence begins should raise this.

    """
    pass


# Exceptions thrown in the at_transceiver module.
class ATTransceiverException(WardModemRunTimeException):
    """
    Something went wrong in ATTranseiver while processing AT commands.

    """
    pass

# Exceptions raised in the state_machine module.
class StateMachineException(WardModemRunTimeException):
    """
    Something went wrong in StateMachine while processing commands.

    """
    pass
