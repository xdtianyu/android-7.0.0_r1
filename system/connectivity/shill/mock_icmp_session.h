//
// Copyright (C) 2015 The Android Open Source Project
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

#ifndef SHILL_MOCK_ICMP_SESSION_H_
#define SHILL_MOCK_ICMP_SESSION_H_

#include "shill/icmp_session.h"

#include <gmock/gmock.h>

#include "shill/net/ip_address.h"

namespace shill {

class MockIcmpSession : public IcmpSession {
 public:
  explicit MockIcmpSession(EventDispatcher* dispatcher);
  ~MockIcmpSession() override;

  MOCK_METHOD2(
      Start,
      bool(const IPAddress& destination,
           const IcmpSession::IcmpSessionResultCallback& result_callback));
  MOCK_METHOD0(Stop, void());

 private:
  DISALLOW_COPY_AND_ASSIGN(MockIcmpSession);
};

}  // namespace shill

#endif  // SHILL_MOCK_ICMP_SESSION_H_
