//
// Copyright (C) 2014 The Android Open Source Project
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

#ifndef SHILL_MOCK_DNS_SERVER_TESTER_H_
#define SHILL_MOCK_DNS_SERVER_TESTER_H_

#include <base/macros.h>
#include <gmock/gmock.h>

#include "shill/dns_server_tester.h"

namespace shill {

class MockDNSServerTester : public DNSServerTester {
 public:
  explicit MockDNSServerTester(ConnectionRefPtr connection);
  ~MockDNSServerTester() override;

  MOCK_METHOD0(Start, void());
  MOCK_METHOD0(Stop, void());

 private:
  DISALLOW_COPY_AND_ASSIGN(MockDNSServerTester);
};

}  // namespace shill

#endif  // SHILL_MOCK_DNS_SERVER_TESTER_H_
