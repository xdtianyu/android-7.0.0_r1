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

#ifndef SHILL_ARP_CLIENT_TEST_HELPER_H_
#define SHILL_ARP_CLIENT_TEST_HELPER_H_

#include <base/macros.h>

#include "shill/arp_packet.h"
#include "shill/mock_arp_client.h"
#include "shill/net/byte_string.h"
#include "shill/net/ip_address.h"

namespace shill {

// Class for simulate ARP client receiving ARP packets for unit test purpose.
class ArpClientTestHelper {
 public:
  explicit ArpClientTestHelper(MockArpClient* client);
  virtual ~ArpClientTestHelper();

  void GeneratePacket(uint16_t operation,
                      const IPAddress& local_ip,
                      const ByteString& local_mac,
                      const IPAddress& remote_ip,
                      const ByteString& remote_mac);

 private:
  bool SimulateReceivePacket(ArpPacket* packet, ByteString* sender);

  MockArpClient* client_;
  ArpPacket packet_;

  DISALLOW_COPY_AND_ASSIGN(ArpClientTestHelper);
};

}  // namespace shill

#endif  // SHILL_ARP_CLIENT_TEST_HELPER_H_
