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

import random
import pprint
import string
import queue
import threading
import time
from acts import utils

from contextlib2 import suppress
from subprocess import call

from acts.logger import LoggerProxy
from acts.test_utils.bt.BleEnum import AdvertiseSettingsAdvertiseMode
from acts.test_utils.bt.BleEnum import ScanSettingsCallbackType
from acts.test_utils.bt.BleEnum import ScanSettingsMatchMode
from acts.test_utils.bt.BleEnum import ScanSettingsMatchNum
from acts.test_utils.bt.BleEnum import ScanSettingsScanResultType
from acts.test_utils.bt.BleEnum import ScanSettingsScanMode
from acts.test_utils.bt.BleEnum import ScanSettingsReportDelaySeconds
from acts.test_utils.bt.BleEnum import AdvertiseSettingsAdvertiseType
from acts.test_utils.bt.BleEnum import AdvertiseSettingsAdvertiseTxPower
from acts.test_utils.bt.BleEnum import ScanSettingsMatchNum
from acts.test_utils.bt.BleEnum import ScanSettingsScanResultType
from acts.test_utils.bt.BleEnum import ScanSettingsScanMode
from acts.test_utils.bt.BtEnum import BluetoothScanModeType
from acts.test_utils.bt.BtEnum import RfcommUuid
from acts.utils import exe_cmd

default_timeout = 15
# bt discovery timeout
default_discovery_timeout = 3
log = LoggerProxy()

# Callback strings
scan_result = "BleScan{}onScanResults"
scan_failed = "BleScan{}onScanFailed"
batch_scan_result = "BleScan{}onBatchScanResult"
adv_fail = "BleAdvertise{}onFailure"
adv_succ = "BleAdvertise{}onSuccess"
bluetooth_off = "BluetoothStateChangedOff"
bluetooth_on = "BluetoothStateChangedOn"

# rfcomm test uuids
rfcomm_secure_uuid = "fa87c0d0-afac-11de-8a39-0800200c9a66"
rfcomm_insecure_uuid = "8ce255c0-200a-11e0-ac64-0800200c9a66"

advertisements_to_devices = {
    "Nexus 4": 0,
    "Nexus 5": 0,
    "Nexus 5X": 15,
    "Nexus 7": 0,
    "Nexus Player": 1,
    "Nexus 6": 4,
    "Nexus 6P": 4,
    "AOSP on Shamu": 4,
    "Nexus 9": 4,
    "Sprout": 10,
    "Micromax AQ4501": 10,
    "4560MMX": 10,
    "G Watch R": 1,
    "Gear Live": 1,
    "SmartWatch 3": 1,
    "Zenwatch": 1,
    "AOSP on Shamu": 4,
    "MSM8992 for arm64": 9,
    "LG Watch Urbane": 1,
    "Pixel C": 4,
    "angler": 4,
    "bullhead": 15,
}

batch_scan_supported_list = {
    "Nexus 4": False,
    "Nexus 5": False,
    "Nexus 7": False,
    "Nexus Player": True,
    "Nexus 6": True,
    "Nexus 6P": True,
    "Nexus 5X": True,
    "AOSP on Shamu": True,
    "Nexus 9": True,
    "Sprout": True,
    "Micromax AQ4501": True,
    "4560MMX": True,
    "Pixel C": True,
    "G Watch R": True,
    "Gear Live": True,
    "SmartWatch 3": True,
    "Zenwatch": True,
    "AOSP on Shamu": True,
    "MSM8992 for arm64": True,
    "LG Watch Urbane": True,
    "angler": True,
    "bullhead": True,
}


def generate_ble_scan_objects(droid):
    filter_list = droid.bleGenFilterList()
    scan_settings = droid.bleBuildScanSetting()
    scan_callback = droid.bleGenScanCallback()
    return filter_list, scan_settings, scan_callback


def generate_ble_advertise_objects(droid):
    advertise_callback = droid.bleGenBleAdvertiseCallback()
    advertise_data = droid.bleBuildAdvertiseData()
    advertise_settings = droid.bleBuildAdvertiseSettings()
    return advertise_callback, advertise_data, advertise_settings


def extract_string_from_byte_array(string_list):
    """Extract the string from array of string list
    """
    start = 1
    end = len(string_list) - 1
    extract_string = string_list[start:end]
    return extract_string


def extract_uuidlist_from_record(uuid_string_list):
    """Extract uuid from Service UUID List
    """
    start = 1
    end = len(uuid_string_list) - 1
    uuid_length = 36
    uuidlist = []
    while start < end:
        uuid = uuid_string_list[start:(start + uuid_length)]
        start += uuid_length + 1
        uuidlist.append(uuid)
    return uuidlist


