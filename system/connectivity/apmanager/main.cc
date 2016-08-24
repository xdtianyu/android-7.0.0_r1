//
// Copyright (C) 2014 The Android Open Source Project
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

#include <vector>

#include <base/bind.h>
#include <base/command_line.h>
#include <base/logging.h>
#include <brillo/minijail/minijail.h>
#include <brillo/syslog_logging.h>

#include "apmanager/daemon.h"

using std::vector;

namespace {

namespace switches {

// Don't daemon()ize; run in foreground.
const char kForeground[] = "foreground";
// Flag that causes apmanager to show the help message and exit.
const char kHelp[] = "help";

// The help message shown if help flag is passed to the program.
const char kHelpMessage[] = "\n"
    "Available Switches: \n"
    "  --foreground\n"
    "    Don\'t daemon()ize; run in foreground.\n";
}  // namespace switches

}  // namespace

namespace {

#if !defined(__ANDROID__)
const char kLoggerCommand[] = "/usr/bin/logger";
const char kLoggerUser[] = "syslog";
#endif  // __ANDROID__

const char kSeccompFilePath[] = "/usr/share/policy/apmanager-seccomp.policy";

}  // namespace

// Always logs to the syslog and logs to stderr if
// we are running in the foreground.
void SetupLogging(brillo::Minijail* minijail,
                  bool foreground,
                  const char* daemon_name) {
  int log_flags = 0;
  log_flags |= brillo::kLogToSyslog;
  log_flags |= brillo::kLogHeader;
  if (foreground) {
    log_flags |= brillo::kLogToStderr;
  }
  brillo::InitLog(log_flags);

#if !defined(__ANDROID__)
  // Logger utility doesn't exist on Android, so do not run it on Android.
  // TODO(zqiu): add support to redirect stderr logs from child processes
  // to Android logging facility.
  if (!foreground) {
    vector<char*> logger_command_line;
    int logger_stdin_fd;
    logger_command_line.push_back(const_cast<char*>(kLoggerCommand));
    logger_command_line.push_back(const_cast<char*>("--priority"));
    logger_command_line.push_back(const_cast<char*>("daemon.err"));
    logger_command_line.push_back(const_cast<char*>("--tag"));
    logger_command_line.push_back(const_cast<char*>(daemon_name));
    logger_command_line.push_back(nullptr);

    struct minijail* jail = minijail->New();
    minijail->DropRoot(jail, kLoggerUser, kLoggerUser);

    if (!minijail->RunPipeAndDestroy(jail, logger_command_line,
                                     nullptr, &logger_stdin_fd)) {
      LOG(ERROR) << "Unable to spawn logger. "
                 << "Writes to stderr will be discarded.";
      return;
    }

    // Note that we don't set O_CLOEXEC here. This means that stderr
    // from any child processes will, by default, be logged to syslog.
    if (dup2(logger_stdin_fd, fileno(stderr)) != fileno(stderr)) {
      LOG(ERROR) << "Failed to redirect stderr to syslog: "
                 << strerror(errno);
    }
    close(logger_stdin_fd);
  }
#endif  // __ANDROID__
}

void DropPrivileges(brillo::Minijail* minijail) {
  struct minijail* jail = minijail->New();
  minijail->DropRoot(jail, apmanager::Daemon::kAPManagerUserName,
                     apmanager::Daemon::kAPManagerGroupName);
  // Permissions needed for the daemon and its child processes for managing
  // network interfaces and binding to network sockets.
  minijail->UseCapabilities(jail, CAP_TO_MASK(CAP_NET_ADMIN) |
                                  CAP_TO_MASK(CAP_NET_RAW) |
                                  CAP_TO_MASK(CAP_NET_BIND_SERVICE));
  minijail->UseSeccompFilter(jail, kSeccompFilePath);
  minijail_enter(jail);
  minijail->Destroy(jail);
}

void OnStartup(const char* daemon_name, base::CommandLine* cl) {
  brillo::Minijail* minijail = brillo::Minijail::GetInstance();
  SetupLogging(minijail, cl->HasSwitch(switches::kForeground), daemon_name);

  LOG(INFO) << __func__ << ": Dropping privileges";

  // TODO(zqiu): apmanager is currently started as the "system" user on Android,
  // so there is no need to drop privileges to the "system" user again.
  // Drop user privileges when we're running apmanager under a different
  // user/group.
#if !defined(__ANDROID__)
  // Now that the daemon has all the resources it needs to run, we can drop
  // privileges further.
  DropPrivileges(minijail);
#endif  // __ANDROID
}

int main(int argc, char* argv[]) {
  base::CommandLine::Init(argc, argv);
  base::CommandLine* cl = base::CommandLine::ForCurrentProcess();

  if (cl->HasSwitch(switches::kHelp)) {
    LOG(INFO) << switches::kHelpMessage;
    return 0;
  }

  apmanager::Daemon daemon(base::Bind(&OnStartup, argv[0], cl));

  daemon.Run();

  return 0;
}
