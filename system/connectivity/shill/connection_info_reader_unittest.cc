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

#include "shill/connection_info_reader.h"

#include <netinet/in.h>

#include <base/files/file_util.h>
#include <base/files/scoped_temp_dir.h>
#include <base/strings/stringprintf.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using base::FilePath;
using base::ScopedTempDir;
using base::StringPrintf;
using std::string;
using std::vector;
using testing::Return;

namespace shill {

namespace {

// TODO(benchan): Test IPv6 addresses.

const char* kConnectionInfoLines[] = {
  "udp      17 30 src=192.168.1.1 dst=192.168.1.2 sport=9000 dport=53 "
  "[UNREPLIED] src=192.168.1.2 dst=192.168.1.1 sport=53 dport=9000 use=2",
  "tcp      6 299 ESTABLISHED src=192.168.2.1 dst=192.168.2.3 sport=8000 "
  "dport=7000 src=192.168.2.3 dst=192.168.2.1 sport=7000 dport=8000 [ASSURED] "
  "use=2",
};

}  // namespace

class ConnectionInfoReaderUnderTest : public ConnectionInfoReader {
 public:
  // Mock out GetConnectionInfoFilePath to use a temporary created connection
  // info file instead of the actual path in procfs (i.e.
  // /proc/net/ip_conntrack).
  MOCK_CONST_METHOD0(GetConnectionInfoFilePath, FilePath());
};

class ConnectionInfoReaderTest : public testing::Test {
 protected:
  IPAddress StringToIPv4Address(const string& address_string) {
    IPAddress ip_address(IPAddress::kFamilyIPv4);
    EXPECT_TRUE(ip_address.SetAddressFromString(address_string));
    return ip_address;
  }

  IPAddress StringToIPv6Address(const string& address_string) {
    IPAddress ip_address(IPAddress::kFamilyIPv6);
    EXPECT_TRUE(ip_address.SetAddressFromString(address_string));
    return ip_address;
  }

  void CreateConnectionInfoFile(const char** lines, size_t num_lines,
                                const FilePath& dir_path, FilePath* file_path) {
    ASSERT_TRUE(base::CreateTemporaryFileInDir(dir_path, file_path));
    for (size_t i = 0; i < num_lines; ++i) {
      string line = lines[i];
      line += '\n';
      ASSERT_TRUE(base::AppendToFile(*file_path, line.data(), line.size()));
    }
  }

  void ExpectConnectionInfoEqual(const ConnectionInfo& info1,
                                 const ConnectionInfo& info2) {
    EXPECT_EQ(info1.protocol(), info2.protocol());
    EXPECT_EQ(info1.time_to_expire_seconds(), info2.time_to_expire_seconds());
    EXPECT_EQ(info1.is_unreplied(), info2.is_unreplied());
    EXPECT_TRUE(info1.original_source_ip_address()
                    .Equals(info2.original_source_ip_address()));
    EXPECT_EQ(info1.original_source_port(), info2.original_source_port());
    EXPECT_TRUE(info1.original_destination_ip_address()
                    .Equals(info2.original_destination_ip_address()));
    EXPECT_EQ(info1.original_destination_port(),
              info2.original_destination_port());
    EXPECT_TRUE(info1.reply_source_ip_address()
                    .Equals(info2.reply_source_ip_address()));
    EXPECT_EQ(info1.reply_source_port(), info2.reply_source_port());
    EXPECT_TRUE(info1.reply_destination_ip_address()
                    .Equals(info2.reply_destination_ip_address()));
    EXPECT_EQ(info1.reply_destination_port(), info2.reply_destination_port());
  }

