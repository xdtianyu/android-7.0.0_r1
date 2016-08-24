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

#include "shill/eap_listener.h"

#include <linux/if_ether.h>
#include <linux/if_packet.h>
#include <netinet/in.h>
#include <string.h>

#include <algorithm>

#include <base/bind.h>
#include <gtest/gtest.h>

#include "shill/eap_protocol.h"
#include "shill/mock_event_dispatcher.h"
#include "shill/mock_log.h"
#include "shill/net/byte_string.h"
#include "shill/net/mock_sockets.h"

using testing::_;
using testing::HasSubstr;
using testing::Invoke;
using testing::Return;
using testing::StrictMock;

namespace shill {

class EapListenerTest : public testing::Test {
 public:
  EapListenerTest() : listener_(&dispatcher_, kInterfaceIndex) {}
  virtual ~EapListenerTest() {}

  virtual void SetUp() {
    sockets_ = new StrictMock<MockSockets>();
    // Passes ownership.
    listener_.sockets_.reset(sockets_);
    listener_.set_request_received_callback(
        base::Bind(&EapListenerTest::ReceiveCallback, base::Unretained(this)));
  }

  virtual void TearDown() {
    if (GetSocket() == kSocketFD) {
      EXPECT_CALL(*sockets_, Close(kSocketFD));
      listener_.Stop();
    }
  }

  ssize_t SimulateRecvFrom(int sockfd, void* buf, size_t len, int flags,
                           struct sockaddr* src_addr, socklen_t* addrlen);

  MOCK_METHOD0(ReceiveCallback, void());

 protected:
  static const int kInterfaceIndex;
  static const int kSocketFD;
  static const uint8_t kEapPacketPayload[];

  bool CreateSocket() { return listener_.CreateSocket(); }
  int GetInterfaceIndex() { return listener_.interface_index_; }
  size_t GetMaxEapPacketLength() { return EapListener::kMaxEapPacketLength; }
  int GetSocket() { return listener_.socket_; }
  void StartListener() { StartListenerWithFD(kSocketFD); }
  void ReceiveRequest() { listener_.ReceiveRequest(kSocketFD); }
  void StartListenerWithFD(int fd);

  MockEventDispatcher dispatcher_;
  EapListener listener_;

  // Owned by EapListener, and tracked here only for mocks.
  MockSockets* sockets_;

