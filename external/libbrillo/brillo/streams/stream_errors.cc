// Copyright 2015 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <brillo/streams/stream_errors.h>

namespace brillo {
namespace errors {
namespace stream {

const char kDomain[] = "stream.io";

const char kStreamClosed[] = "stream_closed";
const char kOperationNotSupported[] = "operation_not_supported";
const char kPartialData[] = "partial_data";
const char kInvalidParameter[] = "invalid_parameter";
const char kTimeout[] = "time_out";

}  // namespace stream
}  // namespace errors
}  // namespace brillo
