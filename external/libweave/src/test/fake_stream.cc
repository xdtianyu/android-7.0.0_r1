// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <weave/test/fake_stream.h>

#include <base/bind.h>
#include <gtest/gtest.h>
#include <weave/provider/task_runner.h>

namespace weave {
namespace test {

FakeStream::FakeStream(provider::TaskRunner* task_runner)
    : task_runner_{task_runner} {}
FakeStream::FakeStream(provider::TaskRunner* task_runner,
                       const std::string& read_data)
    : task_runner_{task_runner}, read_data_{read_data} {}

void FakeStream::CancelPendingOperations() {}

void FakeStream::ExpectWritePacketString(base::TimeDelta,
                                         const std::string& data) {
  write_data_ += data;
}

void FakeStream::AddReadPacketString(base::TimeDelta, const std::string& data) {
  read_data_ += data;
}

void FakeStream::Read(void* buffer,
                      size_t size_to_read,
                      const ReadCallback& callback) {
  if (read_data_.empty()) {
    task_runner_->PostDelayedTask(
        FROM_HERE, base::Bind(&FakeStream::Read, base::Unretained(this), buffer,
                              size_to_read, callback),
        base::TimeDelta::FromSeconds(0));
    return;
  }
  size_t size = std::min(size_to_read, read_data_.size());
  memcpy(buffer, read_data_.data(), size);
  read_data_ = read_data_.substr(size);
  task_runner_->PostDelayedTask(FROM_HERE, base::Bind(callback, size, nullptr),
                                base::TimeDelta::FromSeconds(0));
}

void FakeStream::Write(const void* buffer,
                       size_t size_to_write,
                       const WriteCallback& callback) {
  size_t size = std::min(size_to_write, write_data_.size());
  EXPECT_EQ(write_data_.substr(0, size),
            std::string(reinterpret_cast<const char*>(buffer), size_to_write));
  write_data_ = write_data_.substr(size);
  task_runner_->PostDelayedTask(FROM_HERE, base::Bind(callback, nullptr),
                                base::TimeDelta::FromSeconds(0));
}

}  // namespace test
}  // namespace weave
