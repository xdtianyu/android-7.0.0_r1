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

#include "shill/arp_packet.h"

#include <gtest/gtest.h>

#include "shill/mock_log.h"

using testing::_;
using testing::HasSubstr;
using testing::Test;

namespace shill {

namespace {
const uint8_t kArpRequestV4[] =
    { 0x00, 0x01, 0x08, 0x00, 0x06, 0x04, 0x00, 0x01 };
const uint8_t kArpRequestV6[] =
    { 0x00, 0x01, 0x86, 0xdd, 0x06, 0x10, 0x00, 0x01 };
const uint8_t kArpReplyV4[] =
    { 0x00, 0x01, 0x08, 0x00, 0x06, 0x04, 0x00, 0x02 };
const uint8_t kArpReplyV6[] =
    { 0x00, 0x01, 0x86, 0xdd, 0x06, 0x10, 0x00, 0x02 };
const char kIPv4Address0[] = "192.168.0.1";
const char kIPv4Address1[] = "10.0.12.13";
const char kIPv6Address0[] = "fe80::1aa9:5ff:7ebf:14c5";
const char kIPv6Address1[] = "1980:0:0:1000:1b02:1aa9:5ff:7ebf";
const uint8_t kMACAddress0[] = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05 };
const uint8_t kMACAddress1[] = { 0x88, 0x87, 0x86, 0x85, 0x84, 0x83 };
const uint8_t kInsertedByte[] = { 0x00 };
const size_t kArpPaddingSizeV4 = 18;
const size_t kArpPaddingSizeV6 = 0;
}  // namespace

class ArpPacketTest : public Test {
 public:
  ArpPacketTest()
      : ipv4_address0_(IPAddress::kFamilyIPv4),
        ipv4_address1_(IPAddress::kFamilyIPv4),
        ipv6_address0_(IPAddress::kFamilyIPv6),
        ipv6_address1_(IPAddress::kFamilyIPv6),
        mac_address0_(kMACAddress0, arraysize(kMACAddress0)),
        mac_address1_(kMACAddress1, arraysize(kMACAddress1)),
        inserted_byte_(kInsertedByte, arraysize(kInsertedByte)) {}
  virtual ~ArpPacketTest() {}

  virtual void SetUp() {
    EXPECT_TRUE(ipv4_address0_.SetAddressFromString(kIPv4Address0));
    EXPECT_TRUE(ipv4_address1_.SetAddressFromString(kIPv4Address1));
    EXPECT_TRUE(ipv6_address0_.SetAddressFromString(kIPv6Address0));
    EXPECT_TRUE(ipv6_address1_.SetAddressFromString(kIPv6Address1));
  }

 protected:
  IPAddress ipv4_address0_;
  IPAddress ipv4_address1_;
  IPAddress ipv6_address0_;
  IPAddress ipv6_address1_;
  ByteString mac_address0_;
  ByteString mac_address1_;
  ByteString inserted_byte_;
  ArpPacket packet_;
};

TEST_F(ArpPacketTest, Constructor) {
  EXPECT_FALSE(packet_.local_ip_address().IsValid());
  EXPECT_FALSE(packet_.remote_ip_address().IsValid());
  EXPECT_TRUE(packet_.local_mac_address().IsEmpty());
  EXPECT_TRUE(packet_.remote_mac_address().IsEmpty());
}

TEST_F(ArpPacketTest, GettersAndSetters) {
  packet_.set_local_ip_address(ipv4_address0_);
  packet_.set_remote_ip_address(ipv6_address1_);
  packet_.set_local_mac_address(mac_address0_);
  packet_.set_remote_mac_address(mac_address1_);
  EXPECT_TRUE(ipv4_address0_.Equals(packet_.local_ip_address()));
  EXPECT_TRUE(ipv6_address1_.Equals(packet_.remote_ip_address()));
  EXPECT_TRUE(mac_address0_.Equals(packet_.local_mac_address()));
  EXPECT_TRUE(mac_address1_.Equals(packet_.remote_mac_address()));
}

TEST_F(ArpPacketTest, ParseTinyPacket) {
  ScopedMockLog log;
  EXPECT_CALL(log,
      Log(logging::LOG_ERROR, _,
          HasSubstr("too short to contain ARP header."))).Times(1);

  ByteString arp_bytes(kArpReplyV4, arraysize(kArpReplyV4));
  arp_bytes.Resize(arp_bytes.GetLength() - 1);
  EXPECT_FALSE(packet_.Parse(arp_bytes));
}

TEST_F(ArpPacketTest, ParseBadHRDType) {
  ScopedMockLog log;
  EXPECT_CALL(log,
      Log(logging::LOG_ERROR, _,
          HasSubstr("Packet is of unknown ARPHRD type 257"))).Times(1);

  ByteString arp_bytes(kArpReplyV4, arraysize(kArpReplyV4));
  arp_bytes.GetData()[0] = 0x1;
  EXPECT_FALSE(packet_.Parse(arp_bytes));
}

