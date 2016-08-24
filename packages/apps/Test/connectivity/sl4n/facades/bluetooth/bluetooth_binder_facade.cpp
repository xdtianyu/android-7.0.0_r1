//
//  Copyright (C) 2015 Google, Inc.
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at:
//
//  http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.
//

#include <base.h>
#include <base/at_exit.h>
#include <base/command_line.h>
#include <base/logging.h>
#include <base/macros.h>
#include <base/strings/string_split.h>
#include <base/strings/string_util.h>
#include <binder/IPCThreadState.h>
#include <binder/ProcessState.h>

#include "bluetooth_binder_facade.h"
#include <service/common/bluetooth/binder/IBluetooth.h>
#include <service/common/bluetooth/binder/IBluetoothCallback.h>
#include <service/common/bluetooth/binder/IBluetoothLowEnergy.h>
#include <service/common/bluetooth/low_energy_constants.h>
#include <tuple>

using android::sp;
using ipc::binder::IBluetooth;
using ipc::binder::IBluetoothLowEnergy;

std::atomic_bool ble_registering(false);
std::atomic_int ble_client_id(0);

bool BluetoothBinderFacade::SharedValidator() {
  if (bt_iface == NULL) {
    LOG(ERROR) << sl4n::kTagStr << " IBluetooth interface not initialized";
    return false;
  }
  if (!bt_iface->IsEnabled()) {
    LOG(ERROR) << sl4n::kTagStr << " IBluetooth interface not enabled";
    return false;
  }
  return true;
}

std::tuple<bool, int> BluetoothBinderFacade::BluetoothBinderEnable() {
  if (bt_iface == NULL) {
    LOG(ERROR) << sl4n::kTagStr << ": IBluetooth interface not enabled";
    return std::make_tuple(false, sl4n_error_codes::kFailInt);
  }
  bool result = bt_iface->Enable(false);
  if (!result) {
    LOG(ERROR) << sl4n::kTagStr << ": Failed to enable the Bluetooth service";
    return std::make_tuple(false, sl4n_error_codes::kPassInt);
  } else {
    return std::make_tuple(true, sl4n_error_codes::kPassInt);
  }
}

std::tuple<std::string, int> BluetoothBinderFacade::BluetoothBinderGetAddress() {
  if (!SharedValidator()) {
    return std::make_tuple(sl4n::kFailStr, sl4n_error_codes::kFailInt);
  }
  return std::make_tuple(bt_iface->GetAddress(), sl4n_error_codes::kPassInt);
}

std::tuple<std::string, int> BluetoothBinderFacade::BluetoothBinderGetName() {
  if (!SharedValidator()) {
    return std::make_tuple(sl4n::kFailStr,sl4n_error_codes::kFailInt);
  }
  std::string name = bt_iface->GetName();
  if (name.empty()) {
    LOG(ERROR) << sl4n::kTagStr << ": Failed to get device name";
    return std::make_tuple(sl4n::kFailStr, sl4n_error_codes::kFailInt);
  } else {
    return std::make_tuple(name, sl4n_error_codes::kPassInt);
  }
}

std::tuple<bool, int> BluetoothBinderFacade::BluetoothBinderSetName(
  std::string name) {

  if (!SharedValidator()) {
    return std::make_tuple(false, sl4n_error_codes::kFailInt);
  }
  bool result = bt_iface->SetName(name);
  if (!result) {
    LOG(ERROR) << sl4n::kTagStr << ": Failed to set device name";
    return std::make_tuple(false, sl4n_error_codes::kFailInt);
  }
  return std::make_tuple(true, sl4n_error_codes::kPassInt);
}

std::tuple<bool, int> BluetoothBinderFacade::BluetoothBinderInitInterface() {
  bt_iface = IBluetooth::getClientInterface();
  if(!bt_iface.get()) {
    LOG(ERROR) << sl4n::kTagStr <<
      ": Failed to initialize IBluetooth interface";
    return std::make_tuple(false, sl4n_error_codes::kFailInt);
  }
  return std::make_tuple(true, sl4n_error_codes::kPassInt);
}

std::tuple<bool, int> BluetoothBinderFacade::BluetoothBinderRegisterBLE() {
  // TODO (tturney): verify bt_iface initialized everywhere
  if (!SharedValidator()) {
    return std::make_tuple(false, sl4n_error_codes::kFailInt);
  }
  ble_iface = bt_iface->GetLowEnergyInterface();
  if(!ble_iface.get()) {
    LOG(ERROR) << sl4n::kTagStr << ": Failed to register BLE";
    return std::make_tuple(false, sl4n_error_codes::kFailInt);
  }
  return std::make_tuple(true, sl4n_error_codes::kPassInt);
}

std::tuple<int, int> BluetoothBinderFacade::BluetoothBinderSetAdvSettings(
  int mode, int timeout_seconds, int tx_power_level, bool is_connectable) {
  if (!SharedValidator()) {
    return std::make_tuple(false,sl4n_error_codes::kFailInt);
  }
  bluetooth::AdvertiseSettings::Mode adv_mode;
  switch (mode) {
    case sl4n_ble::kAdvSettingsModeLowPowerInt :
      adv_mode = bluetooth::AdvertiseSettings::Mode::MODE_LOW_POWER;
    case sl4n_ble::kAdvSettingsModeBalancedInt :
      adv_mode = bluetooth::AdvertiseSettings::Mode::MODE_BALANCED;
    case sl4n_ble::kAdvSettingsModeLowLatencyInt :
      adv_mode = bluetooth::AdvertiseSettings::Mode::MODE_LOW_LATENCY;
    default :
      LOG(ERROR) << sl4n::kTagStr <<
        ": Input mode is outside the accepted values";
      return std::make_tuple(
        sl4n::kFailedCounterInt, sl4n_error_codes::kFailInt);
  }

  base::TimeDelta adv_timeout = base::TimeDelta::FromSeconds(
    timeout_seconds);

  bluetooth::AdvertiseSettings::TxPowerLevel adv_tx_power_level;
  switch (tx_power_level) {
    case sl4n_ble::kAdvSettingsTxPowerLevelUltraLowInt: tx_power_level =
      bluetooth::AdvertiseSettings::TxPowerLevel::TX_POWER_LEVEL_ULTRA_LOW;
    case sl4n_ble::kAdvSettingsTxPowerLevelLowInt: tx_power_level =
      bluetooth::AdvertiseSettings::TxPowerLevel::TX_POWER_LEVEL_LOW;
    case sl4n_ble::kAdvSettingsTxPowerLevelMediumInt: tx_power_level =
      bluetooth::AdvertiseSettings::TxPowerLevel::TX_POWER_LEVEL_MEDIUM;
    case sl4n_ble::kAdvSettingsTxPowerLevelHighInt: tx_power_level =
      bluetooth::AdvertiseSettings::TxPowerLevel::TX_POWER_LEVEL_HIGH;
    default :
      LOG(ERROR) << sl4n::kTagStr <<
        ": Input tx power level is outside the accepted values";
      return std::make_tuple(
        sl4n::kFailedCounterInt, sl4n_error_codes::kFailInt);
  }

  bluetooth::AdvertiseSettings adv_settings = bluetooth::AdvertiseSettings(
    adv_mode, adv_timeout, adv_tx_power_level, is_connectable);
  adv_settings_map[adv_settings_count] = adv_settings;
  int adv_settings_id = adv_settings_count;
  adv_settings_count++;
  return std::make_tuple(adv_settings_id, sl4n_error_codes::kPassInt);
}

BluetoothBinderFacade::BluetoothBinderFacade() {
  adv_settings_count = 0;
  manu_data_count = 0;
}
