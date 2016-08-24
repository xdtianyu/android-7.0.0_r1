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

from acts.logger import LoggerProxy
from acts.test_utils.bt.bt_test_utils import get_mac_address_of_generic_advertisement
from acts.test_utils.bt.GattEnum import GattCbErr
from acts.test_utils.bt.GattEnum import GattCbStrings
from acts.test_utils.bt.GattEnum import GattConnectionState
from acts.test_utils.bt.GattEnum import GattCharacteristic
from acts.test_utils.bt.GattEnum import GattDescriptor
from acts.test_utils.bt.GattEnum import GattService
from acts.test_utils.bt.GattEnum import GattConnectionPriority
import pprint
from queue import Empty
from contextlib import suppress

default_timeout = 10
log = LoggerProxy()


def setup_gatt_connection(cen_ad, mac_address, autoconnect):
    test_result = True
    gatt_callback = cen_ad.droid.gattCreateGattCallback()
    log.info("Gatt Connect to mac address {}.".format(mac_address))
    bluetooth_gatt = cen_ad.droid.gattClientConnectGatt(
        gatt_callback, mac_address, autoconnect)
    expected_event = GattCbStrings.GATT_CONN_CHANGE.value.format(gatt_callback)
    try:
        event = cen_ad.ed.pop_event(expected_event, default_timeout)
    except Empty:
        log.error(GattCbErr.GATT_CONN_CHANGE_ERR.value.format(expected_event))
        test_result = False
        return test_result, bluetooth_gatt, gatt_callback
    if event['data']['State'] != GattConnectionState.STATE_CONNECTED.value:
        log.info("Could not establish a connection to peripheral. Event "
                 "Details:".format(pprint.pformat(event)))
        test_result = False
    return test_result, bluetooth_gatt, gatt_callback


def disconnect_gatt_connection(cen_ad, bluetooth_gatt, gatt_callback):
    cen_ad.droid.gattClientDisconnect(bluetooth_gatt)
    expected_event = GattCbStrings.GATT_CONN_CHANGE.value.format(gatt_callback)
    try:
        event = cen_ad.ed.pop_event(expected_event, default_timeout)
    except Empty:
        log.error(GattCbErr.GATT_CONN_CHANGE_ERR.value.format(expected_event))
        return False
    if event['data']['State'] != GattConnectionState.STATE_DISCONNECTED.value:
        return False
    return True


def orchestrate_gatt_connection(cen_ad,
                                per_ad,
                                le=True,
                                mac_address=None,
                                autoconnect=False):
    adv_callback = None
    if mac_address is None:
        if le:
            mac_address, adv_callback = (
                get_mac_address_of_generic_advertisement(cen_ad, per_ad))
        else:
            mac_address = per_ad.droid.bluetoothGetLocalAddress()
            adv_callback = None
    test_result, bluetooth_gatt, gatt_callback = setup_gatt_connection(
        cen_ad, mac_address, autoconnect)
    if not test_result:
        log.error("Could not connect to peripheral.")
        return False, None, None
    return bluetooth_gatt, gatt_callback, adv_callback


