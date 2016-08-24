// Copyright 2015 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <brillo/message_loops/fake_message_loop.h>

#include <base/logging.h>
#include <brillo/location_logging.h>

namespace brillo {

FakeMessageLoop::FakeMessageLoop(base::SimpleTestClock* clock)
  : test_clock_(clock) {
}

MessageLoop::TaskId FakeMessageLoop::PostDelayedTask(
    const tracked_objects::Location& from_here,
    const base::Closure& task,
    base::TimeDelta delay) {
  // If no SimpleTestClock was provided, we use the last time we fired a
  // callback. In this way, tasks scheduled from a Closure will have the right
  // time.
  if (test_clock_)
    current_time_ = test_clock_->Now();
  MessageLoop::TaskId current_id = ++last_id_;
  // FakeMessageLoop is limited to only 2^64 tasks. That should be enough.
  CHECK(current_id);
  tasks_.emplace(current_id, ScheduledTask{from_here, false, task});
  fire_order_.push(std::make_pair(current_time_ + delay, current_id));
  VLOG_LOC(from_here, 1) << "Scheduling delayed task_id " << current_id
                         << " to run at " << current_time_ + delay
                         << " (in " << delay << ").";
  return current_id;
}

MessageLoop::TaskId FakeMessageLoop::WatchFileDescriptor(
    const tracked_objects::Location& from_here,
    int fd,
    WatchMode mode,
    bool persistent,
    const base::Closure& task) {
  MessageLoop::TaskId current_id = ++last_id_;
  // FakeMessageLoop is limited to only 2^64 tasks. That should be enough.
  CHECK(current_id);
  tasks_.emplace(current_id, ScheduledTask{from_here, persistent, task});
  fds_watched_.emplace(std::make_pair(fd, mode), current_id);
  return current_id;
}

bool FakeMessageLoop::CancelTask(TaskId task_id) {
  if (task_id == MessageLoop::kTaskIdNull)
    return false;
  bool ret = tasks_.erase(task_id) > 0;
  VLOG_IF(1, ret) << "Removing task_id " << task_id;
  return ret;
}

bool FakeMessageLoop::RunOnce(bool may_block) {
  if (test_clock_)
    current_time_ = test_clock_->Now();
  // Try to fire ready file descriptors first.
  for (const auto& fd_mode : fds_ready_) {
    const auto& fd_watched =  fds_watched_.find(fd_mode);
    if (fd_watched == fds_watched_.end())
      continue;
    // The fd_watched->second task might have been canceled and we never removed
    // it from the fds_watched_, so we fix that now.
    const auto& scheduled_task_ref = tasks_.find(fd_watched->second);
    if (scheduled_task_ref == tasks_.end()) {
      fds_watched_.erase(fd_watched);
      continue;
    }
    VLOG_LOC(scheduled_task_ref->second.location, 1)
        << "Running task_id " << fd_watched->second
        << " for watching file descriptor " << fd_mode.first << " for "
        << (fd_mode.second == MessageLoop::kWatchRead ? "reading" : "writing")
        << (scheduled_task_ref->second.persistent ?
            " persistently" : " just once")
        << " scheduled from this location.";
    if (scheduled_task_ref->second.persistent) {
      scheduled_task_ref->second.callback.Run();
    } else {
      base::Closure callback = std::move(scheduled_task_ref->second.callback);
      tasks_.erase(scheduled_task_ref);
      fds_watched_.erase(fd_watched);
      callback.Run();
    }
    return true;
  }

  // Try to fire time-based callbacks afterwards.
  while (!fire_order_.empty() &&
         (may_block || fire_order_.top().first <= current_time_)) {
    const auto task_ref = fire_order_.top();
    fire_order_.pop();
    // We need to skip tasks in the priority_queue not in the |tasks_| map.
    // This is normal if the task was canceled, as there is no efficient way
    // to remove a task from the priority_queue.
    const auto scheduled_task_ref = tasks_.find(task_ref.second);
    if (scheduled_task_ref == tasks_.end())
      continue;
    // Advance the clock to the task firing time, if needed.
    if (current_time_ < task_ref.first) {
      current_time_ = task_ref.first;
      if (test_clock_)
        test_clock_->SetNow(current_time_);
    }
    // Move the Closure out of the map before delete it. We need to delete the
    // entry from the map before we call the callback, since calling CancelTask
    // for the task you are running now should fail and return false.
    base::Closure callback = std::move(scheduled_task_ref->second.callback);
    VLOG_LOC(scheduled_task_ref->second.location, 1)
        << "Running task_id " << task_ref.second
        << " at time " << current_time_ << " from this location.";
    tasks_.erase(scheduled_task_ref);

    callback.Run();
    return true;
  }
  return false;
}

void FakeMessageLoop::SetFileDescriptorReadiness(int fd,
                                                 WatchMode mode,
                                                 bool ready) {
  if (ready)
    fds_ready_.emplace(fd, mode);
  else
    fds_ready_.erase(std::make_pair(fd, mode));
}

bool FakeMessageLoop::PendingTasks() {
  for (const auto& task : tasks_) {
    VLOG_LOC(task.second.location, 1)
        << "Pending " << (task.second.persistent ? "persistent " : "")
        << "task_id " << task.first << " scheduled from here.";
  }
  return !tasks_.empty();
}

}  // namespace brillo
