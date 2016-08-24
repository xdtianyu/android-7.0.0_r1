// Copyright 2014 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBBRILLO_BRILLO_DAEMONS_DAEMON_H_
#define LIBBRILLO_BRILLO_DAEMONS_DAEMON_H_

#include <string>

#include <base/at_exit.h>
#include <base/macros.h>
#include <base/message_loop/message_loop.h>
#include <brillo/asynchronous_signal_handler.h>
#include <brillo/brillo_export.h>
#include <brillo/message_loops/base_message_loop.h>

struct signalfd_siginfo;

namespace brillo {

// Daemon is a simple base class for system daemons. It provides a lot
// of useful facilities such as a message loop, handling of SIGTERM, SIGINT, and
// SIGHUP system signals.
// You can use this class directly to implement your daemon or you can
// specialize it by creating your own class and deriving it from
// brillo::Daemon. Override some of the virtual methods provide to fine-tune
// its behavior to suit your daemon's needs.
class BRILLO_EXPORT Daemon : public AsynchronousSignalHandlerInterface {
 public:
  Daemon();
  virtual ~Daemon();

  // Performs proper initialization of the daemon and runs the message loop.
  // Blocks until the daemon is finished. The return value is the error
  // code that should be returned from daemon's main(). Returns EX_OK (0) on
  // success.
  virtual int Run();

  // Can be used by call-backs to trigger shut-down of a running message loop.
  // Calls QuiteWithExitCode(EX_OK);
  // WARNING: This method (as well as QuitWithExitCode) can only be called when
  // the message loop is running (that is, during Daemon::Run() call). Calling
  // these methods before (e.g. during OnInit()) or after (e.g in OnShutdown())
  // will lead to abnormal process termination.
  void Quit();

  // |exit_code| is the status code to be returned when the daemon process
  // quits. See the warning for Quit() above regarding the allowed scope for
  // this method.
  void QuitWithExitCode(int exit_code);

  // AsynchronousSignalHandlerInterface overrides.
  // Register/unregister custom signal handlers for the daemon. The semantics
  // are identical to AsynchronousSignalHandler::RegisterHandler and
  // AsynchronousSignalHandler::UnregisterHandler, except that handlers for
  // SIGTERM, SIGINT, and SIGHUP cannot be modified.
  void RegisterHandler(
      int signal, const
      AsynchronousSignalHandlerInterface::SignalHandler& callback) override;
  void UnregisterHandler(int signal) override;

 protected:
  // Overload to provide your own initialization code that should happen just
  // before running the message loop. Return EX_OK (0) on success or any other
  // non-zero error codes. If an error is returned, the message loop execution
  // is aborted and Daemon::Run() exits early.
  // When overloading, make sure you call the base implementation of OnInit().
  virtual int OnInit();
  // Called when the message loops exits and before Daemon::Run() returns.
  // Overload to clean up the data that was set up during OnInit().
  // |return_code| contains the current error code that will be returned from
  // Run(). You can override this value with your own error code if needed.
  // When overloading, make sure you call the base implementation of
  // OnShutdown().
  virtual void OnShutdown(int* exit_code);
  // Called when the SIGHUP signal is received. In response to this call, your
  // daemon could reset/reload the configuration and re-initialize its state
  // as if the process has been reloaded.
  // Return true if the signal was processed successfully and the daemon
  // reset its configuration. Returning false will force the daemon to
  // quit (and subsequently relaunched by an upstart job, if one is configured).
  // The default implementation just returns false (unhandled), which terminates
  // the daemon, so do not call the base implementation of OnRestart() from
  // your overload.
  virtual bool OnRestart();

  // Returns a delegate to Quit() method in the base::RunLoop instance.
  base::Closure QuitClosure() const {
    return brillo_message_loop_.QuitClosure();
  }

 private:
  // Called when SIGTERM/SIGINT signals are received.
  bool Shutdown(const signalfd_siginfo& info);
  // Called when SIGHUP signal is received.
  bool Restart(const signalfd_siginfo& info);

  // |at_exit_manager_| must be first to make sure it is initialized before
  // other members, especially the |message_loop_|.
  base::AtExitManager at_exit_manager_;
  // The main message loop for the daemon.
  base::MessageLoopForIO message_loop_;
  // The brillo wrapper for the main message loop.
  BaseMessageLoop brillo_message_loop_{&message_loop_};
  // A helper to dispatch signal handlers asynchronously, so that the main
  // system signal handler returns as soon as possible.
  AsynchronousSignalHandler async_signal_handler_;
  // Process exit code specified in QuitWithExitCode() method call.
  int exit_code_;

  DISALLOW_COPY_AND_ASSIGN(Daemon);
};

}  // namespace brillo

#endif  // LIBBRILLO_BRILLO_DAEMONS_DAEMON_H_
