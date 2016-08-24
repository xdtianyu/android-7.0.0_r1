// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBWEAVE_INCLUDE_WEAVE_STREAM_H_
#define LIBWEAVE_INCLUDE_WEAVE_STREAM_H_

#include <string>

#include <base/callback.h>
#include <weave/error.h>

namespace weave {

// Interface for async input streaming.
class InputStream {
 public:
  virtual ~InputStream() {}

  // Callback type for Read.
  using ReadCallback = base::Callback<void(size_t size, ErrorPtr error)>;

  // Implementation should return immediately and post callback after
  // completing operation. Caller guarantees that buffet is alive until callback
  // is called.
  virtual void Read(void* buffer,
                    size_t size_to_read,
                    const ReadCallback& callback) = 0;
};

// Interface for async input streaming.
class OutputStream {
 public:
  virtual ~OutputStream() {}

  using WriteCallback = base::Callback<void(ErrorPtr error)>;

  // Implementation should return immediately and post callback after
  // completing operation. Caller guarantees that buffet is alive until either
  // of callback is called.
  // Success callback must be called only after all data is written.
  virtual void Write(const void* buffer,
                     size_t size_to_write,
                     const WriteCallback& callback) = 0;
};

// Interface for async bi-directional streaming.
class Stream : public InputStream, public OutputStream {
 public:
  ~Stream() override {}

  // Cancels all pending read or write requests. Canceled operations must not
  // call any callbacks.
  virtual void CancelPendingOperations() = 0;
};

}  // namespace weave

#endif  // LIBWEAVE_INCLUDE_WEAVE_STREAM_H_
