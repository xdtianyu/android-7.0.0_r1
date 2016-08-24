// Copyright 2015 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBBRILLO_BRILLO_MESSAGE_LOOPS_MESSAGE_LOOP_UTILS_H_
#define LIBBRILLO_BRILLO_MESSAGE_LOOPS_MESSAGE_LOOP_UTILS_H_

#include <base/callback.h>
#include <base/time/time.h>

#include <brillo/brillo_export.h>
#include <brillo/message_loops/message_loop.h>

namespace brillo {

// Run the MessageLoop until the condition passed in |terminate| returns true
// or the timeout expires.
BRILLO_EXPORT void MessageLoopRunUntil(
    MessageLoop* loop,
    base::TimeDelta timeout,
    base::Callback<bool()> terminate);

// Run the MessageLoop |loop| for up to |iterations| times without blocking.
// Return the number of tasks run.
BRILLO_EXPORT int MessageLoopRunMaxIterations(MessageLoop* loop,
                                              int iterations);

}  // namespace brillo

#endif  // LIBBRILLO_BRILLO_MESSAGE_LOOPS_MESSAGE_LOOP_UTILS_H_
