// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBWEAVE_EXAMPLES_PROVIDER_EVENT_TASK_RUNNER_H_
#define LIBWEAVE_EXAMPLES_PROVIDER_EVENT_TASK_RUNNER_H_

#include <queue>
#include <utility>
#include <vector>

#include <event2/event.h>
#include <weave/provider/task_runner.h>

#include "examples/provider/event_deleter.h"

namespace weave {
namespace examples {

// Simple task runner implemented with libevent message loop.
class EventTaskRunner : public provider::TaskRunner {
 public:
  void PostDelayedTask(const tracked_objects::Location& from_here,
                       const base::Closure& task,
                       base::TimeDelta delay) override;

  // Defines the types of I/O completion events that the
  // application can register to receive on a file descriptor.
  enum IOEvent : int16_t {
    kReadable = 0x01,
    kWriteable = 0x02,
    kClosed = 0x04,
    kReadableWriteable = kReadable | kWriteable,
    kReadableOrClosed = kReadable | kClosed,
    kAll = kReadableOrClosed | kWriteable,
  };

  // Callback type for I/O completion events.
  // Arguments:
  //  fd -      file descriptor that triggered the event
  //  what -    combination of IOEvent flags indicating
  //            which event(s) occurred
  //  sender -  reference to the EventTaskRunner that
  //            called the IoCompletionCallback
  using IoCompletionCallback =
      base::Callback<void(int fd, int16_t what, EventTaskRunner* sender)>;

  // Adds a handler for the specified IO completion events on a file
  // descriptor. The 'what' parameter is a combination of IOEvent flags.
  // Only one callback is allowed per file descriptor; calling this function
  // with an fd that has already been registered will replace the previous
  // callback with the new one.
  void AddIoCompletionTask(int fd,
                           int16_t what,
                           const IoCompletionCallback& task);

  // Remove the callback associated with this fd and stop listening for
  // events related to it.
  void RemoveIoCompletionTask(int fd);

  event_base* GetEventBase() const { return base_.get(); }

  void Run();

 private:
  void ReScheduleEvent(base::TimeDelta delay);
  static void EventHandler(int, int16_t, void* runner);
  static void FreeEvent(event* evnt);
  void Process();

  static void FdEventHandler(int fd, int16_t what, void* runner);
  void ProcessFd(int fd, int16_t what);

  using QueueItem = std::pair<std::pair<base::Time, size_t>, base::Closure>;

  struct Greater {
    bool operator()(const QueueItem& a, const QueueItem& b) const {
      return a.first > b.first;
    }
  };

  size_t counter_{0};  // Keeps order of tasks with the same time.

  std::priority_queue<QueueItem,
                      std::vector<QueueItem>,
                      EventTaskRunner::Greater>
      queue_;

  EventPtr<event_base> base_{event_base_new()};

  EventPtr<event> task_event_{
      event_new(base_.get(), -1, EV_TIMEOUT, &EventHandler, this)};

  std::map<int, std::pair<EventPtr<event>, IoCompletionCallback>> fd_task_map_;
};

}  // namespace examples
}  // namespace weave

#endif  // LIBWEAVE_EXAMPLES_PROVIDER_EVENT_TASK_RUNNER_H_
