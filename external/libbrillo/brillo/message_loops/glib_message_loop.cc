// Copyright 2015 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <brillo/message_loops/glib_message_loop.h>

#include <fcntl.h>
#include <unistd.h>

#include <brillo/location_logging.h>

using base::Closure;

namespace brillo {

GlibMessageLoop::GlibMessageLoop() {
  loop_ = g_main_loop_new(g_main_context_default(), FALSE);
}

GlibMessageLoop::~GlibMessageLoop() {
  // Cancel all pending tasks when destroying the message loop.
  for (const auto& task : tasks_) {
    DVLOG_LOC(task.second->location, 1)
        << "Removing task_id " << task.second->task_id
        << " leaked on GlibMessageLoop, scheduled from this location.";
    g_source_remove(task.second->source_id);
  }
  g_main_loop_unref(loop_);
}

MessageLoop::TaskId GlibMessageLoop::PostDelayedTask(
    const tracked_objects::Location& from_here,
    const Closure &task,
    base::TimeDelta delay) {
  TaskId task_id =  NextTaskId();
  // Note: While we store persistent = false in the ScheduledTask object, we
  // don't check it in OnRanPostedTask() since it is always false for delayed
  // tasks. This is only used for WatchFileDescriptor below.
  ScheduledTask* scheduled_task = new ScheduledTask{
    this, from_here, task_id, 0, false, std::move(task)};
  DVLOG_LOC(from_here, 1) << "Scheduling delayed task_id " << task_id
                          << " to run in " << delay << ".";
  scheduled_task->source_id = g_timeout_add_full(
      G_PRIORITY_DEFAULT,
      delay.InMillisecondsRoundedUp(),
      &GlibMessageLoop::OnRanPostedTask,
      reinterpret_cast<gpointer>(scheduled_task),
      DestroyPostedTask);
  tasks_[task_id] = scheduled_task;
  return task_id;
}

MessageLoop::TaskId GlibMessageLoop::WatchFileDescriptor(
    const tracked_objects::Location& from_here,
    int fd,
    WatchMode mode,
    bool persistent,
    const Closure &task) {
  // Quick check to see if the fd is valid.
  if (fcntl(fd, F_GETFD) == -1 && errno == EBADF)
      return MessageLoop::kTaskIdNull;

  GIOCondition condition = G_IO_NVAL;
  switch (mode) {
    case MessageLoop::kWatchRead:
      condition = static_cast<GIOCondition>(G_IO_IN | G_IO_HUP | G_IO_NVAL);
      break;
    case MessageLoop::kWatchWrite:
      condition = static_cast<GIOCondition>(G_IO_OUT | G_IO_HUP | G_IO_NVAL);
      break;
    default:
      return MessageLoop::kTaskIdNull;
  }

  // TODO(deymo): Used g_unix_fd_add_full() instead of g_io_add_watch_full()
  // when/if we switch to glib 2.36 or newer so we don't need to create a
  // GIOChannel for this.
  GIOChannel* io_channel = g_io_channel_unix_new(fd);
  if (!io_channel)
    return MessageLoop::kTaskIdNull;
  GError* error = nullptr;
  GIOStatus status = g_io_channel_set_encoding(io_channel, nullptr, &error);
  if (status != G_IO_STATUS_NORMAL) {
    LOG(ERROR) << "GError(" << error->code << "): "
               << (error->message ? error->message : "(unknown)");
    g_error_free(error);
    // g_io_channel_set_encoding() documentation states that this should be
    // valid in this context (a new io_channel), but enforce the check in
    // debug mode.
    DCHECK(status == G_IO_STATUS_NORMAL);
    return MessageLoop::kTaskIdNull;
  }

  TaskId task_id =  NextTaskId();
  ScheduledTask* scheduled_task = new ScheduledTask{
    this, from_here, task_id, 0, persistent, std::move(task)};
  scheduled_task->source_id = g_io_add_watch_full(
      io_channel,
      G_PRIORITY_DEFAULT,
      condition,
      &GlibMessageLoop::OnWatchedFdReady,
      reinterpret_cast<gpointer>(scheduled_task),
      DestroyPostedTask);
  // g_io_add_watch_full() increases the reference count on the newly created
  // io_channel, so we can dereference it now and it will be free'd once the
  // source is removed or now if g_io_add_watch_full() failed.
  g_io_channel_unref(io_channel);

  DVLOG_LOC(from_here, 1)
      << "Watching fd " << fd << " for "
      << (mode == MessageLoop::kWatchRead ? "reading" : "writing")
      << (persistent ? " persistently" : " just once")
      << " as task_id " << task_id
      << (scheduled_task->source_id ? " successfully" : " failed.");

  if (!scheduled_task->source_id) {
    delete scheduled_task;
    return MessageLoop::kTaskIdNull;
  }
  tasks_[task_id] = scheduled_task;
  return task_id;
}

bool GlibMessageLoop::CancelTask(TaskId task_id) {
  if (task_id == kTaskIdNull)
    return false;
  const auto task = tasks_.find(task_id);
  // It is a programmer error to attempt to remove a non-existent source.
  if (task == tasks_.end())
    return false;
  DVLOG_LOC(task->second->location, 1)
      << "Removing task_id " << task_id << " scheduled from this location.";
  guint source_id = task->second->source_id;
  // We remove here the entry from the tasks_ map, the pointer will be deleted
  // by the g_source_remove() call.
  tasks_.erase(task);
  return g_source_remove(source_id);
}

bool GlibMessageLoop::RunOnce(bool may_block) {
  return g_main_context_iteration(nullptr, may_block);
}

void GlibMessageLoop::Run() {
  g_main_loop_run(loop_);
}

void GlibMessageLoop::BreakLoop() {
  g_main_loop_quit(loop_);
}

MessageLoop::TaskId GlibMessageLoop::NextTaskId() {
  TaskId res;
  do {
    res = ++last_id_;
    // We would run out of memory before we run out of task ids.
  } while (!res || tasks_.find(res) != tasks_.end());
  return res;
}

gboolean GlibMessageLoop::OnRanPostedTask(gpointer user_data) {
  ScheduledTask* scheduled_task = reinterpret_cast<ScheduledTask*>(user_data);
  DVLOG_LOC(scheduled_task->location, 1)
      << "Running delayed task_id " << scheduled_task->task_id
      << " scheduled from this location.";
  // We only need to remove this task_id from the map. DestroyPostedTask will be
  // called with this same |user_data| where we can delete the ScheduledTask.
  scheduled_task->loop->tasks_.erase(scheduled_task->task_id);
  scheduled_task->closure.Run();
  return FALSE;  // Removes the source since a callback can only be called once.
}

gboolean GlibMessageLoop::OnWatchedFdReady(GIOChannel *source,
                                           GIOCondition condition,
                                           gpointer user_data) {
  ScheduledTask* scheduled_task = reinterpret_cast<ScheduledTask*>(user_data);
  DVLOG_LOC(scheduled_task->location, 1)
      << "Running task_id " << scheduled_task->task_id
      << " for watching a file descriptor, scheduled from this location.";
  if (!scheduled_task->persistent) {
    // We only need to remove this task_id from the map. DestroyPostedTask will
    // be called with this same |user_data| where we can delete the
    // ScheduledTask.
    scheduled_task->loop->tasks_.erase(scheduled_task->task_id);
  }
  scheduled_task->closure.Run();
  return scheduled_task->persistent;
}

void GlibMessageLoop::DestroyPostedTask(gpointer user_data) {
  delete reinterpret_cast<ScheduledTask*>(user_data);
}

}  // namespace brillo
