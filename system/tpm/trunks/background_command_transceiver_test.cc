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

#include "trunks/background_command_transceiver.h"

#include <base/bind.h>
#include <base/logging.h>
#include <base/message_loop/message_loop.h>
#include <base/run_loop.h>
#include <base/threading/platform_thread.h>
#include <base/threading/thread.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "trunks/command_transceiver.h"
#include "trunks/mock_command_transceiver.h"

using testing::_;
using testing::Invoke;
using testing::InvokeWithoutArgs;
using testing::WithArgs;

namespace {

const char kTestThreadName[] = "test_thread";

std::string GetThreadName() {
  return std::string(base::PlatformThread::GetName());
}

void GetThreadNameAndCall(
    const trunks::CommandTransceiver::ResponseCallback& callback) {
  callback.Run(GetThreadName());
}

void Assign(std::string* to, const std::string& from) {
  *to = from;
}

void SendCommandAndWaitAndAssign(trunks::CommandTransceiver* transceiver,
                                 std::string* output) {
  *output = transceiver->SendCommandAndWait("test");
}

}  // namespace

namespace trunks {

class BackgroundTransceiverTest : public testing::Test {
 public:
  BackgroundTransceiverTest() : test_thread_(kTestThreadName) {
    EXPECT_CALL(next_transceiver_, SendCommand(_, _))
        .WillRepeatedly(WithArgs<1>(Invoke(GetThreadNameAndCall)));
    EXPECT_CALL(next_transceiver_, SendCommandAndWait(_))
        .WillRepeatedly(InvokeWithoutArgs(GetThreadName));
    CHECK(test_thread_.Start());
  }

  ~BackgroundTransceiverTest() override {}

 protected:
  base::MessageLoopForIO message_loop_;
  base::Thread test_thread_;
  MockCommandTransceiver next_transceiver_;
};

TEST_F(BackgroundTransceiverTest, Asynchronous) {
  trunks::BackgroundCommandTransceiver background_transceiver(
      &next_transceiver_,
      test_thread_.task_runner());
  std::string output = "not_assigned";
  background_transceiver.SendCommand("test", base::Bind(Assign, &output));
  do {
    base::RunLoop run_loop;
    run_loop.RunUntilIdle();
  } while (output == "not_assigned");
  // The call to our mock should have happened on the background thread.
  EXPECT_EQ(std::string(kTestThreadName), output);
}

TEST_F(BackgroundTransceiverTest, Synchronous) {
  trunks::BackgroundCommandTransceiver background_transceiver(
      &next_transceiver_,
      test_thread_.task_runner());
  std::string output = "not_assigned";
  // Post a synchronous call to be run when we start pumping the loop.
  message_loop_.PostTask(FROM_HERE,
                         base::Bind(SendCommandAndWaitAndAssign,
                                    &background_transceiver,
                                    &output));
  base::RunLoop run_loop;
  run_loop.RunUntilIdle();
  // The call to our mock should have happened on the background thread.
  EXPECT_EQ(std::string("test_thread"), output);
}

}  // namespace trunks
