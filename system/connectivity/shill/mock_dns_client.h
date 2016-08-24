//
// Copyright (C) 2011 The Android Open Source Project
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

#ifndef SHILL_MOCK_DNS_CLIENT_H_
#define SHILL_MOCK_DNS_CLIENT_H_

#include <string>

#include "shill/dns_client.h"

#include <base/macros.h>
#include <gmock/gmock.h>

namespace shill {

class MockDNSClient : public DNSClient {
 public:
  MockDNSClient();
  ~MockDNSClient() override;

  MOCK_METHOD2(Start, bool(const std::string& hostname, Error* error));
  MOCK_METHOD0(Stop, void());
  MOCK_CONST_METHOD0(IsActive, bool());

 private:
  DISALLOW_COPY_AND_ASSIGN(MockDNSClient);
};

}  // namespace shill

#endif  // SHILL_MOCK_DNS_CLIENT_H_
