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

#ifndef SHILL_MOCK_PPP_DEVICE_H_
#define SHILL_MOCK_PPP_DEVICE_H_

#include <map>
#include <string>

#include <gmock/gmock.h>

#include "shill/ppp_device.h"

namespace shill {

class MockPPPDevice : public PPPDevice {
 public:
  MockPPPDevice(ControlInterface* control,
                EventDispatcher* dispatcher,
                Metrics* metrics,
                Manager* manager,
                const std::string& link_name,
                int interface_index);
  ~MockPPPDevice() override;

  MOCK_METHOD2(Stop,
               void(Error* error, const EnabledStateChangedCallback& callback));
  MOCK_METHOD1(UpdateIPConfig, void(const IPConfig::Properties& properties));
  MOCK_METHOD0(DropConnection, void());
  MOCK_METHOD1(SelectService, void(const ServiceRefPtr& service));
  MOCK_METHOD1(SetServiceState, void(Service::ConnectState));
  MOCK_METHOD1(SetServiceFailure, void(Service::ConnectFailure));
  MOCK_METHOD1(SetServiceFailureSilent, void(Service::ConnectFailure));
  MOCK_METHOD1(SetEnabled, void(bool));
  MOCK_METHOD2(UpdateIPConfigFromPPP, void(
      const std::map<std::string, std::string>& config,
      bool blackhole_ipv6));
  MOCK_METHOD3(UpdateIPConfigFromPPPWithMTU, void(
      const std::map<std::string, std::string>& config,
      bool blackhole_ipv6,
      int32_t mtu));
  MOCK_METHOD0(AcquireIPv6Config, bool());

 private:
  DISALLOW_COPY_AND_ASSIGN(MockPPPDevice);
};

}  // namespace shill

#endif  // SHILL_MOCK_PPP_DEVICE_H_
