// Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "brillo/syslog_logging.h"

#include <syslog.h>

#include <string>

// syslog.h and base/logging.h both try to #define LOG_INFO and LOG_WARNING.
// We need to #undef at least these two before including base/logging.h.  The
// others are included to be consistent.
namespace {
const int kSyslogDebug    = LOG_DEBUG;
const int kSyslogInfo     = LOG_INFO;
const int kSyslogWarning  = LOG_WARNING;
const int kSyslogError    = LOG_ERR;
const int kSyslogCritical = LOG_CRIT;

#undef LOG_INFO
#undef LOG_WARNING
#undef LOG_ERR
#undef LOG_CRIT
}  // namespace

#include <base/logging.h>

static std::string s_ident;
static std::string s_accumulated;
static bool s_accumulate;
static bool s_log_to_syslog;
static bool s_log_to_stderr;
static bool s_log_header;

static bool HandleMessage(int severity,
                          const char* /* file */,
                          int /* line */,
                          size_t message_start,
                          const std::string& message) {
  switch (severity) {
    case logging::LOG_INFO:
      severity = kSyslogInfo;
      break;

    case logging::LOG_WARNING:
      severity = kSyslogWarning;
      break;

    case logging::LOG_ERROR:
      severity = kSyslogError;
      break;

    case logging::LOG_FATAL:
      severity = kSyslogCritical;
      break;

    default:
      severity = kSyslogDebug;
      break;
  }

  const char* str;
  if (s_log_header) {
    str = message.c_str();
  } else {
    str = message.c_str() + message_start;
  }

  if (s_log_to_syslog)
    syslog(severity, "%s", str);
  if (s_accumulate)
    s_accumulated.append(str);
  return !s_log_to_stderr && severity != kSyslogCritical;
}

namespace brillo {
void SetLogFlags(int log_flags) {
  s_log_to_syslog = (log_flags & kLogToSyslog) != 0;
  s_log_to_stderr = (log_flags & kLogToStderr) != 0;
  s_log_header = (log_flags & kLogHeader) != 0;
}
int GetLogFlags() {
  int flags = 0;
  flags |= (s_log_to_syslog) ? kLogToSyslog : 0;
  flags |= (s_log_to_stderr) ? kLogToStderr : 0;
  flags |= (s_log_header) ? kLogHeader : 0;
  return flags;
}
void InitLog(int init_flags) {
  logging::LoggingSettings settings;
  settings.logging_dest = logging::LOG_TO_SYSTEM_DEBUG_LOG;
  logging::InitLogging(settings);

  const bool kOptionPID = false;
  const bool kOptionTID = false;
  const bool kOptionTimestamp = false;
  const bool kOptionTickcount = false;
  logging::SetLogItems(
      kOptionPID, kOptionTID, kOptionTimestamp, kOptionTickcount);
  logging::SetLogMessageHandler(HandleMessage);
  SetLogFlags(init_flags);
}
void OpenLog(const char* ident, bool log_pid) {
  s_ident = ident;
  openlog(s_ident.c_str(), log_pid ? LOG_PID : 0, LOG_USER);
}
void LogToString(bool enabled) {
  s_accumulate = enabled;
}
std::string GetLog() {
  return s_accumulated;
}
void ClearLog() {
  s_accumulated.clear();
}
bool FindLog(const char* string) {
  return s_accumulated.find(string) != std::string::npos;
}
}  // namespace brillo
