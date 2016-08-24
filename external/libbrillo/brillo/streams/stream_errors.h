// Copyright 2015 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBBRILLO_BRILLO_STREAMS_STREAM_ERRORS_H_
#define LIBBRILLO_BRILLO_STREAMS_STREAM_ERRORS_H_

#include <brillo/brillo_export.h>

namespace brillo {
namespace errors {
namespace stream {

// Error domain for generic stream-based errors.
BRILLO_EXPORT extern const char kDomain[];

BRILLO_EXPORT extern const char kStreamClosed[];
BRILLO_EXPORT extern const char kOperationNotSupported[];
BRILLO_EXPORT extern const char kPartialData[];
BRILLO_EXPORT extern const char kInvalidParameter[];
BRILLO_EXPORT extern const char kTimeout[];

}  // namespace stream
}  // namespace errors
}  // namespace brillo

#endif  // LIBBRILLO_BRILLO_STREAMS_STREAM_ERRORS_H_
