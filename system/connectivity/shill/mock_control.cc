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

#include "shill/mock_control.h"

#include "shill/mock_adaptors.h"

namespace shill {

MockControl::MockControl() {}

MockControl::~MockControl() {}

DeviceAdaptorInterface* MockControl::CreateDeviceAdaptor(Device* /*device*/) {
  return new DeviceMockAdaptor();
}

IPConfigAdaptorInterface* MockControl::CreateIPConfigAdaptor(
    IPConfig* /*config*/) {
  return new IPConfigMockAdaptor();
}

ManagerAdaptorInterface* MockControl::CreateManagerAdaptor(
    Manager* /*manager*/) {
  return new ManagerMockAdaptor();
}

ProfileAdaptorInterface* MockControl::CreateProfileAdaptor(
    Profile* /*profile*/) {
  return new ProfileMockAdaptor();
}

RPCTaskAdaptorInterface* MockControl::CreateRPCTaskAdaptor(RPCTask* /*task*/) {
  return new RPCTaskMockAdaptor();
}

ServiceAdaptorInterface* MockControl::CreateServiceAdaptor(
    Service* /*service*/) {
  return new ServiceMockAdaptor();
}

#ifndef DISABLE_VPN
ThirdPartyVpnAdaptorInterface* MockControl::CreateThirdPartyVpnAdaptor(
      ThirdPartyVpnDriver* /*driver*/) {
  return new ThirdPartyVpnMockAdaptor();
}
#endif

const std::string& MockControl::NullRPCIdentifier() {
  return null_identifier_;
}

}  // namespace shill
