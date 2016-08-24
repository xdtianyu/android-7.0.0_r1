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

#include "shill/net/ip_address.h"

#include <gtest/gtest.h>

#include <arpa/inet.h>

#include "shill/net/byte_string.h"

using std::string;
using testing::Test;

namespace shill {

namespace {
const char kV4String1[] = "192.168.10.1";
const unsigned char kV4Address1[] = { 192, 168, 10, 1 };
const char kV4String2[] = "192.168.10";
const unsigned char kV4Address2[] = { 192, 168, 10 };
const char kV6String1[] = "fe80::1aa9:5ff:7ebf:14c5";
const unsigned char kV6Address1[] = { 0xfe, 0x80, 0x00, 0x00,
                                      0x00, 0x00, 0x00, 0x00,
                                      0x1a, 0xa9, 0x05, 0xff,
                                      0x7e, 0xbf, 0x14, 0xc5 };
const char kV6String2[] = "1980:0:1000:1b02:1aa9:5ff:7ebf";
const unsigned char kV6Address2[] = { 0x19, 0x80, 0x00, 0x00,
                                      0x10, 0x00, 0x1b, 0x02,
                                      0x1a, 0xa9, 0x05, 0xff,
                                      0x7e, 0xbf };
}  // namespace

class IPAddressTest : public Test {
 protected:
  void TestAddress(IPAddress::Family family,
                   const string& good_string,
                   const ByteString& good_bytes,
                   const string& bad_string,
                   const ByteString& bad_bytes) {
    IPAddress good_addr(family);

    EXPECT_TRUE(good_addr.SetAddressFromString(good_string));
    EXPECT_EQ(IPAddress::GetAddressLength(family), good_addr.GetLength());
    EXPECT_EQ(family, good_addr.family());
    EXPECT_FALSE(good_addr.IsDefault());
    EXPECT_EQ(0, memcmp(good_addr.GetConstData(), good_bytes.GetConstData(),
                        good_bytes.GetLength()));
    EXPECT_TRUE(good_addr.address().Equals(good_bytes));
    string address_string;
    EXPECT_TRUE(good_addr.IntoString(&address_string));
    EXPECT_EQ(good_string, address_string);

    IPAddress good_addr_from_bytes(family, good_bytes);
    EXPECT_TRUE(good_addr.Equals(good_addr_from_bytes));

    IPAddress good_addr_from_string(good_string);
    EXPECT_EQ(family, good_addr_from_string.family());

    IPAddress bad_addr(family);
    EXPECT_FALSE(bad_addr.SetAddressFromString(bad_string));
    EXPECT_FALSE(good_addr.Equals(bad_addr));

    EXPECT_FALSE(bad_addr.IsValid());

    IPAddress bad_addr_from_bytes(family, bad_bytes);
    EXPECT_EQ(family, bad_addr_from_bytes.family());
    EXPECT_FALSE(bad_addr_from_bytes.IsValid());

    IPAddress bad_addr_from_string(bad_string);
    EXPECT_EQ(IPAddress::kFamilyUnknown, bad_addr_from_string.family());

    EXPECT_FALSE(bad_addr.Equals(bad_addr_from_bytes));
    EXPECT_FALSE(bad_addr.IntoString(&address_string));

    sockaddr_storage storage = {};
    auto addr = reinterpret_cast<sockaddr*>(&storage);
    addr->sa_family = family;
    ssize_t addr_size;
    if (family == IPAddress::kFamilyIPv6) {
      auto sin6 = reinterpret_cast<sockaddr_in6*>(addr);
      inet_pton(AF_INET6, good_string.c_str(), &sin6->sin6_addr.s6_addr);
      addr_size = sizeof(sockaddr_in6);
    } else {
      auto sin = reinterpret_cast<sockaddr_in*>(addr);
      inet_pton(AF_INET, good_string.c_str(), &sin->sin_addr.s_addr);
      addr_size = sizeof(sockaddr_in);
    }
    IPAddress from_short_sockaddr(addr, addr_size - 1);
    EXPECT_FALSE(from_short_sockaddr.IsValid());
    IPAddress from_sockaddr(addr, addr_size);
    EXPECT_TRUE(from_sockaddr.IsValid());
    EXPECT_EQ(family, from_sockaddr.family());
    EXPECT_TRUE(from_sockaddr.IntoString(&address_string));
    EXPECT_EQ(good_string, address_string);

    sockaddr_storage storage_empty = {};
    sockaddr_storage storage2 = {};
    auto addr2 = reinterpret_cast<sockaddr*>(&storage2);
    EXPECT_FALSE(from_short_sockaddr.IntoSockAddr(addr2, addr_size));
    EXPECT_EQ(0, memcmp(&storage2, &storage_empty, sizeof(storage2)));
    EXPECT_FALSE(from_sockaddr.IntoSockAddr(addr2, addr_size - 1));
    EXPECT_EQ(0, memcmp(&storage2, &storage_empty, sizeof(storage2)));
    EXPECT_TRUE(from_sockaddr.IntoSockAddr(addr2, addr_size));
    EXPECT_EQ(0, memcmp(&storage2, &storage, sizeof(storage2)));
  }
};

TEST_F(IPAddressTest, Statics) {
  EXPECT_EQ(4, IPAddress::GetAddressLength(IPAddress::kFamilyIPv4));
  EXPECT_EQ(16, IPAddress::GetAddressLength(IPAddress::kFamilyIPv6));

  EXPECT_EQ(0, IPAddress::GetPrefixLengthFromMask(IPAddress::kFamilyIPv4,
                                                  "0.0.0.0"));
  EXPECT_EQ(20, IPAddress::GetPrefixLengthFromMask(IPAddress::kFamilyIPv4,
                                                   "255.255.240.0"));
  EXPECT_EQ(32, IPAddress::GetPrefixLengthFromMask(IPAddress::kFamilyIPv4,
                                                   "255.255.255.255"));
  EXPECT_EQ(32, IPAddress::GetPrefixLengthFromMask(IPAddress::kFamilyIPv4,
                                                   ""));
  EXPECT_EQ(32, IPAddress::GetPrefixLengthFromMask(IPAddress::kFamilyIPv4,
                                                   "foo"));

  IPAddress addr4(IPAddress::kFamilyIPv4);
  addr4.SetAddressToDefault();

  EXPECT_EQ(4, addr4.GetLength());
  EXPECT_EQ(IPAddress::kFamilyIPv4, addr4.family());
  EXPECT_TRUE(addr4.IsDefault());
  EXPECT_TRUE(addr4.address().IsZero());
  EXPECT_TRUE(addr4.address().Equals(ByteString(4)));


  IPAddress addr6(IPAddress::kFamilyIPv6);
  addr6.SetAddressToDefault();

  EXPECT_EQ(16, addr6.GetLength());
  EXPECT_EQ(addr6.family(), IPAddress::kFamilyIPv6);
  EXPECT_TRUE(addr6.IsDefault());
  EXPECT_TRUE(addr6.address().IsZero());
  EXPECT_TRUE(addr6.address().Equals(ByteString(16)));

  EXPECT_FALSE(addr4.Equals(addr6));
}

TEST_F(IPAddressTest, IPv4) {
  TestAddress(IPAddress::kFamilyIPv4,
              kV4String1, ByteString(kV4Address1, sizeof(kV4Address1)),
              kV4String2, ByteString(kV4Address2, sizeof(kV4Address2)));
}


TEST_F(IPAddressTest, IPv6) {
  TestAddress(IPAddress::kFamilyIPv6,
              kV6String1, ByteString(kV6Address1, sizeof(kV6Address1)),
              kV6String2, ByteString(kV6Address2, sizeof(kV6Address2)));
}

TEST_F(IPAddressTest, SetAddressAndPrefixFromString) {
  IPAddress address(IPAddress::kFamilyIPv4);
  const string kString1(kV4String1);
  const string kString2(kV4String2);
  EXPECT_FALSE(address.SetAddressAndPrefixFromString(""));
  EXPECT_FALSE(address.SetAddressAndPrefixFromString(kString1));
  EXPECT_FALSE(address.SetAddressAndPrefixFromString(kString1 + "/"));
  EXPECT_FALSE(address.SetAddressAndPrefixFromString(kString1 + "/10x"));
  EXPECT_FALSE(address.SetAddressAndPrefixFromString(kString2 + "/10"));
  EXPECT_TRUE(address.SetAddressAndPrefixFromString(kString1 + "/0"));
  EXPECT_EQ(0, address.prefix());
  EXPECT_TRUE(address.SetAddressAndPrefixFromString(kString1 + "/32"));
  EXPECT_EQ(32, address.prefix());
  EXPECT_FALSE(address.SetAddressAndPrefixFromString(kString1 + "/33"));
  EXPECT_FALSE(address.SetAddressAndPrefixFromString(kString1 + "/-1"));
  EXPECT_TRUE(address.SetAddressAndPrefixFromString(kString1 + "/10"));
  EXPECT_EQ(10, address.prefix());
  ByteString kAddress1(kV4Address1, sizeof(kV4Address1));
  EXPECT_TRUE(kAddress1.Equals(address.address()));
}

TEST_F(IPAddressTest, HasSameAddressAs) {
  const string kString1(kV4String1);
  IPAddress address0(IPAddress::kFamilyIPv4);
  EXPECT_TRUE(address0.SetAddressAndPrefixFromString(kString1 + "/0"));
  IPAddress address1(IPAddress::kFamilyIPv4);
  EXPECT_TRUE(address1.SetAddressAndPrefixFromString(kString1 + "/10"));
  IPAddress address2(IPAddress::kFamilyIPv4);
  EXPECT_TRUE(address2.SetAddressAndPrefixFromString(kString1 + "/0"));

  EXPECT_FALSE(address0.Equals(address1));
  EXPECT_TRUE(address0.Equals(address2));
  EXPECT_TRUE(address0.HasSameAddressAs(address1));
  EXPECT_TRUE(address0.HasSameAddressAs(address2));
}

struct PrefixMapping {
  PrefixMapping() : family(IPAddress::kFamilyUnknown), prefix(0) {}
  PrefixMapping(IPAddress::Family family_in,
                size_t prefix_in,
                const string& expected_address_in)
      : family(family_in),
        prefix(prefix_in),
        expected_address(expected_address_in) {}
  IPAddress::Family family;
  size_t prefix;
  string expected_address;
};

class IPAddressPrefixMappingTest
    : public testing::TestWithParam<PrefixMapping> {};

TEST_P(IPAddressPrefixMappingTest, TestPrefixMapping) {
  IPAddress address = IPAddress::GetAddressMaskFromPrefix(GetParam().family,
                                                          GetParam().prefix);
  IPAddress expected_address(GetParam().family);
  EXPECT_TRUE(expected_address.SetAddressFromString(
      GetParam().expected_address));
  EXPECT_TRUE(expected_address.Equals(address));
}

INSTANTIATE_TEST_CASE_P(
    IPAddressPrefixMappingTestRun,
    IPAddressPrefixMappingTest,
    ::testing::Values(
        PrefixMapping(IPAddress::kFamilyIPv4, 0, "0.0.0.0"),
        PrefixMapping(IPAddress::kFamilyIPv4, 1, "128.0.0.0"),
        PrefixMapping(IPAddress::kFamilyIPv4, 4, "240.0.0.0"),
        PrefixMapping(IPAddress::kFamilyIPv4, 7, "254.0.0.0"),
        PrefixMapping(IPAddress::kFamilyIPv4, 10, "255.192.0.0"),
        PrefixMapping(IPAddress::kFamilyIPv4, 13, "255.248.0.0"),
        PrefixMapping(IPAddress::kFamilyIPv4, 16, "255.255.0.0"),
        PrefixMapping(IPAddress::kFamilyIPv4, 19, "255.255.224.0"),
        PrefixMapping(IPAddress::kFamilyIPv4, 22, "255.255.252.0"),
        PrefixMapping(IPAddress::kFamilyIPv4, 25, "255.255.255.128"),
        PrefixMapping(IPAddress::kFamilyIPv4, 28, "255.255.255.240"),
        PrefixMapping(IPAddress::kFamilyIPv4, 31, "255.255.255.254"),
        PrefixMapping(IPAddress::kFamilyIPv4, 32, "255.255.255.255"),
        PrefixMapping(IPAddress::kFamilyIPv4, 33, "255.255.255.255"),
        PrefixMapping(IPAddress::kFamilyIPv4, 34, "255.255.255.255"),
        PrefixMapping(IPAddress::kFamilyIPv6, 0, "0::"),
        PrefixMapping(IPAddress::kFamilyIPv6, 1, "8000::"),
        PrefixMapping(IPAddress::kFamilyIPv6, 17, "ffff:8000::"),
        PrefixMapping(IPAddress::kFamilyIPv6, 34, "ffff:ffff:c000::"),
        PrefixMapping(IPAddress::kFamilyIPv6, 51, "ffff:ffff:ffff:e000::"),
        PrefixMapping(IPAddress::kFamilyIPv6, 68,
                      "ffff:ffff:ffff:ffff:f000::"),
        PrefixMapping(IPAddress::kFamilyIPv6, 85,
                      "ffff:ffff:ffff:ffff:ffff:f800::"),
        PrefixMapping(IPAddress::kFamilyIPv6, 102,
                      "ffff:ffff:ffff:ffff:ffff:ffff:fc00::"),
        PrefixMapping(IPAddress::kFamilyIPv6, 119,
                      "ffff:ffff:ffff:ffff:ffff:ffff:ffff:fe00"),
        PrefixMapping(IPAddress::kFamilyIPv6, 128,
                      "ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff"),
        PrefixMapping(IPAddress::kFamilyIPv6, 136,
                      "ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff")));

struct BitOperationMapping {
  BitOperationMapping() : family(IPAddress::kFamilyUnknown) {}
  BitOperationMapping(IPAddress::Family family_in,
              const string& address_a_in,
              const string& address_b_in,
              const string& expected_anded_in,
              const string& expected_orred_in)
      : family(family_in),
        address_a(address_a_in),
        address_b(address_b_in),
        expected_anded(expected_anded_in),
        expected_orred(expected_orred_in) {}
  IPAddress::Family family;
  string address_a;
  string address_b;
  string expected_anded;
  string expected_orred;
};

class IPAddressBitOperationMappingTest
    : public testing::TestWithParam<BitOperationMapping> {};

TEST_P(IPAddressBitOperationMappingTest, TestBitOperationMapping) {
  IPAddress address_a(GetParam().family);
  EXPECT_TRUE(address_a.SetAddressFromString(GetParam().address_a));
  IPAddress address_b(GetParam().family);
  EXPECT_TRUE(address_b.SetAddressFromString(GetParam().address_b));
  IPAddress expected_anded(GetParam().family);
  EXPECT_TRUE(expected_anded.SetAddressFromString(
      GetParam().expected_anded));
  EXPECT_TRUE(expected_anded.Equals(address_a.MaskWith(address_b)));
  IPAddress expected_orred(GetParam().family);
  EXPECT_TRUE(expected_orred.SetAddressFromString(
      GetParam().expected_orred));
  EXPECT_TRUE(expected_orred.Equals(address_a.MergeWith(address_b)));
}

INSTANTIATE_TEST_CASE_P(
    IPAddressBitOperationMappingTestRun,
    IPAddressBitOperationMappingTest,
    ::testing::Values(
        BitOperationMapping(IPAddress::kFamilyIPv4,
                            "255.255.255.255", "0.0.0.0",
                            "0.0.0.0", "255.255.255.255"),
        BitOperationMapping(IPAddress::kFamilyIPv4,
                            "0.0.0.0", "255.255.255.255",
                            "0.0.0.0", "255.255.255.255"),
        BitOperationMapping(IPAddress::kFamilyIPv4,
                            "170.170.170.170", "85.85.85.85",
                            "0.0.0.0", "255.255.255.255"),
        BitOperationMapping(IPAddress::kFamilyIPv4,
                            "238.187.119.221", "119.221.238.187",
                            "102.153.102.153", "255.255.255.255"),
        BitOperationMapping(IPAddress::kFamilyIPv4,
                            "17.68.136.34", "119.221.238.187",
                            "17.68.136.34", "119.221.238.187"),
        BitOperationMapping(IPAddress::kFamilyIPv4,
                            "192.168.1.10", "255.255.255.0",
                            "192.168.1.0", "255.255.255.10")));

struct NetworkPartMapping {
  NetworkPartMapping() : family(IPAddress::kFamilyUnknown) {}
  NetworkPartMapping(IPAddress::Family family_in,
                     const string& address_in,
                     size_t prefix_in,
                     const string& expected_network_in,
                     const string& expected_broadcast_in)
      : family(family_in),
        address(address_in),
        prefix(prefix_in),
        expected_network(expected_network_in),
        expected_broadcast(expected_broadcast_in) {}
  IPAddress::Family family;
  string address;
  size_t prefix;
  string expected_network;
  string expected_broadcast;
};

class IPAddressNetworkPartMappingTest
    : public testing::TestWithParam<NetworkPartMapping> {};

TEST_P(IPAddressNetworkPartMappingTest, TestNetworkPartMapping) {
  IPAddress address(GetParam().family);
  EXPECT_TRUE(address.SetAddressFromString(GetParam().address));
  IPAddress expected_network(GetParam().family);
  EXPECT_TRUE(expected_network.SetAddressFromString(
      GetParam().expected_network));
  address.set_prefix(GetParam().prefix);
  EXPECT_TRUE(expected_network.Equals(address.GetNetworkPart()));
  IPAddress expected_broadcast(GetParam().family);
  EXPECT_TRUE(expected_broadcast.SetAddressFromString(
      GetParam().expected_broadcast));
  EXPECT_TRUE(expected_broadcast.Equals(address.GetDefaultBroadcast()));
}

INSTANTIATE_TEST_CASE_P(
    IPAddressNetworkPartMappingTestRun,
    IPAddressNetworkPartMappingTest,
    ::testing::Values(
        NetworkPartMapping(IPAddress::kFamilyIPv4,
                           "255.255.255.255", 0, "0.0.0.0", "255.255.255.255"),
        NetworkPartMapping(IPAddress::kFamilyIPv4,
                           "255.255.255.255", 32,
                           "255.255.255.255", "255.255.255.255"),
        NetworkPartMapping(IPAddress::kFamilyIPv4,
                           "255.255.255.255", 24,
                           "255.255.255.0", "255.255.255.255"),
        NetworkPartMapping(IPAddress::kFamilyIPv4,
                           "255.255.255.255", 16,
                           "255.255.0.0", "255.255.255.255"),
        NetworkPartMapping(IPAddress::kFamilyIPv4,
                           "0.0.0.0", 0, "0.0.0.0", "255.255.255.255"),
        NetworkPartMapping(IPAddress::kFamilyIPv4,
                           "0.0.0.0", 32, "0.0.0.0", "0.0.0.0"),
        NetworkPartMapping(IPAddress::kFamilyIPv4,
                           "0.0.0.0", 24, "0.0.0.0", "0.0.0.255"),
        NetworkPartMapping(IPAddress::kFamilyIPv4,
                           "0.0.0.0", 16, "0.0.0.0", "0.0.255.255"),
        NetworkPartMapping(IPAddress::kFamilyIPv4,
                           "192.168.1.1", 24, "192.168.1.0", "192.168.1.255"),
        NetworkPartMapping(IPAddress::kFamilyIPv4,
                           "10.1.0.1", 8, "10.0.0.0", "10.255.255.255")));

struct MinPrefixLengthMapping {
  MinPrefixLengthMapping() : family(IPAddress::kFamilyUnknown) {}
  MinPrefixLengthMapping(IPAddress::Family family_in,
                         const string& address_in,
                         size_t expected_min_prefix_in)
      : family(family_in),
        address(address_in),
        expected_min_prefix(expected_min_prefix_in) {}
  IPAddress::Family family;
  string address;
  size_t expected_min_prefix;
};

class IPAddressMinPrefixLengthMappingTest
    : public testing::TestWithParam<MinPrefixLengthMapping> {};

TEST_P(IPAddressMinPrefixLengthMappingTest, TestMinPrefixLengthMapping) {
  IPAddress address(GetParam().family);
  EXPECT_TRUE(address.SetAddressFromString(GetParam().address));
  EXPECT_EQ(GetParam().expected_min_prefix, address.GetMinPrefixLength());
}

INSTANTIATE_TEST_CASE_P(
    IPAddressMinPrefixLengthMappingTestRun,
    IPAddressMinPrefixLengthMappingTest,
    ::testing::Values(
        MinPrefixLengthMapping(IPAddress::kFamilyIPv6, "fe80::", 128),
        MinPrefixLengthMapping(IPAddress::kFamilyIPv4, "255.255.255.255", 32),
        MinPrefixLengthMapping(IPAddress::kFamilyIPv4, "224.0.0.0", 32),
        MinPrefixLengthMapping(IPAddress::kFamilyIPv4, "192.168.0.0", 24),
        MinPrefixLengthMapping(IPAddress::kFamilyIPv4, "172.16.0.0", 16),
        MinPrefixLengthMapping(IPAddress::kFamilyIPv4, "10.10.10.10", 8)));

struct CanReachAddressMapping {
  CanReachAddressMapping() : family(IPAddress::kFamilyUnknown) {}
  CanReachAddressMapping(IPAddress::Family family_in,
                         const string& address_a_in,
                         const string& address_b_in,
                         bool expected_result_in)
      : family(family_in),
        address_a(address_a_in),
        address_b(address_b_in),
        expected_result(expected_result_in) {}
  IPAddress::Family family;
  string address_a;
  string address_b;
  size_t expected_result;
};

class IPAddressCanReachAddressMappingTest
    : public testing::TestWithParam<CanReachAddressMapping> {};

TEST_P(IPAddressCanReachAddressMappingTest, TestCanReachAddressMapping) {
  IPAddress address_a(GetParam().family);
  EXPECT_TRUE(address_a.SetAddressAndPrefixFromString(GetParam().address_a));
  IPAddress address_b(GetParam().family);
  EXPECT_TRUE(address_b.SetAddressAndPrefixFromString(GetParam().address_b));
  EXPECT_EQ(GetParam().expected_result, address_a.CanReachAddress(address_b));
}

INSTANTIATE_TEST_CASE_P(
    IPAddressCanReachAddressMappingTestRun,
    IPAddressCanReachAddressMappingTest,
    ::testing::Values(
        CanReachAddressMapping(IPAddress::kFamilyIPv6,
                               "fe80:1000::/16", "fe80:2000::/16", true),
        CanReachAddressMapping(IPAddress::kFamilyIPv6,
                               "fe80:1000::/16", "fe80:2000::/32", true),
        CanReachAddressMapping(IPAddress::kFamilyIPv6,
                               "fe80:1000::/32", "fe80:2000::/16", false),
        CanReachAddressMapping(IPAddress::kFamilyIPv4,
                               "192.168.1.1/24", "192.168.1.2/24", true),
        CanReachAddressMapping(IPAddress::kFamilyIPv4,
                               "192.168.1.1/24", "192.168.2.2/24", false),
        CanReachAddressMapping(IPAddress::kFamilyIPv4,
                               "192.168.1.1/16", "192.168.2.2/24", true),
        CanReachAddressMapping(IPAddress::kFamilyIPv4,
                               "192.168.1.1/24", "192.168.2.2/16", false)));

}  // namespace shill
