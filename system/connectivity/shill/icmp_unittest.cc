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

#include "shill/icmp.h"

#include <netinet/in.h>
#include <netinet/ip_icmp.h>

#include <gtest/gtest.h>

#include "shill/mock_log.h"
#include "shill/net/ip_address.h"
#include "shill/net/mock_sockets.h"

using testing::_;
using testing::HasSubstr;
using testing::InSequence;
using testing::Return;
using testing::StrictMock;
using testing::Test;

namespace shill {

namespace {

// These binary blobs representing ICMP headers and their respective checksums
// were taken directly from Wireshark ICMP packet captures and are given in big
// endian. The checksum field is zeroed in |kIcmpEchoRequestEvenLen| and
// |kIcmpEchoRequestOddLen| so the checksum can be calculated on the header in
// IcmpTest.ComputeIcmpChecksum.
const uint8_t kIcmpEchoRequestEvenLen[] = {0x08, 0x00, 0x00, 0x00,
                                           0x71, 0x50, 0x00, 0x00};
const uint8_t kIcmpEchoRequestEvenLenChecksum[] = {0x86, 0xaf};
const uint8_t kIcmpEchoRequestOddLen[] = {0x08, 0x00, 0x00, 0x00, 0xac, 0x51,
                                          0x00, 0x00, 0x00, 0x00, 0x01};
const uint8_t kIcmpEchoRequestOddLenChecksum[] = {0x4a, 0xae};

}  // namespace

class IcmpTest : public Test {
 public:
  IcmpTest() {}
  virtual ~IcmpTest() {}

  virtual void SetUp() {
    sockets_ = new StrictMock<MockSockets>();
    // Passes ownership.
    icmp_.sockets_.reset(sockets_);
  }

  virtual void TearDown() {
    if (icmp_.IsStarted()) {
      EXPECT_CALL(*sockets_, Close(kSocketFD));
      icmp_.Stop();
    }
    EXPECT_FALSE(icmp_.IsStarted());
  }

 protected:
  static const int kSocketFD;
  static const char kIPAddress[];

  int GetSocket() { return icmp_.socket_; }
  bool StartIcmp() { return StartIcmpWithFD(kSocketFD); }
  bool StartIcmpWithFD(int fd) {
    EXPECT_CALL(*sockets_, Socket(AF_INET, SOCK_RAW, IPPROTO_ICMP))
        .WillOnce(Return(fd));
    EXPECT_CALL(*sockets_, SetNonBlocking(fd)).WillOnce(Return(0));
    bool start_status = icmp_.Start();
    EXPECT_TRUE(start_status);
    EXPECT_EQ(fd, icmp_.socket_);
    EXPECT_TRUE(icmp_.IsStarted());
    return start_status;
  }
  uint16_t ComputeIcmpChecksum(const struct icmphdr &hdr, size_t len) {
    return Icmp::ComputeIcmpChecksum(hdr, len);
  }

  // Owned by Icmp, and tracked here only for mocks.
  MockSockets* sockets_;

