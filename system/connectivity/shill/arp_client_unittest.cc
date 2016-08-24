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

#include "shill/arp_client.h"

#include <linux/if_packet.h>
#include <net/ethernet.h>
#include <net/if_arp.h>
#include <netinet/in.h>

#include <gtest/gtest.h>

#include "shill/arp_packet.h"
#include "shill/mock_log.h"
#include "shill/net/ip_address.h"
#include "shill/net/mock_sockets.h"

using testing::_;
using testing::AnyNumber;
using testing::HasSubstr;
using testing::InSequence;
using testing::Invoke;
using testing::Mock;
using testing::Return;
using testing::StrictMock;
using testing::Test;

namespace shill {

class ArpClientTest : public Test {
 public:
  ArpClientTest() : client_(kInterfaceIndex) {}
  virtual ~ArpClientTest() {}

  virtual void SetUp() {
    sockets_ = new StrictMock<MockSockets>();
    // Passes ownership.
    client_.sockets_.reset(sockets_);
    memset(&recvfrom_sender_, 0, sizeof(recvfrom_sender_));
  }

  virtual void TearDown() {
    if (GetSocket() == kSocketFD) {
      EXPECT_CALL(*sockets_, Close(kSocketFD));
      client_.Stop();
    }
  }

  ssize_t SimulateRecvFrom(int sockfd, void* buf, size_t len, int flags,
                           struct sockaddr* src_addr, socklen_t* addrlen);

 protected:
  static const int kInterfaceIndex;
  static const int kSocketFD;
  static const char kLocalIPAddress[];
  static const uint8_t kLocalMACAddress[];
  static const char kRemoteIPAddress[];
  static const uint8_t kRemoteMACAddress[];
  static const int kArpOpOffset;

  bool CreateSocket() { return client_.CreateSocket(ARPOP_REPLY); }
  int GetInterfaceIndex() { return client_.interface_index_; }
  size_t GetMaxArpPacketLength() { return ArpClient::kMaxArpPacketLength; }
  int GetSocket() { return client_.socket_; }
  void SetupValidPacket(ArpPacket* packet);
  void StartClient() { StartClientWithFD(kSocketFD); }
  void StartClientWithFD(int fd);