def build_advertise_settings(droid, mode, txpower, type):
    """Build Advertise Settings
    """
    droid.bleSetAdvertiseSettingsAdvertiseMode(mode)
    droid.bleSetAdvertiseSettingsTxPowerLevel(txpower)
    droid.bleSetAdvertiseSettingsIsConnectable(type)
    settings = droid.bleBuildAdvertiseSettings()
    return settings


def setup_multiple_devices_for_bt_test(android_devices):
    log.info("Setting up Android Devices")
    # TODO: Temp fix for an selinux error.
    for ad in android_devices:
        ad.adb.shell("setenforce 0")
    threads = []
    try:
        for a in android_devices:
            thread = threading.Thread(target=reset_bluetooth, args=([[a]]))
            threads.append(thread)
            thread.start()
        for t in threads:
            t.join()

        for a in android_devices:
            d = a.droid
            setup_result = d.bluetoothSetLocalName(generate_id_by_size(4))
            if not setup_result:
                log.error("Failed to set device name.")
                return setup_result
            d.bluetoothDisableBLE()
            bonded_devices = d.bluetoothGetBondedDevices()
            for b in bonded_devices:
                d.bluetoothUnbond(b['address'])
        for a in android_devices:
            setup_result = a.droid.bluetoothConfigHciSnoopLog(True)
            if not setup_result:
                log.error("Failed to enable Bluetooth Hci Snoop Logging.")
                return setup_result
    except Exception as err:
        log.error("Something went wrong in multi device setup: {}".format(err))
        return False
    return setup_result


def reset_bluetooth(android_devices):
    """Resets bluetooth on the list of android devices passed into the function.
    :param android_devices: list of android devices
    :return: bool
    """
    for a in android_devices:
        droid, ed = a.droid, a.ed
        log.info("Reset state of bluetooth on device: {}".format(
            droid.getBuildSerial()))
        if droid.bluetoothCheckState() is True:
            droid.bluetoothToggleState(False)
            expected_bluetooth_off_event_name = bluetooth_off
            try:
                ed.pop_event(expected_bluetooth_off_event_name,
                             default_timeout)
            except Exception:
                log.error("Failed to toggle Bluetooth off.")
                return False
        # temp sleep for b/17723234
        time.sleep(3)
        droid.bluetoothToggleState(True)
        expected_bluetooth_on_event_name = bluetooth_on
        try:
            ed.pop_event(expected_bluetooth_on_event_name, default_timeout)
        except Exception:
            log.info("Failed to toggle Bluetooth on (no broadcast received).")
            # Try one more time to poke at the actual state.
            if droid.bluetoothCheckState() is True:
                log.info(".. actual state is ON")
                return True
            log.info(".. actual state is OFF")
            return False
    return True


def get_advanced_droid_list(android_devices):
    droid_list = []
    for a in android_devices:
        d, e = a.droid, a.ed
        model = d.getBuildModel()
        max_advertisements = 1
        batch_scan_supported = True
        if model in advertisements_to_devices.keys():
            max_advertisements = advertisements_to_devices[model]
        if model in batch_scan_supported_list.keys():
            batch_scan_supported = batch_scan_supported_list[model]
        role = {
            'droid': d,
            'ed': e,
            'max_advertisements': max_advertisements,
            'batch_scan_supported': batch_scan_supported
        }
        droid_list.append(role)
    return droid_list


def generate_id_by_size(
        size,
        chars=(
            string.ascii_lowercase + string.ascii_uppercase + string.digits)):
    return ''.join(random.choice(chars) for _ in range(size))


def cleanup_scanners_and_advertisers(scn_android_device, scan_callback_list,
                                     adv_android_device, adv_callback_list):
    """
    Try to gracefully stop all scanning and advertising instances.
    """
    scan_droid, scan_ed = scn_android_device.droid, scn_android_device.ed
    adv_droid = adv_android_device.droid
    try:
        for scan_callback in scan_callback_list:
            scan_droid.bleStopBleScan(scan_callback)
    except Exception as err:
        log.debug(
            "Failed to stop LE scan... reseting Bluetooth. Error {}".format(
                err))
        reset_bluetooth([scn_android_device])
    try:
        for adv_callback in adv_callback_list:
            adv_droid.bleStopBleAdvertising(adv_callback)
    except Exception as err:
        log.debug(
            "Failed to stop LE advertisement... reseting Bluetooth. Error {}".format(
                err))
        reset_bluetooth([adv_android_device])


