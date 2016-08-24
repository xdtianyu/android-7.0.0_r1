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

#include "shill/logging.h"

#include <string>

#include <base/command_line.h>
#include <base/strings/string_number_conversions.h>

using base::CommandLine;
using std::string;

namespace shill {

namespace switches {

const char kLogLevel[] = "log-level";
const char kLogScopes[] = "log-scopes";

}  // namespace switches

void SetLogLevelFromCommandLine(CommandLine* cl) {
  if (cl->HasSwitch(switches::kLogLevel)) {
    string log_level = cl->GetSwitchValueASCII(switches::kLogLevel);
    int level = 0;
    if (base::StringToInt(log_level, &level) &&
        level < logging::LOG_NUM_SEVERITIES) {
      logging::SetMinLogLevel(level);
      // Like VLOG, SLOG uses negative verbose level.
      shill::ScopeLogger::GetInstance()->set_verbose_level(-level);
    } else {
      LOG(WARNING) << "Bad log level: " << log_level;
    }
  }

  if (cl->HasSwitch(switches::kLogScopes)) {
    string log_scopes = cl->GetSwitchValueASCII(switches::kLogScopes);
    shill::ScopeLogger::GetInstance()->EnableScopesByName(log_scopes);
  }
}

}  // namespace shill