  // Owned by ArpClient, and tracked here only for mocks.
  MockSockets* sockets_;
  ArpClient client_;
  ByteString recvfrom_reply_data_;
  sockaddr_ll recvfrom_sender_;
};


const int ArpClientTest::kInterfaceIndex = 123;
const int ArpClientTest::kSocketFD = 456;
const char ArpClientTest::kLocalIPAddress[] = "10.0.1.1";
const uint8_t ArpClientTest::kLocalMACAddress[] = { 0, 1, 2, 3, 4, 5 };
const char ArpClientTest::kRemoteIPAddress[] = "10.0.1.2";
const uint8_t ArpClientTest::kRemoteMACAddress[] = { 6, 7, 8, 9, 10, 11 };
const int ArpClientTest::kArpOpOffset = 7;


MATCHER_P2(IsLinkAddress, interface_index, destination_mac, "") {
  const struct sockaddr_ll* socket_address =
      reinterpret_cast<const struct sockaddr_ll*>(arg);
  ByteString socket_mac(
      reinterpret_cast<const unsigned char*>(&socket_address->sll_addr),
      destination_mac.GetLength());
  return socket_address->sll_family == AF_PACKET &&
      socket_address->sll_protocol == htons(ETHERTYPE_ARP) &&
      socket_address->sll_ifindex == interface_index &&
      destination_mac.Equals(socket_mac);
}

MATCHER_P(IsByteData, byte_data, "") {
  return ByteString(reinterpret_cast<const unsigned char*>(arg),
                    byte_data.GetLength()).Equals(byte_data);
}

void ArpClientTest::SetupValidPacket(ArpPacket* packet) {
  IPAddress local_ip(IPAddress::kFamilyIPv4);
  EXPECT_TRUE(local_ip.SetAddressFromString(kLocalIPAddress));
  packet->set_local_ip_address(local_ip);
  IPAddress remote_ip(IPAddress::kFamilyIPv4);
  EXPECT_TRUE(remote_ip.SetAddressFromString(kRemoteIPAddress));
  packet->set_remote_ip_address(remote_ip);
  ByteString local_mac(kLocalMACAddress, arraysize(kLocalMACAddress));
  packet->set_local_mac_address(local_mac);
  ByteString remote_mac(kRemoteMACAddress, arraysize(kRemoteMACAddress));
  packet->set_remote_mac_address(remote_mac);
}

ssize_t ArpClientTest::SimulateRecvFrom(int sockfd, void* buf, size_t len,
                                        int flags, struct sockaddr* src_addr,
                                        socklen_t* addrlen) {
  memcpy(buf, recvfrom_reply_data_.GetConstData(),
         recvfrom_reply_data_.GetLength());
  memcpy(src_addr, &recvfrom_sender_, sizeof(recvfrom_sender_));
  return recvfrom_reply_data_.GetLength();
}

void ArpClientTest::StartClientWithFD(int fd) {
  EXPECT_CALL(*sockets_, Socket(PF_PACKET, SOCK_DGRAM, htons(ETHERTYPE_ARP)))
      .WillOnce(Return(fd));
  EXPECT_CALL(*sockets_, AttachFilter(fd, _)).WillOnce(Return(0));
  EXPECT_CALL(*sockets_, SetNonBlocking(fd)).WillOnce(Return(0));
  EXPECT_CALL(*sockets_, Bind(fd,
                              IsLinkAddress(kInterfaceIndex, ByteString()),
                              sizeof(sockaddr_ll))).WillOnce(Return(0));
  EXPECT_TRUE(CreateSocket());
  EXPECT_EQ(fd, client_.socket_);
}

TEST_F(ArpClientTest, Constructor) {
  EXPECT_EQ(kInterfaceIndex, GetInterfaceIndex());
  EXPECT_EQ(-1, GetSocket());
}

TEST_F(ArpClientTest, SocketOpenFail) {
  ScopedMockLog log;
  EXPECT_CALL(log,
      Log(logging::LOG_ERROR, _,
          HasSubstr("Could not create ARP socket"))).Times(1);

  EXPECT_CALL(*sockets_, Socket(PF_PACKET, SOCK_DGRAM, htons(ETHERTYPE_ARP)))
      .WillOnce(Return(-1));
  EXPECT_FALSE(CreateSocket());
}

TEST_F(ArpClientTest, SocketFilterFail) {
  ScopedMockLog log;
  EXPECT_CALL(log,
      Log(logging::LOG_ERROR, _,
          HasSubstr("Could not attach packet filter"))).Times(1);

  EXPECT_CALL(*sockets_, Socket(_, _, _)).WillOnce(Return(kSocketFD));
  EXPECT_CALL(*sockets_, AttachFilter(kSocketFD, _)).WillOnce(Return(-1));
  EXPECT_FALSE(CreateSocket());
}

TEST_F(ArpClientTest, SocketNonBlockingFail) {
  ScopedMockLog log;
  EXPECT_CALL(log,
      Log(logging::LOG_ERROR, _,
          HasSubstr("Could not set socket to be non-blocking"))).Times(1);

  EXPECT_CALL(*sockets_, Socket(_, _, _)).WillOnce(Return(kSocketFD));
  EXPECT_CALL(*sockets_, AttachFilter(kSocketFD, _)).WillOnce(Return(0));
  EXPECT_CALL(*sockets_, SetNonBlocking(kSocketFD)).WillOnce(Return(-1));
  EXPECT_FALSE(CreateSocket());
}

TEST_F(ArpClientTest, SocketBindFail) {
  ScopedMockLog log;
  EXPECT_CALL(log,
      Log(logging::LOG_ERROR, _,
          HasSubstr("Could not bind socket to interface"))).Times(1);

  EXPECT_CALL(*sockets_, Socket(_, _, _)).WillOnce(Return(kSocketFD));
  EXPECT_CALL(*sockets_, AttachFilter(kSocketFD, _)).WillOnce(Return(0));
  EXPECT_CALL(*sockets_, SetNonBlocking(kSocketFD)).WillOnce(Return(0));
  EXPECT_CALL(*sockets_, Bind(kSocketFD, _, _)).WillOnce(Return(-1));
  EXPECT_FALSE(CreateSocket());
}

TEST_F(ArpClientTest, StartSuccess) {
  StartClient();
}

TEST_F(ArpClientTest, StartMultipleTimes) {
  const int kFirstSocketFD = kSocketFD + 1;
  StartClientWithFD(kFirstSocketFD);
  EXPECT_CALL(*sockets_, Close(kFirstSocketFD));
  StartClient();
}

TEST_F(ArpClientTest, Receive) {
  StartClient();
  EXPECT_CALL(*sockets_,
              RecvFrom(kSocketFD, _, GetMaxArpPacketLength(), 0, _, _))
      .WillOnce(Return(-1))
      .WillRepeatedly(Invoke(this, &ArpClientTest::SimulateRecvFrom));
  ArpPacket reply;
  ByteString sender;

  ScopedMockLog log;
  EXPECT_CALL(log, Log(_, _, _)).Times(AnyNumber());
  {
    InSequence seq;

    // RecvFrom returns an error.
    EXPECT_CALL(log,
        Log(logging::LOG_ERROR, _,
            HasSubstr("Socket recvfrom failed"))).Times(1);
    EXPECT_FALSE(client_.ReceivePacket(&reply, &sender));

    // RecvFrom returns an empty response which fails to parse.
    EXPECT_CALL(log,
        Log(logging::LOG_ERROR, _,
            HasSubstr("Failed to parse ARP packet"))).Times(1);
    EXPECT_FALSE(client_.ReceivePacket(&reply, &sender));

    ArpPacket packet;
    SetupValidPacket(&packet);
    packet.FormatRequest(&recvfrom_reply_data_);

    // Hack: Force this packet to be an ARP repsonse instead of an ARP request.
    recvfrom_reply_data_.GetData()[kArpOpOffset] = ARPOP_REPLY;

    static const uint8_t kSenderBytes[] = { 0xa, 0xb, 0xc, 0xd, 0xe, 0xf };
    memcpy(&recvfrom_sender_.sll_addr, kSenderBytes, sizeof(kSenderBytes));
    recvfrom_sender_.sll_halen = sizeof(kSenderBytes);
    EXPECT_TRUE(client_.ReceivePacket(&reply, &sender));
    EXPECT_TRUE(reply.local_ip_address().Equals(packet.local_ip_address()));
    EXPECT_TRUE(reply.local_mac_address().Equals(packet.local_mac_address()));
    EXPECT_TRUE(reply.remote_ip_address().Equals(packet.remote_ip_address()));
    EXPECT_TRUE(reply.remote_mac_address().Equals(packet.remote_mac_address()));
    EXPECT_TRUE(
        sender.Equals(ByteString(kSenderBytes, arraysize(kSenderBytes))));
  }
}

TEST_F(ArpClientTest, Transmit) {
  ArpPacket packet;
  StartClient();
  // Packet isn't valid.
  EXPECT_FALSE(client_.TransmitRequest(packet));

  SetupValidPacket(&packet);
  const ByteString& remote_mac = packet.remote_mac_address();
  ByteString packet_bytes;
  ASSERT_TRUE(packet.FormatRequest(&packet_bytes));
  EXPECT_CALL(*sockets_, SendTo(kSocketFD,
                                IsByteData(packet_bytes),
                                packet_bytes.GetLength(),
                                0,
                                IsLinkAddress(kInterfaceIndex, remote_mac),
                                sizeof(sockaddr_ll)))
      .WillOnce(Return(-1))
      .WillOnce(Return(0))
      .WillOnce(Return(packet_bytes.GetLength() - 1))
      .WillOnce(Return(packet_bytes.GetLength()));
  {
    InSequence seq;
    ScopedMockLog log;
    EXPECT_CALL(log,
        Log(logging::LOG_ERROR, _,
            HasSubstr("Socket sendto failed"))).Times(1);
    EXPECT_CALL(log,
        Log(logging::LOG_ERROR, _,
            HasSubstr("different from expected result"))).Times(2);

    EXPECT_FALSE(client_.TransmitRequest(packet));
    EXPECT_FALSE(client_.TransmitRequest(packet));
    EXPECT_FALSE(client_.TransmitRequest(packet));
    EXPECT_TRUE(client_.TransmitRequest(packet));
  }

  // If the destination MAC address is unset, it should be sent to the
  // broadcast MAC address.
  static const uint8_t kZeroBytes[] = { 0, 0, 0, 0, 0, 0 };
  packet.set_remote_mac_address(ByteString(kZeroBytes, arraysize(kZeroBytes)));
  ASSERT_TRUE(packet.FormatRequest(&packet_bytes));
  static const uint8_t kBroadcastBytes[] =
      { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
  ByteString broadcast_mac(kBroadcastBytes, arraysize(kBroadcastBytes));
  EXPECT_CALL(*sockets_, SendTo(kSocketFD,
                                IsByteData(packet_bytes),
                                packet_bytes.GetLength(),
                                0,
                                IsLinkAddress(kInterfaceIndex, broadcast_mac),
                                sizeof(sockaddr_ll)))
      .WillOnce(Return(packet_bytes.GetLength()));
  EXPECT_TRUE(client_.TransmitRequest(packet));
}

}  // namespace shill