  Icmp icmp_;
};


const int IcmpTest::kSocketFD = 456;
const char IcmpTest::kIPAddress[] = "10.0.1.1";


TEST_F(IcmpTest, Constructor) {
  EXPECT_EQ(-1, GetSocket());
  EXPECT_FALSE(icmp_.IsStarted());
}

TEST_F(IcmpTest, SocketOpenFail) {
  ScopedMockLog log;
  EXPECT_CALL(log,
      Log(logging::LOG_ERROR, _,
          HasSubstr("Could not create ICMP socket"))).Times(1);

  EXPECT_CALL(*sockets_, Socket(AF_INET, SOCK_RAW, IPPROTO_ICMP))
      .WillOnce(Return(-1));
  EXPECT_FALSE(icmp_.Start());
  EXPECT_FALSE(icmp_.IsStarted());
}

TEST_F(IcmpTest, SocketNonBlockingFail) {
  ScopedMockLog log;
  EXPECT_CALL(log,
      Log(logging::LOG_ERROR, _,
          HasSubstr("Could not set socket to be non-blocking"))).Times(1);

  EXPECT_CALL(*sockets_, Socket(_, _, _)).WillOnce(Return(kSocketFD));
  EXPECT_CALL(*sockets_, SetNonBlocking(kSocketFD)).WillOnce(Return(-1));
  EXPECT_CALL(*sockets_, Close(kSocketFD));
  EXPECT_FALSE(icmp_.Start());
  EXPECT_FALSE(icmp_.IsStarted());
}

TEST_F(IcmpTest, StartMultipleTimes) {
  const int kFirstSocketFD = kSocketFD + 1;
  StartIcmpWithFD(kFirstSocketFD);
  EXPECT_CALL(*sockets_, Close(kFirstSocketFD));
  StartIcmp();
}

MATCHER_P(IsIcmpHeader, header, "") {
  return memcmp(arg, &header, sizeof(header)) == 0;
}


MATCHER_P(IsSocketAddress, address, "") {
  const struct sockaddr_in* sock_addr =
      reinterpret_cast<const struct sockaddr_in*>(arg);
  return sock_addr->sin_family == address.family() &&
      memcmp(&sock_addr->sin_addr.s_addr, address.GetConstData(),
             address.GetLength()) == 0;
}

TEST_F(IcmpTest, TransmitEchoRequest) {
  StartIcmp();
  // Address isn't valid.
  EXPECT_FALSE(
      icmp_.TransmitEchoRequest(IPAddress(IPAddress::kFamilyIPv4), 1, 1));

  // IPv6 adresses aren't implemented.
  IPAddress ipv6_destination(IPAddress::kFamilyIPv6);
  EXPECT_TRUE(ipv6_destination.SetAddressFromString(
      "fe80::1aa9:5ff:abcd:1234"));
  EXPECT_FALSE(icmp_.TransmitEchoRequest(ipv6_destination, 1, 1));

  IPAddress ipv4_destination(IPAddress::kFamilyIPv4);
  EXPECT_TRUE(ipv4_destination.SetAddressFromString(kIPAddress));

  struct icmphdr icmp_header;
  memset(&icmp_header, 0, sizeof(icmp_header));
  icmp_header.type = ICMP_ECHO;
  icmp_header.code = Icmp::kIcmpEchoCode;
  icmp_header.un.echo.id = 1;
  icmp_header.un.echo.sequence = 1;
  icmp_header.checksum = ComputeIcmpChecksum(icmp_header, sizeof(icmp_header));

  EXPECT_CALL(*sockets_, SendTo(kSocketFD,
                                IsIcmpHeader(icmp_header),
                                sizeof(icmp_header),
                                0,
                                IsSocketAddress(ipv4_destination),
                                sizeof(sockaddr_in)))
      .WillOnce(Return(-1))
      .WillOnce(Return(0))
      .WillOnce(Return(sizeof(icmp_header) - 1))
      .WillOnce(Return(sizeof(icmp_header)));
  {
    InSequence seq;
    ScopedMockLog log;
    EXPECT_CALL(log,
        Log(logging::LOG_ERROR, _,
            HasSubstr("Socket sendto failed"))).Times(1);
    EXPECT_CALL(log,
        Log(logging::LOG_ERROR, _,
            HasSubstr("less than the expected result"))).Times(2);

    EXPECT_FALSE(icmp_.TransmitEchoRequest(ipv4_destination, 1, 1));
    EXPECT_FALSE(icmp_.TransmitEchoRequest(ipv4_destination, 1, 1));
    EXPECT_FALSE(icmp_.TransmitEchoRequest(ipv4_destination, 1, 1));
    EXPECT_TRUE(icmp_.TransmitEchoRequest(ipv4_destination, 1, 1));
  }
}

TEST_F(IcmpTest, ComputeIcmpChecksum) {
  EXPECT_EQ(*reinterpret_cast<const uint16_t*>(kIcmpEchoRequestEvenLenChecksum),
            ComputeIcmpChecksum(*reinterpret_cast<const struct icmphdr*>(
                                    kIcmpEchoRequestEvenLen),
                                sizeof(kIcmpEchoRequestEvenLen)));
  EXPECT_EQ(*reinterpret_cast<const uint16_t*>(kIcmpEchoRequestOddLenChecksum),
            ComputeIcmpChecksum(*reinterpret_cast<const struct icmphdr*>(
                                    kIcmpEchoRequestOddLen),
                                sizeof(kIcmpEchoRequestOddLen)));
}

}  // namespace shill
