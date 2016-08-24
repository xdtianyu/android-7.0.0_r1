// Copyright 2015 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <brillo/message_loops/fake_message_loop.h>

#include <memory>
#include <vector>

#include <base/bind.h>
#include <base/location.h>
#include <base/test/simple_test_clock.h>
#include <gtest/gtest.h>

#include <brillo/bind_lambda.h>
#include <brillo/message_loops/message_loop.h>

using base::Bind;
using base::Time;
using base::TimeDelta;
using std::vector;

namespace brillo {

using TaskId = MessageLoop::TaskId;

class FakeMessageLoopTest : public ::testing::Test {
 protected:
  void SetUp() override {
    loop_.reset(new FakeMessageLoop(nullptr));
    EXPECT_TRUE(loop_.get());
  }
  void TearDown() override {
    EXPECT_FALSE(loop_->PendingTasks());
  }

  base::SimpleTestClock clock_;
  std::unique_ptr<FakeMessageLoop> loop_;
};

TEST_F(FakeMessageLoopTest, CancelTaskInvalidValuesTest) {
  EXPECT_FALSE(loop_->CancelTask(MessageLoop::kTaskIdNull));
  EXPECT_FALSE(loop_->CancelTask(1234));
}

TEST_F(FakeMessageLoopTest, PostDelayedTaskRunsInOrder) {
  vector<int> order;
  loop_->PostDelayedTask(Bind([&order]() { order.push_back(1); }),
                         TimeDelta::FromSeconds(1));
  loop_->PostDelayedTask(Bind([&order]() { order.push_back(4); }),
                         TimeDelta::FromSeconds(4));
  loop_->PostDelayedTask(Bind([&order]() { order.push_back(3); }),
                         TimeDelta::FromSeconds(3));
  loop_->PostDelayedTask(Bind([&order]() { order.push_back(2); }),
                         TimeDelta::FromSeconds(2));
  // Run until all the tasks are run.
  loop_->Run();
  EXPECT_EQ((vector<int>{1, 2, 3, 4}), order);
}

TEST_F(FakeMessageLoopTest, PostDelayedTaskAdvancesTheTime) {
  Time start = Time::FromInternalValue(1000000);
  clock_.SetNow(start);
  loop_.reset(new FakeMessageLoop(&clock_));
  loop_->PostDelayedTask(Bind(&base::DoNothing), TimeDelta::FromSeconds(1));
  loop_->PostDelayedTask(Bind(&base::DoNothing), TimeDelta::FromSeconds(2));
  EXPECT_FALSE(loop_->RunOnce(false));
  // If the callback didn't run, the time shouldn't change.
  EXPECT_EQ(start, clock_.Now());

  // If we run only one callback, the time should be set to the time that
  // callack ran.
  EXPECT_TRUE(loop_->RunOnce(true));
  EXPECT_EQ(start + TimeDelta::FromSeconds(1), clock_.Now());

  // If the clock is advanced manually, we should be able to run the
  // callback without blocking, since the firing time is in the past.
  clock_.SetNow(start + TimeDelta::FromSeconds(3));
  EXPECT_TRUE(loop_->RunOnce(false));
  // The time should not change even if the callback is due in the past.
  EXPECT_EQ(start + TimeDelta::FromSeconds(3), clock_.Now());
}

TEST_F(FakeMessageLoopTest, WatchFileDescriptorWaits) {
  int fd = 1234;
  // We will simulate this situation. At the beginning, we will watch for a
  // file descriptor that won't trigger for 10s. Then we will pretend it is
  // ready after 10s and expect its callback to run just once.
  int called = 0;
  TaskId task_id = loop_->WatchFileDescriptor(
      FROM_HERE, fd, MessageLoop::kWatchRead, false,
      Bind([&called] { called++; }));
  EXPECT_NE(MessageLoop::kTaskIdNull, task_id);

  EXPECT_NE(MessageLoop::kTaskIdNull,
            loop_->PostDelayedTask(Bind([this] { this->loop_->BreakLoop(); }),
                                   TimeDelta::FromSeconds(10)));
  EXPECT_NE(MessageLoop::kTaskIdNull,
            loop_->PostDelayedTask(Bind([this] { this->loop_->BreakLoop(); }),
                                   TimeDelta::FromSeconds(20)));
  loop_->Run();
  EXPECT_EQ(0, called);

  loop_->SetFileDescriptorReadiness(fd, MessageLoop::kWatchRead, true);
  loop_->Run();
  EXPECT_EQ(1, called);
  EXPECT_FALSE(loop_->CancelTask(task_id));
}

TEST_F(FakeMessageLoopTest, PendingTasksTest) {
  loop_->PostDelayedTask(Bind(&base::DoNothing), TimeDelta::FromSeconds(1));
  EXPECT_TRUE(loop_->PendingTasks());
  loop_->Run();
}

}  // namespace brillo
