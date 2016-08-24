//
// Copyright (C) 2015 The Android Open Source Project
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

#include "shill/arp_client_test_helper.h"

#include <gtest/gtest.h>

using testing::_;
using testing::Invoke;
using testing::Return;

namespace shill {

ArpClientTestHelper::ArpClientTestHelper(MockArpClient* client)
    : client_(client) {}

ArpClientTestHelper::~ArpClientTestHelper() {}

void ArpClientTestHelper::GeneratePacket(uint16_t operation,
                                         const IPAddress& local_ip,
                                         const ByteString& local_mac,
                                         const IPAddress& remote_ip,
                                         const ByteString& remote_mac) {
  packet_.set_operation(operation);
  packet_.set_local_ip_address(local_ip);
  packet_.set_local_mac_address(local_mac);
  packet_.set_remote_ip_address(remote_ip);
  packet_.set_remote_mac_address(remote_mac);

  EXPECT_CALL(*client_, ReceivePacket(_, _))
      .WillOnce(Invoke(this, &ArpClientTestHelper::SimulateReceivePacket));
}

bool ArpClientTestHelper::SimulateReceivePacket(ArpPacket* packet,
                                                ByteString* sender) {
  packet->set_operation(packet_.operation());
  packet->set_local_ip_address(packet_.local_ip_address());
  packet->set_local_mac_address(packet_.local_mac_address());
  packet->set_remote_ip_address(packet_.remote_ip_address());
  packet->set_remote_mac_address(packet_.remote_mac_address());
  return true;
}

}  // namespace shill
