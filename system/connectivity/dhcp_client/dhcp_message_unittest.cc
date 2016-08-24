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

#include "dhcp_client/dhcp_message.h"

#include <netinet/in.h>

#include <cstring>

#include <gtest/gtest.h>
#include <shill/net/byte_string.h>

#include "dhcp_client/dhcp_options.h"

#define SERVER_NAME 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
                    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
                    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
                    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
                    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
                    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
                    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
                    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00

#define BOOT_FILE 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
                  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
                  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
                  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
                  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
                  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
                  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
                  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
                  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
                  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
                  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
                  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
                  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
                  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
                  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
                  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
#define COOKIE 0x63, 0x82, 0x53, 0x63
// The fake client hardware address(the first 6 bytes) contains a zero.
#define CLIENT_HARDWARE_ADDRESS 0xbf, 0x78, 0xa2, 0x00, \
                                0x0c, 0xea, 0x00, 0x00, \
                                0x00, 0x00, 0x00, 0x00, \
                                0x00, 0x00, 0x00, 0x00

#define TRANSACTION_ID 0x0f, 0x22, 0xa3, 0x50
#define CLIENT_IP_ADDRESS 0x00, 0x00, 0x00, 0x00
#define YOUR_IP_ADDRESS 0xaf, 0x23, 0x11, 0x34
#define NEXT_SERVER_IP_ADDRESS 0x00, 0x00, 0x00, 0x00
#define AGENT_IP_ADDRESS 0x00, 0x00, 0x00, 0x00
#define SECONDS 0x00, 0x00
#define FLAGS 0x00, 0x00
#define HOPS 0x00
#define HARDWARE_ADDRESS_LENGTH 0x06
#define HARDWARE_ADDRESS_TYPE 0x01
#define REQUEST 0x01
#define REPLY 0x02
#define END_TAG 0xff
#define SERVER_ID 0x01, 0xa2, 0x01, 0x1b
#define LEASE_TIME 0x00, 0x00, 0x11, 0x11

