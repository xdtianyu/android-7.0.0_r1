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

#ifndef SHILL_MOCK_LOG_H_
#define SHILL_MOCK_LOG_H_

// ScopedMockLog provides a way for unittests to validate log messages.  You can
// set expectations that certain log messages will be emited by your functions.
// To use ScopedMockLog, simply create a ScopedMockLog in your test and set
// expectations on its Log() method.  When the ScopedMockLog object goes out of
// scope, the log messages sent to it will be verified against expectations.
//
// Note: Use only one ScopedMockLog in a test because more than one won't work!
//
// Sample usage:
//
// You can verify that a function "DoSomething" emits a specific log text:
//
//   TEST_F(YourTest, DoesSomething) {
//     ScopedMockLog log;
//     EXPECT_CALL(log, Log(_, _, "Some log message text"));
//     DoSomething();  // Causes "Some log message text" to be logged.
//   }
//
// If the function DoSomething() executes something like:
//
//   LOG(INFO) << "Some log message text";
//
// then this will match the expectation.
//
// The first two parameters to ScopedMockLog::Log are the log severity and
// filename.  You can use them like this:
//
//   TEST_F(MockLogTest, MockLogSeverityAndFileAndMessage) {
//     ScopedMockLog log;
//     EXPECT_CALL(log, Log(logging::LOG_INFO, "your_file.cc", "your message"));
//     DoSomething();
//   }
//
// You can also use gMock matchers for matching arguments to Log():
//
//   TEST_F(MockLogTest, MatchWithGmockMatchers) {
//     ScopedMockLog log;
//     EXPECT_CALL(log, Log(::testing::Lt(::logging::LOG_ERROR),
//                          ::testing::EndsWith(".cc"),
//                          ::testing::StartsWith("Some")));
//     DoSomething();
//   }
//
// For some examples, see mock_log_unittest.cc.

#include <string>
#include <gmock/gmock.h>

#include "shill/logging.h"

namespace shill {

class ScopedMockLog {
 public:
  ScopedMockLog();
  virtual ~ScopedMockLog();

  // Users set expecations on this method.  |severity| is defined in
  // base/logging.h, like logging:::LOG_INFO.  |file| is the filename which
  // issues the log message, like "foo.cc".  |user_messages| is the message you
  // expect to see.  Arguments can be ignored by specifying ::testing::_.  You
  // can also specify gMock matchers for arguments.
  MOCK_METHOD3(Log, void(int severity, const char* file,
                         const std::string& user_message));

 private:
  // This function gets invoked by the logging subsystem for each message that
  // is logged.  It calls ScopedMockLog::Log() declared above.  It must be a
  // static method because the logging subsystem does not allow for an object to
  // be passed.  See the typedef LogMessageHandlerFunction in base/logging.h for
  // this function signature.
  static bool HandleLogMessages(int severity,
                                const char* file,
                                int line,
                                size_t message_start,
                                const std::string& full_message);

  // A pointer to the current ScopedMockLog object.
  static ScopedMockLog* instance_;

  // A pointer to any pre-existing message hander function in the logging
  // system.  It is invoked after calling ScopedMockLog::Log().
  ::logging::LogMessageHandlerFunction previous_handler_;
};

// A NiceScopedMockLog is the same as ScopedMockLog, except it creates an
// implicit expectation on any Log() call.  This allows tests to avoid having
// to explictly expect log messages they don't care about.
class NiceScopedMockLog : public ScopedMockLog {
 public:
  NiceScopedMockLog();
  ~NiceScopedMockLog() override;
};

}  // namespace shill

#endif  // SHILL_MOCK_LOG_H_