TEST_F(ArpPacketTest, ParseBadProtocol) {
  ScopedMockLog log;
  EXPECT_CALL(log,
      Log(logging::LOG_ERROR, _,
          HasSubstr("Packet has unknown protocol 2049"))).Times(1);

  ByteString arp_bytes(kArpReplyV4, arraysize(kArpReplyV4));
  arp_bytes.GetData()[3] = 0x1;
  EXPECT_FALSE(packet_.Parse(arp_bytes));
}

TEST_F(ArpPacketTest, ParseBadHardwareLength) {
  ScopedMockLog log;
  EXPECT_CALL(log,
      Log(logging::LOG_ERROR, _,
          HasSubstr("Packet has unexpected hardware address length"))).Times(1);

  ByteString arp_bytes(kArpReplyV4, arraysize(kArpReplyV4));
  arp_bytes.GetData()[4] = 0x1;
  EXPECT_FALSE(packet_.Parse(arp_bytes));
}

TEST_F(ArpPacketTest, ParseBadProtocolLength) {
  ScopedMockLog log;
  EXPECT_CALL(log,
      Log(logging::LOG_ERROR, _,
          HasSubstr("Packet has unexpected protocol address length"))).Times(1);

  ByteString arp_bytes(kArpReplyV4, arraysize(kArpReplyV4));
  arp_bytes.GetData()[5] = 0x1;
  EXPECT_FALSE(packet_.Parse(arp_bytes));
}

TEST_F(ArpPacketTest, ParseBadOpCode) {
  ScopedMockLog log;
  EXPECT_CALL(log,
      Log(logging::LOG_ERROR, _,
          HasSubstr("Packet is not an ARP reply or request but of type 258")))
              .Times(1);

  ByteString arp_bytes(kArpReplyV4, arraysize(kArpReplyV4));
  arp_bytes.GetData()[6] = 0x1;
  EXPECT_FALSE(packet_.Parse(arp_bytes));
}

TEST_F(ArpPacketTest, ParseShortPacket) {
  ScopedMockLog log;
  EXPECT_CALL(log,
      Log(logging::LOG_ERROR, _,
          HasSubstr("is too small to contain entire ARP payload"))).Times(1);

  ByteString arp_bytes(kArpReplyV6, arraysize(kArpReplyV6));
  arp_bytes.Append(mac_address1_);
  arp_bytes.Append(ipv6_address0_.address());
  arp_bytes.Append(mac_address0_);
  arp_bytes.Append(ipv6_address1_.address());
  arp_bytes.Resize(arp_bytes.GetLength() - 1);
  EXPECT_FALSE(packet_.Parse(arp_bytes));
}

TEST_F(ArpPacketTest, ParseIPv4) {
  ByteString arp_bytes(kArpReplyV4, arraysize(kArpReplyV4));
  arp_bytes.Append(mac_address0_);
  arp_bytes.Append(ipv4_address0_.address());
  arp_bytes.Append(mac_address1_);
  arp_bytes.Append(ipv4_address1_.address());
  EXPECT_TRUE(packet_.Parse(arp_bytes));
  EXPECT_TRUE(packet_.IsReply());
  EXPECT_TRUE(ipv4_address0_.Equals(packet_.local_ip_address()));
  EXPECT_TRUE(ipv4_address1_.Equals(packet_.remote_ip_address()));
  EXPECT_TRUE(mac_address0_.Equals(packet_.local_mac_address()));
  EXPECT_TRUE(mac_address1_.Equals(packet_.remote_mac_address()));

  // Parse should succeed with arbitrary trailing padding.
  arp_bytes.Append(ByteString(1000));
  EXPECT_TRUE(packet_.Parse(arp_bytes));
}

TEST_F(ArpPacketTest, ParseIPv6) {
  ByteString arp_bytes(kArpReplyV6, arraysize(kArpReplyV6));
  arp_bytes.Append(mac_address1_);
  arp_bytes.Append(ipv6_address0_.address());
  arp_bytes.Append(mac_address0_);
  arp_bytes.Append(ipv6_address1_.address());
  EXPECT_TRUE(packet_.Parse(arp_bytes));
  EXPECT_TRUE(packet_.IsReply());
  EXPECT_TRUE(ipv6_address0_.Equals(packet_.local_ip_address()));
  EXPECT_TRUE(ipv6_address1_.Equals(packet_.remote_ip_address()));
  EXPECT_TRUE(mac_address1_.Equals(packet_.local_mac_address()));
  EXPECT_TRUE(mac_address0_.Equals(packet_.remote_mac_address()));
}

