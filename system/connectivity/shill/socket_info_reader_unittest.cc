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

#include "shill/socket_info_reader.h"

#include <base/files/file_util.h>
#include <base/files/scoped_temp_dir.h>
#include <base/strings/stringprintf.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using base::FilePath;
using base::ScopedTempDir;
using std::string;
using std::vector;
using testing::Return;

namespace shill {

namespace {

const char kIPv4AddressAllZeros[] = "0.0.0.0";
const char kIPv4AddressAllOnes[] = "255.255.255.255";
const char kIPv4Address_127_0_0_1[] = "127.0.0.1";
const char kIPv4Address_192_168_1_10[] = "192.168.1.10";
const char kIPv6AddressAllZeros[] = "0000:0000:0000:0000:0000:0000:0000:0000";
const char kIPv6AddressAllOnes[] = "ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff";
const char kIPv6AddressPattern1[] = "0123:4567:89ab:cdef:ffee:ddcc:bbaa:9988";

const char* kIPv4SocketInfoLines[] = {
    "  sl  local_address rem_address   st tx_queue rx_queue tr tm->when "
    "retrnsmt   uid  timeout inode                                      ",
    "   0: 0100007F:0019 00000000:0000 0A 0000000A:00000005 00:00000000 "
    "00000000     0        0 36948 1 0000000000000000 100 0 0 10 -1     ",
    "   1: 0A01A8C0:0050 0100007F:03FC 01 00000000:00000000 00:00000000 "
    "00000000 65534        0 2787034 1 0000000000000000 100 0 0 10 -1   ",
};
const char* kIPv6SocketInfoLines[] = {
    "  sl  local_address                         "
    "remote_address                        st tx_queue rx_queue tr tm->when "
    "retrnsmt   uid  timeout inode",
    "   0: 67452301EFCDAB89CCDDEEFF8899AABB:0019 "
    "00000000000000000000000000000000:0000 0A 0000000A:00000005 00:00000000 "
    "00000000     0        0 36412 1 0000000000000000 100 0 0 2 -1",
    "   1: 00000000000000000000000000000000:0050 "
    "67452301EFCDAB89CCDDEEFF8899AABB:03FC 01 00000000:00000000 00:00000000 "
    "00000000     0        0 36412 1 0000000000000000 100 0 0 2 -1",
};

}  // namespace

class SocketInfoReaderUnderTest : public SocketInfoReader {
 public:
  // Mock out GetTcpv4SocketInfoFilePath and GetTcpv6SocketInfoFilePath to
  // use a temporary created socket info file instead of the actual path
  // in procfs (i.e. /proc/net/tcp and /proc/net/tcp6).
  MOCK_CONST_METHOD0(GetTcpv4SocketInfoFilePath, FilePath());
  MOCK_CONST_METHOD0(GetTcpv6SocketInfoFilePath, FilePath());
};

class SocketInfoReaderTest : public testing::Test {
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

  void CreateSocketInfoFile(const char** lines, size_t num_lines,
                            const FilePath& dir_path, FilePath* file_path) {
    ASSERT_TRUE(base::CreateTemporaryFileInDir(dir_path, file_path));
    for (size_t i = 0; i < num_lines; ++i) {
      string line = lines[i];
      line += '\n';
      ASSERT_TRUE(base::AppendToFile(*file_path, line.data(), line.size()));
    }
  }

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

