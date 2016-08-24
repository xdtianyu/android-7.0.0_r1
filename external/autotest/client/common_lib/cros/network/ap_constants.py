# Copyright (c) 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# These constants are used by the chaos_runner to raise TestError based on
# failure
AP_CONFIG_FAIL = 'AP configuration failed'
AP_PDU_DOWN = 'PDU is down'
AP_SECURITY_MISMATCH = 'AP security mismatch'
AP_SSID_NOTFOUND = 'SSID was not found'
WORK_CLI_CONNECT_FAIL = 'Work client was not able to connect to the AP'

# These constants are used by the AP configurator to indicate the type of
# configuration failure or success.
CONFIG_SUCCESS = 0
PDU_FAIL = 1
CONFIG_FAIL = 2

# These constants are used by the AP configurator to determine if this is
# a chaos vs clique test.
AP_TEST_TYPE_CHAOS = 1
AP_TEST_TYPE_CLIQUE = 2

# This constant is used by the chaos_runner to determine maximum APs/SSIDs
# that are up in the lab.
MAX_SSID_COUNT = 30 
MAX_SCAN_TIMEOUT = 30
