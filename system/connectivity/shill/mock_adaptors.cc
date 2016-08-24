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

#include "shill/mock_adaptors.h"

#include <string>

using std::string;

namespace shill {

// static
const char DeviceMockAdaptor::kRpcId[] = "/device_rpc";
// static
const char DeviceMockAdaptor::kRpcConnId[] = "/device_rpc_conn";

DeviceMockAdaptor::DeviceMockAdaptor()
    : rpc_id_(kRpcId),
      rpc_conn_id_(kRpcConnId) {
}

DeviceMockAdaptor::~DeviceMockAdaptor() {}

const string& DeviceMockAdaptor::GetRpcIdentifier() { return rpc_id_; }

// static
const char IPConfigMockAdaptor::kRpcId[] = "/ipconfig_rpc";

IPConfigMockAdaptor::IPConfigMockAdaptor() : rpc_id_(kRpcId) {}

IPConfigMockAdaptor::~IPConfigMockAdaptor() {}

const string& IPConfigMockAdaptor::GetRpcIdentifier() { return rpc_id_; }

// static
const char ManagerMockAdaptor::kRpcId[] = "/manager_rpc";

ManagerMockAdaptor::ManagerMockAdaptor() : rpc_id_(kRpcId) {}

ManagerMockAdaptor::~ManagerMockAdaptor() {}

const string& ManagerMockAdaptor::GetRpcIdentifier() { return rpc_id_; }

// static
const char ProfileMockAdaptor::kRpcId[] = "/profile_rpc";

ProfileMockAdaptor::ProfileMockAdaptor() : rpc_id_(kRpcId) {}

ProfileMockAdaptor::~ProfileMockAdaptor() {}

const string& ProfileMockAdaptor::GetRpcIdentifier() { return rpc_id_; }

// static
const char RPCTaskMockAdaptor::kRpcId[] = "/rpc_task_rpc";
const char RPCTaskMockAdaptor::kRpcConnId[] = "/rpc_task_rpc_conn";

RPCTaskMockAdaptor::RPCTaskMockAdaptor()
    : rpc_id_(kRpcId),
      rpc_conn_id_(kRpcConnId) {}

RPCTaskMockAdaptor::~RPCTaskMockAdaptor() {}

const string& RPCTaskMockAdaptor::GetRpcIdentifier() { return rpc_id_; }
const string& RPCTaskMockAdaptor::GetRpcConnectionIdentifier() {
  return rpc_conn_id_;
}

// static
const char ServiceMockAdaptor::kRpcId[] = "/service_rpc";

ServiceMockAdaptor::ServiceMockAdaptor() : rpc_id_(kRpcId) {}

ServiceMockAdaptor::~ServiceMockAdaptor() {}

const string& ServiceMockAdaptor::GetRpcIdentifier() { return rpc_id_; }

#ifndef DISABLE_VPN
ThirdPartyVpnMockAdaptor::ThirdPartyVpnMockAdaptor() {}

ThirdPartyVpnMockAdaptor::~ThirdPartyVpnMockAdaptor() {}
#endif

}  // namespace shill
