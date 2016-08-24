// Copyright 2014 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBBRILLO_BRILLO_ERRORS_ERROR_CODES_H_
#define LIBBRILLO_BRILLO_ERRORS_ERROR_CODES_H_

#include <string>

#include <brillo/brillo_export.h>
#include <brillo/errors/error.h>

namespace brillo {
namespace errors {

namespace dbus {
BRILLO_EXPORT extern const char kDomain[];
}  // namespace dbus

namespace json {
BRILLO_EXPORT extern const char kDomain[];
BRILLO_EXPORT extern const char kParseError[];
BRILLO_EXPORT extern const char kObjectExpected[];
}  // namespace json

namespace http {
BRILLO_EXPORT extern const char kDomain[];
}  // namespace http

namespace system {
BRILLO_EXPORT extern const char kDomain[];

// Adds an Error object to the error chain identified by |error|, using
// the system error code (see "errno").
BRILLO_EXPORT void AddSystemError(ErrorPtr* error,
                                  const tracked_objects::Location& location,
                                  int errnum);
}  // namespace system

}  // namespace errors
}  // namespace brillo

#endif  // LIBBRILLO_BRILLO_ERRORS_ERROR_CODES_H_
