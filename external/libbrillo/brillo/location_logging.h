// Copyright 2015 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBBRILLO_BRILLO_LOCATION_LOGGING_H_
#define LIBBRILLO_BRILLO_LOCATION_LOGGING_H_

// These macros help to log Location objects in verbose mode.

#include <base/logging.h>

#define VLOG_LOC_STREAM(from_here, verbose_level)                       \
  logging::LogMessage(from_here.file_name(), from_here.line_number(),   \
                      -verbose_level).stream()

#define VLOG_LOC(from_here, verbose_level)                              \
  LAZY_STREAM(VLOG_LOC_STREAM(from_here, verbose_level),                \
              VLOG_IS_ON(verbose_level))

#define DVLOG_LOC(from_here, verbose_level)                             \
  LAZY_STREAM(VLOG_LOC_STREAM(from_here, verbose_level),                \
              ::logging::DEBUG_MODE && VLOG_IS_ON(verbose_level))

#endif  // LIBBRILLO_BRILLO_LOCATION_LOGGING_H_