namespace dhcp_client {
namespace {
const uint8_t kFakeBufferEvenLength[] = {0x08, 0x00, 0x00, 0x00,
                                         0x71, 0x50, 0x00, 0x00};
const size_t kFakeBufferEvenLengthSize = 8;
const uint16_t kFakeBufferEvenLengthChecksum = 0x86af;

const uint8_t kFakeBufferOddLength[] = {0x08, 0x00, 0x00, 0x00, 0xac, 0x51,
                                        0x00, 0x00, 0x00, 0x00, 0x01};
const size_t kFakeBufferOddLengthSize = 11;
const uint16_t kFakeBufferOddLengthChecksum = 0x4aae;

const uint8_t kFakeDHCPOfferMessage[] = {
    REPLY,  // op, offer is a reply message
    HARDWARE_ADDRESS_TYPE,  // htype
    HARDWARE_ADDRESS_LENGTH,  // hlen
    HOPS,  // hops
    TRANSACTION_ID,  // xid
    SECONDS,  // secs
    FLAGS,  // flags
    CLIENT_IP_ADDRESS,  // ciaddr
    YOUR_IP_ADDRESS,  // yiaddr
    NEXT_SERVER_IP_ADDRESS,  // siaddr
    AGENT_IP_ADDRESS,  // giaddr
    CLIENT_HARDWARE_ADDRESS,  // chaddr
    SERVER_NAME,  // sname
    BOOT_FILE,  // file
    COOKIE,  // cookie
    kDHCPOptionMessageType, 0x01, kDHCPMessageTypeOffer,  // message type option
    kDHCPOptionLeaseTime, 0x04, LEASE_TIME,  // lease time option
    kDHCPOptionServerIdentifier, 0x04, SERVER_ID,  // server identifier option
    END_TAG  // options end tag
};

const uint8_t kFakeDHCPAckMessage[] = {
    REPLY,  // op, ack is a reply message
    HARDWARE_ADDRESS_TYPE,  // htype
    HARDWARE_ADDRESS_LENGTH,  // hlen
    HOPS,  // hops
    TRANSACTION_ID,  // xid
    SECONDS,  // secs
    FLAGS,  // flags
    CLIENT_IP_ADDRESS,  // ciaddr
    YOUR_IP_ADDRESS,  // yiaddr
    NEXT_SERVER_IP_ADDRESS,  // siaddr
    AGENT_IP_ADDRESS,  // giaddr
    CLIENT_HARDWARE_ADDRESS,  // chaddr
    SERVER_NAME,  // sname
    BOOT_FILE,  // file
    COOKIE,  // cookie
    kDHCPOptionMessageType, 0x01, kDHCPMessageTypeAck,  // message type option
    kDHCPOptionLeaseTime, 0x04, LEASE_TIME,  // lease time option
    kDHCPOptionServerIdentifier, 0x04, SERVER_ID,  // server identifier option
    END_TAG  // options end tag
};

const uint8_t kFakeDHCPNakMessage[] = {
    REPLY,  // op, nak is a reply message
    HARDWARE_ADDRESS_TYPE,  // htype
    HARDWARE_ADDRESS_LENGTH,  // hlen
    HOPS,  // hops
    TRANSACTION_ID,  // xid
    SECONDS,  // secs
    FLAGS,  // flags
    CLIENT_IP_ADDRESS,  // ciaddr
    YOUR_IP_ADDRESS,  // yiaddr
    NEXT_SERVER_IP_ADDRESS,  // siaddr
    AGENT_IP_ADDRESS,  // giaddr
    CLIENT_HARDWARE_ADDRESS,  // chaddr
    SERVER_NAME,  // sname
    BOOT_FILE,  // file
    COOKIE,  // cookie
    kDHCPOptionMessageType, 0x01, kDHCPMessageTypeNak,  // message type option
    kDHCPOptionServerIdentifier, 0x04, SERVER_ID,  // server identifier option
    END_TAG  // options end tag
};
const uint8_t kFakeTransactionID[] = {TRANSACTION_ID};
const uint8_t kFakeServerIdentifier[] = {SERVER_ID};
const uint8_t kFakeLeaseTime[] = {LEASE_TIME};
const uint8_t kFakeYourIPAddress[] = {YOUR_IP_ADDRESS};
const uint8_t kFakeHardwareAddress[] = {CLIENT_HARDWARE_ADDRESS};
size_t kFakeDHCPOfferMessageLength = sizeof(kFakeDHCPOfferMessage);
size_t kFakeDHCPAckMessageLength = sizeof(kFakeDHCPAckMessage);
size_t kFakeDHCPNakMessageLength = sizeof(kFakeDHCPNakMessage);
}  // namespace

class DHCPMessageTest : public testing::Test {
 public:
  DHCPMessageTest() {}
 protected:
};

TEST_F(DHCPMessageTest, ComputeChecksumEvenLengthTest) {
  uint16_t checksum = DHCPMessage::ComputeChecksum(kFakeBufferEvenLength,
                                                   kFakeBufferEvenLengthSize);
  EXPECT_EQ(kFakeBufferEvenLengthChecksum, checksum);
}

TEST_F(DHCPMessageTest, ComputeChecksumOddLengthTest) {
  uint16_t checksum = DHCPMessage::ComputeChecksum(kFakeBufferOddLength,
                                                   kFakeBufferOddLengthSize);
  EXPECT_EQ(kFakeBufferOddLengthChecksum, checksum);
}

TEST_F(DHCPMessageTest, InitFromBufferMessageTypeOffer) {
  DHCPMessage msg;
  EXPECT_TRUE(DHCPMessage::InitFromBuffer(kFakeDHCPOfferMessage,
                                          kFakeDHCPOfferMessageLength,
                                          &msg));
  EXPECT_EQ(kDHCPMessageTypeOffer, msg.message_type());
  EXPECT_EQ(ntohl(*reinterpret_cast<const uint32_t*>(kFakeTransactionID)),
            msg.transaction_id());
  EXPECT_EQ(ntohl(*reinterpret_cast<const uint32_t*>(kFakeServerIdentifier)),
            msg.server_identifier());
  EXPECT_EQ(ntohl(*reinterpret_cast<const uint32_t*>(kFakeLeaseTime)),
            msg.lease_time());
  EXPECT_EQ(ntohl(*reinterpret_cast<const uint32_t*>(kFakeYourIPAddress)),
            msg.your_ip_address());
  EXPECT_EQ(0, std::memcmp(kFakeHardwareAddress,
                           msg.client_hardware_address().GetConstData(),
                           msg.client_hardware_address().GetLength()));
}

TEST_F(DHCPMessageTest, InitFromBufferMessageTypeAck) {
  DHCPMessage msg;
  EXPECT_TRUE(DHCPMessage::InitFromBuffer(kFakeDHCPAckMessage,
                                          kFakeDHCPAckMessageLength,
                                          &msg));
  EXPECT_EQ(kDHCPMessageTypeAck, msg.message_type());
  EXPECT_EQ(ntohl(*reinterpret_cast<const uint32_t*>(kFakeTransactionID)),
            msg.transaction_id());
  EXPECT_EQ(ntohl(*reinterpret_cast<const uint32_t*>(kFakeServerIdentifier)),
            msg.server_identifier());
  EXPECT_EQ(ntohl(*reinterpret_cast<const uint32_t*>(kFakeLeaseTime)),
            msg.lease_time());
  EXPECT_EQ(ntohl(*reinterpret_cast<const uint32_t*>(kFakeYourIPAddress)),
            msg.your_ip_address());
  EXPECT_EQ(0, std::memcmp(kFakeHardwareAddress,
                           msg.client_hardware_address().GetConstData(),
                           msg.client_hardware_address().GetLength()));
}

TEST_F(DHCPMessageTest, InitFromBufferMessageTypeNak) {
  DHCPMessage msg;
  EXPECT_TRUE(DHCPMessage::InitFromBuffer(kFakeDHCPNakMessage,
                                          kFakeDHCPNakMessageLength,
                                          &msg));
  EXPECT_EQ(kDHCPMessageTypeNak, msg.message_type());
  EXPECT_EQ(ntohl(*reinterpret_cast<const uint32_t*>(kFakeTransactionID)),
            msg.transaction_id());
  EXPECT_EQ(ntohl(*reinterpret_cast<const uint32_t*>(kFakeServerIdentifier)),
            msg.server_identifier());
  EXPECT_EQ(0, std::memcmp(kFakeHardwareAddress,
                           msg.client_hardware_address().GetConstData(),
                           msg.client_hardware_address().GetLength()));
}

}  // namespace dhcp_client
