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

#ifndef SHILL_MOCK_ARP_CLIENT_H_
#define SHILL_MOCK_ARP_CLIENT_H_

#include "shill/arp_client.h"

#include <gmock/gmock.h>

#include "shill/arp_packet.h"

namespace shill {

class MockArpClient : public ArpClient {
 public:
  MockArpClient();
  ~MockArpClient() override;

  MOCK_METHOD0(StartReplyListener, bool());
  MOCK_METHOD0(StartRequestListener, bool());
  MOCK_METHOD0(Stop, void());
  MOCK_CONST_METHOD2(ReceivePacket, bool(ArpPacket* packet,
                                         ByteString* sender));
  MOCK_CONST_METHOD1(TransmitRequest, bool(const ArpPacket& packet));
  MOCK_CONST_METHOD0(socket, int());

 private:
  DISALLOW_COPY_AND_ASSIGN(MockArpClient);
};

}  // namespace shill

#endif  // SHILL_MOCK_ARP_CLIENT_H_
