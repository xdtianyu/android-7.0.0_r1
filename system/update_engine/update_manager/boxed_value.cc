//
// Copyright (C) 2014 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

#include "update_engine/update_manager/boxed_value.h"

#include <stdint.h>

#include <set>
#include <string>

#include <base/strings/string_number_conversions.h>
#include <base/time/time.h>

#include "update_engine/common/utils.h"
#include "update_engine/update_manager/shill_provider.h"
#include "update_engine/update_manager/updater_provider.h"

using std::set;
using std::string;

namespace chromeos_update_manager {

// Template instantiation for common types; used in BoxedValue::ToString().
// Keep in sync with boxed_value_unitttest.cc.

template<>
string BoxedValue::ValuePrinter<string>(const void* value) {
  const string* val = reinterpret_cast<const string*>(value);
  return *val;
}

template<>
string BoxedValue::ValuePrinter<int>(const void* value) {
  const int* val = reinterpret_cast<const int*>(value);
  return base::IntToString(*val);
}

template<>
string BoxedValue::ValuePrinter<unsigned int>(const void* value) {
  const unsigned int* val = reinterpret_cast<const unsigned int*>(value);
  return base::UintToString(*val);
}

template<>
string BoxedValue::ValuePrinter<int64_t>(const void* value) {
  const int64_t* val = reinterpret_cast<const int64_t*>(value);
  return base::Int64ToString(*val);
}

template<>
string BoxedValue::ValuePrinter<uint64_t>(const void* value) {
  const uint64_t* val =
    reinterpret_cast<const uint64_t*>(value);
  return base::Uint64ToString(static_cast<uint64_t>(*val));
}

template<>
string BoxedValue::ValuePrinter<bool>(const void* value) {
  const bool* val = reinterpret_cast<const bool*>(value);
  return *val ? "true" : "false";
}

template<>
string BoxedValue::ValuePrinter<double>(const void* value) {
  const double* val = reinterpret_cast<const double*>(value);
  return base::DoubleToString(*val);
}

template<>
string BoxedValue::ValuePrinter<base::Time>(const void* value) {
  const base::Time* val = reinterpret_cast<const base::Time*>(value);
  return chromeos_update_engine::utils::ToString(*val);
}

template<>
string BoxedValue::ValuePrinter<base::TimeDelta>(const void* value) {
  const base::TimeDelta* val = reinterpret_cast<const base::TimeDelta*>(value);
  return chromeos_update_engine::utils::FormatTimeDelta(*val);
}

static string ConnectionTypeToString(ConnectionType type) {
  switch (type) {
    case ConnectionType::kEthernet:
      return "Ethernet";
    case ConnectionType::kWifi:
      return "Wifi";
    case ConnectionType::kWimax:
      return "Wimax";
    case ConnectionType::kBluetooth:
      return "Bluetooth";
    case ConnectionType::kCellular:
      return "Cellular";
    case ConnectionType::kUnknown:
      return "Unknown";
  }
  NOTREACHED();
  return "Unknown";
}

template<>
string BoxedValue::ValuePrinter<ConnectionType>(const void* value) {
  const ConnectionType* val = reinterpret_cast<const ConnectionType*>(value);
  return ConnectionTypeToString(*val);
}

template<>
string BoxedValue::ValuePrinter<set<ConnectionType>>(const void* value) {
  string ret = "";
  const set<ConnectionType>* val =
      reinterpret_cast<const set<ConnectionType>*>(value);
  for (auto& it : *val) {
    ConnectionType type = it;
    if (ret.size() > 0)
      ret += ",";
    ret += ConnectionTypeToString(type);
  }
  return ret;
}

template<>
string BoxedValue::ValuePrinter<ConnectionTethering>(const void* value) {
  const ConnectionTethering* val =
      reinterpret_cast<const ConnectionTethering*>(value);
  switch (*val) {
    case ConnectionTethering::kNotDetected:
      return "Not Detected";
    case ConnectionTethering::kSuspected:
      return "Suspected";
    case ConnectionTethering::kConfirmed:
      return "Confirmed";
    case ConnectionTethering::kUnknown:
      return "Unknown";
  }
  NOTREACHED();
  return "Unknown";
}

template<>
string BoxedValue::ValuePrinter<Stage>(const void* value) {
  const Stage* val = reinterpret_cast<const Stage*>(value);
  switch (*val) {
    case Stage::kIdle:
      return "Idle";
    case Stage::kCheckingForUpdate:
      return "Checking For Update";
    case Stage::kUpdateAvailable:
      return "Update Available";
    case Stage::kDownloading:
      return "Downloading";
    case Stage::kVerifying:
      return "Verifying";
    case Stage::kFinalizing:
      return "Finalizing";
    case Stage::kUpdatedNeedReboot:
      return "Updated, Need Reboot";
    case Stage::kReportingErrorEvent:
      return "Reporting Error Event";
    case Stage::kAttemptingRollback:
      return "Attempting Rollback";
  }
  NOTREACHED();
  return "Unknown";
}

template<>
string BoxedValue::ValuePrinter<UpdateRequestStatus>(const void* value) {
  const UpdateRequestStatus* val =
      reinterpret_cast<const UpdateRequestStatus*>(value);
  switch (*val) {
    case UpdateRequestStatus::kNone:
      return "None";
    case UpdateRequestStatus::kInteractive:
      return "Interactive";
    case UpdateRequestStatus::kPeriodic:
      return "Periodic";
  }
  NOTREACHED();
  return "Unknown";
}

}  // namespace chromeos_update_manager
