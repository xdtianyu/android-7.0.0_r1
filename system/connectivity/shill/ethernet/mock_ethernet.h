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

#ifndef SHILL_ETHERNET_MOCK_ETHERNET_H_
#define SHILL_ETHERNET_MOCK_ETHERNET_H_

#include <string>

#include <gmock/gmock.h>

#include "shill/ethernet/ethernet.h"
#include "shill/refptr_types.h"

namespace shill {

class ControlInterface;
class Error;
class EventDispatcher;

class MockEthernet : public Ethernet {
 public:
  MockEthernet(ControlInterface* control_interface,
               EventDispatcher* dispatcher,
               Metrics* metrics,
               Manager* manager,
               const std::string& link_name,
               const std::string& address,
               int interface_index);
  ~MockEthernet() override;

  MOCK_METHOD2(Start, void(Error* error,
                           const EnabledStateChangedCallback& callback));
  MOCK_METHOD2(Stop, void(Error* error,
                          const EnabledStateChangedCallback& callback));
  MOCK_METHOD1(ConnectTo, void(EthernetService* service));
  MOCK_METHOD1(DisconnectFrom, void(EthernetService* service));
  MOCK_CONST_METHOD0(IsConnectedViaTether, bool());
  MOCK_CONST_METHOD0(link_up, bool());

 private:
  DISALLOW_COPY_AND_ASSIGN(MockEthernet);
};

}  // namespace shill

#endif  // SHILL_ETHERNET_MOCK_ETHERNET_H_
