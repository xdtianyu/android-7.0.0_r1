//
// Copyright (C) 2009 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

#ifndef UPDATE_ENGINE_PAYLOAD_CONSUMER_EXTENT_WRITER_H_
#define UPDATE_ENGINE_PAYLOAD_CONSUMER_EXTENT_WRITER_H_

#include <vector>

#include <base/logging.h>
#include <brillo/secure_blob.h>

#include "update_engine/common/utils.h"
#include "update_engine/payload_consumer/file_descriptor.h"
#include "update_engine/update_metadata.pb.h"

// ExtentWriter is an abstract class which synchronously writes to a given
// file descriptor at the extents given.

namespace chromeos_update_engine {

class ExtentWriter {
 public:
  ExtentWriter() = default;
  virtual ~ExtentWriter() {
    LOG_IF(ERROR, !end_called_) << "End() not called on ExtentWriter.";
  }

  // Returns true on success.
  virtual bool Init(FileDescriptorPtr fd,
                    const std::vector<Extent>& extents,
                    uint32_t block_size) = 0;

  // Returns true on success.
  virtual bool Write(const void* bytes, size_t count) = 0;

  // Should be called when all writing is complete. Returns true on success.
  // The fd is not closed. Caller is responsible for closing it.
  bool End() {
    end_called_ = true;
    return EndImpl();
  }
  virtual bool EndImpl() = 0;
 private:
  bool end_called_{false};
};

// DirectExtentWriter is probably the simplest ExtentWriter implementation.
// It writes the data directly into the extents.

class DirectExtentWriter : public ExtentWriter {
 public:
  DirectExtentWriter() = default;
  ~DirectExtentWriter() override = default;

  bool Init(FileDescriptorPtr fd,
            const std::vector<Extent>& extents,
            uint32_t block_size) override {
    fd_ = fd;
    block_size_ = block_size;
    extents_ = extents;
    return true;
  }
  bool Write(const void* bytes, size_t count) override;
  bool EndImpl() override { return true; }

 private:
  FileDescriptorPtr fd_{nullptr};

  size_t block_size_{0};
  // Bytes written into next_extent_index_ thus far
  uint64_t extent_bytes_written_{0};
  std::vector<Extent> extents_;
  // The next call to write should correspond to extents_[next_extent_index_]
  std::vector<Extent>::size_type next_extent_index_{0};
};

// Takes an underlying ExtentWriter to which all operations are delegated.
// When End() is called, ZeroPadExtentWriter ensures that the total number
// of bytes written is a multiple of block_size_. If not, it writes zeros
// to pad as needed.

class ZeroPadExtentWriter : public ExtentWriter {
 public:
  explicit ZeroPadExtentWriter(
      std::unique_ptr<ExtentWriter> underlying_extent_writer)
      : underlying_extent_writer_(std::move(underlying_extent_writer)) {}
  ~ZeroPadExtentWriter() override = default;

  bool Init(FileDescriptorPtr fd,
            const std::vector<Extent>& extents,
            uint32_t block_size) override {
    block_size_ = block_size;
    return underlying_extent_writer_->Init(fd, extents, block_size);
  }
  bool Write(const void* bytes, size_t count) override {
    if (underlying_extent_writer_->Write(bytes, count)) {
      bytes_written_mod_block_size_ += count;
      bytes_written_mod_block_size_ %= block_size_;
      return true;
    }
    return false;
  }
  bool EndImpl() override {
    if (bytes_written_mod_block_size_) {
      const size_t write_size = block_size_ - bytes_written_mod_block_size_;
      brillo::Blob zeros(write_size, 0);
      TEST_AND_RETURN_FALSE(underlying_extent_writer_->Write(zeros.data(),
                                                             write_size));
    }
    return underlying_extent_writer_->End();
  }

 private:
  std::unique_ptr<ExtentWriter> underlying_extent_writer_;
  size_t block_size_{0};
  size_t bytes_written_mod_block_size_{0};
};

}  // namespace chromeos_update_engine

#endif  // UPDATE_ENGINE_PAYLOAD_CONSUMER_EXTENT_WRITER_H_
