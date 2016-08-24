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

#pragma once

#include <rapidjson/document.h>
#include <service/common/bluetooth/binder/IBluetooth.h>
#include <service/common/bluetooth/binder/IBluetoothLowEnergy.h>
#include <tuple>

// BluetoothBinderFacade provides simple wrappers to call Binder apis.
// Each public function returns a tuple of the return type and an integer
// representing the pass/fail value of the function. The functions check to see
// if the API call is actually possible. If it is the function's tuple will
// contain the actual result and an integer that indicates the value passed. If
// the function is not possible then there will be a dummy return value in the
// first position of the tuple and the second value in the tuple indicates the
// value failed. Therefore it is up to the function to decide whether the
// expected api call is actually possible before calling it.
//
// TODO(tturney): Instead of using an integer in the tuple to represent
// pass/fail, create a class that properly represents the result of the
// function.
class BluetoothBinderFacade {
 public:
  BluetoothBinderFacade();
  std::tuple<bool, int> BluetoothBinderEnable();
  std::tuple<std::string, int> BluetoothBinderGetAddress();
  std::tuple<std::string, int> BluetoothBinderGetName();
  std::tuple<bool, int> BluetoothBinderInitInterface();
  std::tuple<bool, int> BluetoothBinderRegisterBLE();
  std::tuple<int, int> BluetoothBinderSetAdvSettings(
    int mode, int timeout_seconds, int tx_power_level, bool is_connectable);
  std::tuple<bool, int> BluetoothBinderSetName(std::string name);

 private:
  bool SharedValidator();
  // Returns a handle to the IBluetooth Binder from the Android ServiceManager.
  // Binder client code can use this to make calls to the service.
  android::sp<ipc::binder::IBluetooth> bt_iface;

  // Returns a handle to the IBluetoothLowEnergy Binder from the Android
  // ServiceManager. Binder client code can use this to make calls to the
  // service.
  android::sp<ipc::binder::IBluetoothLowEnergy> ble_iface;
  std::map<int, bluetooth::AdvertiseSettings> adv_settings_map;
  int adv_settings_count;
  int manu_data_count;
};
