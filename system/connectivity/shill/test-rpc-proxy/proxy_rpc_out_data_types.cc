//
// Copyright (C) 2015 The Android Open Source Project
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

#include "proxy_rpc_out_data_types.h"

namespace {
// Autotest Server test encodes the object type in this key.
static const char kXmlRpcStructTypeKey[] = "xmlrpc_struct_type_key";
} // namespace

AssociationResult::AssociationResult(
    bool success,
    double discovery_time,
    double association_time,
    double configuration_time,
    const std::string& failure_reason)
  : success_(success),
    discovery_time_(discovery_time),
    association_time_(association_time),
    configuration_time_(configuration_time),
    failure_reason_(failure_reason) {}

XmlRpc::XmlRpcValue AssociationResult::ConvertToXmlRpcValue() {
  XmlRpc::XmlRpcValue value;
  value["discovery_time"] = discovery_time_;
  value["association_time"] = association_time_;
  value["configuration_time"] = configuration_time_;
  value["failure_reason"] = failure_reason_;
  value["success"] = success_;
  value[kXmlRpcStructTypeKey] = "AssociationResult";
  return value;
}
