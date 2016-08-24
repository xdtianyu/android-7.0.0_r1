// Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "brillo/process.h"

#include <fcntl.h>
#include <signal.h>
#include <stdint.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <map>

#include <base/files/file_util.h>
#include <base/logging.h>
#include <base/posix/eintr_wrapper.h>
#include <base/process/process_metrics.h>
#include <base/strings/string_number_conversions.h>
#include <base/strings/string_util.h>
#include <base/time/time.h>

#ifndef __linux__
#define setresuid(_u1, _u2, _u3) setreuid(_u1, _u2)
#define setresgid(_g1, _g2, _g3) setregid(_g1, _g2)
#endif  // !__linux__

namespace brillo {

bool ReturnTrue() {
  return true;
}

Process::Process() {
}

Process::~Process() {
}

bool Process::ProcessExists(pid_t pid) {
  return base::DirectoryExists(
      base::FilePath(base::StringPrintf("/proc/%d", pid)));
}

ProcessImpl::ProcessImpl()
    : pid_(0),
      uid_(-1),
      gid_(-1),
      pre_exec_(base::Bind(&ReturnTrue)),
      search_path_(false),
      inherit_parent_signal_mask_(false),
      close_unused_file_descriptors_(false) {
}

ProcessImpl::~ProcessImpl() {
  Reset(0);
}

void ProcessImpl::AddArg(const std::string& arg) {
  arguments_.push_back(arg);
}

void ProcessImpl::RedirectOutput(const std::string& output_file) {
  output_file_ = output_file;
}

void ProcessImpl::RedirectUsingPipe(int child_fd, bool is_input) {
  PipeInfo info;
  info.is_input_ = is_input;
  info.is_bound_ = false;
  pipe_map_[child_fd] = info;
}

void ProcessImpl::BindFd(int parent_fd, int child_fd) {
  PipeInfo info;
  info.is_bound_ = true;

  // info.child_fd_ is the 'child half' of the pipe, which gets dup2()ed into
  // place over child_fd. Since we already have the child we want to dup2() into
  // place, we can set info.child_fd_ to parent_fd and leave info.parent_fd_
  // invalid.
  info.child_fd_ = parent_fd;
  info.parent_fd_ = -1;
  pipe_map_[child_fd] = info;
}

void ProcessImpl::SetCloseUnusedFileDescriptors(bool close_unused_fds) {
  close_unused_file_descriptors_ = close_unused_fds;
}

void ProcessImpl::SetUid(uid_t uid) {
  uid_ = uid;
}

void ProcessImpl::SetGid(gid_t gid) {
  gid_ = gid;
}

void ProcessImpl::SetCapabilities(uint64_t /*capmask*/) {
  // No-op, since ProcessImpl does not support sandboxing.
  return;
}

void ProcessImpl::ApplySyscallFilter(const std::string& /*path*/) {
  // No-op, since ProcessImpl does not support sandboxing.
  return;
}

void ProcessImpl::EnterNewPidNamespace() {
  // No-op, since ProcessImpl does not support sandboxing.
  return;
}

void ProcessImpl::SetInheritParentSignalMask(bool inherit) {
  inherit_parent_signal_mask_ = inherit;
}

void ProcessImpl::SetPreExecCallback(const PreExecCallback& cb) {
  pre_exec_ = cb;
}

void ProcessImpl::SetSearchPath(bool search_path) {
  search_path_ = search_path;
}

int ProcessImpl::GetPipe(int child_fd) {
  PipeMap::iterator i = pipe_map_.find(child_fd);
  if (i == pipe_map_.end())
    return -1;
  else
    return i->second.parent_fd_;
}

bool ProcessImpl::PopulatePipeMap() {
  // Verify all target fds are already open.  With this assumption we
  // can be sure that the pipe fds created below do not overlap with
  // any of the target fds which simplifies how we dup2 to them.  Note
  // that multi-threaded code could close i->first between this loop
  // and the next.
  for (PipeMap::iterator i = pipe_map_.begin(); i != pipe_map_.end(); ++i) {
    struct stat stat_buffer;
    if (fstat(i->first, &stat_buffer) < 0) {
      int saved_errno = errno;
      LOG(ERROR) << "Unable to fstat fd " << i->first << ": " << saved_errno;
      return false;
    }
  }

  for (PipeMap::iterator i = pipe_map_.begin(); i != pipe_map_.end(); ++i) {
    if (i->second.is_bound_) {
      // already have a parent fd, and the child fd gets dup()ed later.
      continue;
    }
    int pipefds[2];
    if (pipe(pipefds) < 0) {
      int saved_errno = errno;
      LOG(ERROR) << "pipe call failed with: " << saved_errno;
      return false;
    }
    if (i->second.is_input_) {
      // pipe is an input from the prospective of the child.
      i->second.parent_fd_ = pipefds[1];
      i->second.child_fd_ = pipefds[0];
    } else {
      i->second.parent_fd_ = pipefds[0];
      i->second.child_fd_ = pipefds[1];
    }
  }
  return true;
}

bool ProcessImpl::IsFileDescriptorInPipeMap(int fd) const {
  for (const auto& pipe : pipe_map_) {
    if (fd == pipe.second.parent_fd_ ||
        fd == pipe.second.child_fd_ ||
        fd == pipe.first) {
      return true;
    }
  }
  return false;
}

void ProcessImpl::CloseUnusedFileDescriptors() {
  size_t max_fds = base::GetMaxFds();
  for (size_t i = 0; i < max_fds; i++) {
    const int fd = static_cast<int>(i);

    // Ignore STD file descriptors.
    if (fd == STDIN_FILENO || fd == STDOUT_FILENO || fd == STDERR_FILENO) {
      continue;
    }

    // Ignore file descriptors used by the PipeMap, they will be handled
    // by this process later on.
    if (IsFileDescriptorInPipeMap(fd)) {
      continue;
    }

    // Since we're just trying to close anything we can find,
    // ignore any error return values of close().
    IGNORE_EINTR(close(fd));
 }
}

bool ProcessImpl::Start() {
  // If no arguments are provided, fail.
  if (arguments_.empty()) {
    return false;
  }
  scoped_ptr<char*[]> argv(new char*[arguments_.size() + 1]);

  for (size_t i = 0; i < arguments_.size(); ++i)
    argv[i] = const_cast<char*>(arguments_[i].c_str());

  argv[arguments_.size()] = nullptr;

  if (!PopulatePipeMap()) {
    LOG(ERROR) << "Failing to start because pipe creation failed";
    return false;
  }

  pid_t pid = fork();
  int saved_errno = errno;
  if (pid < 0) {
    LOG(ERROR) << "Fork failed: " << saved_errno;
    Reset(0);
    return false;
  }

  if (pid == 0) {
    // Executing inside the child process.
    // Close unused file descriptors.
    if (close_unused_file_descriptors_) {
      CloseUnusedFileDescriptors();
    }
    // Close parent's side of the child pipes. dup2 ours into place and
    // then close our ends.
    for (PipeMap::iterator i = pipe_map_.begin(); i != pipe_map_.end(); ++i) {
      if (i->second.parent_fd_ != -1)
        IGNORE_EINTR(close(i->second.parent_fd_));
      HANDLE_EINTR(dup2(i->second.child_fd_, i->first));
    }
    // Defer the actual close() of the child fd until afterward; this lets the
    // same child fd be bound to multiple fds using BindFd
    for (PipeMap::iterator i = pipe_map_.begin(); i != pipe_map_.end(); ++i) {
      IGNORE_EINTR(close(i->second.child_fd_));
    }
    if (!output_file_.empty()) {
      int output_handle = HANDLE_EINTR(open(
          output_file_.c_str(), O_CREAT | O_WRONLY | O_TRUNC | O_NOFOLLOW,
          0666));
      if (output_handle < 0) {
        PLOG(ERROR) << "Could not create " << output_file_;
        // Avoid exit() to avoid atexit handlers from parent.
        _exit(kErrorExitStatus);
      }
      HANDLE_EINTR(dup2(output_handle, STDOUT_FILENO));
      HANDLE_EINTR(dup2(output_handle, STDERR_FILENO));
      // Only close output_handle if it does not happen to be one of
      // the two standard file descriptors we are trying to redirect.
      if (output_handle != STDOUT_FILENO && output_handle != STDERR_FILENO) {
        IGNORE_EINTR(close(output_handle));
      }
    }
    if (gid_ != static_cast<gid_t>(-1) && setresgid(gid_, gid_, gid_) < 0) {
      int saved_errno = errno;
      LOG(ERROR) << "Unable to set GID to " << gid_ << ": " << saved_errno;
      _exit(kErrorExitStatus);
    }
    if (uid_ != static_cast<uid_t>(-1) && setresuid(uid_, uid_, uid_) < 0) {
      int saved_errno = errno;
      LOG(ERROR) << "Unable to set UID to " << uid_ << ": " << saved_errno;
      _exit(kErrorExitStatus);
    }
    if (!pre_exec_.Run()) {
      LOG(ERROR) << "Pre-exec callback failed";
      _exit(kErrorExitStatus);
    }
    // Reset signal mask for the child process if not inheriting signal mask
    // from the parent process.
    if (!inherit_parent_signal_mask_) {
      sigset_t signal_mask;
      CHECK_EQ(0, sigemptyset(&signal_mask));
      CHECK_EQ(0, sigprocmask(SIG_SETMASK, &signal_mask, nullptr));
    }
    if (search_path_) {
      execvp(argv[0], &argv[0]);
    } else {
      execv(argv[0], &argv[0]);
    }
    PLOG(ERROR) << "Exec of " << argv[0] << " failed:";
    _exit(kErrorExitStatus);
  } else {
    // Still executing inside the parent process with known child pid.
    arguments_.clear();
    UpdatePid(pid);
    // Close our copy of child side pipes.
    for (PipeMap::iterator i = pipe_map_.begin(); i != pipe_map_.end(); ++i) {
      IGNORE_EINTR(close(i->second.child_fd_));
    }
  }
  return true;
}

int ProcessImpl::Wait() {
  int status = 0;
  if (pid_ == 0) {
    LOG(ERROR) << "Process not running";
    return -1;
  }
  if (HANDLE_EINTR(waitpid(pid_, &status, 0)) < 0) {
    int saved_errno = errno;
    LOG(ERROR) << "Problem waiting for pid " << pid_ << ": " << saved_errno;
    return -1;
  }
  pid_t old_pid = pid_;
  // Update the pid to 0 - do not Reset as we do not want to try to
  // kill the process that has just exited.
  UpdatePid(0);
  if (!WIFEXITED(status)) {
    DCHECK(WIFSIGNALED(status)) << old_pid
                                << " neither exited, nor died on a signal?";
    LOG(ERROR) << "Process " << old_pid
               << " did not exit normally: " << WTERMSIG(status);
    return -1;
  }
  return WEXITSTATUS(status);
}

int ProcessImpl::Run() {
  if (!Start()) {
    return -1;
  }
  return Wait();
}

pid_t ProcessImpl::pid() {
  return pid_;
}

bool ProcessImpl::Kill(int signal, int timeout) {
  if (pid_ == 0) {
    // Passing pid == 0 to kill is committing suicide.  Check specifically.
    LOG(ERROR) << "Process not running";
    return false;
  }
  if (kill(pid_, signal) < 0) {
    int saved_errno = errno;
    LOG(ERROR) << "Unable to send signal to " << pid_ << " error "
               << saved_errno;
    return false;
  }
  base::TimeTicks start_signal = base::TimeTicks::Now();
  do {
    int status = 0;
    pid_t w = waitpid(pid_, &status, WNOHANG);
    int saved_errno = errno;
    if (w < 0) {
      if (saved_errno == ECHILD)
        return true;
      LOG(ERROR) << "Waitpid returned " << w << ", errno " << saved_errno;
      return false;
    }
    if (w > 0) {
      Reset(0);
      return true;
    }
    usleep(100);
  } while ((base::TimeTicks::Now() - start_signal).InSecondsF() <= timeout);
  LOG(INFO) << "process " << pid_ << " did not exit from signal " << signal
            << " in " << timeout << " seconds";
  return false;
}

void ProcessImpl::UpdatePid(pid_t new_pid) {
  pid_ = new_pid;
}

void ProcessImpl::Reset(pid_t new_pid) {
  arguments_.clear();
  // Close our side of all pipes to this child giving the child to
  // handle sigpipes and shutdown nicely, though likely it won't
  // have time.
  for (PipeMap::iterator i = pipe_map_.begin(); i != pipe_map_.end(); ++i)
    IGNORE_EINTR(close(i->second.parent_fd_));
  pipe_map_.clear();
  if (pid_)
    Kill(SIGKILL, 0);
  UpdatePid(new_pid);
}

bool ProcessImpl::ResetPidByFile(const std::string& pid_file) {
  std::string contents;
  if (!base::ReadFileToString(base::FilePath(pid_file), &contents)) {
    LOG(ERROR) << "Could not read pid file" << pid_file;
    return false;
  }
  base::TrimWhitespaceASCII(contents, base::TRIM_TRAILING, &contents);
  int64_t pid_int64 = 0;
  if (!base::StringToInt64(contents, &pid_int64)) {
    LOG(ERROR) << "Unexpected pid file contents";
    return false;
  }
  Reset(pid_int64);
  return true;
}

pid_t ProcessImpl::Release() {
  pid_t old_pid = pid_;
  pid_ = 0;
  return old_pid;
}

}  // namespace brillo
