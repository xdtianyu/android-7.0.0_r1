// Copyright 2014 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "brillo/asynchronous_signal_handler.h"

#include <signal.h>
#include <sys/types.h>
#include <unistd.h>

#include <vector>

#include <base/bind.h>
#include <base/macros.h>
#include <base/message_loop/message_loop.h>
#include <base/run_loop.h>
#include <brillo/message_loops/base_message_loop.h>
#include <gtest/gtest.h>

namespace brillo {

class AsynchronousSignalHandlerTest : public ::testing::Test {
 public:
  AsynchronousSignalHandlerTest() {}
  virtual ~AsynchronousSignalHandlerTest() {}

  virtual void SetUp() {
    brillo_loop_.SetAsCurrent();
    handler_.Init();
  }

  virtual void TearDown() {}

  bool RecordInfoAndQuit(bool response, const struct signalfd_siginfo& info) {
    infos_.push_back(info);
    brillo_loop_.PostTask(FROM_HERE, brillo_loop_.QuitClosure());
    return response;
  }

 protected:
  base::MessageLoopForIO base_loop_;
  BaseMessageLoop brillo_loop_{&base_loop_};
  std::vector<struct signalfd_siginfo> infos_;
  AsynchronousSignalHandler handler_;

 private:
  DISALLOW_COPY_AND_ASSIGN(AsynchronousSignalHandlerTest);
};

TEST_F(AsynchronousSignalHandlerTest, CheckTerm) {
  handler_.RegisterHandler(
      SIGTERM,
      base::Bind(&AsynchronousSignalHandlerTest::RecordInfoAndQuit,
                 base::Unretained(this),
                 true));
  EXPECT_EQ(0, infos_.size());
  EXPECT_EQ(0, kill(getpid(), SIGTERM));

  // Spin the message loop.
  MessageLoop::current()->Run();

  ASSERT_EQ(1, infos_.size());
  EXPECT_EQ(SIGTERM, infos_[0].ssi_signo);
}

TEST_F(AsynchronousSignalHandlerTest, CheckSignalUnregistration) {
  handler_.RegisterHandler(
      SIGCHLD,
      base::Bind(&AsynchronousSignalHandlerTest::RecordInfoAndQuit,
                 base::Unretained(this),
                 true));
  EXPECT_EQ(0, infos_.size());
  EXPECT_EQ(0, kill(getpid(), SIGCHLD));

  // Spin the message loop.
  MessageLoop::current()->Run();

  ASSERT_EQ(1, infos_.size());
  EXPECT_EQ(SIGCHLD, infos_[0].ssi_signo);

  EXPECT_EQ(0, kill(getpid(), SIGCHLD));

  // Run the loop with a timeout, as no message are expected.
  brillo_loop_.PostDelayedTask(FROM_HERE,
                               base::Bind(&MessageLoop::BreakLoop,
                                          base::Unretained(&brillo_loop_)),
                               base::TimeDelta::FromMilliseconds(10));
  MessageLoop::current()->Run();

  // The signal handle should have been unregistered. No new message are
  // expected.
  EXPECT_EQ(1, infos_.size());
}

TEST_F(AsynchronousSignalHandlerTest, CheckMultipleSignal) {
  const uint8_t NB_SIGNALS = 5;
  handler_.RegisterHandler(
      SIGCHLD,
      base::Bind(&AsynchronousSignalHandlerTest::RecordInfoAndQuit,
                 base::Unretained(this),
                 false));
  EXPECT_EQ(0, infos_.size());
  for (int i = 0; i < NB_SIGNALS; ++i) {
    EXPECT_EQ(0, kill(getpid(), SIGCHLD));

    // Spin the message loop.
    MessageLoop::current()->Run();
  }

  ASSERT_EQ(NB_SIGNALS, infos_.size());
  for (int i = 0; i < NB_SIGNALS; ++i) {
    EXPECT_EQ(SIGCHLD, infos_[i].ssi_signo);
  }
}

TEST_F(AsynchronousSignalHandlerTest, CheckChld) {
  handler_.RegisterHandler(
      SIGCHLD,
      base::Bind(&AsynchronousSignalHandlerTest::RecordInfoAndQuit,
                 base::Unretained(this),
                 false));
  pid_t child_pid = fork();
  if (child_pid == 0) {
    _Exit(EXIT_SUCCESS);
  }

  EXPECT_EQ(0, infos_.size());
  // Spin the message loop.
  MessageLoop::current()->Run();

  ASSERT_EQ(1, infos_.size());
  EXPECT_EQ(SIGCHLD, infos_[0].ssi_signo);
  EXPECT_EQ(child_pid, infos_[0].ssi_pid);
  EXPECT_EQ(static_cast<int>(CLD_EXITED), infos_[0].ssi_code);
  EXPECT_EQ(EXIT_SUCCESS, infos_[0].ssi_status);
}

}  // namespace brillo