  // Tests can assign this in order to set the data isreturned in our
  // mock implementation of Sockets::RecvFrom().
  ByteString recvfrom_reply_data_;
};

// static
const int EapListenerTest::kInterfaceIndex = 123;
const int EapListenerTest::kSocketFD = 456;
const uint8_t EapListenerTest::kEapPacketPayload[] = {
  eap_protocol::kIeee8021xEapolVersion2,
  eap_protocol::kIIeee8021xTypeEapPacket,
  0x00, 0x00,  // Payload length (should be 5, but unparsed by EapListener).
  eap_protocol::kEapCodeRequest,
  0x00,        // Identifier (unparsed).
  0x00, 0x00,  // Packet length (should be 5, but unparsed by EapListener).
  0x01         // Request type: Identity (not parsed by EapListener).
};

ssize_t EapListenerTest::SimulateRecvFrom(int sockfd, void* buf, size_t len,
                                          int flags, struct sockaddr* src_addr,
                                          socklen_t* addrlen) {
  // Mimic behavior of the real recvfrom -- copy no more than requested.
  int copy_length = std::min(recvfrom_reply_data_.GetLength(), len);
  memcpy(buf, recvfrom_reply_data_.GetConstData(), copy_length);
  return copy_length;
}

MATCHER_P(IsEapLinkAddress, interface_index, "") {
  const struct sockaddr_ll* socket_address =
      reinterpret_cast<const struct sockaddr_ll*>(arg);
  return socket_address->sll_family == AF_PACKET &&
      socket_address->sll_protocol == htons(ETH_P_PAE) &&
      socket_address->sll_ifindex == interface_index;
}

void EapListenerTest::StartListenerWithFD(int fd) {
  EXPECT_CALL(*sockets_, Socket(PF_PACKET, SOCK_DGRAM, htons(ETH_P_PAE)))
      .WillOnce(Return(fd));
  EXPECT_CALL(*sockets_, SetNonBlocking(fd)).WillOnce(Return(0));
  EXPECT_CALL(*sockets_,
              Bind(fd, IsEapLinkAddress(kInterfaceIndex), sizeof(sockaddr_ll)))
      .WillOnce(Return(0));
  EXPECT_CALL(dispatcher_, CreateReadyHandler(fd, IOHandler::kModeInput, _));
  EXPECT_TRUE(listener_.Start());
  EXPECT_EQ(fd, listener_.socket_);
}

TEST_F(EapListenerTest, Constructor) {
  EXPECT_EQ(kInterfaceIndex, GetInterfaceIndex());
  EXPECT_EQ(8, GetMaxEapPacketLength());
  EXPECT_EQ(-1, GetSocket());
}

TEST_F(EapListenerTest, SocketOpenFail) {
  ScopedMockLog log;
  EXPECT_CALL(log,
      Log(logging::LOG_ERROR, _,
          HasSubstr("Could not create EAP listener socket"))).Times(1);

  EXPECT_CALL(*sockets_, Socket(PF_PACKET, SOCK_DGRAM, htons(ETH_P_PAE)))
      .WillOnce(Return(-1));
  EXPECT_FALSE(CreateSocket());
}

TEST_F(EapListenerTest, SocketNonBlockingFail) {
  ScopedMockLog log;
  EXPECT_CALL(log,
      Log(logging::LOG_ERROR, _,
          HasSubstr("Could not set socket to be non-blocking"))).Times(1);

  EXPECT_CALL(*sockets_, Socket(_, _, _)).WillOnce(Return(kSocketFD));
  EXPECT_CALL(*sockets_, SetNonBlocking(kSocketFD)).WillOnce(Return(-1));
  EXPECT_FALSE(CreateSocket());
}

TEST_F(EapListenerTest, SocketBindFail) {
  ScopedMockLog log;
  EXPECT_CALL(log,
      Log(logging::LOG_ERROR, _,
          HasSubstr("Could not bind socket to interface"))).Times(1);

  EXPECT_CALL(*sockets_, Socket(_, _, _)).WillOnce(Return(kSocketFD));
  EXPECT_CALL(*sockets_, SetNonBlocking(kSocketFD)).WillOnce(Return(0));
  EXPECT_CALL(*sockets_, Bind(kSocketFD, _, _)).WillOnce(Return(-1));
  EXPECT_FALSE(CreateSocket());
}

TEST_F(EapListenerTest, StartSuccess) {
  StartListener();
}

TEST_F(EapListenerTest, StartMultipleTimes) {
  const int kFirstSocketFD = kSocketFD + 1;
  StartListenerWithFD(kFirstSocketFD);
  EXPECT_CALL(*sockets_, Close(kFirstSocketFD));
  StartListener();
}

TEST_F(EapListenerTest, Stop) {
  StartListener();
  EXPECT_EQ(kSocketFD, GetSocket());
  EXPECT_CALL(*sockets_, Close(kSocketFD));
  listener_.Stop();
  EXPECT_EQ(-1, GetSocket());
}


TEST_F(EapListenerTest, ReceiveFail) {
  StartListener();
  EXPECT_CALL(*sockets_,
              RecvFrom(kSocketFD, _, GetMaxEapPacketLength(), 0, _, _))
      .WillOnce(Return(-1));
  EXPECT_CALL(*this, ReceiveCallback()).Times(0);
  EXPECT_CALL(*sockets_, Close(kSocketFD));

  ScopedMockLog log;
  // RecvFrom returns an error.
  EXPECT_CALL(log,
      Log(logging::LOG_ERROR, _,
          HasSubstr("Socket recvfrom failed"))).Times(1);
  ReceiveRequest();
}

TEST_F(EapListenerTest, ReceiveEmpty) {
  StartListener();
  EXPECT_CALL(*sockets_,
              RecvFrom(kSocketFD, _, GetMaxEapPacketLength(), 0, _, _))
      .WillOnce(Return(0));
  EXPECT_CALL(*this, ReceiveCallback()).Times(0);
  ReceiveRequest();
}

TEST_F(EapListenerTest, ReceiveShort) {
  StartListener();
  recvfrom_reply_data_ = ByteString(kEapPacketPayload,
                                    GetMaxEapPacketLength() - 1);
  EXPECT_CALL(*sockets_,
              RecvFrom(kSocketFD, _, GetMaxEapPacketLength(), 0, _, _))
      .WillOnce(Invoke(this, &EapListenerTest::SimulateRecvFrom));
  EXPECT_CALL(*this, ReceiveCallback()).Times(0);
  ScopedMockLog log;
  EXPECT_CALL(log,
      Log(logging::LOG_INFO, _,
          HasSubstr("Short EAP packet received"))).Times(1);
  ReceiveRequest();
}

TEST_F(EapListenerTest, ReceiveInvalid) {
  StartListener();
  // We're partially initializing this field, just making sure at least one
  // part of it is incorrect.
  uint8_t bad_payload[sizeof(kEapPacketPayload)] = {
    eap_protocol::kIeee8021xEapolVersion1 - 1
  };
  recvfrom_reply_data_ = ByteString(bad_payload, sizeof(bad_payload));
  EXPECT_CALL(*sockets_,
              RecvFrom(kSocketFD, _, GetMaxEapPacketLength(), 0, _, _))
      .WillOnce(Invoke(this, &EapListenerTest::SimulateRecvFrom));
  EXPECT_CALL(*this, ReceiveCallback()).Times(0);
  ScopedMockLog log;
  EXPECT_CALL(log,
      Log(logging::LOG_INFO, _,
          HasSubstr("Packet is not a valid EAP request"))).Times(1);
  ReceiveRequest();
}

TEST_F(EapListenerTest, ReceiveSuccess) {
  StartListener();
  recvfrom_reply_data_ =
      ByteString(kEapPacketPayload, sizeof(kEapPacketPayload));
  EXPECT_CALL(*sockets_,
              RecvFrom(kSocketFD, _, GetMaxEapPacketLength(), 0, _, _))
      .WillOnce(Invoke(this, &EapListenerTest::SimulateRecvFrom));
  EXPECT_CALL(*this, ReceiveCallback()).Times(1);
  ReceiveRequest();
}

}  // namespace shill
