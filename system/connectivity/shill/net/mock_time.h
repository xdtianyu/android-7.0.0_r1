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

#ifndef SHILL_NET_MOCK_TIME_H_
#define SHILL_NET_MOCK_TIME_H_

#include "shill/net/shill_time.h"

#include <base/macros.h>
#include <gmock/gmock.h>

namespace shill {

class MockTime : public Time {
 public:
  MockTime() {}
  ~MockTime() override {}

  MOCK_METHOD1(GetSecondsMonotonic, bool(time_t* seconds));
  MOCK_METHOD1(GetSecondsBoottime, bool(time_t* seconds));
  MOCK_METHOD1(GetTimeMonotonic, int(struct timeval* tv));
  MOCK_METHOD1(GetTimeBoottime, int(struct timeval* tv));
  MOCK_METHOD2(GetTimeOfDay, int(struct timeval* tv, struct timezone* tz));
  MOCK_METHOD0(GetNow, Timestamp());
  MOCK_CONST_METHOD0(GetSecondsSinceEpoch, time_t());

 private:
  DISALLOW_COPY_AND_ASSIGN(MockTime);
};

}  // namespace shill

#endif  // SHILL_NET_MOCK_TIME_H_
