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

#include "shill/net/netlink_packet.h"

#include <linux/netlink.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using testing::Test;

namespace shill {

class NetlinkPacketTest : public Test {
};

TEST_F(NetlinkPacketTest, Constructor) {
  // A null pointer should not crash the constructor, but should yield
  // an invalid packet.
  NetlinkPacket null_packet(nullptr, 100);
  EXPECT_FALSE(null_packet.IsValid());

  unsigned char data[sizeof(nlmsghdr) + 1];
  memset(&data, 0, sizeof(data));

  // A packet that is too short to contain an nlmsghdr should be invalid.
  NetlinkPacket short_packet(data, sizeof(nlmsghdr) - 1);
  EXPECT_FALSE(short_packet.IsValid());

  // A packet that contains an invalid nlmsg_len (should be at least
  // as large as sizeof(nlmgsghdr)) should be invalid.
  NetlinkPacket invalid_packet(data, sizeof(nlmsghdr));
  EXPECT_FALSE(invalid_packet.IsValid());

  // Successfully parse a well-formed packet that has no payload.
  nlmsghdr hdr;
  memset(&hdr, 0, sizeof(hdr));
  hdr.nlmsg_len = sizeof(hdr);
  hdr.nlmsg_type = 1;
  memcpy(&data, &hdr, sizeof(hdr));
  NetlinkPacket empty_packet(data, sizeof(nlmsghdr));
  EXPECT_TRUE(empty_packet.IsValid());
  EXPECT_EQ(sizeof(nlmsghdr), empty_packet.GetLength());
  EXPECT_EQ(1, empty_packet.GetMessageType());
  char payload_byte = 0;
  EXPECT_FALSE(empty_packet.ConsumeData(1, &payload_byte));

  // A packet that contains an nlmsg_len that is larger than the
  // data provided should be invalid.
  hdr.nlmsg_len = sizeof(hdr) + 1;
  hdr.nlmsg_type = 2;
  memcpy(&data, &hdr, sizeof(hdr));
  NetlinkPacket incomplete_packet(data, sizeof(nlmsghdr));
  EXPECT_FALSE(incomplete_packet.IsValid());

  // Retrieve a byte from a well-formed packet.  After that byte is
  // retrieved, no more data can be consumed.
  data[sizeof(nlmsghdr)] = 10;
  NetlinkPacket complete_packet(data, sizeof(nlmsghdr) + 1);
  EXPECT_TRUE(complete_packet.IsValid());
  EXPECT_EQ(sizeof(nlmsghdr) + 1, complete_packet.GetLength());
  EXPECT_EQ(2, complete_packet.GetMessageType());
  EXPECT_EQ(1, complete_packet.GetRemainingLength());
  EXPECT_TRUE(complete_packet.ConsumeData(1, &payload_byte));
  EXPECT_EQ(10, payload_byte);
  EXPECT_FALSE(complete_packet.ConsumeData(1, &payload_byte));
}

TEST_F(NetlinkPacketTest, ConsumeData) {
  // This code assumes that the value of NLMSG_ALIGNTO is 4, and that nlmsghdr
  // is aligned to a 4-byte boundary.
  static_assert(NLMSG_ALIGNTO == 4, "NLMSG_ALIGNTO sized has changed");
  static_assert((sizeof(nlmsghdr) % NLMSG_ALIGNTO) == 0,
                "nlmsghdr is not aligned with NLMSG_ALIGNTO");

  const char kString1[] = "A";
  const char kString2[] = "pattern";
  const char kString3[] = "so";
  const char kString4[] = "grand";

  // Assert string sizes (with null terminator).
  ASSERT_EQ(2, sizeof(kString1));
  ASSERT_EQ(8, sizeof(kString2));
  ASSERT_EQ(3, sizeof(kString3));
  ASSERT_EQ(6, sizeof(kString4));

  unsigned char data[sizeof(nlmsghdr) + 22];
  memset(data, 0, sizeof(data));
  nlmsghdr hdr;
  memset(&hdr, 0, sizeof(hdr));
  hdr.nlmsg_len = sizeof(data);
  memcpy(data, &hdr, sizeof(hdr));
  memcpy(data + sizeof(nlmsghdr), kString1, sizeof(kString1));
  memcpy(data + sizeof(nlmsghdr) + 4, kString2, sizeof(kString2));
  memcpy(data + sizeof(nlmsghdr) + 12, kString3, sizeof(kString3));
  memcpy(data + sizeof(nlmsghdr) + 16, kString4, sizeof(kString4));

  NetlinkPacket packet(data, sizeof(data));
  EXPECT_EQ(22, packet.GetRemainingLength());

  // Consuming 2 bytes of data also consumed 2 bytes of padding.
  char string_piece[8];
  EXPECT_TRUE(packet.ConsumeData(2, &string_piece));
  EXPECT_STREQ(kString1, string_piece);
  EXPECT_EQ(18, packet.GetRemainingLength());

  // An aligned read (8 bytes) should read no more than this number.
  EXPECT_TRUE(packet.ConsumeData(8, &string_piece));
  EXPECT_STREQ(kString2, string_piece);
  EXPECT_EQ(10, packet.GetRemainingLength());

  // Try an odd-numbered unaligned read.
  EXPECT_TRUE(packet.ConsumeData(3, &string_piece));
  EXPECT_STREQ(kString3, string_piece);
  EXPECT_EQ(6, packet.GetRemainingLength());

  // Reading more than is left should fail, and should not consume anything.
  EXPECT_FALSE(packet.ConsumeData(7, &string_piece));
  EXPECT_EQ(6, packet.GetRemainingLength());

  // Reading a correctly-sized unalinged value which consumes the rest of
  // the buffer should succeed.
  EXPECT_TRUE(packet.ConsumeData(6, &string_piece));
  EXPECT_STREQ(kString4, string_piece);
  EXPECT_EQ(0, packet.GetRemainingLength());
}

}  // namespace shill
