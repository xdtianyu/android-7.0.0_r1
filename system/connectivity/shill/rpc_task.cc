//
// Copyright (C) 2012 The Android Open Source Project
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

#include "shill/rpc_task.h"

#include <base/strings/string_number_conversions.h>

#include "shill/adaptor_interfaces.h"
#include "shill/control_interface.h"
#include "shill/logging.h"

using std::map;
using std::string;
using std::vector;

namespace shill {

// static
unsigned int RPCTask::serial_number_ = 0;

RPCTask::RPCTask(ControlInterface* control_interface, RPCTaskDelegate* delegate)
    : delegate_(delegate),
      unique_name_(base::UintToString(serial_number_++)),
      adaptor_(control_interface->CreateRPCTaskAdaptor(this)) {
  CHECK(delegate);
  LOG(INFO) << "RPCTask " + unique_name_ + " created.";
}

RPCTask::~RPCTask() {
  LOG(INFO) << "RPCTask " + unique_name_ + " destroyed.";
}

void RPCTask::GetLogin(string* user, string* password) const {
  delegate_->GetLogin(user, password);
}

void RPCTask::Notify(const string& reason, const map<string, string>& dict) {
  delegate_->Notify(reason, dict);
}

map<string, string> RPCTask::GetEnvironment() const {
  map<string, string> env;
  env.emplace(kRPCTaskServiceVariable, adaptor_->GetRpcConnectionIdentifier());
  env.emplace(kRPCTaskPathVariable, adaptor_->GetRpcIdentifier());
  return env;
}

// TODO(quiche): remove after moving OpenVPNDriver over to ExternalTask.
string RPCTask::GetRpcIdentifier() const {
  return adaptor_->GetRpcIdentifier();
}

// TODO(quiche): remove after moving OpenVPNDriver over to ExternalTask.
string RPCTask::GetRpcConnectionIdentifier() const {
  return adaptor_->GetRpcConnectionIdentifier();
}

}  // namespace shill
