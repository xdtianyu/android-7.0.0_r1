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

#include "shill/shims/netfilter_queue_processor.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <string>

#include <gtest/gtest.h>

using std::string;

namespace shill {

namespace shims {

class NetfilterQueueProcessorTest : public testing::Test {
 public:
  NetfilterQueueProcessorTest() : processor_(kInputQueue, kOutputQueue) {}

 protected:
  int GetInputQueue() { return processor_.input_queue_; }
  int GetOutputQueue() { return processor_.output_queue_; }
  struct nfq_handle* GetNFQHandle() { return processor_.nfq_handle_; }
  struct nfq_q_handle* GetInputQueueHandle() {
    return processor_.input_queue_handle_;
  }
  struct nfq_q_handle* GetOutputQueueHandle() {
    return processor_.output_queue_handle_;
  }
  std::deque<NetfilterQueueProcessor::ListenerEntryPtr>& GetListeners() {
    return processor_.listeners_;
  }
  NetfilterQueueProcessor::ListenerEntry& GetListener(int index) {
    return *processor_.listeners_[index];
  }
  int GetExpirationInterval() {
    return NetfilterQueueProcessor::kExpirationIntervalSeconds;
  }
  string AddressAndPortToString(uint32_t ip, uint16_t port) {
    return NetfilterQueueProcessor::AddressAndPortToString(ip, port);
  }

  void SetPacketValues(int in_device,
                       int out_device,
                       bool is_udp,
                       uint32_t packet_id,
                       uint32_t source_ip,
                       uint32_t destination_ip,
                       uint16_t source_port,
                       uint16_t destination_port) {
    packet_.SetValues(in_device, out_device, is_udp, packet_id,
                      source_ip, destination_ip, source_port, destination_port);
  }

  void LogOutgoingPacket(time_t now) {
    processor_.LogOutgoingPacket(packet_, now);
  }
  bool IsIncomingPacketAllowed(time_t now) {
    return processor_.IsIncomingPacketAllowed(packet_, now);
  }

  NetfilterQueueProcessor processor_;
  NetfilterQueueProcessor::Packet packet_;

