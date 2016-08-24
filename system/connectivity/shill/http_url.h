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

#ifndef SHILL_HTTP_URL_H_
#define SHILL_HTTP_URL_H_

#include <base/macros.h>

#include <string>

namespace shill {

// Simple URL parsing class.
class HTTPURL {
 public:
  enum Protocol {
    kProtocolUnknown,
    kProtocolHTTP,
    kProtocolHTTPS
  };

  static const int kDefaultHTTPPort;
  static const int kDefaultHTTPSPort;

  HTTPURL();
  virtual ~HTTPURL();

  // Parse a URL from |url_string|.
  bool ParseFromString(const std::string& url_string);

  const std::string& host() const { return host_; }
  const std::string& path() const { return path_; }
  int port() const { return port_; }
  Protocol protocol() const { return protocol_; }

 private:
  static const char kDelimiters[];
  static const char kPortSeparator;
  static const char kPrefixHTTP[];
  static const char kPrefixHTTPS[];

  std::string host_;
  std::string path_;
  int port_;
  Protocol protocol_;

  DISALLOW_COPY_AND_ASSIGN(HTTPURL);
};

}  // namespace shill

#endif  // SHILL_HTTP_URL_H_