def get_mac_address_of_generic_advertisement(scan_ad, adv_ad):
    adv_ad.droid.bleSetAdvertiseDataIncludeDeviceName(True)
    adv_ad.droid.bleSetAdvertiseSettingsAdvertiseMode(
        AdvertiseSettingsAdvertiseMode.ADVERTISE_MODE_LOW_LATENCY.value)
    adv_ad.droid.bleSetAdvertiseSettingsIsConnectable(True)
    adv_ad.droid.bleSetAdvertiseSettingsTxPowerLevel(
        AdvertiseSettingsAdvertiseTxPower.ADVERTISE_TX_POWER_HIGH.value)
    advertise_callback, advertise_data, advertise_settings = (
        generate_ble_advertise_objects(adv_ad.droid))
    adv_ad.droid.bleStartBleAdvertising(advertise_callback, advertise_data,
                                        advertise_settings)
    adv_ad.ed.pop_event("BleAdvertise{}onSuccess".format(advertise_callback),
                        default_timeout)
    filter_list = scan_ad.droid.bleGenFilterList()
    scan_settings = scan_ad.droid.bleBuildScanSetting()
    scan_callback = scan_ad.droid.bleGenScanCallback()
    scan_ad.droid.bleSetScanFilterDeviceName(
        adv_ad.droid.bluetoothGetLocalName())
    scan_ad.droid.bleBuildScanFilter(filter_list)
    scan_ad.droid.bleStartBleScan(filter_list, scan_settings, scan_callback)
    event = scan_ad.ed.pop_event(
        "BleScan{}onScanResults".format(scan_callback), default_timeout)
    mac_address = event['data']['Result']['deviceInfo']['address']
    scan_ad.droid.bleStopBleScan(scan_callback)
    return mac_address, advertise_callback


def get_device_local_info(droid):
    local_info_dict = {}
    local_info_dict['name'] = droid.bluetoothGetLocalName()
    local_info_dict['uuids'] = droid.bluetoothGetLocalUuids()
    return local_info_dict


def enable_bluetooth(droid, ed):
    if droid.bluetoothCheckState() is False:
        droid.bluetoothToggleState(True)
        if droid.bluetoothCheckState() is False:
            return False
    return True


def disable_bluetooth(droid, ed):
    if droid.bluetoothCheckState() is True:
        droid.bluetoothToggleState(False)
        if droid.bluetoothCheckState() is True:
            return False
    return True


def set_bt_scan_mode(ad, scan_mode_value):
    droid, ed = ad.droid, ad.ed
    if scan_mode_value == BluetoothScanModeType.STATE_OFF.value:
        disable_bluetooth(droid, ed)
        scan_mode = droid.bluetoothGetScanMode()
        reset_bluetooth([ad])
        if scan_mode != scan_mode_value:
            return False
    elif scan_mode_value == BluetoothScanModeType.SCAN_MODE_NONE.value:
        droid.bluetoothMakeUndiscoverable()
        scan_mode = droid.bluetoothGetScanMode()
        if scan_mode != scan_mode_value:
            return False
    elif scan_mode_value == BluetoothScanModeType.SCAN_MODE_CONNECTABLE.value:
        droid.bluetoothMakeUndiscoverable()
        droid.bluetoothMakeConnectable()
        scan_mode = droid.bluetoothGetScanMode()
        if scan_mode != scan_mode_value:
            return False
    elif (scan_mode_value ==
          BluetoothScanModeType.SCAN_MODE_CONNECTABLE_DISCOVERABLE.value):
        droid.bluetoothMakeDiscoverable()
        scan_mode = droid.bluetoothGetScanMode()
        if scan_mode != scan_mode_value:
            return False
    else:
        # invalid scan mode
        return False
    return True


def set_device_name(droid, name):
    droid.bluetoothSetLocalName(name)
    time.sleep(2)
    droid_name = droid.bluetoothGetLocalName()
    if droid_name != name:
        return False
    return True


def check_device_supported_profiles(droid):
    profile_dict = {}
    profile_dict['hid'] = droid.bluetoothHidIsReady()
    profile_dict['hsp'] = droid.bluetoothHspIsReady()
    profile_dict['a2dp'] = droid.bluetoothA2dpIsReady()
    profile_dict['avrcp'] = droid.bluetoothAvrcpIsReady()
    return profile_dict


def log_energy_info(droids, state):
    return_string = "{} Energy info collection:\n".format(state)
    for d in droids:
        with suppress(Exception):
            if (d.getBuildModel() != "Nexus 5" or
                    d.getBuildModel() != "Nexus 4"):

                description = ("Device: {}\tEnergyStatus: {}\n".format(
                    d.getBuildSerial(),
                    d.bluetoothGetControllerActivityEnergyInfo(1)))
                return_string = return_string + description
    return return_string


