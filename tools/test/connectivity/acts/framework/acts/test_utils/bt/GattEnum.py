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


class GattCbErr(Enum):
    CHAR_WRITE_REQ_ERR = "Characteristic Write Request event not found. Expected {}"
    CHAR_WRITE_ERR = "Characteristic Write event not found. Expected {}"
    DESC_WRITE_REQ_ERR = "Descriptor Write Request event not found. Expected {}"
    DESC_WRITE_ERR = "Descriptor Write event not found. Expected {}"
    RD_REMOTE_RSSI_ERR = "Read Remote RSSI event not found. Expected {}"
    GATT_SERV_DISC_ERR = "GATT Services Discovered event not found. Expected {}"
    SERV_ADDED_ERR = "Service Added event not found. Expected {}"
    MTU_CHANGED_ERR = "MTU Changed event not found. Expected {}"
    GATT_CONN_CHANGE_ERR = "GATT Connection Changed event not found. Expected {}"


class GattCbStrings(Enum):
    CHAR_WRITE_REQ = "GattServer{}onCharacteristicWriteRequest"
    CHAR_WRITE = "GattConnect{}onCharacteristicWrite"
    DESC_WRITE_REQ = "GattServer{}onDescriptorWriteRequest"
    DESC_WRITE = "GattConnect{}onDescriptorWrite"
    RD_REMOTE_RSSI = "GattConnect{}onReadRemoteRssi"
    GATT_SERV_DISC = "GattConnect{}onServicesDiscovered"
    SERV_ADDED = "GattServer{}onServiceAdded"
    MTU_CHANGED = "GattConnect{}onMtuChanged"
    GATT_CONN_CHANGE = "GattConnect{}onConnectionStateChange"


class GattConnectionState(Enum):
    STATE_DISCONNECTED = 0
    STATE_CONNECTING = 1
    STATE_CONNECTED = 2
    STATE_DISCONNECTING = 3


class GattCharacteristic(Enum):
    PROPERTY_BROADCAST = 0x01
    PROPERTY_READ = 0x02
    PROPERTY_WRITE_NO_RESPONSE = 0x04
    PROPERTY_WRITE = 0x08
    PROPERTY_NOTIFY = 0x10
    PROPERTY_INDICATE = 0x20
    PROPERTY_SIGNED_WRITE = 0x40
    PROPERTY_EXTENDED_PROPS = 0x80
    PERMISSION_READ = 0x01
    PERMISSION_READ_ENCRYPTED = 0x02
    PERMISSION_READ_ENCRYPTED_MITM = 0x04
    PERMISSION_WRITE = 0x10
    PERMISSION_WRITE_ENCRYPTED = 0x20
    PERMISSION_WRITE_ENCRYPTED_MITM = 0x40
    PERMISSION_WRITE_SIGNED = 0x80
    PERMISSION_WRITE_SIGNED_MITM = 0x100
    WRITE_TYPE_DEFAULT = 0x02
    WRITE_TYPE_NO_RESPONSE = 0x01
    WRITE_TYPE_SIGNED = 0x04
    FORMAT_UINT8 = 0x11
    FORMAT_UINT16 = 0x12
    FORMAT_UINT32 = 0x14
    FORMAT_SINT8 = 0x21
    FORMAT_SINT16 = 0x22
    FORMAT_SINT32 = 0x24
    FORMAT_SFLOAT = 0x32
    FORMAT_FLOAT = 0x34


class GattDescriptor(Enum):
    ENABLE_NOTIFICATION_VALUE = [0x01, 0x00]
    ENABLE_INDICATION_VALUE = [0x02, 0x00]
    DISABLE_NOTIFICATION_VALUE = [0x00, 0x00]
    PERMISSION_READ = 0x01
    PERMISSION_READ_ENCRYPTED = 0x02
    PERMISSION_READ_ENCRYPTED_MITM = 0x04
    PERMISSION_WRITE = 0x10
    PERMISSION_WRITE_ENCRYPTED = 0x20
    PERMISSION_WRITE_ENCRYPTED_MITM = 0x40
    PERMISSION_WRITE_SIGNED = 0x80
    PERMISSION_WRITE_SIGNED_MITM = 0x100


class GattService(Enum):
    SERVICE_TYPE_PRIMARY = 0
    SERVICE_TYPE_SECONDARY = 1


class GattConnectionPriority(Enum):
    CONNECTION_PRIORITY_BALANCED = 0
    CONNECTION_PRIORITY_HIGH = 1
    CONNECTION_PRIORITY_LOW_POWER = 2


class MtuSize(Enum):
    MIN = 23
    MAX = 217


class BluetoothGatt(Enum):
    GATT_SUCCESS = 0
    GATT_FAILURE = 0x101
