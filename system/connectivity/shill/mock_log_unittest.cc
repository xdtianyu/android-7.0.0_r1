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

#include "shill/mock_log.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "shill/logging.h"


using ::std::string;
using ::testing::_;

namespace shill {

namespace Logging {
static auto kModuleLogScope = ScopeLogger::kManager;
static string ObjectID(testing::Test* m) { return "(mock_log_test)"; }
}

class MockLogTest : public testing::Test {
 protected:
  MockLogTest() {}

  void LogSomething(const string& message) const {
    LOG(INFO) << message;
  }
  void SlogSomething(testing::Test* t, const string& message) const {
    ScopeLogger::GetInstance()->EnableScopesByName("manager");
    ScopeLogger::GetInstance()->set_verbose_level(2);
    SLOG(t, 2) << message;
    ScopeLogger::GetInstance()->EnableScopesByName("-manager");
    ScopeLogger::GetInstance()->set_verbose_level(0);
  }
};

TEST_F(MockLogTest, MatchMessageOnly) {
  ScopedMockLog log;
  const string kMessage("Something");
  EXPECT_CALL(log, Log(_, _, kMessage));
  LogSomething(kMessage);
}

TEST_F(MockLogTest, MatchSeverityAndMessage) {
  ScopedMockLog log;
  const string kMessage("Something");
  EXPECT_CALL(log, Log(logging::LOG_INFO, _, kMessage));
  LogSomething(kMessage);
}

TEST_F(MockLogTest, MatchSeverityAndFileAndMessage) {
  ScopedMockLog log;
  const string kMessage("Something");
  EXPECT_CALL(log, Log(logging::LOG_INFO,
              ::testing::EndsWith("mock_log_unittest.cc"), kMessage));
  LogSomething(kMessage);
}

TEST_F(MockLogTest, MatchEmptyString) {
  ScopedMockLog log;
  const string kMessage("");
  EXPECT_CALL(log, Log(_, _, kMessage));
  LogSomething(kMessage);
}

TEST_F(MockLogTest, MatchMessageContainsBracketAndNewline) {
  ScopedMockLog log;
  const string kMessage("blah [and more blah] \n yet more blah\n\n\n");
  EXPECT_CALL(log, Log(_, _, kMessage));
  LogSomething(kMessage);
}

TEST_F(MockLogTest, MatchSlog) {
  ScopedMockLog log;
  const string kMessage("Something");
  const string kLogMessage("(anon) Something");
  EXPECT_CALL(log, Log(_, _, kLogMessage));
  SlogSomething(nullptr, kMessage);
}

TEST_F(MockLogTest, MatchSlogWithObject) {
  ScopedMockLog log;
  const string kMessage("Something");
  const string kLogMessage("(mock_log_test) Something");
  EXPECT_CALL(log, Log(_, _, kLogMessage));
  SlogSomething(this, kMessage);
}

TEST_F(MockLogTest, MatchWithGmockMatchers) {
  ScopedMockLog log;
  const string kMessage("Something");
  EXPECT_CALL(log, Log(::testing::Lt(::logging::LOG_ERROR),
                       ::testing::EndsWith(".cc"),
                       ::testing::StartsWith("Some")));
  LogSomething(kMessage);
}

}  // namespace shill
