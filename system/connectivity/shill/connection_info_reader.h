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

#ifndef SHILL_CONNECTION_INFO_READER_H_
#define SHILL_CONNECTION_INFO_READER_H_

#include <string>
#include <vector>

#include <base/macros.h>
#include <base/files/file_path.h>
#include <gtest/gtest_prod.h>

#include "shill/connection_info.h"

namespace shill {

class ConnectionInfoReader {
 public:
  ConnectionInfoReader();
  virtual ~ConnectionInfoReader();

  // Returns the file path (/proc/net/ip_conntrack by default) from where
  // IP connection tracking information are read. Overloadded by unit tests
  // to return a different file path.
  virtual base::FilePath GetConnectionInfoFilePath() const;

  // Loads IP connection tracking information from the file path returned by
  // GetConnectionInfoFilePath(). Existing entries in |info_list| are always
  // discarded. Returns true on success.
  virtual bool LoadConnectionInfo(std::vector<ConnectionInfo>* info_list);

 private:
  FRIEND_TEST(ConnectionInfoReaderTest, ParseConnectionInfo);
  FRIEND_TEST(ConnectionInfoReaderTest, ParseIPAddress);
  FRIEND_TEST(ConnectionInfoReaderTest, ParseIsUnreplied);
  FRIEND_TEST(ConnectionInfoReaderTest, ParsePort);
  FRIEND_TEST(ConnectionInfoReaderTest, ParseProtocol);
  FRIEND_TEST(ConnectionInfoReaderTest, ParseTimeToExpireSeconds);

  bool ParseConnectionInfo(const std::string& input, ConnectionInfo* info);
  bool ParseProtocol(const std::string& input, int* protocol);
  bool ParseTimeToExpireSeconds(const std::string& input,
                                int64_t* time_to_expire_seconds);
  bool ParseIsUnreplied(const std::string& input, bool* is_unreplied);
  bool ParseIPAddress(const std::string& input,
                      IPAddress* ip_address, bool* is_source);
  bool ParsePort(const std::string& input, uint16_t* port, bool* is_source);

  DISALLOW_COPY_AND_ASSIGN(ConnectionInfoReader);
};

}  // namespace shill

#endif  // SHILL_CONNECTION_INFO_READER_H_
