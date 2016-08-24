//
// Copyright (C) 2011 The Android Open Source Project
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

#include <base/at_exit.h>
#include <base/command_line.h>
#include <brillo/syslog_logging.h>
#include <gtest/gtest.h>

#include "shill/logging.h"

namespace switches {

static const char kHelp[] = "help";

static const char kHelpMessage[] = "\n"
    "Additional (non-gtest) switches:\n"
    "  --log-level=N\n"
    "    Logging level:\n"
    "      0 = LOG(INFO), 1 = LOG(WARNING), 2 = LOG(ERROR),\n"
    "      -1 = SLOG(..., 1), -2 = SLOG(..., 2), etc.\n"
    "  --log-scopes=\"*scope1+scope2\".\n"
    "    Scopes to enable for SLOG()-based logging.\n";

}  // namespace switches

int main(int argc, char** argv) {
  base::AtExitManager exit_manager;
  base::CommandLine::Init(argc, argv);
  base::CommandLine* cl = base::CommandLine::ForCurrentProcess();
  brillo::InitLog(brillo::kLogToStderr);
  shill::SetLogLevelFromCommandLine(cl);
  ::testing::InitGoogleTest(&argc, argv);

  if (cl->HasSwitch(switches::kHelp)) {
    std::cerr << switches::kHelpMessage;
  }

  return RUN_ALL_TESTS();
}
