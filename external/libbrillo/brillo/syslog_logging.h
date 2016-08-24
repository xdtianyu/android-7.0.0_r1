// Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBBRILLO_BRILLO_SYSLOG_LOGGING_H_
#define LIBBRILLO_BRILLO_SYSLOG_LOGGING_H_

#include <string>

#include <brillo/brillo_export.h>

namespace brillo {

enum InitFlags {
  kLogToSyslog = 1,
  kLogToStderr = 2,
  kLogHeader = 4,
};

// Initialize logging subsystem.  |init_flags| is a bitfield, with bits defined
// in InitFlags above.
BRILLO_EXPORT void InitLog(int init_flags);
// Gets the current logging flags.
BRILLO_EXPORT int GetLogFlags();
// Sets the current logging flags.
BRILLO_EXPORT void SetLogFlags(int log_flags);
// Convenience function for configuring syslog via openlog.  Users
// could call openlog directly except for naming collisions between
// base/logging.h and syslog.h.  Similarly users cannot pass the
// normal parameters so we pick a representative set.  |log_pid|
// causes pid to be logged with |ident|.
BRILLO_EXPORT void OpenLog(const char* ident, bool log_pid);
// Start accumulating the logs to a string.  This is inefficient, so
// do not set to true if large numbers of log messages are coming.
// Accumulated logs are only ever cleared when the clear function ings
// called.
BRILLO_EXPORT void LogToString(bool enabled);
// Get the accumulated logs as a string.
BRILLO_EXPORT std::string GetLog();
// Clear the accumulated logs.
BRILLO_EXPORT void ClearLog();
// Returns true if the accumulated log contains the given string.  Useful
// for testing.
BRILLO_EXPORT bool FindLog(const char* string);

}  // namespace brillo

#endif  // LIBBRILLO_BRILLO_SYSLOG_LOGGING_H_
