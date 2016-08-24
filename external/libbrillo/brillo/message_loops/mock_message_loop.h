// Copyright 2015 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBBRILLO_BRILLO_MESSAGE_LOOPS_MOCK_MESSAGE_LOOP_H_
#define LIBBRILLO_BRILLO_MESSAGE_LOOPS_MOCK_MESSAGE_LOOP_H_

#include <gmock/gmock.h>

#include <base/location.h>
#include <base/test/simple_test_clock.h>
#include <base/time/time.h>

#include <brillo/brillo_export.h>
#include <brillo/message_loops/fake_message_loop.h>
#include <brillo/message_loops/message_loop.h>

namespace brillo {

// The MockMessageLoop is a mockable MessageLoop that will by default act as a
// FakeMessageLoop. It is possible to set expectations with EXPECT_CALL without
// any action associated and they will call the same methods in the underlying
// FakeMessageLoop implementation.
// This message loop implementation is useful to check interaction with the
// message loop when running unittests.
class BRILLO_EXPORT MockMessageLoop : public MessageLoop {
 public:
  // Create a FakeMessageLoop optionally using a SimpleTestClock to update the
  // time when Run() or RunOnce(true) are called and should block.
  explicit MockMessageLoop(base::SimpleTestClock* clock)
    : fake_loop_(clock) {
    // Redirect all actions to calling the underlying FakeMessageLoop by
    // default. For the overloaded methods, we need to disambiguate between the
    // different options by specifying the type of the method pointer.
    ON_CALL(*this, PostDelayedTask(::testing::_, ::testing::_, ::testing::_))
      .WillByDefault(::testing::Invoke(
          &fake_loop_,
          static_cast<TaskId(FakeMessageLoop::*)(
                      const tracked_objects::Location&,
                      const base::Closure&,
                      base::TimeDelta)>(
              &FakeMessageLoop::PostDelayedTask)));
    ON_CALL(*this, WatchFileDescriptor(
        ::testing::_, ::testing::_, ::testing::_, ::testing::_, ::testing::_))
      .WillByDefault(::testing::Invoke(
          &fake_loop_,
          static_cast<TaskId(FakeMessageLoop::*)(
                      const tracked_objects::Location&, int, WatchMode, bool,
                      const base::Closure&)>(
              &FakeMessageLoop::WatchFileDescriptor)));
    ON_CALL(*this, CancelTask(::testing::_))
      .WillByDefault(::testing::Invoke(&fake_loop_,
                                       &FakeMessageLoop::CancelTask));
    ON_CALL(*this, RunOnce(::testing::_))
      .WillByDefault(::testing::Invoke(&fake_loop_,
                                       &FakeMessageLoop::RunOnce));
  }
  ~MockMessageLoop() override = default;

  MOCK_METHOD3(PostDelayedTask,
               TaskId(const tracked_objects::Location& from_here,
                      const base::Closure& task,
                      base::TimeDelta delay));
  using MessageLoop::PostDelayedTask;
  MOCK_METHOD5(WatchFileDescriptor,
               TaskId(const tracked_objects::Location& from_here,
                      int fd,
                      WatchMode mode,
                      bool persistent,
                      const base::Closure& task));
  using MessageLoop::WatchFileDescriptor;
  MOCK_METHOD1(CancelTask, bool(TaskId task_id));
  MOCK_METHOD1(RunOnce, bool(bool may_block));

  // Returns the actual FakeMessageLoop instance so default actions can be
  // override with other actions or call
  FakeMessageLoop* fake_loop() {
    return &fake_loop_;
  }

 private:
  FakeMessageLoop fake_loop_;

  DISALLOW_COPY_AND_ASSIGN(MockMessageLoop);
};

}  // namespace brillo

#endif  // LIBBRILLO_BRILLO_MESSAGE_LOOPS_MOCK_MESSAGE_LOOP_H_
