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

#include "shill/nice_mock_control.h"

#include <gmock/gmock.h>

#include "shill/mock_adaptors.h"

using ::testing::NiceMock;

namespace shill {

NiceMockControl::NiceMockControl() {}

NiceMockControl::~NiceMockControl() {}

DeviceAdaptorInterface* NiceMockControl::CreateDeviceAdaptor(
    Device* /*device*/) {
  return new NiceMock<DeviceMockAdaptor>();
}

IPConfigAdaptorInterface* NiceMockControl::CreateIPConfigAdaptor(
    IPConfig* /*config*/) {
  return new NiceMock<IPConfigMockAdaptor>();
}

ManagerAdaptorInterface* NiceMockControl::CreateManagerAdaptor(
    Manager* /*manager*/) {
  return new NiceMock<ManagerMockAdaptor>();
}

ProfileAdaptorInterface* NiceMockControl::CreateProfileAdaptor(
    Profile* /*profile*/) {
  return new NiceMock<ProfileMockAdaptor>();
}

RPCTaskAdaptorInterface* NiceMockControl::CreateRPCTaskAdaptor(
    RPCTask* /*task*/) {
  return new NiceMock<RPCTaskMockAdaptor>();
}

ServiceAdaptorInterface* NiceMockControl::CreateServiceAdaptor(
    Service* /*service*/) {
  return new NiceMock<ServiceMockAdaptor>();
}

#ifndef DISABLE_VPN
ThirdPartyVpnAdaptorInterface* NiceMockControl::CreateThirdPartyVpnAdaptor(
      ThirdPartyVpnDriver* /*driver*/) {
  return new NiceMock<ThirdPartyVpnMockAdaptor>();
}
#endif

const std::string& NiceMockControl::NullRPCIdentifier() {
  return null_identifier_;
}

}  // namespace shill
