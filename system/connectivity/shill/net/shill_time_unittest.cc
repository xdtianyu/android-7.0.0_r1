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

#include "shill/net/shill_time.h"

#include <time.h>

#include <gtest/gtest.h>

using std::string;
using testing::Test;

namespace shill {

class TimeTest : public Test {
};

TEST_F(TimeTest, FormatTime) {
  const time_t kEpochStart = 0;
  const char kEpochStartString[] = "1970-01-01T00:00:00.000000+0000";
  struct tm epoch_start_tm;
  gmtime_r(&kEpochStart, &epoch_start_tm);
  EXPECT_EQ(kEpochStartString, Time::FormatTime(epoch_start_tm, 0));
}

}  // namespace shill