  static const int kInputQueue;
  static const int kOutputQueue;
};

const int NetfilterQueueProcessorTest::kInputQueue = 1;
const int NetfilterQueueProcessorTest::kOutputQueue = 2;

TEST_F(NetfilterQueueProcessorTest, Init) {
  EXPECT_EQ(kInputQueue, GetInputQueue());
  EXPECT_EQ(kOutputQueue, GetOutputQueue());
  EXPECT_TRUE(GetNFQHandle() == NULL);
  EXPECT_TRUE(GetInputQueueHandle() == NULL);
  EXPECT_TRUE(GetOutputQueueHandle() == NULL);
  EXPECT_TRUE(GetListeners().empty());
}

TEST_F(NetfilterQueueProcessorTest, LogOutgoingPacket) {
  const int kDevice1 = 1000;
  const int kDevice2 = 2000;
  const int kPacketId = 0;
  const uint32_t kMulticastAddress = ntohl(inet_addr("224.0.0.1"));
  const uint32_t kUnicastAddress = ntohl(inet_addr("10.0.0.1"));
  const uint16_t kPort1 = 100;
  const uint16_t kPort2 = 200;
  const time_t kTime0 = 0;

  // Ignore non-UDP packets.
  SetPacketValues(kDevice1, kDevice2, false, kPacketId,
                  kUnicastAddress, kMulticastAddress, kPort1, kPort2);
  LogOutgoingPacket(kTime0);
  EXPECT_TRUE(GetListeners().empty());

  // Ignore UDP packets not sent to a multicast address.
  SetPacketValues(kDevice1, kDevice2, true, kPacketId,
                  kUnicastAddress, kUnicastAddress, kPort1, kPort2);
  LogOutgoingPacket(kTime0);
  EXPECT_TRUE(GetListeners().empty());

  // Ignore UDP packets sent to an unknown output device.
  SetPacketValues(kDevice1, 0, true, kPacketId,
                  kUnicastAddress, kMulticastAddress, kPort1, kPort2);
  LogOutgoingPacket(kTime0);
  EXPECT_TRUE(GetListeners().empty());

  // Add a listener for an outgoing UDP packet.
  SetPacketValues(kDevice1, kDevice2, true, kPacketId,
                  kUnicastAddress, kMulticastAddress, kPort1, kPort2);
  LogOutgoingPacket(kTime0);
  EXPECT_EQ(1, GetListeners().size());
  EXPECT_EQ(kTime0, GetListener(0).last_transmission);
  EXPECT_EQ(kPort1, GetListener(0).port);
  EXPECT_EQ(kDevice2, GetListener(0).device_index);
  EXPECT_EQ(kUnicastAddress, GetListener(0).address);
  // Not testing netmask, but it should be zero since we have a bogus
  // device index.

  // Add a second listener for a newer outgoing UDP packet to a different port.
  const time_t kTime1 = kTime0 + GetExpirationInterval();
  SetPacketValues(kDevice2, kDevice1, true, kPacketId,
                  kUnicastAddress, kMulticastAddress, kPort2, kPort1);
  LogOutgoingPacket(kTime1);
  EXPECT_EQ(2, GetListeners().size());
  EXPECT_EQ(kTime1, GetListener(0).last_transmission);
  EXPECT_EQ(kPort2, GetListener(0).port);
  EXPECT_EQ(kDevice1, GetListener(0).device_index);
  EXPECT_EQ(kUnicastAddress, GetListener(0).address);

  EXPECT_EQ(kTime0, GetListener(1).last_transmission);
  EXPECT_EQ(kPort1, GetListener(1).port);
  EXPECT_EQ(kDevice2, GetListener(1).device_index);
  EXPECT_EQ(kUnicastAddress, GetListener(1).address);

  // Resending the first packet should simply swap the two entries, and update
  // the transmission time of the first.
  const time_t kTime2 = kTime1 + GetExpirationInterval();
  SetPacketValues(kDevice1, kDevice2, true, kPacketId,
                  kUnicastAddress, kMulticastAddress, kPort1, kPort2);
  LogOutgoingPacket(kTime2);
  EXPECT_EQ(2, GetListeners().size());
  EXPECT_EQ(kTime2, GetListener(0).last_transmission);
  EXPECT_EQ(kPort1, GetListener(0).port);
  EXPECT_EQ(kDevice2, GetListener(0).device_index);
  EXPECT_EQ(kUnicastAddress, GetListener(0).address);

  EXPECT_EQ(kTime1, GetListener(1).last_transmission);
  EXPECT_EQ(kPort2, GetListener(1).port);
  EXPECT_EQ(kDevice1, GetListener(1).device_index);
  EXPECT_EQ(kUnicastAddress, GetListener(1).address);

  // A new transmission after the expiration interval will expire the
  // older entry.
  const time_t kTime3 = kTime2 + GetExpirationInterval() + 1;
  SetPacketValues(kDevice2, kDevice1, true, kPacketId,
                  kUnicastAddress, kMulticastAddress, kPort2, kPort1);
  LogOutgoingPacket(kTime3);
  EXPECT_EQ(1, GetListeners().size());
  EXPECT_EQ(kTime3, GetListener(0).last_transmission);
  EXPECT_EQ(kPort2, GetListener(0).port);
  EXPECT_EQ(kDevice1, GetListener(0).device_index);
  EXPECT_EQ(kUnicastAddress, GetListener(0).address);
}

TEST_F(NetfilterQueueProcessorTest, IsIncomingPacketAllowedUnicast) {
  const int kDevice1 = 1000;
  const int kDevice2 = 2000;
  const int kPacketId = 0;
  const uint32_t kMulticastAddress = ntohl(inet_addr("224.0.0.1"));
  const uint32_t kLocalAddress = ntohl(inet_addr("10.0.0.1"));
  const uint32_t kNeighborAddress = ntohl(inet_addr("10.0.0.2"));
  const uint16_t kPort1 = 100;
  const uint16_t kPort2 = 200;
  const time_t kTime0 = 0;

  // An incomng packet received before a listener is present wll be rejected.
  SetPacketValues(kDevice2, kDevice1, true, kPacketId,
                  kNeighborAddress, kLocalAddress, kPort2, kPort1);
  EXPECT_FALSE(IsIncomingPacketAllowed(kTime0));

  SetPacketValues(kDevice1, kDevice2, true, kPacketId,
                  kLocalAddress, kMulticastAddress, kPort1, kPort2);
  LogOutgoingPacket(kTime0);
  const uint32_t kNetmask = ntohl(inet_addr("255.255.255.0"));
  // Set the netmask manually since we don't have the mocks to do so.
  GetListener(0).netmask = kNetmask;

  // Expect that this listener entry will not allow incoming multicasts.
  EXPECT_EQ(0, GetListener(0).destination);

  // Packet is not UDP.
  SetPacketValues(kDevice2, kDevice1, false, kPacketId,
                  kNeighborAddress, kLocalAddress, kPort2, kPort1);
  EXPECT_FALSE(IsIncomingPacketAllowed(kTime0));

  // Packet arrives on the wrong interface.
  SetPacketValues(kDevice1, kDevice2, true, kPacketId,
                  kNeighborAddress, kLocalAddress, kPort2, kPort1);
  EXPECT_FALSE(IsIncomingPacketAllowed(kTime0));

  // Packet arrives addressed to a multicast address.  Ensure that since
  // the source and destination address of the listener do not match,
  // multicast traffic to neither port will work.
  SetPacketValues(kDevice2, kDevice1, true, kPacketId,
                  kNeighborAddress, kMulticastAddress, kPort2, kPort1);
  EXPECT_FALSE(IsIncomingPacketAllowed(kTime0));
  SetPacketValues(kDevice2, kDevice1, true, kPacketId,
                  kNeighborAddress, kMulticastAddress, kPort1, kPort2);
  EXPECT_FALSE(IsIncomingPacketAllowed(kTime0));

  // Packet arrives addressed to an address other than the address associated
  // with the outgoing packet.
  SetPacketValues(kDevice2, kDevice1, true, kPacketId,
                  kNeighborAddress, kNeighborAddress, kPort2, kPort1);
  EXPECT_FALSE(IsIncomingPacketAllowed(kTime0));

  // Packet comes from a network address outside the allowed netmask.
  const uint32_t kRemoteAddress = ntohl(inet_addr("10.0.1.1"));
  SetPacketValues(kDevice2, kDevice1, true, kPacketId,
                  kRemoteAddress, kLocalAddress, kPort2, kPort1);
  EXPECT_FALSE(IsIncomingPacketAllowed(kTime0));

  // Packet arrives addresssed to the wrong port.
  SetPacketValues(kDevice2, kDevice1, true, kPacketId,
                  kNeighborAddress, kLocalAddress, kPort1, kPort2);
  EXPECT_FALSE(IsIncomingPacketAllowed(kTime0));

  // This packet should successfully be accepted.
  SetPacketValues(kDevice2, kDevice1, true, kPacketId,
                  kNeighborAddress, kLocalAddress, kPort2, kPort1);
  EXPECT_TRUE(IsIncomingPacketAllowed(kTime0 + GetExpirationInterval()));

  // The same packet arriving after the expiration interval will be rejected.
  EXPECT_FALSE(IsIncomingPacketAllowed(kTime0 + GetExpirationInterval() + 1));

  // Moreover the expiration has removed the listener entry.
  EXPECT_TRUE(GetListeners().empty());
}

TEST_F(NetfilterQueueProcessorTest, IsIncomingPacketAllowedMulticast) {
  const int kDevice1 = 1000;
  const int kDevice2 = 2000;
  const int kPacketId = 0;
  const uint32_t kMulticastAddress1 = ntohl(inet_addr("224.0.0.1"));
  const uint32_t kMulticastAddress2 = ntohl(inet_addr("224.0.0.2"));
  const uint32_t kLocalAddress = ntohl(inet_addr("10.0.0.1"));
  const uint32_t kNeighborAddress = ntohl(inet_addr("10.0.0.2"));
  const uint16_t kPort1 = 100;
  const uint16_t kPort2 = 200;
  const time_t kTime0 = 0;

  // Send a packet to a multicast address where the source and destination
  // ports match.  This will create a non-zero "destination" listener.
  SetPacketValues(kDevice1, kDevice2, true, kPacketId,
                  kLocalAddress, kMulticastAddress1, kPort1, kPort1);
  LogOutgoingPacket(kTime0);
  const uint32_t kNetmask = ntohl(inet_addr("255.255.255.0"));
  // Set the netmask manually since we don't have the mocks to do so.
  GetListener(0).netmask = kNetmask;

  // Expect that this listener entry will allow incoming multicasts.
  EXPECT_EQ(kMulticastAddress1, GetListener(0).destination);

  // Packet arrives addressed to a different multicast address.
  SetPacketValues(kDevice2, kDevice1, true, kPacketId,
                  kNeighborAddress, kMulticastAddress2, kPort1, kPort1);
  EXPECT_FALSE(IsIncomingPacketAllowed(kTime0));

  // Packet arrives addressed to a different port.
  SetPacketValues(kDevice2, kDevice1, true, kPacketId,
                  kNeighborAddress, kMulticastAddress1, kPort1, kPort2);
  EXPECT_FALSE(IsIncomingPacketAllowed(kTime0));

  // This packet should successfully be accepted.
  SetPacketValues(kDevice2, kDevice1, true, kPacketId,
                  kNeighborAddress, kMulticastAddress1, kPort2, kPort1);
  EXPECT_TRUE(IsIncomingPacketAllowed(kTime0));

  // So will a unicast packet (other unicast cases are tested above in
  // IsIncomingPacketAllowedUnicast).
  SetPacketValues(kDevice2, kDevice1, true, kPacketId,
                  kNeighborAddress, kLocalAddress, kPort1, kPort1);
  EXPECT_TRUE(IsIncomingPacketAllowed(kTime0));
}


TEST_F(NetfilterQueueProcessorTest, AddressAndPortToString) {
  EXPECT_EQ("1.2.3.4:5678", AddressAndPortToString(0x01020304, 5678));
}

}  // namespace shims

}  // namespace shill
