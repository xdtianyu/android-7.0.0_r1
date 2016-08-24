// Copyright 2014 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "brillo/asynchronous_signal_handler.h"

#include <signal.h>
#include <sys/types.h>
#include <unistd.h>

#include <base/bind.h>
#include <base/files/file_util.h>
#include <base/logging.h>
#include <base/message_loop/message_loop.h>
#include <base/posix/eintr_wrapper.h>

namespace {
const int kInvalidDescriptor = -1;
}  // namespace

namespace brillo {

AsynchronousSignalHandler::AsynchronousSignalHandler()
    : descriptor_(kInvalidDescriptor) {
  CHECK_EQ(sigemptyset(&signal_mask_), 0) << "Failed to initialize signal mask";
  CHECK_EQ(sigemptyset(&saved_signal_mask_), 0)
      << "Failed to initialize signal mask";
}

AsynchronousSignalHandler::~AsynchronousSignalHandler() {
  if (descriptor_ != kInvalidDescriptor) {
    MessageLoop::current()->CancelTask(fd_watcher_task_);

    if (IGNORE_EINTR(close(descriptor_)) != 0)
      PLOG(WARNING) << "Failed to close file descriptor";

    descriptor_ = kInvalidDescriptor;
    CHECK_EQ(0, sigprocmask(SIG_SETMASK, &saved_signal_mask_, nullptr));
  }
}

void AsynchronousSignalHandler::Init() {
  CHECK_EQ(kInvalidDescriptor, descriptor_);
  CHECK_EQ(0, sigprocmask(SIG_BLOCK, &signal_mask_, &saved_signal_mask_));
  descriptor_ =
      signalfd(descriptor_, &signal_mask_, SFD_CLOEXEC | SFD_NONBLOCK);
  CHECK_NE(kInvalidDescriptor, descriptor_);
  fd_watcher_task_ = MessageLoop::current()->WatchFileDescriptor(
      FROM_HERE,
      descriptor_,
      MessageLoop::WatchMode::kWatchRead,
      true,
      base::Bind(&AsynchronousSignalHandler::OnFileCanReadWithoutBlocking,
                 base::Unretained(this)));
  CHECK(fd_watcher_task_ != MessageLoop::kTaskIdNull)
      << "Watching shutdown pipe failed.";
}

void AsynchronousSignalHandler::RegisterHandler(int signal,
                                                const SignalHandler& callback) {
  registered_callbacks_[signal] = callback;
  CHECK_EQ(0, sigaddset(&signal_mask_, signal));
  UpdateSignals();
}

void AsynchronousSignalHandler::UnregisterHandler(int signal) {
  Callbacks::iterator callback_it = registered_callbacks_.find(signal);
  if (callback_it != registered_callbacks_.end()) {
    registered_callbacks_.erase(callback_it);
    ResetSignal(signal);
  }
}

void AsynchronousSignalHandler::OnFileCanReadWithoutBlocking() {
  struct signalfd_siginfo info;
  while (base::ReadFromFD(descriptor_,
                          reinterpret_cast<char*>(&info), sizeof(info))) {
    int signal = info.ssi_signo;
    Callbacks::iterator callback_it = registered_callbacks_.find(signal);
    if (callback_it == registered_callbacks_.end()) {
      LOG(WARNING) << "Unable to find a signal handler for signal: " << signal;
      // Can happen if a signal has been called multiple time, and the callback
      // asked to be unregistered the first time.
      continue;
    }
    const SignalHandler& callback = callback_it->second;
    bool must_unregister = callback.Run(info);
    if (must_unregister) {
      UnregisterHandler(signal);
    }
  }
}

void AsynchronousSignalHandler::ResetSignal(int signal) {
  CHECK_EQ(0, sigdelset(&signal_mask_, signal));
  UpdateSignals();
}

void AsynchronousSignalHandler::UpdateSignals() {
  if (descriptor_ != kInvalidDescriptor) {
    CHECK_EQ(0, sigprocmask(SIG_SETMASK, &saved_signal_mask_, nullptr));
    CHECK_EQ(0, sigprocmask(SIG_BLOCK, &signal_mask_, nullptr));
    CHECK_EQ(descriptor_,
             signalfd(descriptor_, &signal_mask_, SFD_CLOEXEC | SFD_NONBLOCK));
  }
}

}  // namespace brillo
