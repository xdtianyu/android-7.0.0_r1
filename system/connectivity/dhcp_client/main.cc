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


#include <base/command_line.h>
#include <base/logging.h>
#include <brillo/syslog_logging.h>
#include "dhcp_client/daemon.h"

using std::vector;

namespace {

namespace switches {

// Don't daemon()ize; run in foreground.
const char kForeground[] = "foreground";
// Flag to show the help message.
const char kHelp[] = "help";
// The help message shown if help flag is passed to the program.
const char kHelpMessage[] = "\n help message \n";

}  // namespace switches

}  // namespace

// Always logs to the syslog and logs to stderr if
// we are running in the foreground.
void SetupLogging(bool foreground,
                  const char* daemon_name) {
  int log_flags = 0;
  log_flags |= brillo::kLogToSyslog;
  log_flags |= brillo::kLogHeader;
  if (foreground) {
    log_flags |= brillo::kLogToStderr;
  }
  brillo::InitLog(log_flags);
}

void OnStartup(const char* daemon_name, base::CommandLine* cl) {
  LOG(INFO) << __func__;
  SetupLogging(cl->HasSwitch(switches::kForeground), daemon_name);
  return;
}

int main(int argc, char* argv[]) {
  base::CommandLine::Init(argc, argv);
  base::CommandLine* cl = base::CommandLine::ForCurrentProcess();

  if (cl->HasSwitch(switches::kHelp)) {
    LOG(INFO) << switches::kHelpMessage;
    return 0;
  }

  dhcp_client::Daemon daemon(base::Bind(&OnStartup, argv[0], cl));

  daemon.Run();

  return 0;
}
