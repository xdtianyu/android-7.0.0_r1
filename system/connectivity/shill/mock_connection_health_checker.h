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

#ifndef SHILL_MOCK_CONNECTION_HEALTH_CHECKER_H_
#define SHILL_MOCK_CONNECTION_HEALTH_CHECKER_H_

#include <string>

#include <gmock/gmock.h>

#include "shill/connection_health_checker.h"

namespace shill {

class MockConnectionHealthChecker : public ConnectionHealthChecker {
 public:
  MockConnectionHealthChecker(
      ConnectionRefPtr connection,
      EventDispatcher* dispatcher,
      IPAddressStore* remote_ips,
      const base::Callback<void(Result)>& result_callback);
  ~MockConnectionHealthChecker() override;

  MOCK_METHOD1(AddRemoteURL, void(const std::string& url_string));
  MOCK_METHOD1(AddRemoteIP, void(IPAddress ip));
  MOCK_METHOD0(Start, void());
  MOCK_METHOD0(Stop, void());
  MOCK_CONST_METHOD0(health_check_in_progress, bool());

 private:
  DISALLOW_COPY_AND_ASSIGN(MockConnectionHealthChecker);
};

}  // namespace shill

#endif  // SHILL_MOCK_CONNECTION_HEALTH_CHECKER_H_
