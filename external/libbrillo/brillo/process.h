// Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBBRILLO_BRILLO_PROCESS_H_
#define LIBBRILLO_BRILLO_PROCESS_H_

#include <sys/types.h>

#include <map>
#include <string>
#include <vector>

#include <base/bind.h>
#include <base/callback.h>
#include <base/strings/string_util.h>
#include <base/strings/stringprintf.h>
#include <brillo/brillo_export.h>
#include <gtest/gtest_prod.h>

namespace brillo {
// Manages a process.  Can create the process, attach to an existing
// process by pid or pid file, and kill the process.  Upon destruction
// any managed process is killed with SIGKILL.  Use Release() to
// release the process from management.  A given system process may
// only be managed by one Process at a time.
class BRILLO_EXPORT Process {
 public:
  Process();
  virtual ~Process();

  // Adds |arg| to the executable command-line to be run.  The
  // executable name itself is the first argument.
  virtual void AddArg(const std::string& arg) = 0;

  // Adds |option| and |value| as an option with a string value to the
  // command line to be run.
  inline void AddStringOption(const std::string& option,
                              const std::string& value) {
    AddArg(option);
    AddArg(value);
  }

  // Adds |option| and |value| as an option which takes an integer
  // value to the command line to be run.
  inline void AddIntOption(const std::string& option, int value) {
    AddArg(option);
    AddArg(base::StringPrintf("%d", value));
  }

  // Redirects stderr and stdout to |output_file|.
  virtual void RedirectOutput(const std::string& output_file) = 0;

  // Indicates we want to redirect |child_fd| in the child process's
  // file table to a pipe.  |child_fd| will be available for reading
  // from child process's perspective iff |is_input|.
  virtual void RedirectUsingPipe(int child_fd, bool is_input) = 0;

  // Binds the given file descriptor in the parent to the given file
  // descriptor in the child.
  virtual void BindFd(int parent_fd, int child_fd) = 0;

  // Set a flag |close_unused_fds| to indicate if the child process
  // should close all unused file descriptors inherited from the
  // parent process.  This will not close the file descriptors for
  // the standard streams (stdin, stdout, and stderr).
  virtual void SetCloseUnusedFileDescriptors(bool close_unused_fds) = 0;

  // Set the real/effective/saved user ID of the child process.
  virtual void SetUid(uid_t uid) = 0;

  // Set the real/effective/saved group ID of the child process.
  virtual void SetGid(gid_t gid) = 0;

  // Set the capabilities assigned to the child process.
  // NOTE: |capmask| is indeed a mask and should be passed in as the result of
  // the CAP_TO_MASK(capability) macro, e.g.
  //     my_process.SetCapabilities(CAP_TO_MASK(CAP_SETUID) |
  //                                CAP_TO_MASK(CAP_SETGID));
  // NOTE: supporting this sandboxing feature is optional (provide no-op
  // implementation if your Process implementation does not support this).
  virtual void SetCapabilities(uint64_t capmask) = 0;

  // Apply a syscall filter to the process using the policy file at |path|.
  // NOTE: supporting this sandboxing feature is optional (provide no-op
  // implementation if your Process implementation does not support this).
  virtual void ApplySyscallFilter(const std::string& path) = 0;

  // Enter new PID namespace when this process is run.
  // NOTE: supporting this sandboxing feature is optional (provide no-op
  // implementation if your Process implementation does not support this).
  virtual void EnterNewPidNamespace() = 0;

  // Set a flag |inherit| to indicate if the child process intend to
  // inherit signal mask from the parent process. When |inherit| is
  // set to true, the child process will inherit signal mask from the
  // parent process. This could cause unintended side effect, where all
  // the signals to the child process might be blocked if they are set
  // in the parent's signal mask.
  virtual void SetInheritParentSignalMask(bool inherit) = 0;

  typedef base::Callback<bool(void)> PreExecCallback;

  // Set the pre-exec callback. This is called after all setup is complete but
  // before we exec() the process. The callback may return false to cause Start
  // to return false without starting the process.
  virtual void SetPreExecCallback(const PreExecCallback& cb) = 0;

  // Sets whether starting the process should search the system path or not.
  // By default the system path will not be searched.
  virtual void SetSearchPath(bool search_path) = 0;

  // Gets the pipe file descriptor mapped to the process's |child_fd|.
  virtual int GetPipe(int child_fd) = 0;

  // Starts this process, returning true if successful.
  virtual bool Start() = 0;

