//
// Copyright (C) 2013 The Android Open Source Project
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

#ifndef SHILL_NET_MOCK_NETLINK_MANAGER_H_
#define SHILL_NET_MOCK_NETLINK_MANAGER_H_

#include "shill/net/netlink_manager.h"

#include <map>
#include <string>

#include <gmock/gmock.h>

namespace shill {

class NetlinkMessage;

class MockNetlinkManager : public NetlinkManager {
 public:
  MockNetlinkManager() {}
  ~MockNetlinkManager() override {}

  MOCK_METHOD0(Init, bool());
  MOCK_METHOD0(Start, void());
  MOCK_METHOD2(
      GetFamily,
      uint16_t(const std::string& family_name,
               const NetlinkMessageFactory::FactoryMethod& message_factory));
  MOCK_METHOD1(RemoveBroadcastHandler,
               bool(const NetlinkMessageHandler& message_handler));
  MOCK_METHOD1(AddBroadcastHandler,
               bool(const NetlinkMessageHandler& messge_handler));
  MOCK_METHOD4(SendControlMessage,
               bool(ControlNetlinkMessage* message,
                    const ControlNetlinkMessageHandler& message_handler,
                    const NetlinkAckHandler& ack_handler,
                    const NetlinkAuxilliaryMessageHandler& error_handler));
  MOCK_METHOD4(SendNl80211Message,
               bool(Nl80211Message* message,
                    const Nl80211MessageHandler& message_handler,
                    const NetlinkAckHandler& ack_handler,
                    const NetlinkAuxilliaryMessageHandler& error_handler));
  MOCK_METHOD2(SubscribeToEvents,
               bool(const std::string& family, const std::string& group));

 private:
  DISALLOW_COPY_AND_ASSIGN(MockNetlinkManager);
};

}  // namespace shill

#endif  // SHILL_NET_MOCK_NETLINK_MANAGER_H_
