// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <weave/provider/test/fake_task_runner.h>

namespace weave {
namespace provider {
namespace test {

class FakeTaskRunner::TestClock : public base::Clock {
 public:
  base::Time Now() override { return now_; }

  void SetNow(base::Time now) { now_ = now; }

 private:
  base::Time now_{base::Time::Now()};
};

FakeTaskRunner::FakeTaskRunner() : test_clock_{new TestClock} {}

FakeTaskRunner::~FakeTaskRunner() {}

bool FakeTaskRunner::RunOnce() {
  if (queue_.empty())
    return false;
  auto top = queue_.top();
  queue_.pop();
  test_clock_->SetNow(std::max(test_clock_->Now(), top.first.first));
  top.second.Run();
  return true;
}

void FakeTaskRunner::Run(size_t number_of_iterations) {
  break_ = false;
  for (size_t i = 0; i < number_of_iterations && !break_ && RunOnce(); ++i) {
  }
}

void FakeTaskRunner::Break() {
  break_ = true;
}

base::Clock* FakeTaskRunner::GetClock() {
  return test_clock_.get();
}

void FakeTaskRunner::PostDelayedTask(const tracked_objects::Location& from_here,
                                     const base::Closure& task,
                                     base::TimeDelta delay) {
  queue_.emplace(std::make_pair(test_clock_->Now() + delay, ++counter_), task);
}

size_t FakeTaskRunner::GetTaskQueueSize() const {
  return queue_.size();
}

}  // namespace test
}  // namespace provider
}  // namespace weave
