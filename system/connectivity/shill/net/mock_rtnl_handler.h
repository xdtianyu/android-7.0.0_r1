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

#ifndef SHILL_NET_MOCK_RTNL_HANDLER_H_
#define SHILL_NET_MOCK_RTNL_HANDLER_H_

#include <string>

#include <base/macros.h>
#include <gmock/gmock.h>

#include "shill/net/rtnl_handler.h"

namespace shill {

class MockRTNLHandler : public RTNLHandler {
 public:
  MockRTNLHandler() {}
  ~MockRTNLHandler() override {}

  MOCK_METHOD1(Start, void(uint32_t netlink_groups_mask));
  MOCK_METHOD1(AddListener, void(RTNLListener* to_add));
  MOCK_METHOD1(RemoveListener, void(RTNLListener* to_remove));
  MOCK_METHOD3(SetInterfaceFlags, void(int interface_index,
                                       unsigned int flags,
                                       unsigned int change));
  MOCK_METHOD2(SetInterfaceMTU, void(int interface_index, unsigned int mtu));
  MOCK_METHOD4(AddInterfaceAddress, bool(int interface_index,
                                         const IPAddress& local,
                                         const IPAddress& broadcast,
                                         const IPAddress& peer));
  MOCK_METHOD2(RemoveInterfaceAddress, bool(int interface_index,
                                            const IPAddress& local));
  MOCK_METHOD1(RemoveInterface, bool(int interface_index));
  MOCK_METHOD1(RequestDump, void(int request_flags));
  MOCK_METHOD1(GetInterfaceIndex, int(const std::string& interface_name));
  MOCK_METHOD2(SendMessageWithErrorMask, bool(RTNLMessage* message,
                                              const ErrorMask& error_mask));
  MOCK_METHOD1(SendMessage, bool(RTNLMessage* message));

 private:
  DISALLOW_COPY_AND_ASSIGN(MockRTNLHandler);
};

}  // namespace shill

#endif  // SHILL_NET_MOCK_RTNL_HANDLER_H_
