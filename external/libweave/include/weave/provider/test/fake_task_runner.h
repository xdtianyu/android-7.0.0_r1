// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBWEAVE_INCLUDE_WEAVE_PROVIDER_TEST_FAKE_TASK_RUNNER_H_
#define LIBWEAVE_INCLUDE_WEAVE_PROVIDER_TEST_FAKE_TASK_RUNNER_H_

#include <weave/provider/task_runner.h>

#include <algorithm>
#include <queue>
#include <utility>
#include <vector>

#include <base/time/clock.h>

namespace weave {
namespace provider {
namespace test {

class FakeTaskRunner : public TaskRunner {
 public:
  FakeTaskRunner();
  ~FakeTaskRunner() override;

  void PostDelayedTask(const tracked_objects::Location& from_here,
                       const base::Closure& task,
                       base::TimeDelta delay) override;

  bool RunOnce();
  void Run(size_t number_of_iterations = 1000);
  void Break();
  base::Clock* GetClock();
  size_t GetTaskQueueSize() const;

 private:
  void SaveTask(const tracked_objects::Location& from_here,
                const base::Closure& task,
                base::TimeDelta delay);

  using QueueItem = std::pair<std::pair<base::Time, size_t>, base::Closure>;

  struct Greater {
    bool operator()(const QueueItem& a, const QueueItem& b) const {
      return a.first > b.first;
    }
  };

  bool break_{false};
  size_t counter_{0};  // Keeps order of tasks with the same time.

  class TestClock;
  std::unique_ptr<TestClock> test_clock_;

  std::priority_queue<QueueItem,
                      std::vector<QueueItem>,
                      FakeTaskRunner::Greater>
      queue_;
};

}  // namespace test
}  // namespace provider
}  // namespace weave

#endif  // LIBWEAVE_INCLUDE_WEAVE_PROVIDER_TEST_FAKE_TASK_RUNNER_H_
