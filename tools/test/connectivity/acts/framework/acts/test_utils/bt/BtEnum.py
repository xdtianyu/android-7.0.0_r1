#/usr/bin/env python3.4
#
# Copyright (C) 2016 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License"); you may not
# use this file except in compliance with the License. You may obtain a copy of
# the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
# License for the specific language governing permissions and limitations under
# the License.

from enum import Enum


class BluetoothScanModeType(Enum):
    STATE_OFF = -1
    SCAN_MODE_NONE = 0
    SCAN_MODE_CONNECTABLE = 1
    SCAN_MODE_CONNECTABLE_DISCOVERABLE = 3


class BluetoothAdapterState(Enum):
    STATE_OFF = 10
    STATE_TURNING_ON = 11
    STATE_ON = 12
    STATE_TURNING_OFF = 13
    STATE_BLE_TURNING_ON = 14
    STATE_BLE_ON = 15
    STATE_BLE_TURNING_OFF = 16

class RfcommUuid(Enum):
    DEFAULT_UUID = "457807c0-4897-11df-9879-0800200c9a66"
