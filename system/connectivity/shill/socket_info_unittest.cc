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

#include "shill/socket_info.h"

#include <gtest/gtest.h>

namespace shill {

namespace {

const unsigned char kIPAddress1[] = { 192, 168, 1, 1 };
const unsigned char kIPAddress2[] = { 192, 168, 1, 2 };
const unsigned char kIPAddress3[] = { 192, 168, 1, 3 };
const uint16_t kPort1 = 1000;
const uint16_t kPort2 = 2000;
const uint16_t kPort3 = 3000;

}  // namespace

class SocketInfoTest : public testing::Test {
 protected:
  void ExpectSocketInfoEqual(const SocketInfo& info1, const SocketInfo& info2) {
    EXPECT_EQ(info1.connection_state(), info2.connection_state());
    EXPECT_TRUE(info1.local_ip_address().Equals(info2.local_ip_address()));
    EXPECT_EQ(info1.local_port(), info2.local_port());
    EXPECT_TRUE(info1.remote_ip_address().Equals(info2.remote_ip_address()));
    EXPECT_EQ(info1.remote_port(), info2.remote_port());
    EXPECT_EQ(info1.transmit_queue_value(), info2.transmit_queue_value());
    EXPECT_EQ(info1.receive_queue_value(), info2.receive_queue_value());
    EXPECT_EQ(info1.timer_state(), info2.timer_state());
  }
};

TEST_F(SocketInfoTest, CopyConstructor) {
  SocketInfo info(SocketInfo::kConnectionStateEstablished,
                  IPAddress(IPAddress::kFamilyIPv4,
                            ByteString(kIPAddress1, sizeof(kIPAddress1))),
                  kPort1,
                  IPAddress(IPAddress::kFamilyIPv4,
                            ByteString(kIPAddress2, sizeof(kIPAddress2))),
                  kPort2,
                  10,
                  20,
                  SocketInfo::kTimerStateRetransmitTimerPending);

  SocketInfo info_copy(info);
  ExpectSocketInfoEqual(info, info_copy);
}

TEST_F(SocketInfoTest, AssignmentOperator) {
  SocketInfo info(SocketInfo::kConnectionStateEstablished,
                  IPAddress(IPAddress::kFamilyIPv4,
                            ByteString(kIPAddress1, sizeof(kIPAddress1))),
                  kPort1,
                  IPAddress(IPAddress::kFamilyIPv4,
                            ByteString(kIPAddress2, sizeof(kIPAddress2))),
                  kPort2,
                  10,
                  20,
                  SocketInfo::kTimerStateRetransmitTimerPending);

  SocketInfo info_copy = info;
  ExpectSocketInfoEqual(info, info_copy);
}

TEST_F(SocketInfoTest, IsSameSocketAs) {
  IPAddress ip_address1(IPAddress::kFamilyIPv4,
                        ByteString(kIPAddress1, sizeof(kIPAddress1)));
  IPAddress ip_address2(IPAddress::kFamilyIPv4,
                        ByteString(kIPAddress2, sizeof(kIPAddress2)));
  IPAddress ip_address3(IPAddress::kFamilyIPv4,
                        ByteString(kIPAddress3, sizeof(kIPAddress3)));

  SocketInfo info(SocketInfo::kConnectionStateEstablished,
                  ip_address1,
                  kPort1,
                  ip_address2,
                  kPort2,
                  0,
                  0,
                  SocketInfo::kTimerStateNoTimerPending);

  // Differs only by local address.
  EXPECT_FALSE(info.IsSameSocketAs(
      SocketInfo(SocketInfo::kConnectionStateEstablished,
                 ip_address3,
                 kPort1,
                 ip_address2,
                 kPort2,
                 0,
                 0,
                 SocketInfo::kTimerStateNoTimerPending)));

  // Differs only by local port.
  EXPECT_FALSE(info.IsSameSocketAs(
      SocketInfo(SocketInfo::kConnectionStateEstablished,
                 ip_address1,
                 kPort3,
                 ip_address2,
                 kPort2,
                 0,
                 0,
                 SocketInfo::kTimerStateNoTimerPending)));

  // Differs only by remote address.
  EXPECT_FALSE(info.IsSameSocketAs(
      SocketInfo(SocketInfo::kConnectionStateEstablished,
                 ip_address1,
                 kPort1,
                 ip_address3,
                 kPort2,
                 0,
                 0,
                 SocketInfo::kTimerStateNoTimerPending)));

  // Differs only by remote port.
  EXPECT_FALSE(info.IsSameSocketAs(
      SocketInfo(SocketInfo::kConnectionStateEstablished,
                 ip_address1,
                 kPort1,
                 ip_address2,
                 kPort3,
                 0,
                 0,
                 SocketInfo::kTimerStateNoTimerPending)));

  // Only local address, local port, remote address, and remote port are
  // identical.
  EXPECT_TRUE(info.IsSameSocketAs(
      SocketInfo(SocketInfo::kConnectionStateClosing,
                 ip_address1,
                 kPort1,
                 ip_address2,
                 kPort2,
                 10,
                 20,
                 SocketInfo::kTimerStateRetransmitTimerPending)));
}

}  // namespace shill