  SocketInfoReaderUnderTest reader_;
};

TEST_F(SocketInfoReaderTest, LoadTcpSocketInfo) {
  FilePath invalid_path("/non-existent-file"), v4_path, v6_path;
  ScopedTempDir temp_dir;
  ASSERT_TRUE(temp_dir.CreateUniqueTempDir());
  CreateSocketInfoFile(kIPv4SocketInfoLines, 2, temp_dir.path(), &v4_path);
  CreateSocketInfoFile(kIPv6SocketInfoLines, 2, temp_dir.path(), &v6_path);

  SocketInfo v4_info(SocketInfo::kConnectionStateListen,
                     StringToIPv4Address(kIPv4Address_127_0_0_1),
                     25,
                     StringToIPv4Address(kIPv4AddressAllZeros),
                     0,
                     10,
                     5,
                     SocketInfo::kTimerStateNoTimerPending);
  SocketInfo v6_info(SocketInfo::kConnectionStateListen,
                     StringToIPv6Address(kIPv6AddressPattern1),
                     25,
                     StringToIPv6Address(kIPv6AddressAllZeros),
                     0,
                     10,
                     5,
                     SocketInfo::kTimerStateNoTimerPending);

  vector<SocketInfo> info_list;
  EXPECT_CALL(reader_, GetTcpv4SocketInfoFilePath())
      .WillOnce(Return(invalid_path));
  EXPECT_CALL(reader_, GetTcpv6SocketInfoFilePath())
      .WillOnce(Return(invalid_path));
  EXPECT_FALSE(reader_.LoadTcpSocketInfo(&info_list));

  EXPECT_CALL(reader_, GetTcpv4SocketInfoFilePath())
      .WillOnce(Return(v4_path));
  EXPECT_CALL(reader_, GetTcpv6SocketInfoFilePath())
      .WillOnce(Return(invalid_path));
  EXPECT_TRUE(reader_.LoadTcpSocketInfo(&info_list));
  EXPECT_EQ(1, info_list.size());
  ExpectSocketInfoEqual(v4_info, info_list[0]);

  EXPECT_CALL(reader_, GetTcpv4SocketInfoFilePath())
      .WillOnce(Return(invalid_path));
  EXPECT_CALL(reader_, GetTcpv6SocketInfoFilePath())
      .WillOnce(Return(v6_path));
  EXPECT_TRUE(reader_.LoadTcpSocketInfo(&info_list));
  EXPECT_EQ(1, info_list.size());
  ExpectSocketInfoEqual(v6_info, info_list[0]);

  EXPECT_CALL(reader_, GetTcpv4SocketInfoFilePath())
      .WillOnce(Return(v4_path));
  EXPECT_CALL(reader_, GetTcpv6SocketInfoFilePath())
      .WillOnce(Return(v6_path));
  EXPECT_TRUE(reader_.LoadTcpSocketInfo(&info_list));
  EXPECT_EQ(2, info_list.size());
  ExpectSocketInfoEqual(v4_info, info_list[0]);
  ExpectSocketInfoEqual(v6_info, info_list[1]);
}

TEST_F(SocketInfoReaderTest, AppendSocketInfo) {
  FilePath file_path("/non-existent-file");
  vector<SocketInfo> info_list;

  EXPECT_FALSE(reader_.AppendSocketInfo(file_path, &info_list));
  EXPECT_TRUE(info_list.empty());

  ScopedTempDir temp_dir;
  ASSERT_TRUE(temp_dir.CreateUniqueTempDir());

  CreateSocketInfoFile(kIPv4SocketInfoLines, 1, temp_dir.path(), &file_path);
  EXPECT_TRUE(reader_.AppendSocketInfo(file_path, &info_list));
  EXPECT_TRUE(info_list.empty());

  SocketInfo v4_info1(SocketInfo::kConnectionStateListen,
                      StringToIPv4Address(kIPv4Address_127_0_0_1),
                      25,
                      StringToIPv4Address(kIPv4AddressAllZeros),
                      0,
                      10,
                      5,
                      SocketInfo::kTimerStateNoTimerPending);
  SocketInfo v4_info2(SocketInfo::kConnectionStateEstablished,
                      StringToIPv4Address(kIPv4Address_192_168_1_10),
                      80,
                      StringToIPv4Address(kIPv4Address_127_0_0_1),
                      1020,
                      0,
                      0,
                      SocketInfo::kTimerStateNoTimerPending);
  SocketInfo v6_info1(SocketInfo::kConnectionStateListen,
                      StringToIPv6Address(kIPv6AddressPattern1),
                      25,
                      StringToIPv6Address(kIPv6AddressAllZeros),
                      0,
                      10,
                      5,
                      SocketInfo::kTimerStateNoTimerPending);
  SocketInfo v6_info2(SocketInfo::kConnectionStateEstablished,
                      StringToIPv6Address(kIPv6AddressAllZeros),
                      80,
                      StringToIPv6Address(kIPv6AddressPattern1),
                      1020,
                      0,
                      0,
                      SocketInfo::kTimerStateNoTimerPending);

  CreateSocketInfoFile(kIPv4SocketInfoLines, arraysize(kIPv4SocketInfoLines),
                       temp_dir.path(), &file_path);
  EXPECT_TRUE(reader_.AppendSocketInfo(file_path, &info_list));
  EXPECT_EQ(arraysize(kIPv4SocketInfoLines) - 1, info_list.size());
  ExpectSocketInfoEqual(v4_info1, info_list[0]);
  ExpectSocketInfoEqual(v4_info2, info_list[1]);

  CreateSocketInfoFile(kIPv6SocketInfoLines, arraysize(kIPv6SocketInfoLines),
                       temp_dir.path(), &file_path);
  EXPECT_TRUE(reader_.AppendSocketInfo(file_path, &info_list));
  EXPECT_EQ(
      arraysize(kIPv4SocketInfoLines) + arraysize(kIPv6SocketInfoLines) - 2,
      info_list.size());
  ExpectSocketInfoEqual(v4_info1, info_list[0]);
  ExpectSocketInfoEqual(v4_info2, info_list[1]);
  ExpectSocketInfoEqual(v6_info1, info_list[2]);
  ExpectSocketInfoEqual(v6_info2, info_list[3]);
}

TEST_F(SocketInfoReaderTest, ParseSocketInfo) {
  SocketInfo info;

  EXPECT_FALSE(reader_.ParseSocketInfo("", &info));
  EXPECT_FALSE(reader_.ParseSocketInfo(kIPv4SocketInfoLines[0], &info));

  EXPECT_TRUE(reader_.ParseSocketInfo(kIPv4SocketInfoLines[1], &info));
  ExpectSocketInfoEqual(SocketInfo(SocketInfo::kConnectionStateListen,
                                   StringToIPv4Address(kIPv4Address_127_0_0_1),
                                   25,
                                   StringToIPv4Address(kIPv4AddressAllZeros),
                                   0,
                                   10,
                                   5,
                                   SocketInfo::kTimerStateNoTimerPending),
                        info);
}

TEST_F(SocketInfoReaderTest, ParseIPAddressAndPort) {
  IPAddress ip_address(IPAddress::kFamilyUnknown);
  uint16_t port = 0;

  EXPECT_FALSE(reader_.ParseIPAddressAndPort("", &ip_address, &port));
  EXPECT_FALSE(reader_.ParseIPAddressAndPort("00000000", &ip_address, &port));
  EXPECT_FALSE(reader_.ParseIPAddressAndPort("00000000:", &ip_address, &port));
  EXPECT_FALSE(reader_.ParseIPAddressAndPort(":0000", &ip_address, &port));
  EXPECT_FALSE(reader_.ParseIPAddressAndPort(
      "0000000Y:0000", &ip_address, &port));
  EXPECT_FALSE(reader_.ParseIPAddressAndPort(
      "00000000:000Y", &ip_address, &port));

  EXPECT_FALSE(reader_.ParseIPAddressAndPort(
      "00000000000000000000000000000000", &ip_address, &port));
  EXPECT_FALSE(reader_.ParseIPAddressAndPort(
      "00000000000000000000000000000000:", &ip_address, &port));
  EXPECT_FALSE(reader_.ParseIPAddressAndPort(
      "00000000000000000000000000000000Y:0000", &ip_address, &port));
  EXPECT_FALSE(reader_.ParseIPAddressAndPort(
      "000000000000000000000000000000000:000Y", &ip_address, &port));

  EXPECT_TRUE(reader_.ParseIPAddressAndPort(
      "0a01A8c0:0050", &ip_address, &port));
  EXPECT_TRUE(ip_address.Equals(
      StringToIPv4Address(kIPv4Address_192_168_1_10)));
  EXPECT_EQ(80, port);

  EXPECT_TRUE(reader_.ParseIPAddressAndPort(
      "67452301efcdab89CCDDEEFF8899AABB:1F90", &ip_address, &port));
  EXPECT_TRUE(ip_address.Equals(StringToIPv6Address(kIPv6AddressPattern1)));
  EXPECT_EQ(8080, port);
}

TEST_F(SocketInfoReaderTest, ParseIPAddress) {
  IPAddress ip_address(IPAddress::kFamilyUnknown);

  EXPECT_FALSE(reader_.ParseIPAddress("", &ip_address));
  EXPECT_FALSE(reader_.ParseIPAddress("0", &ip_address));
  EXPECT_FALSE(reader_.ParseIPAddress("00", &ip_address));
  EXPECT_FALSE(reader_.ParseIPAddress("0000000Y", &ip_address));
  EXPECT_FALSE(reader_.ParseIPAddress("0000000000000000000000000000000Y",
                                      &ip_address));

  EXPECT_TRUE(reader_.ParseIPAddress("00000000", &ip_address));
  EXPECT_TRUE(ip_address.Equals(StringToIPv4Address(kIPv4AddressAllZeros)));

  EXPECT_TRUE(reader_.ParseIPAddress("0100007F", &ip_address));
  EXPECT_TRUE(ip_address.Equals(StringToIPv4Address(kIPv4Address_127_0_0_1)));

  EXPECT_TRUE(reader_.ParseIPAddress("0a01A8c0", &ip_address));
  EXPECT_TRUE(ip_address.Equals(
      StringToIPv4Address(kIPv4Address_192_168_1_10)));

  EXPECT_TRUE(reader_.ParseIPAddress("ffffffff", &ip_address));
  EXPECT_TRUE(ip_address.Equals(
      StringToIPv4Address(kIPv4AddressAllOnes)));

  EXPECT_TRUE(reader_.ParseIPAddress("00000000000000000000000000000000",
                                     &ip_address));
  EXPECT_TRUE(ip_address.Equals(StringToIPv6Address(kIPv6AddressAllZeros)));

  EXPECT_TRUE(reader_.ParseIPAddress("67452301efcdab89CCDDEEFF8899AABB",
                                     &ip_address));
  EXPECT_TRUE(ip_address.Equals(StringToIPv6Address(kIPv6AddressPattern1)));

  EXPECT_TRUE(reader_.ParseIPAddress("ffffffffffffffffffffffffffffffff",
                                     &ip_address));
  EXPECT_TRUE(ip_address.Equals(StringToIPv6Address(kIPv6AddressAllOnes)));
}

TEST_F(SocketInfoReaderTest, ParsePort) {
  uint16_t port = 0;

  EXPECT_FALSE(reader_.ParsePort("", &port));
  EXPECT_FALSE(reader_.ParsePort("0", &port));
  EXPECT_FALSE(reader_.ParsePort("00", &port));
  EXPECT_FALSE(reader_.ParsePort("000", &port));
  EXPECT_FALSE(reader_.ParsePort("000Y", &port));

  EXPECT_TRUE(reader_.ParsePort("0000", &port));
  EXPECT_EQ(0, port);

  EXPECT_TRUE(reader_.ParsePort("0050", &port));
  EXPECT_EQ(80, port);

  EXPECT_TRUE(reader_.ParsePort("abCD", &port));
  EXPECT_EQ(43981, port);

  EXPECT_TRUE(reader_.ParsePort("ffff", &port));
  EXPECT_EQ(65535, port);
}

TEST_F(SocketInfoReaderTest, ParseTransimitAndReceiveQueueValues) {
  uint64_t transmit_queue_value = 0, receive_queue_value = 0;

  EXPECT_FALSE(reader_.ParseTransimitAndReceiveQueueValues(
      "", &transmit_queue_value, &receive_queue_value));
  EXPECT_FALSE(reader_.ParseTransimitAndReceiveQueueValues(
      "00000000", &transmit_queue_value, &receive_queue_value));
  EXPECT_FALSE(reader_.ParseTransimitAndReceiveQueueValues(
      "00000000:", &transmit_queue_value, &receive_queue_value));
  EXPECT_FALSE(reader_.ParseTransimitAndReceiveQueueValues(
      ":00000000", &transmit_queue_value, &receive_queue_value));
  EXPECT_FALSE(reader_.ParseTransimitAndReceiveQueueValues(
      "0000000Y:00000000", &transmit_queue_value, &receive_queue_value));
  EXPECT_FALSE(reader_.ParseTransimitAndReceiveQueueValues(
      "00000000:0000000Y", &transmit_queue_value, &receive_queue_value));

  EXPECT_TRUE(reader_.ParseTransimitAndReceiveQueueValues(
      "00000001:FFFFFFFF", &transmit_queue_value, &receive_queue_value));
  EXPECT_EQ(1, transmit_queue_value);
  EXPECT_EQ(0xffffffff, receive_queue_value);
}

TEST_F(SocketInfoReaderTest, ParseConnectionState) {
  SocketInfo::ConnectionState connection_state =
      SocketInfo::kConnectionStateUnknown;

  EXPECT_FALSE(reader_.ParseConnectionState("", &connection_state));
  EXPECT_FALSE(reader_.ParseConnectionState("0", &connection_state));
  EXPECT_FALSE(reader_.ParseConnectionState("X", &connection_state));

  EXPECT_TRUE(reader_.ParseConnectionState("00", &connection_state));
  EXPECT_EQ(SocketInfo::kConnectionStateUnknown, connection_state);
  EXPECT_TRUE(reader_.ParseConnectionState("01", &connection_state));
  EXPECT_EQ(SocketInfo::kConnectionStateEstablished, connection_state);
  EXPECT_TRUE(reader_.ParseConnectionState("02", &connection_state));
  EXPECT_EQ(SocketInfo::kConnectionStateSynSent, connection_state);
  EXPECT_TRUE(reader_.ParseConnectionState("03", &connection_state));
  EXPECT_EQ(SocketInfo::kConnectionStateSynRecv, connection_state);
  EXPECT_TRUE(reader_.ParseConnectionState("04", &connection_state));
  EXPECT_EQ(SocketInfo::kConnectionStateFinWait1, connection_state);
  EXPECT_TRUE(reader_.ParseConnectionState("05", &connection_state));
  EXPECT_EQ(SocketInfo::kConnectionStateFinWait2, connection_state);
  EXPECT_TRUE(reader_.ParseConnectionState("06", &connection_state));
  EXPECT_EQ(SocketInfo::kConnectionStateTimeWait, connection_state);
  EXPECT_TRUE(reader_.ParseConnectionState("07", &connection_state));
  EXPECT_EQ(SocketInfo::kConnectionStateClose, connection_state);
  EXPECT_TRUE(reader_.ParseConnectionState("08", &connection_state));
  EXPECT_EQ(SocketInfo::kConnectionStateCloseWait, connection_state);
  EXPECT_TRUE(reader_.ParseConnectionState("09", &connection_state));
  EXPECT_EQ(SocketInfo::kConnectionStateLastAck, connection_state);
  EXPECT_TRUE(reader_.ParseConnectionState("0A", &connection_state));
  EXPECT_EQ(SocketInfo::kConnectionStateListen, connection_state);
  EXPECT_TRUE(reader_.ParseConnectionState("0B", &connection_state));
  EXPECT_EQ(SocketInfo::kConnectionStateClosing, connection_state);

  for (int i = SocketInfo::kConnectionStateMax; i < 256; ++i) {
    EXPECT_TRUE(reader_.ParseConnectionState(
        base::StringPrintf("%02X", i), &connection_state));
    EXPECT_EQ(SocketInfo::kConnectionStateUnknown, connection_state);
  }
}

TEST_F(SocketInfoReaderTest, ParseTimerState) {
  SocketInfo::TimerState timer_state = SocketInfo::kTimerStateUnknown;

  EXPECT_FALSE(reader_.ParseTimerState("", &timer_state));
  EXPECT_FALSE(reader_.ParseTimerState("0", &timer_state));
  EXPECT_FALSE(reader_.ParseTimerState("X", &timer_state));
  EXPECT_FALSE(reader_.ParseTimerState("00", &timer_state));

  EXPECT_TRUE(reader_.ParseTimerState("00:00000000", &timer_state));
  EXPECT_EQ(SocketInfo::kTimerStateNoTimerPending, timer_state);
  EXPECT_TRUE(reader_.ParseTimerState("01:00000000", &timer_state));
  EXPECT_EQ(SocketInfo::kTimerStateRetransmitTimerPending, timer_state);
  EXPECT_TRUE(reader_.ParseTimerState("02:00000000", &timer_state));
  EXPECT_EQ(SocketInfo::kTimerStateAnotherTimerPending, timer_state);
  EXPECT_TRUE(reader_.ParseTimerState("03:00000000", &timer_state));
  EXPECT_EQ(SocketInfo::kTimerStateInTimeWaitState, timer_state);
  EXPECT_TRUE(reader_.ParseTimerState("04:00000000", &timer_state));
  EXPECT_EQ(SocketInfo::kTimerStateZeroWindowProbeTimerPending, timer_state);

  for (int i = SocketInfo::kTimerStateMax; i < 256; ++i) {
    EXPECT_TRUE(reader_.ParseTimerState(
        base::StringPrintf("%02X:00000000", i), &timer_state));
    EXPECT_EQ(SocketInfo::kTimerStateUnknown, timer_state);
  }
}

}  // namespace shill
