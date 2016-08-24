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

#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>
#include <map>
#include <string>
#include <stdio.h>
#include <tuple>

#include <base.h>
#include <facades/bluetooth/bluetooth_binder_facade.h>
#include <service/common/bluetooth/binder/IBluetooth.h>
#include <utils/command_receiver.h>
#include <utils/common_utils.h>

using android::sp;
using ipc::binder::IBluetooth;

typedef std::map<std::string, MFP> function_map;
function_map* _funcMap = NULL;
BluetoothBinderFacade bt_binder;

void _clean_result(rapidjson::Document &doc) {
  doc.RemoveMember(sl4n::kMethodStr);
  doc.RemoveMember(sl4n::kParamsStr);
}

void initiate(rapidjson::Document &doc) {
  doc.AddMember(sl4n::kStatusStr, sl4n::kSuccessStr, doc.GetAllocator());
}

// Begin Wrappers ... I'm the hiphopopotamus my lyrics are bottomless...
void bluetooth_binder_get_local_name_wrapper(rapidjson::Document &doc) {
  int expected_param_size = 0;
  if (!CommonUtils::IsParamLengthMatching(doc, expected_param_size)) {
    return;
  }
  //check for kfailedstr or NULL???
  std::string name;
  int error_code;
  std::tie(name, error_code) = bt_binder.BluetoothBinderGetName();
  if (error_code == sl4n_error_codes::kFailInt) {
    doc.AddMember(sl4n::kResultStr, sl4n::kFailStr, doc.GetAllocator());
    doc.AddMember(sl4n::kErrorStr, sl4n::kFailStr, doc.GetAllocator());
    return;
  }
  rapidjson::Value tmp;
  tmp.SetString(name.c_str(), doc.GetAllocator());
  doc.AddMember(sl4n::kResultStr, tmp, doc.GetAllocator());
  doc.AddMember(sl4n::kErrorStr, NULL, doc.GetAllocator());
  return;
}

void bluetooth_binder_init_interface_wapper(rapidjson::Document &doc) {
  int expected_param_size = 0;
  if (!CommonUtils::IsParamLengthMatching(doc, expected_param_size)) {
    return;
  }
  bool init_result;
  int error_code;
  std::tie(init_result, error_code) = bt_binder.BluetoothBinderInitInterface();
  if (error_code == sl4n_error_codes::kFailInt) {
    doc.AddMember(sl4n::kErrorStr, sl4n::kFailStr, doc.GetAllocator());
  } else {
    doc.AddMember(sl4n::kErrorStr, NULL, doc.GetAllocator());
  }
  doc.AddMember(sl4n::kResultStr, init_result, doc.GetAllocator());
  return;
}

void bluetooth_binder_set_local_name_wrapper(rapidjson::Document &doc) {
  int expected_param_size = 1;
  if (!CommonUtils::IsParamLengthMatching(doc, expected_param_size)) {
    return;
  }
  std::string name;
  if (!doc[sl4n::kParamsStr][0].IsString()) {
    LOG(ERROR) << sl4n::kTagStr << ": Expected String input for name";
    doc.AddMember(sl4n::kResultStr, false, doc.GetAllocator());
    doc.AddMember(sl4n::kErrorStr, sl4n::kFailStr, doc.GetAllocator());
    return;
  } else {
    name = doc[sl4n::kParamsStr][0].GetString();
  }
  bool set_result;
  int error_code;
  std::tie(set_result, error_code) = bt_binder.BluetoothBinderSetName(name);
  if (error_code == sl4n_error_codes::kFailInt) {
    doc.AddMember(sl4n::kResultStr, false, doc.GetAllocator());
    doc.AddMember(sl4n::kErrorStr, sl4n::kFailStr, doc.GetAllocator());
  } else {
    doc.AddMember(sl4n::kResultStr, set_result, doc.GetAllocator());
    doc.AddMember(sl4n::kErrorStr, NULL, doc.GetAllocator());
  }
  return;
}

void bluetooth_binder_get_local_address_wrapper(rapidjson::Document &doc) {
  int expected_param_size = 0;
  if (!CommonUtils::IsParamLengthMatching(doc, expected_param_size)) {
    return;
  }
  //check for kfailedstr or NULL???
  std::string address;
  int error_code;
  std::tie(address, error_code) = bt_binder.BluetoothBinderGetAddress();
  if (error_code == sl4n_error_codes::kFailInt) {
    doc.AddMember(sl4n::kResultStr, sl4n::kFailStr, doc.GetAllocator());
    doc.AddMember(sl4n::kErrorStr, sl4n::kFailStr, doc.GetAllocator());
  } else {
    rapidjson::Value tmp;
    tmp.SetString(address.c_str(), doc.GetAllocator());
    doc.AddMember(sl4n::kResultStr, tmp, doc.GetAllocator());
    doc.AddMember(sl4n::kErrorStr, NULL, doc.GetAllocator());
  }
  return;
}

void bluetooth_binder_enable_wrapper(rapidjson::Document &doc) {
  int expected_param_size = 0;
  if (!CommonUtils::IsParamLengthMatching(doc, expected_param_size)) {
    return;
  }
  bool enable_result;
  int error_code;
  std::tie(enable_result, error_code) = bt_binder.BluetoothBinderEnable();
  if (error_code == sl4n_error_codes::kFailInt) {
    doc.AddMember(sl4n::kResultStr, false, doc.GetAllocator());
    doc.AddMember(sl4n::kErrorStr, sl4n::kFailStr, doc.GetAllocator());
  } else {
    doc.AddMember(sl4n::kResultStr, enable_result, doc.GetAllocator());
    doc.AddMember(sl4n::kErrorStr, NULL, doc.GetAllocator());
  }
}