  // Waits for this process to finish.  Returns the process's exit
  // status if it exited normally, or otherwise returns -1.  Note
  // that kErrorExitStatus may be returned if an error occurred
  // after forking and before execing the child process.
  virtual int Wait() = 0;

  // Start and wait for this process to finish.  Returns same value as
  // Wait().
  virtual int Run() = 0;

  // Returns the pid of this process or else returns 0 if there is no
  // corresponding process (either because it has not yet been started
  // or has since exited).
  virtual pid_t pid() = 0;

  // Sends |signal| to process and wait |timeout| seconds until it
  // dies.  If process is not a child, returns immediately with a
  // value based on whether kill was successful.  If the process is a
  // child and |timeout| is non-zero, returns true if the process is
  // able to be reaped within the given |timeout| in seconds.
  virtual bool Kill(int signal, int timeout) = 0;

  // Resets this Process object to refer to the process with |pid|.
  // If |pid| is zero, this object no longer refers to a process.
  virtual void Reset(pid_t new_pid) = 0;

  // Same as Reset but reads the pid from |pid_file|.  Returns false
  // only when the file cannot be read/parsed.
  virtual bool ResetPidByFile(const std::string& pid_file) = 0;

  // Releases the process so that on destruction, the process is not killed.
  virtual pid_t Release() = 0;

  // Returns if |pid| is a currently running process.
  static bool ProcessExists(pid_t pid);

  // When returned from Wait or Run, indicates an error may have occurred
  // creating the process.
  enum { kErrorExitStatus = 127 };
};

class BRILLO_EXPORT ProcessImpl : public Process {
 public:
  ProcessImpl();
  virtual ~ProcessImpl();

  virtual void AddArg(const std::string& arg);
  virtual void RedirectOutput(const std::string& output_file);
  virtual void RedirectUsingPipe(int child_fd, bool is_input);
  virtual void BindFd(int parent_fd, int child_fd);
  virtual void SetCloseUnusedFileDescriptors(bool close_unused_fds);
  virtual void SetUid(uid_t uid);
  virtual void SetGid(gid_t gid);
  virtual void SetCapabilities(uint64_t capmask);
  virtual void ApplySyscallFilter(const std::string& path);
  virtual void EnterNewPidNamespace();
  virtual void SetInheritParentSignalMask(bool inherit);
  virtual void SetPreExecCallback(const PreExecCallback& cb);
  virtual void SetSearchPath(bool search_path);
  virtual int GetPipe(int child_fd);
  virtual bool Start();
  virtual int Wait();
  virtual int Run();
  virtual pid_t pid();
  virtual bool Kill(int signal, int timeout);
  virtual void Reset(pid_t pid);
  virtual bool ResetPidByFile(const std::string& pid_file);
  virtual pid_t Release();

 protected:
  struct PipeInfo {
    PipeInfo() : parent_fd_(-1), child_fd_(-1), is_input_(false) {}
    // Parent (our) side of the pipe to the child process.
    int parent_fd_;
    // Child's side of the pipe to the parent.
    int child_fd_;
    // Is this an input or output pipe from child's perspective.
    bool is_input_;
    // Is this a bound (pre-existing) file descriptor?
    bool is_bound_;
  };
  typedef std::map<int, PipeInfo> PipeMap;

  void UpdatePid(pid_t new_pid);
  bool PopulatePipeMap();

 private:
  FRIEND_TEST(ProcessTest, ResetPidByFile);

  bool IsFileDescriptorInPipeMap(int fd) const;
  void CloseUnusedFileDescriptors();

  // Pid of currently managed process or 0 if no currently managed
  // process.  pid must not be modified except by calling
  // UpdatePid(new_pid).
  pid_t pid_;
  std::string output_file_;
  std::vector<std::string> arguments_;
  // Map of child target file descriptors (first) to information about
  // pipes created (second).
  PipeMap pipe_map_;
  uid_t uid_;
  gid_t gid_;
  PreExecCallback pre_exec_;
  bool search_path_;
  // Flag indicating to inherit signal mask from the parent process. It
  // is set to false by default, which means by default the child process
  // will not inherit signal mask from the parent process.
  bool inherit_parent_signal_mask_;
  // Flag indicating to close unused file descriptors inherited from the
  // parent process when starting the child process, which avoids leaking
  // unnecessary file descriptors to the child process.
  bool close_unused_file_descriptors_;
};

}  // namespace brillo

#endif  // LIBBRILLO_BRILLO_PROCESS_H_
