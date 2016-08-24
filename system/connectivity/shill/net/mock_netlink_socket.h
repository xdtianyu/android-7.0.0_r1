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

#ifndef SHILL_NET_MOCK_NETLINK_SOCKET_H_
#define SHILL_NET_MOCK_NETLINK_SOCKET_H_

#include "shill/net/netlink_socket.h"

#include <base/macros.h>

#include <gmock/gmock.h>

namespace shill {

class ByteString;

class MockNetlinkSocket : public NetlinkSocket {
 public:
  MockNetlinkSocket() {}
  MOCK_METHOD0(Init, bool());

  uint32_t GetLastSequenceNumber() const { return sequence_number_; }
  MOCK_CONST_METHOD0(file_descriptor, int());
  MOCK_METHOD1(SendMessage, bool(const ByteString& out_string));
  MOCK_METHOD1(SubscribeToEvents, bool(uint32_t group_id));
  MOCK_METHOD1(RecvMessage, bool(ByteString* message));

 private:
  DISALLOW_COPY_AND_ASSIGN(MockNetlinkSocket);
};

}  // namespace shill

#endif  // SHILL_NET_MOCK_NETLINK_SOCKET_H_