void bluetooth_binder_register_ble_wrapper(rapidjson::Document &doc) {
  int expected_param_size = 0;
  if (!CommonUtils::IsParamLengthMatching(doc, expected_param_size)) {
    return;
  }
  bool register_result;
  int error_code;
  std::tie(register_result, error_code) =
    bt_binder.BluetoothBinderRegisterBLE();
  if (error_code == sl4n_error_codes::kFailInt) {
    doc.AddMember(sl4n::kResultStr, false, doc.GetAllocator());
    doc.AddMember(sl4n::kErrorStr, sl4n::kFailStr, doc.GetAllocator());
  } else {
    doc.AddMember(sl4n::kResultStr, register_result, doc.GetAllocator());
    doc.AddMember(sl4n::kErrorStr, NULL, doc.GetAllocator());
  }
}

void bluetooth_binder_set_adv_settings_wrapper(rapidjson::Document &doc) {
  int expected_param_size = 4;
  if (!CommonUtils::IsParamLengthMatching(doc, expected_param_size)) {
    return;
  }
  int mode;
  int timeout_seconds;
  int tx_power_level;
  bool is_connectable;
  // TODO(tturney) Verify inputs better
  if (!doc[sl4n::kParamsStr][0].IsInt()) {
    LOG(ERROR) << sl4n::kTagStr << ": Expected Int input for mode";
    doc.AddMember(sl4n::kResultStr, false, doc.GetAllocator());
    doc.AddMember(sl4n::kErrorStr, sl4n::kInvalidParamStr, doc.GetAllocator());
    return;
  } else {
    mode = doc[sl4n::kParamsStr][0].GetInt();
  }
  if (!doc[sl4n::kParamsStr][1].IsInt()) {
    LOG(ERROR) << sl4n::kTagStr << ": Expected Int input for timeout";
    doc.AddMember(sl4n::kResultStr, false, doc.GetAllocator());
    doc.AddMember(sl4n::kErrorStr, sl4n::kInvalidParamStr, doc.GetAllocator());
    return;
  } else {
    timeout_seconds = doc[sl4n::kParamsStr][1].GetInt();
  }
  if (!doc[sl4n::kParamsStr][2].IsInt()) {
    LOG(ERROR) << sl4n::kTagStr << ": Expected Int input for tx power level";
    doc.AddMember(sl4n::kResultStr, false, doc.GetAllocator());
    doc.AddMember(sl4n::kErrorStr, sl4n::kInvalidParamStr, doc.GetAllocator());
    return;
  } else {
    tx_power_level = doc[sl4n::kParamsStr][2].GetInt();
  }
  if (!doc[sl4n::kParamsStr][3].IsBool()) {
    LOG(ERROR) << sl4n::kTagStr << ": Expected Bool input for connectable";
    doc.AddMember(sl4n::kResultStr, false, doc.GetAllocator());
    doc.AddMember(sl4n::kErrorStr, sl4n::kInvalidParamStr, doc.GetAllocator());
    return;
  } else {
    is_connectable = doc[sl4n::kParamsStr][3].GetBool();
  }

  int adv_settings;
  int error_code;
  std::tie(adv_settings, error_code) = bt_binder.BluetoothBinderSetAdvSettings(
    mode, timeout_seconds, tx_power_level, is_connectable);
  if(error_code == sl4n_error_codes::kFailInt) {
    doc.AddMember(
      sl4n::kResultStr, sl4n_error_codes::kFailInt, doc.GetAllocator());
    doc.AddMember(sl4n::kErrorStr, sl4n::kFailedCounterInt, doc.GetAllocator());
    return;
  } else {
    doc.AddMember(sl4n::kResultStr, adv_settings, doc.GetAllocator());
    doc.AddMember(sl4n::kErrorStr, NULL, doc.GetAllocator());
  }
}

// End Wrappers ... I'm not a large water dwelling mammal...

CommandReceiver::CommandReceiver() {
  if (_funcMap == NULL) {
    _funcMap = new function_map();
  }
  _funcMap->insert(std::make_pair("initiate", &initiate));
  _funcMap->insert(std::make_pair("BluetoothBinderInitInterface",
    &bluetooth_binder_init_interface_wapper));
  _funcMap->insert(std::make_pair("BluetoothBinderGetName",
    &bluetooth_binder_get_local_name_wrapper));
  _funcMap->insert(std::make_pair("BluetoothBinderSetName",
    &bluetooth_binder_set_local_name_wrapper));
  _funcMap->insert(std::make_pair("BluetoothBinderGetAddress",
    &bluetooth_binder_get_local_address_wrapper));
  _funcMap->insert(std::make_pair("BluetoothBinderEnable",
    &bluetooth_binder_enable_wrapper));
  _funcMap->insert(std::make_pair("BluetoothBinderRegisterBLE",
    &bluetooth_binder_register_ble_wrapper));
  _funcMap->insert(std::make_pair("BluetoothBinderSetAdvSettings",
    &bluetooth_binder_set_adv_settings_wrapper));
}

void CommandReceiver::Call(rapidjson::Document& doc) {
  std::string cmd;
  if (doc.HasMember(sl4n::kCmdStr)) {
    cmd = doc[sl4n::kCmdStr].GetString();
  } else if (doc.HasMember(sl4n::kMethodStr)) {
    cmd = doc[sl4n::kMethodStr].GetString();
  }

  function_map::const_iterator iter = _funcMap->find(cmd);
  if (iter != _funcMap->end()) {
    iter->second(doc);
  }
  _clean_result(doc);
}

void CommandReceiver::RegisterCommand(std::string name, MFP command) {
  if (_funcMap == NULL) {
    _funcMap = new function_map();
  }

  _funcMap->insert(std::make_pair(name, command));
}
