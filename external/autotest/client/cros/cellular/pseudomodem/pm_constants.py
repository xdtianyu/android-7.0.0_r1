# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""
This module provides bindings for pseudomodem DBus constants.

"""

# Unix user that that the modemmanager service runs as.
MM1_USER = 'modem'

# PseudoModemManager Testing Interfaces
TESTING_PATH = '/org/chromium/Pseudomodem/Testing'
I_TESTING = 'org.chromium.Pseudomodem.Testing'
# Interactive state machine interface.
I_TESTING_ISM = 'org.chromium.Pseudomodem.Testing.InteractiveStateMachine'

# Other constants used and exported by pseudomodem
# State machine names
STATE_MACHINE_CDMA_ACTIVATE = 'CdmaActivateMachine'
STATE_MACHINE_CONNECT = 'ConnectMachine'
STATE_MACHINE_CONNECT_CDMA = 'ConnectCdmaMachine'
STATE_MACHINE_DISABLE = 'DisableMachine'
STATE_MACHINE_DISCONNECT = 'DisconnectMachine'
STATE_MACHINE_ENABLE = 'EnableMachine'
STATE_MACHINE_REGISTER = 'RegisterMachine'
STATE_MACHINE_REGISTER_CDMA = 'RegisterCdmaMachine'

DEFAULT_TEST_NETWORK_PREFIX = 'Test Network'
