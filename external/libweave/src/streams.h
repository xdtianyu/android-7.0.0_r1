// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBWEAVE_SRC_STREAMS_H_
#define LIBWEAVE_SRC_STREAMS_H_

#include <base/memory/weak_ptr.h>
#include <weave/stream.h>

namespace weave {

namespace provider {
class TaskRunner;
}

class MemoryStream : public InputStream, public OutputStream {
 public:
  MemoryStream(const std::vector<uint8_t>& data,
               provider::TaskRunner* task_runner);

  void Read(void* buffer,
            size_t size_to_read,
            const ReadCallback& callback) override;

  void Write(const void* buffer,
             size_t size_to_write,
             const WriteCallback& callback) override;

  const std::vector<uint8_t>& GetData() const { return data_; }

 private:
  std::vector<uint8_t> data_;
  provider::TaskRunner* task_runner_{nullptr};
  size_t read_position_{0};
};

class StreamCopier {
 public:
  StreamCopier(InputStream* source, OutputStream* destination);

  void Copy(const InputStream::ReadCallback& callback);

 private:
  void OnWriteDone(const InputStream::ReadCallback& callback, ErrorPtr error);
  void OnReadDone(const InputStream::ReadCallback& callback,
                  size_t size,
                  ErrorPtr error);

  InputStream* source_{nullptr};
  OutputStream* destination_{nullptr};

  size_t size_done_{0};
  std::vector<uint8_t> buffer_;

  base::WeakPtrFactory<StreamCopier> weak_ptr_factory_{this};
};

}  // namespace weave

#endif  // LIBWEAVE_SRC_STREAMS_H_
