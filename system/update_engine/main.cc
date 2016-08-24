//
// Copyright (C) 2012 The Android Open Source Project
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

#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <xz.h>

#include <string>

#include <base/at_exit.h>
#include <base/command_line.h>
#include <base/files/file_util.h>
#include <base/logging.h>
#include <base/strings/string_util.h>
#include <base/strings/stringprintf.h>
#include <brillo/flag_helper.h>

#include "update_engine/common/terminator.h"
#include "update_engine/common/utils.h"
#include "update_engine/daemon.h"

using std::string;

namespace chromeos_update_engine {
namespace {

void SetupLogSymlink(const string& symlink_path, const string& log_path) {
  // TODO(petkov): To ensure a smooth transition between non-timestamped and
  // timestamped logs, move an existing log to start the first timestamped
  // one. This code can go away once all clients are switched to this version or
  // we stop caring about the old-style logs.
  if (utils::FileExists(symlink_path.c_str()) &&
      !utils::IsSymlink(symlink_path.c_str())) {
    base::ReplaceFile(base::FilePath(symlink_path),
                      base::FilePath(log_path),
                      nullptr);
  }
  base::DeleteFile(base::FilePath(symlink_path), true);
  if (symlink(log_path.c_str(), symlink_path.c_str()) == -1) {
    PLOG(ERROR) << "Unable to create symlink " << symlink_path
                << " pointing at " << log_path;
  }
}

string GetTimeAsString(time_t utime) {
  struct tm tm;
  CHECK_EQ(localtime_r(&utime, &tm), &tm);
  char str[16];
  CHECK_EQ(strftime(str, sizeof(str), "%Y%m%d-%H%M%S", &tm), 15u);
  return str;
}

string SetupLogFile(const string& kLogsRoot) {
  const string kLogSymlink = kLogsRoot + "/update_engine.log";
  const string kLogsDir = kLogsRoot + "/update_engine";
  const string kLogPath =
      base::StringPrintf("%s/update_engine.%s",
                         kLogsDir.c_str(),
                         GetTimeAsString(::time(nullptr)).c_str());
  mkdir(kLogsDir.c_str(), 0755);
  SetupLogSymlink(kLogSymlink, kLogPath);
  return kLogSymlink;
}

void SetupLogging(bool log_to_std_err) {
  string log_file;
  logging::LoggingSettings log_settings;
  log_settings.lock_log = logging::DONT_LOCK_LOG_FILE;
  log_settings.delete_old = logging::APPEND_TO_OLD_LOG_FILE;

  if (log_to_std_err) {
    // Log to stderr initially.
    log_settings.log_file = nullptr;
    log_settings.logging_dest = logging::LOG_TO_SYSTEM_DEBUG_LOG;
  } else {
    log_file = SetupLogFile("/var/log");
    log_settings.log_file = log_file.c_str();
    log_settings.logging_dest = logging::LOG_TO_FILE;
  }

  logging::InitLogging(log_settings);
}

}  // namespace
}  // namespace chromeos_update_engine

int main(int argc, char** argv) {
  DEFINE_bool(logtostderr, false,
              "Write logs to stderr instead of to a file in log_dir.");
  DEFINE_bool(foreground, false,
              "Don't daemon()ize; run in foreground.");

  chromeos_update_engine::Terminator::Init();
  brillo::FlagHelper::Init(argc, argv, "Chromium OS Update Engine");
  chromeos_update_engine::SetupLogging(FLAGS_logtostderr);
  if (!FLAGS_foreground)
    PLOG_IF(FATAL, daemon(0, 0) == 1) << "daemon() failed";

  LOG(INFO) << "Chrome OS Update Engine starting";

  // xz-embedded requires to initialize its CRC-32 table once on startup.
  xz_crc32_init();

  // Ensure that all written files have safe permissions.
  // This is a mask, so we _block_ all permissions for the group owner and other
  // users but allow all permissions for the user owner. We allow execution
  // for the owner so we can create directories.
  // Done _after_ log file creation.
  umask(S_IRWXG | S_IRWXO);

  chromeos_update_engine::UpdateEngineDaemon update_engine_daemon;
  int exit_code = update_engine_daemon.Run();

  LOG(INFO) << "Chrome OS Update Engine terminating with exit code "
            << exit_code;
  return exit_code;
}
