// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBWEAVE_INCLUDE_WEAVE_PROVIDER_TASK_RUNNER_H_
#define LIBWEAVE_INCLUDE_WEAVE_PROVIDER_TASK_RUNNER_H_

#include <string>
#include <utility>
#include <vector>

#include <base/callback.h>
#include <base/location.h>
#include <base/time/time.h>

namespace weave {
namespace provider {

// Interface with methods to post tasks into platform-specific message loop of
// the current thread.
class TaskRunner {
 public:
  // Posts tasks to be executed with the given delay.
  // |from_here| argument is used for debugging and usually just provided by
  // FROM_HERE macro. Implementation may ignore this argument.
  virtual void PostDelayedTask(const tracked_objects::Location& from_here,
                               const base::Closure& task,
                               base::TimeDelta delay) = 0;

 protected:
  virtual ~TaskRunner() {}
};

}  // namespace provider
}  // namespace weave

#endif  // LIBWEAVE_INCLUDE_WEAVE_PROVIDER_TASK_RUNNER_H_
