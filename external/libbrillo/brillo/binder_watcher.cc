/*
 * Copyright (C) 2015 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <brillo/binder_watcher.h>

#include <base/bind.h>
#include <base/logging.h>
#include <binder/IPCThreadState.h>
#include <binder/ProcessState.h>

using android::IPCThreadState;
using android::ProcessState;

namespace {
// Called from the message loop whenever the binder file descriptor is ready.
void OnBinderReadReady() {
  IPCThreadState::self()->handlePolledCommands();
}
}  // namespace

namespace brillo {

BinderWatcher::BinderWatcher(MessageLoop* message_loop)
    : message_loop_(message_loop) {}

BinderWatcher::BinderWatcher() : message_loop_(nullptr) {}

BinderWatcher::~BinderWatcher() {
  if (task_id_ != MessageLoop::kTaskIdNull)
    message_loop_->CancelTask(task_id_);
}

bool BinderWatcher::Init() {
  if (!message_loop_)
    message_loop_ = MessageLoop::current();
  if (!message_loop_) {
    LOG(ERROR) << "Must initialize a brillo::MessageLoop to use BinderWatcher";
    return false;
  }

  int binder_fd = -1;
  ProcessState::self()->setThreadPoolMaxThreadCount(0);
  IPCThreadState::self()->disableBackgroundScheduling(true);
  int err = IPCThreadState::self()->setupPolling(&binder_fd);
  if (err != 0) {
    LOG(ERROR) << "Error setting up binder polling: "
               << logging::SystemErrorCodeToString(err);
    return false;
  }
  if (binder_fd < 0) {
    LOG(ERROR) << "Invalid binder FD " << binder_fd;
    return false;
  }
  VLOG(1) << "Got binder FD " << binder_fd;

  task_id_ = message_loop_->WatchFileDescriptor(
      FROM_HERE,
      binder_fd,
      MessageLoop::kWatchRead,
      true /* persistent */,
      base::Bind(&OnBinderReadReady));
  if (task_id_ == MessageLoop::kTaskIdNull) {
    LOG(ERROR) << "Failed to watch binder FD";
    return false;
  }
  return true;
}

}  // namespace brillo