def run_continuous_write_descriptor(cen_droid, cen_ed, per_droid, per_ed,
                                    gatt_server, gatt_server_callback,
                                    bluetooth_gatt, services_count,
                                    discovered_services_index):
    log.info("Starting continuous write")
    bt_device_id = 0
    status = 1
    offset = 1
    test_value = "1,2,3,4,5,6,7"
    test_value_return = "1,2,3"
    with suppress(Exception):
        for x in range(100000):
            for i in range(services_count):
                characteristic_uuids = (
                    cen_droid.gattClientGetDiscoveredCharacteristicUuids(
                        discovered_services_index, i))
                log.info(characteristic_uuids)
                for characteristic in characteristic_uuids:
                    descriptor_uuids = (
                        cen_droid.gattClientGetDiscoveredDescriptorUuids(
                            discovered_services_index, i, characteristic))
                    log.info(descriptor_uuids)
                    for descriptor in descriptor_uuids:
                        log.info("descriptor to be written {}".format(
                            descriptor))
                        cen_droid.gattClientDescriptorSetValue(
                            bluetooth_gatt, discovered_services_index, i,
                            characteristic, descriptor, test_value)
                        cen_droid.gattClientWriteDescriptor(
                            bluetooth_gatt, discovered_services_index, i,
                            characteristic, descriptor)
                        expected_event = GattCbStrings.DESC_WRITE_REQ.value.format(
                            gatt_server_callback)
                        try:
                            event = per_ed.pop_event(expected_event,
                                                     default_timeout)
                        except Empty:
                            log.error(
                                GattCbErr.DESC_WRITE_REQ_ERR.value.format(
                                    expected_event))
                            return False
                        request_id = event['data']['requestId']
                        found_value = event['data']['value']
                        if found_value != test_value:
                            log.error(
                                "Values didn't match. Found: {}, Expected: "
                                "{}".format(found_value, test_value))
                        per_droid.gattServerSendResponse(
                            gatt_server, bt_device_id, request_id, status,
                            offset, test_value_return)
                        expected_event = GattCbStrings.DESC_WRITE.value.format(
                            bluetooth_gatt)
                        try:
                            cen_ed.pop_event(expected_event, default_timeout)
                        except Empty:
                            log.error(GattCbErr.DESC_WRITE_ERR.value.format(
                                expected_event))
                            return False


def setup_characteristics_and_descriptors(droid):
    characteristic_input = [
        {
            'uuid': "aa7edd5a-4d1d-4f0e-883a-d145616a1630",
            'property': GattCharacteristic.PROPERTY_WRITE.value
            | GattCharacteristic.PROPERTY_WRITE_NO_RESPONSE.value,
            'permission': GattCharacteristic.PROPERTY_WRITE.value
        },
        {
            'uuid': "21c0a0bf-ad51-4a2d-8124-b74003e4e8c8",
            'property': GattCharacteristic.PROPERTY_NOTIFY.value
            | GattCharacteristic.PROPERTY_READ.value,
            'permission': GattCharacteristic.PERMISSION_READ.value
        },
        {
            'uuid': "6774191f-6ec3-4aa2-b8a8-cf830e41fda6",
            'property': GattCharacteristic.PROPERTY_NOTIFY.value
            | GattCharacteristic.PROPERTY_READ.value,
            'permission': GattCharacteristic.PERMISSION_READ.value
        },
    ]
    descriptor_input = [
        {
            'uuid': "aa7edd5a-4d1d-4f0e-883a-d145616a1630",
            'property': GattDescriptor.PERMISSION_READ.value
            | GattDescriptor.PERMISSION_WRITE.value,
        }, {
            'uuid': "76d5ed92-ca81-4edb-bb6b-9f019665fb32",
            'property': GattDescriptor.PERMISSION_READ.value
            | GattCharacteristic.PERMISSION_WRITE.value,
        }
    ]
    characteristic_list = setup_gatt_characteristics(droid,
                                                     characteristic_input)
    descriptor_list = setup_gatt_descriptors(droid, descriptor_input)
    return characteristic_list, descriptor_list