  ConnectionInfoReaderUnderTest reader_;
};

TEST_F(ConnectionInfoReaderTest, LoadConnectionInfo) {
  vector<ConnectionInfo> info_list;
  ScopedTempDir temp_dir;
  ASSERT_TRUE(temp_dir.CreateUniqueTempDir());

  // Loading a non-existent file should fail.
  FilePath info_file("/non-existent-file");
  EXPECT_CALL(reader_, GetConnectionInfoFilePath()).WillOnce(Return(info_file));
  EXPECT_FALSE(reader_.LoadConnectionInfo(&info_list));

  // Loading an empty file should succeed.
  CreateConnectionInfoFile(kConnectionInfoLines, 0, temp_dir.path(),
                           &info_file);
  EXPECT_CALL(reader_, GetConnectionInfoFilePath()).WillOnce(Return(info_file));
  EXPECT_TRUE(reader_.LoadConnectionInfo(&info_list));
  EXPECT_TRUE(info_list.empty());

  // Loading a non-empty file should succeed.
  CreateConnectionInfoFile(kConnectionInfoLines,
                           arraysize(kConnectionInfoLines),
                           temp_dir.path(),
                           &info_file);
  EXPECT_CALL(reader_, GetConnectionInfoFilePath()).WillOnce(Return(info_file));
  EXPECT_TRUE(reader_.LoadConnectionInfo(&info_list));
  EXPECT_EQ(arraysize(kConnectionInfoLines), info_list.size());

  ExpectConnectionInfoEqual(ConnectionInfo(IPPROTO_UDP,
                                           30,
                                           true,
                                           StringToIPv4Address("192.168.1.1"),
                                           9000,
                                           StringToIPv4Address("192.168.1.2"),
                                           53,
                                           StringToIPv4Address("192.168.1.2"),
                                           53,
                                           StringToIPv4Address("192.168.1.1"),
                                           9000),
                            info_list[0]);
  ExpectConnectionInfoEqual(ConnectionInfo(IPPROTO_TCP,
                                           299,
                                           false,
                                           StringToIPv4Address("192.168.2.1"),
                                           8000,
                                           StringToIPv4Address("192.168.2.3"),
                                           7000,
                                           StringToIPv4Address("192.168.2.3"),
                                           7000,
                                           StringToIPv4Address("192.168.2.1"),
                                           8000),
                            info_list[1]);
}

TEST_F(ConnectionInfoReaderTest, ParseConnectionInfo) {
  ConnectionInfo info;

  EXPECT_FALSE(reader_.ParseConnectionInfo("", &info));

  EXPECT_TRUE(reader_.ParseConnectionInfo(kConnectionInfoLines[0], &info));
  ExpectConnectionInfoEqual(ConnectionInfo(IPPROTO_UDP,
                                           30,
                                           true,
                                           StringToIPv4Address("192.168.1.1"),
                                           9000,
                                           StringToIPv4Address("192.168.1.2"),
                                           53,
                                           StringToIPv4Address("192.168.1.2"),
                                           53,
                                           StringToIPv4Address("192.168.1.1"),
                                           9000),
                            info);
}

TEST_F(ConnectionInfoReaderTest, ParseProtocol) {
  int protocol = 0;

  EXPECT_FALSE(reader_.ParseProtocol("", &protocol));
  EXPECT_FALSE(reader_.ParseProtocol("a", &protocol));
  EXPECT_FALSE(reader_.ParseProtocol("-1", &protocol));
  EXPECT_FALSE(reader_.ParseProtocol(StringPrintf("%d", IPPROTO_MAX),
                                     &protocol));

  for (int i = 0; i < IPPROTO_MAX; ++i) {
    EXPECT_TRUE(reader_.ParseProtocol(StringPrintf("%d", i), &protocol));
    EXPECT_EQ(i, protocol);
  }
}

TEST_F(ConnectionInfoReaderTest, ParseTimeToExpireSeconds) {
  int64_t time_to_expire = 0;

  EXPECT_FALSE(reader_.ParseTimeToExpireSeconds("", &time_to_expire));
  EXPECT_FALSE(reader_.ParseTimeToExpireSeconds("a", &time_to_expire));
  EXPECT_FALSE(reader_.ParseTimeToExpireSeconds("-1", &time_to_expire));

  EXPECT_TRUE(reader_.ParseTimeToExpireSeconds("100", &time_to_expire));
  EXPECT_EQ(100, time_to_expire);
}

TEST_F(ConnectionInfoReaderTest, ParseIPAddress) {
  IPAddress ip_address(IPAddress::kFamilyUnknown);
  bool is_source = false;

  EXPECT_FALSE(reader_.ParseIPAddress("", &ip_address, &is_source));
  EXPECT_FALSE(reader_.ParseIPAddress("abc", &ip_address, &is_source));
  EXPECT_FALSE(reader_.ParseIPAddress("src=", &ip_address, &is_source));
  EXPECT_FALSE(reader_.ParseIPAddress("src=abc", &ip_address, &is_source));
  EXPECT_FALSE(reader_.ParseIPAddress("dst=", &ip_address, &is_source));
  EXPECT_FALSE(reader_.ParseIPAddress("dst=abc", &ip_address, &is_source));

  EXPECT_TRUE(reader_.ParseIPAddress("src=192.168.1.1",
                                     &ip_address, &is_source));
  EXPECT_TRUE(ip_address.Equals(StringToIPv4Address("192.168.1.1")));
  EXPECT_TRUE(is_source);
  EXPECT_TRUE(reader_.ParseIPAddress("dst=192.168.1.2",
                                     &ip_address, &is_source));
  EXPECT_TRUE(ip_address.Equals(StringToIPv4Address("192.168.1.2")));
  EXPECT_FALSE(is_source);
}

TEST_F(ConnectionInfoReaderTest, ParsePort) {
  uint16_t port = 0;
  bool is_source = false;

  EXPECT_FALSE(reader_.ParsePort("", &port, &is_source));
  EXPECT_FALSE(reader_.ParsePort("a", &port, &is_source));
  EXPECT_FALSE(reader_.ParsePort("0", &port, &is_source));
  EXPECT_FALSE(reader_.ParsePort("sport=", &port, &is_source));
  EXPECT_FALSE(reader_.ParsePort("sport=a", &port, &is_source));
  EXPECT_FALSE(reader_.ParsePort("sport=-1", &port, &is_source));
  EXPECT_FALSE(reader_.ParsePort("sport=65536", &port, &is_source));
  EXPECT_FALSE(reader_.ParsePort("dport=", &port, &is_source));
  EXPECT_FALSE(reader_.ParsePort("dport=a", &port, &is_source));
  EXPECT_FALSE(reader_.ParsePort("dport=-1", &port, &is_source));
  EXPECT_FALSE(reader_.ParsePort("dport=65536", &port, &is_source));

  EXPECT_TRUE(reader_.ParsePort("sport=53", &port, &is_source));
  EXPECT_EQ(53, port);
  EXPECT_TRUE(is_source);
  EXPECT_TRUE(reader_.ParsePort("dport=80", &port, &is_source));
  EXPECT_EQ(80, port);
  EXPECT_FALSE(is_source);
}

}  // namespace shill
