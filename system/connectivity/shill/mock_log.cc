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

#include <string>

#include <gtest/gtest.h>

using std::string;
using testing::_;
using testing::AnyNumber;

namespace shill {

ScopedMockLog* ScopedMockLog::instance_ = nullptr;

ScopedMockLog::ScopedMockLog() {
  previous_handler_ = ::logging::GetLogMessageHandler();
  ::logging::SetLogMessageHandler(HandleLogMessages);
  instance_ = this;
}

ScopedMockLog::~ScopedMockLog() {
  ::logging::SetLogMessageHandler(previous_handler_);
  instance_ = nullptr;
}

// static
bool ScopedMockLog::HandleLogMessages(int severity,
                                      const char* file,
                                      int line,
                                      size_t message_start,
                                      const string& full_message) {
  CHECK(instance_);

  // |full_message| looks like this if it came through MemoryLog:
  //   "[0514/165501:INFO:mock_log_unittest.cc(22)] Some message\n"
  // The user wants to match just the substring "Some message".  Strip off the
  // extra stuff.  |message_start| is the position where "Some message" begins.
  //
  // Note that the -1 is to remove the trailing return line.
  const string::size_type message_length =
      full_message.length() - message_start - 1;
  const string message(full_message, message_start, message_length);

  // Call Log.  Because Log is a mock method, this sets in motion the mocking
  // magic.
  instance_->Log(severity, file, message);

  // Invoke the previously installed message handler if there was one.
  if (instance_->previous_handler_) {
    return (*instance_->previous_handler_)(severity, file, line,
                                           message_start, full_message);
  }

  // Return false so that messages show up on stderr.
  return false;
}

NiceScopedMockLog::NiceScopedMockLog() : ScopedMockLog() {
  EXPECT_CALL(*this, Log(_, _, _)).Times(AnyNumber());
}

NiceScopedMockLog::~NiceScopedMockLog() {}

}  // namespace shill
