// Copyright 2014 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBBRILLO_BRILLO_ASYNCHRONOUS_SIGNAL_HANDLER_H_
#define LIBBRILLO_BRILLO_ASYNCHRONOUS_SIGNAL_HANDLER_H_

#include <signal.h>
#include <sys/signalfd.h>

#include <map>

#include <base/callback.h>
#include <base/compiler_specific.h>
#include <base/macros.h>
#include <base/memory/scoped_ptr.h>
#include <base/message_loop/message_loop.h>
#include <brillo/asynchronous_signal_handler_interface.h>
#include <brillo/brillo_export.h>
#include <brillo/message_loops/message_loop.h>

namespace brillo {
// Sets up signal handlers for registered signals, and converts signal receipt
// into a write on a pipe. Watches that pipe for data and, when some appears,
// execute the associated callback.
class BRILLO_EXPORT AsynchronousSignalHandler final :
    public AsynchronousSignalHandlerInterface {
 public:
  AsynchronousSignalHandler();
  ~AsynchronousSignalHandler() override;

  using AsynchronousSignalHandlerInterface::SignalHandler;

  // Initialize the handler.
  void Init();

  // AsynchronousSignalHandlerInterface overrides.
  void RegisterHandler(int signal, const SignalHandler& callback) override;
  void UnregisterHandler(int signal) override;

 private:
  // Called from the main loop when we can read from |descriptor_|, indicated
  // that a signal was processed.
  void OnFileCanReadWithoutBlocking();

  // Controller used to manage watching of signalling pipe.
  MessageLoop::TaskId fd_watcher_task_{MessageLoop::kTaskIdNull};

  // The registered callbacks.
  typedef std::map<int, SignalHandler> Callbacks;
  Callbacks registered_callbacks_;

  // File descriptor for accepting signals indicated by |signal_mask_|.
  int descriptor_;

  // A set of signals to be handled after the dispatcher is running.
  sigset_t signal_mask_;

  // A copy of the signal mask before the dispatcher starts, which will be
  // used to restore to the original state when the dispatcher stops.
  sigset_t saved_signal_mask_;

  // Resets the given signal to its default behavior. Doesn't touch
  // |registered_callbacks_|.
  BRILLO_PRIVATE void ResetSignal(int signal);

  // Updates the set of signals that this handler listens to.
  BRILLO_PRIVATE void UpdateSignals();

  DISALLOW_COPY_AND_ASSIGN(AsynchronousSignalHandler);
};

}  // namespace brillo

#endif  // LIBBRILLO_BRILLO_ASYNCHRONOUS_SIGNAL_HANDLER_H_
