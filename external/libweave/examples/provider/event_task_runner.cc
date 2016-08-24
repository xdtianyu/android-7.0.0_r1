// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "examples/provider/event_task_runner.h"

#include <signal.h>

namespace weave {
namespace examples {

namespace {
event_base* g_event_base = nullptr;
}

void EventTaskRunner::PostDelayedTask(
    const tracked_objects::Location& from_here,
    const base::Closure& task,
    base::TimeDelta delay) {
  base::Time new_time = base::Time::Now() + delay;
  if (queue_.empty() || new_time < queue_.top().first.first) {
    ReScheduleEvent(delay);
  }
  queue_.emplace(std::make_pair(new_time, ++counter_), task);
}

void EventTaskRunner::AddIoCompletionTask(
    int fd,
    int16_t what,
    const EventTaskRunner::IoCompletionCallback& task) {
  int16_t flags = EV_PERSIST | EV_ET;
  flags |= (what & kReadable) ? EV_READ : 0;
  flags |= (what & kWriteable) ? EV_WRITE : 0;
#if LIBEVENT_VERSION_NUMBER >= 0x02010400
  flags |= (what & kClosed) ? EV_CLOSED : 0;
#endif
  event* ioevent = event_new(base_.get(), fd, flags, FdEventHandler, this);
  EventPtr<event> ioeventPtr{ioevent};
  fd_task_map_.insert(
      std::make_pair(fd, std::make_pair(std::move(ioeventPtr), task)));
  event_add(ioevent, nullptr);
}

void EventTaskRunner::RemoveIoCompletionTask(int fd) {
  fd_task_map_.erase(fd);
}

void EventTaskRunner::Run() {
  g_event_base = base_.get();

  struct sigaction sa = {};
  sa.sa_handler = [](int signal) {
    event_base_loopexit(g_event_base, nullptr);
  };
  sigfillset(&sa.sa_mask);
  sigaction(SIGINT, &sa, nullptr);

  do {
    event_base_loop(g_event_base, EVLOOP_ONCE);
  } while (!event_base_got_exit(g_event_base));
  g_event_base = nullptr;
}

void EventTaskRunner::ReScheduleEvent(base::TimeDelta delay) {
  timespec ts = delay.ToTimeSpec();
  timeval tv = {ts.tv_sec, ts.tv_nsec / 1000};
  event_add(task_event_.get(), &tv);
}

void EventTaskRunner::EventHandler(int /* fd */,
                                   int16_t /* what */,
                                   void* runner) {
  static_cast<EventTaskRunner*>(runner)->Process();
}

void EventTaskRunner::FreeEvent(event* evnt) {
  event_del(evnt);
  event_free(evnt);
}

void EventTaskRunner::Process() {
  while (!queue_.empty() && queue_.top().first.first <= base::Time::Now()) {
    auto cb = queue_.top().second;
    queue_.pop();
    cb.Run();
  }
  if (!queue_.empty()) {
    base::TimeDelta delta = std::max(
        base::TimeDelta(), queue_.top().first.first - base::Time::Now());
    ReScheduleEvent(delta);
  }
}

void EventTaskRunner::FdEventHandler(int fd, int16_t what, void* runner) {
  static_cast<EventTaskRunner*>(runner)->ProcessFd(fd, what);
}

void EventTaskRunner::ProcessFd(int fd, int16_t what) {
  auto it = fd_task_map_.find(fd);
  if (it != fd_task_map_.end()) {
    const IoCompletionCallback& callback = it->second.second;
    callback.Run(fd, what, this);
  }
}

}  // namespace examples
}  // namespace weave
