// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/logging.h"

#include <sys/syscall.h>
#include <time.h>
#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include <algorithm>
#include <cstring>
#include <ctime>
#include <iomanip>
#include <ostream>
#include <string>

#include "base/posix/eintr_wrapper.h"
#include "base/strings/string_piece.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversion_utils.h"

namespace logging {

namespace {

const char* const log_severity_names[LOG_NUM_SEVERITIES] = {
  "INFO", "WARNING", "ERROR", "FATAL" };

const char* log_severity_name(int severity) {
  if (severity >= 0 && severity < LOG_NUM_SEVERITIES)
    return log_severity_names[severity];
  return "UNKNOWN";
}

int g_min_log_level = 0;

LoggingDestination g_logging_destination = LOG_DEFAULT;

// For LOG_ERROR and above, always print to stderr.
const int kAlwaysPrintErrorLevel = LOG_ERROR;

// What should be prepended to each message?
bool g_log_timestamp = true;

// Should we pop up fatal debug messages in a dialog?
bool show_error_dialogs = false;

// An assert handler override specified by the client to be called instead of
// the debug message dialog and process termination.
LogAssertHandlerFunction log_assert_handler = nullptr;
// A log message handler that gets notified of every log message we process.
LogMessageHandlerFunction log_message_handler = nullptr;

// Helper functions to wrap platform differences.

}  // namespace

LoggingSettings::LoggingSettings()
    : logging_dest(LOG_DEFAULT) {}

bool BaseInitLoggingImpl(const LoggingSettings& settings) {
  g_logging_destination = settings.logging_dest;

  return true;
}

void SetMinLogLevel(int level) {
  g_min_log_level = std::min(LOG_FATAL, level);
}

int GetMinLogLevel() {
  return g_min_log_level;
}

bool ShouldCreateLogMessage(int severity) {
  if (severity < g_min_log_level)
    return false;

  // Return true here unless we know ~LogMessage won't do anything. Note that
  // ~LogMessage writes to stderr if severity_ >= kAlwaysPrintErrorLevel, even
  // when g_logging_destination is LOG_NONE.
  return g_logging_destination != LOG_NONE || log_message_handler ||
         severity >= kAlwaysPrintErrorLevel;
}

int GetVlogVerbosity() {
  return std::max(-1, LOG_INFO - GetMinLogLevel());
}

void SetLogItems(bool enable_process_id, bool enable_thread_id,
                 bool enable_timestamp, bool enable_tickcount) {
  g_log_timestamp = enable_timestamp;
}

void SetShowErrorDialogs(bool enable_dialogs) {
  show_error_dialogs = enable_dialogs;
}

void SetLogAssertHandler(LogAssertHandlerFunction handler) {
  log_assert_handler = handler;
}

void SetLogMessageHandler(LogMessageHandlerFunction handler) {
  log_message_handler = handler;
}

LogMessageHandlerFunction GetLogMessageHandler() {
  return log_message_handler;
}

// Explicit instantiations for commonly used comparisons.
template std::string* MakeCheckOpString<int, int>(
    const int&, const int&, const char* names);
template std::string* MakeCheckOpString<unsigned long, unsigned long>(
    const unsigned long&, const unsigned long&, const char* names);
template std::string* MakeCheckOpString<unsigned long, unsigned int>(
    const unsigned long&, const unsigned int&, const char* names);
template std::string* MakeCheckOpString<unsigned int, unsigned long>(
    const unsigned int&, const unsigned long&, const char* names);
template std::string* MakeCheckOpString<std::string, std::string>(
    const std::string&, const std::string&, const char* name);

LogMessage::LogMessage(const char* file, int line, LogSeverity severity)
    : severity_(severity), file_(file), line_(line) {
  Init(file, line);
}

LogMessage::LogMessage(const char* file, int line, const char* condition)
    : severity_(LOG_FATAL), file_(file), line_(line) {
  Init(file, line);
  stream_ << "Check failed: " << condition << ". ";
}

LogMessage::LogMessage(const char* file, int line, std::string* result)
    : severity_(LOG_FATAL), file_(file), line_(line) {
  Init(file, line);
  stream_ << "Check failed: " << *result;
  delete result;
}

LogMessage::LogMessage(const char* file, int line, LogSeverity severity,
                       std::string* result)
    : severity_(severity), file_(file), line_(line) {
  Init(file, line);
  stream_ << "Check failed: " << *result;
  delete result;
}

LogMessage::~LogMessage() {
  stream_ << std::endl;
  std::string str_newline(stream_.str());

  // Give any log message handler first dibs on the message.
  if (log_message_handler &&
      log_message_handler(severity_, file_, line_,
                          message_start_, str_newline)) {
    // The handler took care of it, no further processing.
    return;
  }

  if ((g_logging_destination & LOG_TO_SYSTEM_DEBUG_LOG) != 0) {
    ignore_result(fwrite(str_newline.data(), str_newline.size(), 1, stderr));
    fflush(stderr);
  } else if (severity_ >= kAlwaysPrintErrorLevel) {
    // When we're only outputting to a log file, above a certain log level, we
    // should still output to stderr so that we can better detect and diagnose
    // problems with unit tests, especially on the buildbots.
    ignore_result(fwrite(str_newline.data(), str_newline.size(), 1, stderr));
    fflush(stderr);
  }

  if (severity_ == LOG_FATAL) {
    // Ensure the first characters of the string are on the stack so they
    // are contained in minidumps for diagnostic purposes.
    char str_stack[1024];
    str_newline.copy(str_stack, arraysize(str_stack));

    if (log_assert_handler) {
      // Make a copy of the string for the handler out of paranoia.
      log_assert_handler(std::string(stream_.str()));
    } else {
      // Crash the process to generate a dump.
      abort();
    }
  }
}

// writes the common header info to the stream
void LogMessage::Init(const char* file, int line) {
  base::StringPiece filename(file);
  size_t last_slash_pos = filename.find_last_of("\\/");
  if (last_slash_pos != base::StringPiece::npos)
    filename.remove_prefix(last_slash_pos + 1);

  // TODO(darin): It might be nice if the columns were fixed width.

  stream_ <<  '[';
  if (g_log_timestamp) {
    time_t t = time(nullptr);
    struct tm local_time;
    memset(&local_time, 0, sizeof(local_time));
#ifdef _MSC_VER
    localtime_s(&local_time, &t);
#else
    localtime_r(&t, &local_time);
#endif
    struct tm* tm_time = &local_time;
    stream_ << std::setfill('0')
            << std::setw(2) << 1 + tm_time->tm_mon
            << std::setw(2) << tm_time->tm_mday
            << '/'
            << std::setw(2) << tm_time->tm_hour
            << std::setw(2) << tm_time->tm_min
            << std::setw(2) << tm_time->tm_sec
            << ':';
  }
  if (severity_ >= 0)
    stream_ << log_severity_name(severity_);
  else
    stream_ << "VERBOSE" << -severity_;

  stream_ << ":" << filename << "(" << line << ")] ";

  message_start_ = stream_.str().length();
}

void RawLog(int level, const char* message) {
  if (level >= g_min_log_level) {
    size_t bytes_written = 0;
    const size_t message_len = strlen(message);
    int rv;
    while (bytes_written < message_len) {
      rv = HANDLE_EINTR(
          write(STDERR_FILENO, message + bytes_written,
                message_len - bytes_written));
      if (rv < 0) {
        // Give up, nothing we can do now.
        break;
      }
      bytes_written += rv;
    }

    if (message_len > 0 && message[message_len - 1] != '\n') {
      do {
        rv = HANDLE_EINTR(write(STDERR_FILENO, "\n", 1));
        if (rv < 0) {
          // Give up, nothing we can do now.
          break;
        }
      } while (rv != 1);
    }
  }

  if (level == LOG_FATAL)
    abort();
}

// This was defined at the beginning of this file.
#undef write

void LogErrorNotReached(const char* file, int line) {
  LogMessage(file, line, LOG_ERROR).stream()
      << "NOTREACHED() hit.";
}

}  // namespace logging
