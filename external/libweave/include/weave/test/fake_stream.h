// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBWEAVE_INCLUDE_WEAVE_TEST_FAKE_STREAM_H_
#define LIBWEAVE_INCLUDE_WEAVE_TEST_FAKE_STREAM_H_

#include <weave/stream.h>

#include <string>

#include <base/time/time.h>
#include <gmock/gmock.h>

namespace weave {

namespace provider {
class TaskRunner;
}

namespace test {

class FakeStream : public Stream {
 public:
  explicit FakeStream(provider::TaskRunner* task_runner);
  FakeStream(provider::TaskRunner* task_runner, const std::string& read_data);

  void ExpectWritePacketString(base::TimeDelta, const std::string& data);
  void AddReadPacketString(base::TimeDelta, const std::string& data);

  void CancelPendingOperations() override;
  void Read(void* buffer,
            size_t size_to_read,
            const ReadCallback& callback) override;
  void Write(const void* buffer,
             size_t size_to_write,
             const WriteCallback& callback) override;

 private:
  provider::TaskRunner* task_runner_{nullptr};
  std::string write_data_;
  std::string read_data_;
};

}  // namespace test
}  // namespace weave

#endif  // LIBWEAVE_INCLUDE_WEAVE_TEST_FAKE_STREAM_H_