def pair_pri_to_sec(pri_droid, sec_droid):
    # Enable discovery on sec_droid so that pri_droid can find it.
    # The timeout here is based on how much time it would take for two devices
    # to pair with each other once pri_droid starts seeing devices.
    log.info(
        "Bonding device {} to {}".format(pri_droid.bluetoothGetLocalAddress(),
                                         sec_droid.bluetoothGetLocalAddress()))
    sec_droid.bluetoothMakeDiscoverable(default_timeout)
    target_address = sec_droid.bluetoothGetLocalAddress()
    log.debug("Starting paring helper on each device")
    pri_droid.bluetoothStartPairingHelper()
    sec_droid.bluetoothStartPairingHelper()
    log.info("Primary device starting discovery and executing bond")
    result = pri_droid.bluetoothDiscoverAndBond(target_address)
    # Loop until we have bonded successfully or timeout.
    end_time = time.time() + default_timeout
    log.info("Verifying devices are bonded")
    while time.time() < end_time:
        bonded_devices = pri_droid.bluetoothGetBondedDevices()
        bonded = False
        for d in bonded_devices:
            if d['address'] == target_address:
                log.info("Successfully bonded to device")
                return True
        time.sleep(1)
    # Timed out trying to bond.
    log.debug("Failed to bond devices.")
    return False


def take_btsnoop_logs(android_devices, testcase, testname):
    for a in android_devices:
        take_btsnoop_log(a, testcase, testname)


def take_btsnoop_log(ad, testcase, test_name):
    """Grabs the btsnoop_hci log on a device and stores it in the log directory
    of the test class.

    If you want grab the btsnoop_hci log, call this function with android_device
    objects in on_fail. Bug report takes a relative long time to take, so use
    this cautiously.

    Params:
      test_name: Name of the test case that triggered this bug report.
      android_device: The android_device instance to take bugreport on.
    """
    test_name = "".join(x for x in test_name if x.isalnum())
    with suppress(Exception):
        serial = ad.droid.getBuildSerial()
        device_model = ad.droid.getBuildModel()
        device_model = device_model.replace(" ", "")
        out_name = ','.join((test_name, device_model, serial))
        snoop_path = ad.log_path + "/BluetoothSnoopLogs"
        utils.create_dir(snoop_path)
        cmd = ''.join(("adb -s ", serial, " pull /sdcard/btsnoop_hci.log ",
                       snoop_path + '/' + out_name, ".btsnoop_hci.log"))
        testcase.log.info("Test failed, grabbing the bt_snoop logs on {} {}."
                          .format(device_model, serial))
        exe_cmd(cmd)


def kill_bluetooth_process(ad):
    log.info("Killing Bluetooth process.")
    pid = ad.adb.shell(
        "ps | grep com.android.bluetooth | awk '{print $2}'").decode('ascii')
    call(["adb -s " + ad.serial + " shell kill " + pid],
            shell=True)


def rfcomm_connect(ad, device_address):
    rf_client_ad = ad
    log.debug("Performing RFCOMM connection to {}".format(device_address))
    try:
        ad.droid.bluetoothRfcommConnect(device_address)
    except Exception as err:
        log.error("Failed to connect: {}".format(err))
        ad.droid.bluetoothRfcommCloseSocket()
        return
    finally:
        return
    return


def rfcomm_accept(ad):
    rf_server_ad = ad
    log.debug("Performing RFCOMM accept")
    try:
        ad.droid.bluetoothRfcommAccept(RfcommUuid.DEFAULT_UUID.value,
            default_timeout)
    except Exception as err:
        log.error("Failed to accept: {}".format(err))
        ad.droid.bluetoothRfcommCloseSocket()
        return
    finally:
        return
    return


def write_read_verify_data(client_ad, server_ad, msg, binary=False):
    log.info("Write message.")
    try:
        if binary:
            client_ad.droid.bluetoothRfcommWriteBinary(msg)
        else:
            client_ad.droid.bluetoothRfcommWrite(msg)
    except Exception as err:
        log.error("Failed to write data: {}".format(err))
        return False
    log.info("Read message.")
    try:
        if binary:
            read_msg = server_ad.droid.bluetoothRfcommReadBinary().rstrip("\r\n")
        else:
            read_msg = server_ad.droid.bluetoothRfcommRead()
    except Exception as err:
        log.error("Failed to read data: {}".format(err))
        return False
    log.info("Verify message.")
    if msg != read_msg:
        log.error("Mismatch! Read: {}, Expected: {}".format(
            read_msg, msg))
        return False
    return True
