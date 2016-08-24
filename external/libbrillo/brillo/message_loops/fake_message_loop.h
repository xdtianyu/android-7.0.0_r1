// Copyright 2015 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBBRILLO_BRILLO_MESSAGE_LOOPS_FAKE_MESSAGE_LOOP_H_
#define LIBBRILLO_BRILLO_MESSAGE_LOOPS_FAKE_MESSAGE_LOOP_H_

#include <functional>
#include <map>
#include <queue>
#include <set>
#include <utility>
#include <vector>

#include <base/location.h>
#include <base/test/simple_test_clock.h>
#include <base/time/time.h>

#include <brillo/brillo_export.h>
#include <brillo/message_loops/message_loop.h>

namespace brillo {

// The FakeMessageLoop implements a message loop that doesn't block or wait for
// time based tasks to be ready. The tasks are executed in the order they should
// be executed in a real message loop implementation, but the time is advanced
// to the time when the first task should be executed instead of blocking.
// To keep a consistent notion of time for other classes, FakeMessageLoop
// optionally updates a SimpleTestClock instance when it needs to advance the
// clock.
// This message loop implementation is useful for unittests.
class BRILLO_EXPORT FakeMessageLoop : public MessageLoop {
 public:
  // Create a FakeMessageLoop optionally using a SimpleTestClock to update the
  // time when Run() or RunOnce(true) are called and should block.
  explicit FakeMessageLoop(base::SimpleTestClock* clock);
  ~FakeMessageLoop() override = default;

  TaskId PostDelayedTask(const tracked_objects::Location& from_here,
                         const base::Closure& task,
                         base::TimeDelta delay) override;
  using MessageLoop::PostDelayedTask;
  TaskId WatchFileDescriptor(const tracked_objects::Location& from_here,
                             int fd,
                             WatchMode mode,
                             bool persistent,
                             const base::Closure& task) override;
  using MessageLoop::WatchFileDescriptor;
  bool CancelTask(TaskId task_id) override;
  bool RunOnce(bool may_block) override;

  // FakeMessageLoop methods:

  // Pretend, for the purpose of the FakeMessageLoop watching for file
  // descriptors, that the file descriptor |fd| readiness to perform the
  // operation described by |mode| is |ready|. Initially, no file descriptor
  // is ready for any operation.
  void SetFileDescriptorReadiness(int fd, WatchMode mode, bool ready);

  // Return whether there are peding tasks. Useful to check that no
  // callbacks were leaked.
  bool PendingTasks();

 private:
  struct ScheduledTask {
    tracked_objects::Location location;
    bool persistent;
    base::Closure callback;
  };

  // The sparse list of scheduled pending callbacks.
  std::map<MessageLoop::TaskId, ScheduledTask> tasks_;

  // Using std::greater<> for the priority_queue means that the top() of the
  // queue is the lowest (earliest) time, and for the same time, the smallest
  // TaskId. This determines the order in which the tasks will be fired.
  std::priority_queue<
      std::pair<base::Time, MessageLoop::TaskId>,
      std::vector<std::pair<base::Time, MessageLoop::TaskId>>,
      std::greater<std::pair<base::Time, MessageLoop::TaskId>>> fire_order_;

  // The bag of watched (fd, mode) pair associated with the TaskId that's
  // watching them.
  std::multimap<std::pair<int, WatchMode>, MessageLoop::TaskId> fds_watched_;

  // The set of (fd, mode) pairs that are faked as ready.
  std::set<std::pair<int, WatchMode>> fds_ready_;

  base::SimpleTestClock* test_clock_ = nullptr;
  base::Time current_time_ = base::Time::FromDoubleT(1246996800.);

  MessageLoop::TaskId last_id_ = kTaskIdNull;

  DISALLOW_COPY_AND_ASSIGN(FakeMessageLoop);
};

}  // namespace brillo

#endif  // LIBBRILLO_BRILLO_MESSAGE_LOOPS_FAKE_MESSAGE_LOOP_H_