def setup_multiple_services(per_ad):
    per_droid, per_ed = per_ad.droid, per_ad.ed
    gatt_server_callback = per_droid.gattServerCreateGattServerCallback()
    gatt_server = per_droid.gattServerOpenGattServer(gatt_server_callback)
    characteristic_list, descriptor_list = (
        setup_characteristics_and_descriptors(per_droid))
    per_droid.gattServerCharacteristicAddDescriptor(characteristic_list[1],
                                                    descriptor_list[0])
    per_droid.gattServerCharacteristicAddDescriptor(characteristic_list[2],
                                                    descriptor_list[1])
    gattService = per_droid.gattServerCreateService(
        "00000000-0000-1000-8000-00805f9b34fb",
        GattService.SERVICE_TYPE_PRIMARY.value)
    gattService2 = per_droid.gattServerCreateService(
        "FFFFFFFF-0000-1000-8000-00805f9b34fb",
        GattService.SERVICE_TYPE_PRIMARY.value)
    gattService3 = per_droid.gattServerCreateService(
        "3846D7A0-69C8-11E4-BA00-0002A5D5C51B",
        GattService.SERVICE_TYPE_PRIMARY.value)
    for characteristic in characteristic_list:
        per_droid.gattServerAddCharacteristicToService(gattService,
                                                       characteristic)
    per_droid.gattServerAddService(gatt_server, gattService)
    expected_event = GattCbStrings.SERV_ADDED.value.format(
        gatt_server_callback)
    try:
        per_ed.pop_event(expected_event, default_timeout)
    except Empty:
        log.error(GattCbErr.SERV_ADDED_ERR.value.format(expected_event))
        return False
    for characteristic in characteristic_list:
        per_droid.gattServerAddCharacteristicToService(gattService2,
                                                       characteristic)
    per_droid.gattServerAddService(gatt_server, gattService2)
    try:
        per_ed.pop_event(expected_event, default_timeout)
    except Empty:
        log.error(GattCbErr.SERV_ADDED_ERR.value.format(expected_event))
        return False
    for characteristic in characteristic_list:
        per_droid.gattServerAddCharacteristicToService(gattService3,
                                                       characteristic)
    per_droid.gattServerAddService(gatt_server, gattService3)
    try:
        per_ed.pop_event(expected_event, default_timeout)
    except Empty:
        log.error(GattCbErr.SERV_ADDED_ERR.value.format(expected_event))
        return False
    return gatt_server_callback, gatt_server


def setup_characteristics_and_descriptors(droid):
    characteristic_input = [
        {
            'uuid': "aa7edd5a-4d1d-4f0e-883a-d145616a1630",
            'property': GattCharacteristic.PROPERTY_WRITE.value
            | GattCharacteristic.PROPERTY_WRITE_NO_RESPONSE.value,
            'permission': GattCharacteristic.PROPERTY_WRITE.value
        },
        {
            'uuid': "21c0a0bf-ad51-4a2d-8124-b74003e4e8c8",
            'property': GattCharacteristic.PROPERTY_NOTIFY.value
            | GattCharacteristic.PROPERTY_READ.value,
            'permission': GattCharacteristic.PERMISSION_READ.value
        },
        {
            'uuid': "6774191f-6ec3-4aa2-b8a8-cf830e41fda6",
            'property': GattCharacteristic.PROPERTY_NOTIFY.value
            | GattCharacteristic.PROPERTY_READ.value,
            'permission': GattCharacteristic.PERMISSION_READ.value
        },
    ]
    descriptor_input = [
        {
            'uuid': "aa7edd5a-4d1d-4f0e-883a-d145616a1630",
            'property': GattDescriptor.PERMISSION_READ.value
            | GattDescriptor.PERMISSION_WRITE.value,
        }, {
            'uuid': "76d5ed92-ca81-4edb-bb6b-9f019665fb32",
            'property': GattDescriptor.PERMISSION_READ.value
            | GattCharacteristic.PERMISSION_WRITE.value,
        }
    ]
    characteristic_list = setup_gatt_characteristics(droid,
                                                     characteristic_input)
    descriptor_list = setup_gatt_descriptors(droid, descriptor_input)
    return characteristic_list, descriptor_list


def setup_gatt_characteristics(droid, input):
    characteristic_list = []
    for item in input:
        index = droid.gattServerCreateBluetoothGattCharacteristic(
            item['uuid'], item['property'], item['permission'])
        characteristic_list.append(index)
    return characteristic_list


def setup_gatt_descriptors(droid, input):
    descriptor_list = []
    for item in input:
        index = droid.gattServerCreateBluetoothGattDescriptor(
            item['uuid'],
            item['property'], )
        descriptor_list.append(index)
    log.info("setup descriptor list: {}".format(descriptor_list))
    return descriptor_list
