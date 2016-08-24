// Copyright 2015 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <brillo/message_loops/message_loop.h>

// These are the common tests for all the brillo::MessageLoop implementations
// that should conform to this interface's contracts. For extra
// implementation-specific tests see the particular implementation unittests in
// the *_unittest.cc files.

#include <memory>
#include <vector>

#include <base/bind.h>
#include <base/location.h>
#include <base/posix/eintr_wrapper.h>
#include <gtest/gtest.h>

#include <brillo/bind_lambda.h>
#include <brillo/unittest_utils.h>
#include <brillo/message_loops/base_message_loop.h>
#include <brillo/message_loops/glib_message_loop.h>
#include <brillo/message_loops/message_loop_utils.h>

using base::Bind;
using base::TimeDelta;

namespace brillo {

using TaskId = MessageLoop::TaskId;

template <typename T>
class MessageLoopTest : public ::testing::Test {
 protected:
  void SetUp() override {
    MessageLoopSetUp();
    EXPECT_TRUE(this->loop_.get());
  }

  std::unique_ptr<base::MessageLoopForIO> base_loop_;

  std::unique_ptr<MessageLoop> loop_;

 private:
  // These MessageLoopSetUp() methods are used to setup each MessageLoop
  // according to its constructor requirements.
  void MessageLoopSetUp();
};

template <>
void MessageLoopTest<GlibMessageLoop>::MessageLoopSetUp() {
  loop_.reset(new GlibMessageLoop());
}

template <>
void MessageLoopTest<BaseMessageLoop>::MessageLoopSetUp() {
  base_loop_.reset(new base::MessageLoopForIO());
  loop_.reset(new BaseMessageLoop(base::MessageLoopForIO::current()));
}

// This setups gtest to run each one of the following TYPED_TEST test cases on
// on each implementation.
typedef ::testing::Types<
  GlibMessageLoop,
  BaseMessageLoop> MessageLoopTypes;
TYPED_TEST_CASE(MessageLoopTest, MessageLoopTypes);


TYPED_TEST(MessageLoopTest, CancelTaskInvalidValuesTest) {
  EXPECT_FALSE(this->loop_->CancelTask(MessageLoop::kTaskIdNull));
  EXPECT_FALSE(this->loop_->CancelTask(1234));
}

TYPED_TEST(MessageLoopTest, PostTaskTest) {
  bool called = false;
  TaskId task_id = this->loop_->PostTask(FROM_HERE,
                                         Bind([&called]() { called = true; }));
  EXPECT_NE(MessageLoop::kTaskIdNull, task_id);
  MessageLoopRunMaxIterations(this->loop_.get(), 100);
  EXPECT_TRUE(called);
}

// Tests that we can cancel tasks right after we schedule them.
TYPED_TEST(MessageLoopTest, PostTaskCancelledTest) {
  bool called = false;
  TaskId task_id = this->loop_->PostTask(FROM_HERE,
                                         Bind([&called]() { called = true; }));
  EXPECT_TRUE(this->loop_->CancelTask(task_id));
  MessageLoopRunMaxIterations(this->loop_.get(), 100);
  EXPECT_FALSE(called);
  // Can't remove a task you already removed.
  EXPECT_FALSE(this->loop_->CancelTask(task_id));
}

TYPED_TEST(MessageLoopTest, PostDelayedTaskRunsEventuallyTest) {
  bool called = false;
  TaskId task_id = this->loop_->PostDelayedTask(
      FROM_HERE,
      Bind([&called]() { called = true; }),
      TimeDelta::FromMilliseconds(50));
  EXPECT_NE(MessageLoop::kTaskIdNull, task_id);
  MessageLoopRunUntil(this->loop_.get(),
                      TimeDelta::FromSeconds(10),
                      Bind([&called]() { return called; }));
  // Check that the main loop finished before the 10 seconds timeout, so it
  // finished due to the callback being called and not due to the timeout.
  EXPECT_TRUE(called);
}

// Test that you can call the overloaded version of PostDelayedTask from
// MessageLoop. This is important because only one of the two methods is
// virtual, so you need to unhide the other when overriding the virtual one.
TYPED_TEST(MessageLoopTest, PostDelayedTaskWithoutLocation) {
  this->loop_->PostDelayedTask(Bind(&base::DoNothing), TimeDelta());
  EXPECT_EQ(1, MessageLoopRunMaxIterations(this->loop_.get(), 100));
}

TYPED_TEST(MessageLoopTest, WatchForInvalidFD) {
  bool called = false;
  EXPECT_EQ(MessageLoop::kTaskIdNull, this->loop_->WatchFileDescriptor(
      FROM_HERE, -1, MessageLoop::kWatchRead, true,
      Bind([&called] { called = true; })));
  EXPECT_EQ(MessageLoop::kTaskIdNull, this->loop_->WatchFileDescriptor(
      FROM_HERE, -1, MessageLoop::kWatchWrite, true,
      Bind([&called] { called = true; })));
  EXPECT_EQ(0, MessageLoopRunMaxIterations(this->loop_.get(), 100));
  EXPECT_FALSE(called);
}

TYPED_TEST(MessageLoopTest, CancelWatchedFileDescriptor) {
  ScopedPipe pipe;
  bool called = false;
  TaskId task_id = this->loop_->WatchFileDescriptor(
      FROM_HERE, pipe.reader, MessageLoop::kWatchRead, true,
      Bind([&called] { called = true; }));
  EXPECT_NE(MessageLoop::kTaskIdNull, task_id);
  // The reader end is blocked because we didn't write anything to the writer
  // end.
  EXPECT_EQ(0, MessageLoopRunMaxIterations(this->loop_.get(), 100));
  EXPECT_FALSE(called);
  EXPECT_TRUE(this->loop_->CancelTask(task_id));
}

// When you watch a file descriptor for reading, the guaranties are that a
// blocking call to read() on that file descriptor will not block. This should
// include the case when the other end of a pipe is closed or the file is empty.
TYPED_TEST(MessageLoopTest, WatchFileDescriptorTriggersWhenPipeClosed) {
  ScopedPipe pipe;
  bool called = false;
  EXPECT_EQ(0, HANDLE_EINTR(close(pipe.writer)));
  pipe.writer = -1;
  TaskId task_id = this->loop_->WatchFileDescriptor(
      FROM_HERE, pipe.reader, MessageLoop::kWatchRead, true,
      Bind([&called] { called = true; }));
  EXPECT_NE(MessageLoop::kTaskIdNull, task_id);
  // The reader end is not blocked because we closed the writer end so a read on
  // the reader end would return 0 bytes read.
  EXPECT_NE(0, MessageLoopRunMaxIterations(this->loop_.get(), 10));
  EXPECT_TRUE(called);
  EXPECT_TRUE(this->loop_->CancelTask(task_id));
}

// When a WatchFileDescriptor task is scheduled with |persistent| = true, we
// should keep getting a call whenever the file descriptor is ready.
TYPED_TEST(MessageLoopTest, WatchFileDescriptorPersistently) {
  ScopedPipe pipe;
  EXPECT_EQ(1, HANDLE_EINTR(write(pipe.writer, "a", 1)));

  int called = 0;
  TaskId task_id = this->loop_->WatchFileDescriptor(
      FROM_HERE, pipe.reader, MessageLoop::kWatchRead, true,
      Bind([&called] { called++; }));
  EXPECT_NE(MessageLoop::kTaskIdNull, task_id);
  // We let the main loop run for 20 iterations to give it enough iterations to
  // verify that our callback was called more than one. We only check that our
  // callback is called more than once.
  EXPECT_EQ(20, MessageLoopRunMaxIterations(this->loop_.get(), 20));
  EXPECT_LT(1, called);
  EXPECT_TRUE(this->loop_->CancelTask(task_id));
}

TYPED_TEST(MessageLoopTest, WatchFileDescriptorNonPersistent) {
  ScopedPipe pipe;
  EXPECT_EQ(1, HANDLE_EINTR(write(pipe.writer, "a", 1)));

  int called = 0;
  TaskId task_id = this->loop_->WatchFileDescriptor(
      FROM_HERE, pipe.reader, MessageLoop::kWatchRead, false,
      Bind([&called] { called++; }));
  EXPECT_NE(MessageLoop::kTaskIdNull, task_id);
  // We let the main loop run for 20 iterations but we just expect it to run
  // at least once. The callback should be called exactly once since we
  // scheduled it non-persistently. After it ran, we shouldn't be able to cancel
  // this task.
  EXPECT_LT(0, MessageLoopRunMaxIterations(this->loop_.get(), 20));
  EXPECT_EQ(1, called);
  EXPECT_FALSE(this->loop_->CancelTask(task_id));
}

TYPED_TEST(MessageLoopTest, WatchFileDescriptorForReadAndWriteSimultaneously) {
  ScopedSocketPair socks;
  EXPECT_EQ(1, HANDLE_EINTR(write(socks.right, "a", 1)));
  // socks.left should be able to read this "a" and should also be able to write
  // without blocking since the kernel has some buffering for it.

  TaskId read_task_id = this->loop_->WatchFileDescriptor(
      FROM_HERE, socks.left, MessageLoop::kWatchRead, true,
      Bind([this, &read_task_id] {
        EXPECT_TRUE(this->loop_->CancelTask(read_task_id))
            << "task_id" << read_task_id;
      }));
  EXPECT_NE(MessageLoop::kTaskIdNull, read_task_id);

  TaskId write_task_id = this->loop_->WatchFileDescriptor(
      FROM_HERE, socks.left, MessageLoop::kWatchWrite, true,
      Bind([this, &write_task_id] {
        EXPECT_TRUE(this->loop_->CancelTask(write_task_id));
      }));
  EXPECT_NE(MessageLoop::kTaskIdNull, write_task_id);

  EXPECT_LT(0, MessageLoopRunMaxIterations(this->loop_.get(), 20));

  EXPECT_FALSE(this->loop_->CancelTask(read_task_id));
  EXPECT_FALSE(this->loop_->CancelTask(write_task_id));
}

// Test that we can cancel the task we are running, and should just fail.
TYPED_TEST(MessageLoopTest, DeleteTaskFromSelf) {
  bool cancel_result = true;  // We would expect this to be false.
  MessageLoop* loop_ptr = this->loop_.get();
  TaskId task_id;
  task_id = this->loop_->PostTask(
      FROM_HERE,
      Bind([&cancel_result, loop_ptr, &task_id]() {
        cancel_result = loop_ptr->CancelTask(task_id);
      }));
  EXPECT_EQ(1, MessageLoopRunMaxIterations(this->loop_.get(), 100));
  EXPECT_FALSE(cancel_result);
}

// Test that we can cancel a non-persistent file descriptor watching callback,
// which should fail.
TYPED_TEST(MessageLoopTest, DeleteNonPersistenIOTaskFromSelf) {
  ScopedPipe pipe;
  TaskId task_id = this->loop_->WatchFileDescriptor(
      FROM_HERE, pipe.writer, MessageLoop::kWatchWrite, false /* persistent */,
      Bind([this, &task_id] {
        EXPECT_FALSE(this->loop_->CancelTask(task_id));
        task_id = MessageLoop::kTaskIdNull;
      }));
  EXPECT_NE(MessageLoop::kTaskIdNull, task_id);
  EXPECT_EQ(1, MessageLoopRunMaxIterations(this->loop_.get(), 100));
  EXPECT_EQ(MessageLoop::kTaskIdNull, task_id);
}

// Test that we can cancel a persistent file descriptor watching callback from
// the same callback.
TYPED_TEST(MessageLoopTest, DeletePersistenIOTaskFromSelf) {
  ScopedPipe pipe;
  TaskId task_id = this->loop_->WatchFileDescriptor(
      FROM_HERE, pipe.writer, MessageLoop::kWatchWrite, true /* persistent */,
      Bind([this, &task_id] {
        EXPECT_TRUE(this->loop_->CancelTask(task_id));
        task_id = MessageLoop::kTaskIdNull;
      }));
  EXPECT_NE(MessageLoop::kTaskIdNull, task_id);
  EXPECT_EQ(1, MessageLoopRunMaxIterations(this->loop_.get(), 100));
  EXPECT_EQ(MessageLoop::kTaskIdNull, task_id);
}

// Test that we can cancel several persistent file descriptor watching callbacks
// from a scheduled callback. In the BaseMessageLoop implementation, this code
// will cause us to cancel an IOTask that has a pending delayed task, but
// otherwise is a valid test case on all implementations.
TYPED_TEST(MessageLoopTest, DeleteAllPersistenIOTaskFromSelf) {
  const int kNumTasks = 5;
  ScopedPipe pipes[kNumTasks];
  TaskId task_ids[kNumTasks];

  for (int i = 0; i < kNumTasks; ++i) {
    task_ids[i] = this->loop_->WatchFileDescriptor(
        FROM_HERE, pipes[i].writer, MessageLoop::kWatchWrite,
        true /* persistent */,
        Bind([this, kNumTasks, &task_ids] {
          for (int j = 0; j < kNumTasks; ++j) {
            // Once we cancel all the tasks, none should run, so this code runs
            // only once from one callback.
            EXPECT_TRUE(this->loop_->CancelTask(task_ids[j]));
            task_ids[j] = MessageLoop::kTaskIdNull;
          }
        }));
  }
  MessageLoopRunMaxIterations(this->loop_.get(), 100);
  for (int i = 0; i < kNumTasks; ++i) {
    EXPECT_EQ(MessageLoop::kTaskIdNull, task_ids[i]);
  }
}

// Test that if there are several tasks watching for file descriptors to be
// available or simply waiting in the message loop are fairly scheduled to run.
// In other words, this test ensures that having a file descriptor always
// available doesn't prevent other file descriptors watching tasks or delayed
// tasks to be dispatched, causing starvation.
TYPED_TEST(MessageLoopTest, AllTasksAreEqual) {
  int total_calls = 0;

  // First, schedule a repeating timeout callback to run from the main loop.
  int timeout_called = 0;
  base::Closure timeout_callback;
  MessageLoop::TaskId timeout_task;
  timeout_callback = base::Bind(
      [this, &timeout_called, &total_calls, &timeout_callback, &timeout_task] {
        timeout_called++;
        total_calls++;
        timeout_task = this->loop_->PostTask(FROM_HERE, Bind(timeout_callback));
        if (total_calls > 100)
          this->loop_->BreakLoop();
      });
  timeout_task = this->loop_->PostTask(FROM_HERE, timeout_callback);

  // Second, schedule several file descriptor watchers.
  const int kNumTasks = 3;
  ScopedPipe pipes[kNumTasks];
  MessageLoop::TaskId tasks[kNumTasks];

  int reads[kNumTasks] = {};
  auto fd_callback = [this, &pipes, &reads, &total_calls](int i) {
    reads[i]++;
    total_calls++;
    char c;
    EXPECT_EQ(1, HANDLE_EINTR(read(pipes[i].reader, &c, 1)));
    if (total_calls > 100)
      this->loop_->BreakLoop();
  };

  for (int i = 0; i < kNumTasks; ++i) {
    tasks[i] = this->loop_->WatchFileDescriptor(
        FROM_HERE, pipes[i].reader, MessageLoop::kWatchRead,
        true /* persistent */,
        Bind(fd_callback, i));
    // Make enough bytes available on each file descriptor. This should not
    // block because we set the size of the file descriptor buffer when
    // creating it.
    std::vector<char> blob(1000, 'a');
    EXPECT_EQ(blob.size(),
              HANDLE_EINTR(write(pipes[i].writer, blob.data(), blob.size())));
  }
  this->loop_->Run();
  EXPECT_GT(total_calls, 100);
  // We run the loop up 100 times and expect each callback to run at least 10
  // times. A good scheduler should balance these callbacks.
  EXPECT_GE(timeout_called, 10);
  EXPECT_TRUE(this->loop_->CancelTask(timeout_task));
  for (int i = 0; i < kNumTasks; ++i) {
    EXPECT_GE(reads[i], 10) << "Reading from pipes[" << i << "], fd "
                            << pipes[i].reader;
    EXPECT_TRUE(this->loop_->CancelTask(tasks[i]));
  }
}

}  // namespace brillo
