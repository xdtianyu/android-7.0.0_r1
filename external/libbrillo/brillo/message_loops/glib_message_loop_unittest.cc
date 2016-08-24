// Copyright 2015 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <brillo/message_loops/glib_message_loop.h>

#include <fcntl.h>
#include <unistd.h>

#include <memory>

#include <base/bind.h>
#include <base/location.h>
#include <base/posix/eintr_wrapper.h>
#include <gtest/gtest.h>

#include <brillo/bind_lambda.h>
#include <brillo/message_loops/message_loop.h>
#include <brillo/message_loops/message_loop_utils.h>

using base::Bind;

namespace brillo {

using TaskId = MessageLoop::TaskId;

class GlibMessageLoopTest : public ::testing::Test {
 protected:
  void SetUp() override {
    loop_.reset(new GlibMessageLoop());
    EXPECT_TRUE(loop_.get());
  }

  std::unique_ptr<GlibMessageLoop> loop_;
};

// When you watch a file descriptor for reading, the guaranties are that a
// blocking call to read() on that file descriptor will not block. This should
// include the case when the other end of a pipe is closed or the file is empty.
TEST_F(GlibMessageLoopTest, WatchFileDescriptorTriggersWhenEmpty) {
  int fd = HANDLE_EINTR(open("/dev/null", O_RDONLY));
  int called = 0;
  TaskId task_id = loop_->WatchFileDescriptor(
      FROM_HERE, fd, MessageLoop::kWatchRead, true,
      Bind([&called] { called++; }));
  EXPECT_NE(MessageLoop::kTaskIdNull, task_id);
  EXPECT_NE(0, MessageLoopRunMaxIterations(loop_.get(), 10));
  EXPECT_LT(2, called);
  EXPECT_TRUE(loop_->CancelTask(task_id));
}

// Test that an invalid file descriptor triggers the callback.
TEST_F(GlibMessageLoopTest, WatchFileDescriptorTriggersWhenInvalid) {
  int fd = HANDLE_EINTR(open("/dev/zero", O_RDONLY));
  int called = 0;
  TaskId task_id = loop_->WatchFileDescriptor(
      FROM_HERE, fd, MessageLoop::kWatchRead, true,
      Bind([&called, fd] {
        if (!called)
          IGNORE_EINTR(close(fd));
        called++;
      }));
  EXPECT_NE(MessageLoop::kTaskIdNull, task_id);
  EXPECT_NE(0, MessageLoopRunMaxIterations(loop_.get(), 10));
  EXPECT_LT(2, called);
  EXPECT_TRUE(loop_->CancelTask(task_id));
}

}  // namespace brillo
