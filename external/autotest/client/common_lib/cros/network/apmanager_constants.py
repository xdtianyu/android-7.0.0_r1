# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# These constants are configuration keys for configuring AP service through
# apmanager's DBus interface. These are names of DBus properties on
# org.chromium.apmanager.Config DBus interface.
CONFIG_BRIDGE_INTERFACE = 'BridgeInterface'
CONFIG_CHANNEL = 'Channel'
CONFIG_HIDDEN_NETWORK = 'HiddenNetwork'
CONFIG_HW_MODE = 'HwMode'
CONFIG_INTERFACE_NAME = 'InterfaceName'
CONFIG_OPERATION_MODE = 'OperationMode'
CONFIG_PASSPHRASE = 'Passphrase'
CONFIG_SECURITY_MODE = 'SecurityMode'
CONFIG_SERVER_ADDRESS_INDEX = 'ServerAddressIndex'
CONFIG_SSID = 'Ssid'

# Configuration value definitions
OPERATION_MODE_BRIDGE = 'bridge'
OPERATION_MODE_SERVER = 'server'

# Default configuration values.
DEFAULT_CHANNEL_NUMBER = 6
