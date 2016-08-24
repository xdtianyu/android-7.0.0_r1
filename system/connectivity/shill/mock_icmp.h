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

#ifndef SHILL_MOCK_ICMP_H_
#define SHILL_MOCK_ICMP_H_

#include "shill/icmp.h"

#include <gmock/gmock.h>

#include "shill/net/ip_address.h"

namespace shill {

class MockIcmp : public Icmp {
 public:
  MockIcmp();
  ~MockIcmp() override;

  MOCK_METHOD0(Start, bool());
  MOCK_METHOD0(Stop, void());
  MOCK_CONST_METHOD0(IsStarted, bool());
  MOCK_METHOD3(TransmitEchoRequest, bool(const IPAddress& destination,
                                         uint16_t id, uint16_t seq_num));

 private:
  DISALLOW_COPY_AND_ASSIGN(MockIcmp);
};

}  // namespace shill

#endif  // SHILL_MOCK_ICMP_H_
