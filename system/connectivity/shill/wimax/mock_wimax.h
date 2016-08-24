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

#ifndef SHILL_WIMAX_MOCK_WIMAX_H_
#define SHILL_WIMAX_MOCK_WIMAX_H_

#include <string>

#include <gmock/gmock.h>

#include "shill/wimax/wimax.h"

namespace shill {

class ControlInterface;
class Error;
class EventDispatcher;

class MockWiMax : public WiMax {
 public:
  MockWiMax(ControlInterface* control,
            EventDispatcher* dispatcher,
            Metrics* metrics,
            Manager* manager,
            const std::string& link_name,
            const std::string& address,
            int interface_index,
            const RpcIdentifier& path);
  ~MockWiMax() override;

  MOCK_METHOD2(Start, void(Error* error,
                           const EnabledStateChangedCallback& callback));
  MOCK_METHOD2(Stop, void(Error* error,
                          const EnabledStateChangedCallback& callback));
  MOCK_METHOD2(ConnectTo, void(const WiMaxServiceRefPtr& service,
                               Error* error));
  MOCK_METHOD2(DisconnectFrom, void(const ServiceRefPtr& service,
                                    Error* error));
  MOCK_CONST_METHOD0(IsIdle, bool());
  MOCK_METHOD1(OnServiceStopped, void(const WiMaxServiceRefPtr& service));
  MOCK_METHOD0(OnDeviceVanished, void());

 private:
  DISALLOW_COPY_AND_ASSIGN(MockWiMax);
};

}  // namespace shill

#endif  // SHILL_WIMAX_MOCK_WIMAX_H_
