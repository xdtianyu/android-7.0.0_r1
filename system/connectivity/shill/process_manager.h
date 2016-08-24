//
// Copyright (C) 2015 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

#ifndef SHILL_PROCESS_MANAGER_H_
#define SHILL_PROCESS_MANAGER_H_

#include <map>
#include <string>
#include <vector>

#include <base/callback.h>
#include <base/cancelable_callback.h>
#include <base/files/file_path.h>
#include <base/lazy_instance.h>
#include <base/memory/weak_ptr.h>
#include <base/tracked_objects.h>
#include <brillo/minijail/minijail.h>
#include <brillo/process.h>
#include <brillo/process_reaper.h>

namespace shill {

class EventDispatcher;

// The ProcessManager is a singleton providing process creation and
// asynchronous process termination. Need to initialize it once with
// Init method call.
class ProcessManager {
 public:
  virtual ~ProcessManager();

  // This is a singleton -- use ProcessManager::GetInstance()->Foo().
  static ProcessManager* GetInstance();

  // Register async signal handler and setup process reaper.
  virtual void Init(EventDispatcher* dispatcher);

  // Call on shutdown to release async_signal_handler_.
  virtual void Stop();

  // Create and start a process for |program| with |arguments|. |enivronment|
  // variables will be setup in the child process before exec the |program|.
  // |terminate_with_parent| is used to indicate if child process should
  // self terminate if the parent process exits.  |exit_callback| will be
  // invoked when child process exits (not terminated by us).  Return -1
  // if failed to start the process, otherwise, return the pid of the child
  // process.
  virtual pid_t StartProcess(
      const tracked_objects::Location& spawn_source,
      const base::FilePath& program,
      const std::vector<std::string>& arguments,
      const std::map<std::string, std::string>& environment,
      bool terminate_with_parent,
      const base::Callback<void(int)>& exit_callback);

  // Similar to StartProcess(), with the following differences:
  // - environment variables are not supported (no need yet)
  // - terminate_with_parent is not supported (may be non-trivial)
  // - the child process will run as |user| and |group|
  // - the |capmask| argument can be used to provide the child process
  //   with capabilities, which |user| might not have on its own
  virtual pid_t StartProcessInMinijail(
      const tracked_objects::Location& spawn_source,
      const base::FilePath& program,
      const std::vector<std::string>& arguments,
      const std::string& user,
      const std::string& group,
      uint64_t capmask,
      const base::Callback<void(int)>& exit_callback) {
    return StartProcessInMinijailWithPipes(
        spawn_source, program, arguments, user, group, capmask, exit_callback,
        nullptr, nullptr, nullptr);
  }

  // Similar to StartProcessInMinijail(), with the additional ability to
  // pipe the child's stdin/stdout/stderr back to us. If any of those
  // streams is not needed, simply pass nullptr for the corresponding
  // 'fd' argument. If no pipes are needed, use StartProcessInMinijail().
  virtual pid_t StartProcessInMinijailWithPipes(
      const tracked_objects::Location& spawn_source,
      const base::FilePath& program,
      const std::vector<std::string>& arguments,
      const std::string& user,
      const std::string& group,
      uint64_t capmask,
      const base::Callback<void(int)>& exit_callback,
      int* stdin_fd,
      int* stdout_fd,
      int* stderr_fd);

  // Stop the given |pid|.  Previously registered |exit_callback| will be
  // unregistered, since the caller is not interested in this process anymore
  // and that callback might not be valid by the time this process terminates.
  // This will attempt to terminate the child process by sending a SIGTERM
  // signal first.  If the process doesn't terminate within a certain time,
  // ProcessManager will attempt to send a SIGKILL signal.  It will give up
  // with an error log If the process still doesn't terminate within a certain
  // time.
  virtual bool StopProcess(pid_t pid);

  // Stop the given |pid| in a synchronous manner.
  virtual bool StopProcessAndBlock(pid_t pid);

  // Replace the current exit callback for |pid| with |new_callback|.
  virtual bool UpdateExitCallback(
      pid_t pid,
      const base::Callback<void(int)>& new_callback);

 protected:
  ProcessManager();

 private:
  friend class ProcessManagerTest;
  friend struct base::DefaultLazyInstanceTraits<ProcessManager>;

  using TerminationTimeoutCallback = base::CancelableClosure;

  // Invoked when process |pid| exited.
  void OnProcessExited(pid_t pid, const siginfo_t& info);

  // Invoked when process |pid| did not terminate within a certain timeout.
  // |kill_signal| indicates the signal used for termination. When it is set
  // to true, SIGKILL was used to terminate the process, otherwise, SIGTERM
  // was used.
  void ProcessTerminationTimeoutHandler(pid_t pid, bool kill_signal);

  // Send a termination signal to process |pid|. If |kill_signal| is set to
  // true, SIGKILL is sent, otherwise, SIGTERM is sent.  After signal is sent,
  // |pid| and timeout handler is added to |pending_termination_processes_|
  // list, to make sure process |pid| does exit in timely manner.
  bool TerminateProcess(pid_t pid, bool kill_signal);

  // Kill process |pid|. If |kill_signal| is true it will send SIGKILL,
  // otherwise it will send SIGTERM.
  // It returns true when the process was already dead or killed within
  // the timeout.
  // It returns false when the process failed to exit within the timeout
  // or the system failed to send kill singal.
  bool KillProcessWithTimeout(pid_t pid, bool kill_signal);

  // Kill process |pid| using signal |signal|.
  // The |killed| will be set true when the process was already dead.
  // It returns true when it sent the |signal| successfully or the
  // process was already dead.
  // It returns false when the system failed to send |signal|.
  bool KillProcess(pid_t pid, int signal, bool* killed);

  // Wait for process |pid| to exit. This function will check it for at most
  // |tries| times. The interval of waiting time grows exponentially from
  // |sleep_ms| and it has an |upper_bound_ms| upper bound.
  bool WaitpidWithTimeout(pid_t pid,
                          unsigned int sleep_ms,
                          unsigned int upper_bound_ms,
                          int tries);

  // Used to watch processes.
  std::unique_ptr<brillo::AsynchronousSignalHandler> async_signal_handler_;
  brillo::ProcessReaper process_reaper_;

  EventDispatcher* dispatcher_;
  brillo::Minijail* minijail_;

  // Processes to watch for the caller.
  std::map<pid_t, base::Callback<void(int)>> watched_processes_;
  // Processes being terminated by us.  Use a timer to make sure process
  // does exit, log an error if it failed to exit within a specific timeout.
  std::map<pid_t, std::unique_ptr<TerminationTimeoutCallback>>
      pending_termination_processes_;

  base::WeakPtrFactory<ProcessManager> weak_factory_{this};
  DISALLOW_COPY_AND_ASSIGN(ProcessManager);
};

}  // namespace shill

#endif  // SHILL_PROCESS_MANAGER_H_
