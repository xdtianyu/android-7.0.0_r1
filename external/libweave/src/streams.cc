// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/streams.h"

#include <base/bind.h>
#include <base/callback.h>
#include <weave/provider/task_runner.h>
#include <weave/stream.h>

namespace weave {

namespace {}  // namespace

MemoryStream::MemoryStream(const std::vector<uint8_t>& data,
                           provider::TaskRunner* task_runner)
    : data_{data}, task_runner_{task_runner} {}

void MemoryStream::Read(void* buffer,
                        size_t size_to_read,
                        const ReadCallback& callback) {
  CHECK_LE(read_position_, data_.size());
  size_t size_read = std::min(size_to_read, data_.size() - read_position_);
  if (size_read > 0)
    memcpy(buffer, data_.data() + read_position_, size_read);
  read_position_ += size_read;
  task_runner_->PostDelayedTask(FROM_HERE,
                                base::Bind(callback, size_read, nullptr), {});
}

void MemoryStream::Write(const void* buffer,
                         size_t size_to_write,
                         const WriteCallback& callback) {
  data_.insert(data_.end(), static_cast<const char*>(buffer),
               static_cast<const char*>(buffer) + size_to_write);
  task_runner_->PostDelayedTask(FROM_HERE, base::Bind(callback, nullptr), {});
}

StreamCopier::StreamCopier(InputStream* source, OutputStream* destination)
    : source_{source}, destination_{destination}, buffer_(4096) {}

void StreamCopier::Copy(const InputStream::ReadCallback& callback) {
  source_->Read(buffer_.data(), buffer_.size(),
                base::Bind(&StreamCopier::OnReadDone,
                           weak_ptr_factory_.GetWeakPtr(), callback));
}

void StreamCopier::OnReadDone(const InputStream::ReadCallback& callback,
                              size_t size,
                              ErrorPtr error) {
  if (error)
    return callback.Run(0, std::move(error));

  size_done_ += size;
  if (size) {
    return destination_->Write(
        buffer_.data(), size,
        base::Bind(&StreamCopier::OnWriteDone, weak_ptr_factory_.GetWeakPtr(),
                   callback));
  }
  callback.Run(size_done_, nullptr);
}

void StreamCopier::OnWriteDone(const InputStream::ReadCallback& callback,
                               ErrorPtr error) {
  if (error)
    return callback.Run(size_done_, std::move(error));
  Copy(callback);
}

}  // namespace weave
