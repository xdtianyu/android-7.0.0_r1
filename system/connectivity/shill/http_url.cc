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

#include "shill/http_url.h"

#include <string>
#include <vector>

#include <base/strings/string_number_conversions.h>
#include <base/strings/string_split.h>

using std::string;
using std::vector;

namespace shill {

const int HTTPURL::kDefaultHTTPPort = 80;
const int HTTPURL::kDefaultHTTPSPort = 443;

const char HTTPURL::kDelimiters[] = " /#?";
const char HTTPURL::kPortSeparator = ':';
const char HTTPURL::kPrefixHTTP[] = "http://";
const char HTTPURL::kPrefixHTTPS[] = "https://";

HTTPURL::HTTPURL()
    : port_(kDefaultHTTPPort),
      protocol_(kProtocolHTTP) {}

HTTPURL::~HTTPURL() {}

bool HTTPURL::ParseFromString(const string& url_string) {
  Protocol protocol = kProtocolUnknown;
  size_t host_start = 0;
  int port = 0;
  const string http_url_prefix(kPrefixHTTP);
  const string https_url_prefix(kPrefixHTTPS);
  if (url_string.substr(0, http_url_prefix.length()) == http_url_prefix) {
    host_start = http_url_prefix.length();
    port = kDefaultHTTPPort;
    protocol = kProtocolHTTP;
  } else if (
      url_string.substr(0, https_url_prefix.length()) == https_url_prefix) {
    host_start = https_url_prefix.length();
    port = kDefaultHTTPSPort;
    protocol = kProtocolHTTPS;
  } else {
    return false;
  }

  size_t host_end = url_string.find_first_of(kDelimiters, host_start);
  if (host_end == string::npos) {
    host_end = url_string.length();
  }
  vector<string> host_parts = base::SplitString(
      url_string.substr(host_start, host_end - host_start),
      std::string{kPortSeparator}, base::TRIM_WHITESPACE, base::SPLIT_WANT_ALL);

  if (host_parts.empty() || host_parts[0].empty() || host_parts.size() > 2) {
    return false;
  } else if (host_parts.size() == 2) {
    if (!base::StringToInt(host_parts[1], &port)) {
      return false;
    }
  }

  protocol_ = protocol;
  host_ = host_parts[0];
  port_ = port;
  path_ = url_string.substr(host_end);
  if (path_.empty() || path_[0] != '/') {
    path_ = "/" + path_;
  }

  return true;
}

}  // namespace shill
