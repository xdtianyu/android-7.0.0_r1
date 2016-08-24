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

#ifndef SHILL_SOCKET_INFO_READER_H_
#define SHILL_SOCKET_INFO_READER_H_

#include <string>
#include <vector>

#include <base/files/file_path.h>
#include <base/macros.h>
#include <gtest/gtest_prod.h>

#include "shill/socket_info.h"

namespace shill {

class SocketInfoReader {
 public:
  SocketInfoReader();
  virtual ~SocketInfoReader();

  // Returns the file path (/proc/net/tcp by default) from where TCP/IPv4
  // socket information are read. Overloadded by unit tests to return a
  // different file path.
  virtual base::FilePath GetTcpv4SocketInfoFilePath() const;

  // Returns the file path (/proc/net/tcp6 by default) from where TCP/IPv6
  // socket information are read. Overloadded by unit tests to return a
  // different file path.
  virtual base::FilePath GetTcpv6SocketInfoFilePath() const;

  // Loads TCP socket information from /proc/net/tcp and /proc/net/tcp6.
  // Existing entries in |info_list| are always discarded. Returns false
  // if when neither /proc/net/tcp nor /proc/net/tcp6 can be read.
  virtual bool LoadTcpSocketInfo(std::vector<SocketInfo>* info_list);

 private:
  FRIEND_TEST(SocketInfoReaderTest, AppendSocketInfo);
  FRIEND_TEST(SocketInfoReaderTest, ParseConnectionState);
  FRIEND_TEST(SocketInfoReaderTest, ParseIPAddress);
  FRIEND_TEST(SocketInfoReaderTest, ParseIPAddressAndPort);
  FRIEND_TEST(SocketInfoReaderTest, ParsePort);
  FRIEND_TEST(SocketInfoReaderTest, ParseSocketInfo);
  FRIEND_TEST(SocketInfoReaderTest, ParseTimerState);
  FRIEND_TEST(SocketInfoReaderTest, ParseTransimitAndReceiveQueueValues);

  bool AppendSocketInfo(const base::FilePath& info_file_path,
                        std::vector<SocketInfo>* info_list);
  bool ParseSocketInfo(const std::string& input, SocketInfo* socket_info);
  bool ParseIPAddressAndPort(
      const std::string& input, IPAddress* ip_address, uint16_t* port);
  bool ParseIPAddress(const std::string& input, IPAddress* ip_address);
  bool ParsePort(const std::string& input, uint16_t* port);
  bool ParseTransimitAndReceiveQueueValues(
      const std::string& input,
      uint64_t* transmit_queue_value, uint64_t* receive_queue_value);
  bool ParseConnectionState(const std::string& input,
                            SocketInfo::ConnectionState* connection_state);
  bool ParseTimerState(const std::string& input,
                       SocketInfo::TimerState* timer_state);

  DISALLOW_COPY_AND_ASSIGN(SocketInfoReader);
};

}  // namespace shill

#endif  // SHILL_SOCKET_INFO_READER_H_