TEST_F(ArpPacketTest, ParseRequest) {
  ByteString arp_bytes(kArpRequestV4, arraysize(kArpRequestV4));
  arp_bytes.Append(mac_address0_);
  arp_bytes.Append(ipv4_address0_.address());
  arp_bytes.Append(mac_address1_);
  arp_bytes.Append(ipv4_address1_.address());
  EXPECT_TRUE(packet_.Parse(arp_bytes));
  EXPECT_FALSE(packet_.IsReply());
  EXPECT_TRUE(ipv4_address0_.Equals(packet_.local_ip_address()));
  EXPECT_TRUE(ipv4_address1_.Equals(packet_.remote_ip_address()));
  EXPECT_TRUE(mac_address0_.Equals(packet_.local_mac_address()));
  EXPECT_TRUE(mac_address1_.Equals(packet_.remote_mac_address()));
}

TEST_F(ArpPacketTest, FormatRequestInvalidAddress) {
  ScopedMockLog log;
  EXPECT_CALL(log,
      Log(logging::LOG_ERROR, _,
          HasSubstr("Local or remote IP address is not valid"))).Times(3);

  ByteString arp_bytes;
  EXPECT_FALSE(packet_.FormatRequest(&arp_bytes));
  packet_.set_local_ip_address(ipv4_address0_);
  EXPECT_FALSE(packet_.FormatRequest(&arp_bytes));
  packet_.set_local_ip_address(IPAddress(IPAddress::kFamilyUnknown));
  packet_.set_remote_ip_address(ipv4_address0_);
  EXPECT_FALSE(packet_.FormatRequest(&arp_bytes));
}

TEST_F(ArpPacketTest, FormatRequestMismatchedAddresses) {
  ScopedMockLog log;
  EXPECT_CALL(log,
      Log(logging::LOG_ERROR, _,
          HasSubstr("IP address families do not match"))).Times(1);

  ByteString arp_bytes;
  packet_.set_local_ip_address(ipv4_address0_);
  packet_.set_remote_ip_address(ipv6_address1_);
  EXPECT_FALSE(packet_.FormatRequest(&arp_bytes));
}

TEST_F(ArpPacketTest, FormatRequestBadMACAddressLength) {
  ScopedMockLog log;
  EXPECT_CALL(log,
      Log(logging::LOG_ERROR, _,
          HasSubstr("MAC address length is incorrect"))).Times(3);

  ByteString arp_bytes;
  packet_.set_local_ip_address(ipv4_address0_);
  packet_.set_remote_ip_address(ipv4_address1_);
  EXPECT_FALSE(packet_.FormatRequest(&arp_bytes));
  packet_.set_local_mac_address(mac_address0_);
  EXPECT_FALSE(packet_.FormatRequest(&arp_bytes));
  packet_.set_local_mac_address(ByteString());
  packet_.set_remote_mac_address(mac_address0_);
  EXPECT_FALSE(packet_.FormatRequest(&arp_bytes));
}

TEST_F(ArpPacketTest, FormatRequestIPv4) {
  ByteString arp_bytes;
  packet_.set_local_ip_address(ipv4_address0_);
  packet_.set_remote_ip_address(ipv4_address1_);
  packet_.set_local_mac_address(mac_address0_);
  packet_.set_remote_mac_address(mac_address1_);
  EXPECT_TRUE(packet_.FormatRequest(&arp_bytes));

  ByteString expected_bytes(kArpRequestV4, arraysize(kArpRequestV4));
  expected_bytes.Append(mac_address0_);
  expected_bytes.Append(ipv4_address0_.address());
  expected_bytes.Append(mac_address1_);
  expected_bytes.Append(ipv4_address1_.address());
  expected_bytes.Append(ByteString(kArpPaddingSizeV4));
  EXPECT_TRUE(expected_bytes.Equals(arp_bytes));
}

TEST_F(ArpPacketTest, FormatRequestIPv6) {
  ByteString arp_bytes;
  packet_.set_local_ip_address(ipv6_address0_);
  packet_.set_remote_ip_address(ipv6_address1_);
  packet_.set_local_mac_address(mac_address1_);
  packet_.set_remote_mac_address(mac_address0_);
  EXPECT_TRUE(packet_.FormatRequest(&arp_bytes));

  ByteString expected_bytes(kArpRequestV6, arraysize(kArpRequestV6));
  expected_bytes.Append(mac_address1_);
  expected_bytes.Append(ipv6_address0_.address());
  expected_bytes.Append(mac_address0_);
  expected_bytes.Append(ipv6_address1_.address());
  expected_bytes.Append(ByteString(kArpPaddingSizeV6));
  EXPECT_TRUE(expected_bytes.Equals(arp_bytes));
}

}  // namespace shill
