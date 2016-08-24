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

#include <limits>

#include <base/strings/string_number_conversions.h>
#include <base/strings/string_split.h>
#include <base/strings/string_util.h>

#include "shill/file_reader.h"
#include "shill/logging.h"

using base::FilePath;
using std::string;
using std::vector;

namespace shill {

namespace Logging {
static auto kModuleLogScope = ScopeLogger::kLink;
static string ObjectID(ConnectionInfoReader* c) {
  return "(connection_info_reader)";
}
}

namespace {

const char kConnectionInfoFilePath[] = "/proc/net/ip_conntrack";
const char kSourceIPAddressTag[] = "src=";
const char kSourcePortTag[] = "sport=";
const char kDestinationIPAddressTag[] = "dst=";
const char kDestinationPortTag[] = "dport=";
const char kUnrepliedTag[] = "[UNREPLIED]";

}  // namespace

ConnectionInfoReader::ConnectionInfoReader() {}

ConnectionInfoReader::~ConnectionInfoReader() {}

FilePath ConnectionInfoReader::GetConnectionInfoFilePath() const {
  return FilePath(kConnectionInfoFilePath);
}

bool ConnectionInfoReader::LoadConnectionInfo(
    vector<ConnectionInfo>* info_list) {
  info_list->clear();

  FilePath info_file_path = GetConnectionInfoFilePath();
  FileReader file_reader;
  if (!file_reader.Open(info_file_path)) {
    SLOG(this, 2) << __func__ << ": Failed to open '"
                  << info_file_path.value() << "'.";
    return false;
  }

  string line;
  while (file_reader.ReadLine(&line)) {
    ConnectionInfo info;
    if (ParseConnectionInfo(line, &info))
      info_list->push_back(info);
  }
  return true;
}

bool ConnectionInfoReader::ParseConnectionInfo(const string& input,
                                               ConnectionInfo* info) {
  vector<string> tokens = base::SplitString(input, base::kWhitespaceASCII,
                                            base::KEEP_WHITESPACE,
                                            base::SPLIT_WANT_NONEMPTY);
  if (tokens.size() < 10) {
    return false;
  }

  int index = 0;

  int protocol = 0;
  if (!ParseProtocol(tokens[++index], &protocol)) {
    return false;
  }
  info->set_protocol(protocol);

  int64_t time_to_expire_seconds = 0;
  if (!ParseTimeToExpireSeconds(tokens[++index], &time_to_expire_seconds)) {
    return false;
  }
  info->set_time_to_expire_seconds(time_to_expire_seconds);

  if (protocol == IPPROTO_TCP)
    ++index;

  IPAddress ip_address(IPAddress::kFamilyUnknown);
  uint16_t port = 0;
  bool is_source = false;

  if (!ParseIPAddress(tokens[++index], &ip_address, &is_source) || !is_source) {
    return false;
  }
  info->set_original_source_ip_address(ip_address);
  if (!ParseIPAddress(tokens[++index], &ip_address, &is_source) || is_source) {
    return false;
  }
  info->set_original_destination_ip_address(ip_address);

  if (!ParsePort(tokens[++index], &port, &is_source) || !is_source) {
    return false;
  }
  info->set_original_source_port(port);
  if (!ParsePort(tokens[++index], &port, &is_source) || is_source) {
    return false;
  }
  info->set_original_destination_port(port);

  if (tokens[index + 1] == kUnrepliedTag) {
    info->set_is_unreplied(true);
    ++index;
  } else {
    info->set_is_unreplied(false);
  }

  if (!ParseIPAddress(tokens[++index], &ip_address, &is_source) || !is_source) {
    return false;
  }
  info->set_reply_source_ip_address(ip_address);
  if (!ParseIPAddress(tokens[++index], &ip_address, &is_source) || is_source) {
    return false;
  }
  info->set_reply_destination_ip_address(ip_address);

  if (!ParsePort(tokens[++index], &port, &is_source) || !is_source) {
    return false;
  }
  info->set_reply_source_port(port);
  if (!ParsePort(tokens[++index], &port, &is_source) || is_source) {
    return false;
  }
  info->set_reply_destination_port(port);

  return true;
}

bool ConnectionInfoReader::ParseProtocol(const string& input, int* protocol) {
  if (!base::StringToInt(input, protocol) ||
      *protocol < 0 || *protocol >= IPPROTO_MAX) {
    return false;
  }
  return true;
}

bool ConnectionInfoReader::ParseTimeToExpireSeconds(
    const string& input, int64_t* time_to_expire_seconds) {
  if (!base::StringToInt64(input, time_to_expire_seconds) ||
      *time_to_expire_seconds < 0) {
    return false;
  }
  return true;
}

bool ConnectionInfoReader::ParseIPAddress(
    const string& input, IPAddress* ip_address, bool* is_source) {
  string ip_address_string;

  if (base::StartsWith(input, kSourceIPAddressTag,
                       base::CompareCase::INSENSITIVE_ASCII)) {
    *is_source = true;
    ip_address_string = input.substr(strlen(kSourceIPAddressTag));
  } else if (base::StartsWith(input, kDestinationIPAddressTag,
                              base::CompareCase::INSENSITIVE_ASCII)) {
    *is_source = false;
    ip_address_string = input.substr(strlen(kDestinationIPAddressTag));
  } else {
    return false;
  }

  IPAddress ipv4_address(IPAddress::kFamilyIPv4);
  if (ipv4_address.SetAddressFromString(ip_address_string)) {
    *ip_address = ipv4_address;
    return true;
  }

  IPAddress ipv6_address(IPAddress::kFamilyIPv6);
  if (ipv6_address.SetAddressFromString(ip_address_string)) {
    *ip_address = ipv6_address;
    return true;
  }

  return false;
}

bool ConnectionInfoReader::ParsePort(
    const string& input, uint16_t* port, bool* is_source) {
  int result = 0;
  string port_string;

  if (base::StartsWith(input, kSourcePortTag,
                       base::CompareCase::INSENSITIVE_ASCII)) {
    *is_source = true;
    port_string = input.substr(strlen(kSourcePortTag));
  } else if (base::StartsWith(input, kDestinationPortTag,
                              base::CompareCase::INSENSITIVE_ASCII)) {
    *is_source = false;
    port_string = input.substr(strlen(kDestinationPortTag));
  } else {
    return false;
  }

  if (!base::StringToInt(port_string, &result) ||
      result < 0 || result > std::numeric_limits<uint16_t>::max()) {
    return false;
  }

  *port = result;
  return true;
}

}  // namespace shill
