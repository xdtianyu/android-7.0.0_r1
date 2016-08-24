// Copyright 2015 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBBRILLO_BRILLO_ASYNCHRONOUS_SIGNAL_HANDLER_INTERFACE_H_
#define LIBBRILLO_BRILLO_ASYNCHRONOUS_SIGNAL_HANDLER_INTERFACE_H_

#include <sys/signalfd.h>

#include <base/callback.h>
#include <brillo/brillo_export.h>

namespace brillo {

// Sets up signal handlers for registered signals, and converts signal receipt
// into a write on a pipe. Watches that pipe for data and, when some appears,
// execute the associated callback.
class BRILLO_EXPORT AsynchronousSignalHandlerInterface {
 public:
  virtual ~AsynchronousSignalHandlerInterface() = default;

  // The callback called when a signal is received.
  using SignalHandler = base::Callback<bool(const struct signalfd_siginfo&)>;

  // Register a new handler for the given |signal|, replacing any previously
  // registered handler. |callback| will be called on the thread the
  // |AsynchronousSignalHandlerInterface| implementation is bound to when a
  // signal is received. The received |signalfd_siginfo| will be passed to
  // |callback|. |callback| must returns |true| if the signal handler must be
  // unregistered, and |false| otherwise. Due to an implementation detail, you
  // cannot set any sigaction flags you might be accustomed to using. This might
  // matter if you hoped to use SA_NOCLDSTOP to avoid getting a SIGCHLD when a
  // child process receives a SIGSTOP.
  virtual void RegisterHandler(int signal, const SignalHandler& callback) = 0;

  // Unregister a previously registered handler for the given |signal|.
  virtual void UnregisterHandler(int signal) = 0;
};

}  // namespace brillo

#endif  // LIBBRILLO_BRILLO_ASYNCHRONOUS_SIGNAL_HANDLER_INTERFACE_H_
