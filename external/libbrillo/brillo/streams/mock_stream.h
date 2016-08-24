// Copyright 2015 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBBRILLO_BRILLO_STREAMS_MOCK_STREAM_H_
#define LIBBRILLO_BRILLO_STREAMS_MOCK_STREAM_H_

#include <gmock/gmock.h>

#include <brillo/streams/stream.h>

namespace brillo {

// Mock Stream implementation for testing.
class MockStream : public Stream {
 public:
  MockStream() = default;

  MOCK_CONST_METHOD0(IsOpen, bool());
  MOCK_CONST_METHOD0(CanRead, bool());
  MOCK_CONST_METHOD0(CanWrite, bool());
  MOCK_CONST_METHOD0(CanSeek, bool());
  MOCK_CONST_METHOD0(CanGetSize, bool());

  MOCK_CONST_METHOD0(GetSize, uint64_t());
  MOCK_METHOD2(SetSizeBlocking, bool(uint64_t, ErrorPtr*));
  MOCK_CONST_METHOD0(GetRemainingSize, uint64_t());

  MOCK_CONST_METHOD0(GetPosition, uint64_t());
  MOCK_METHOD4(Seek, bool(int64_t, Whence, uint64_t*, ErrorPtr*));

  MOCK_METHOD5(ReadAsync, bool(void*,
                               size_t,
                               const base::Callback<void(size_t)>&,
                               const ErrorCallback&,
                               ErrorPtr*));
  MOCK_METHOD5(ReadAllAsync, bool(void*,
                                  size_t,
                                  const base::Closure&,
                                  const ErrorCallback&,
                                  ErrorPtr*));
  MOCK_METHOD5(ReadNonBlocking, bool(void*, size_t, size_t*, bool*, ErrorPtr*));
  MOCK_METHOD4(ReadBlocking, bool(void*, size_t, size_t*, ErrorPtr*));
  MOCK_METHOD3(ReadAllBlocking, bool(void*, size_t, ErrorPtr*));

  MOCK_METHOD5(WriteAsync, bool(const void*,
                                size_t,
                                const base::Callback<void(size_t)>&,
                                const ErrorCallback&,
                                ErrorPtr*));
  MOCK_METHOD5(WriteAllAsync, bool(const void*,
                                   size_t,
                                   const base::Closure&,
                                   const ErrorCallback&,
                                   ErrorPtr*));
  MOCK_METHOD4(WriteNonBlocking, bool(const void*, size_t, size_t*, ErrorPtr*));
  MOCK_METHOD4(WriteBlocking, bool(const void*, size_t, size_t*, ErrorPtr*));
  MOCK_METHOD3(WriteAllBlocking, bool(const void*, size_t, ErrorPtr*));

  MOCK_METHOD1(FlushBlocking, bool(ErrorPtr*));
  MOCK_METHOD1(CloseBlocking, bool(ErrorPtr*));

  MOCK_METHOD3(WaitForData, bool(AccessMode,
                                 const base::Callback<void(AccessMode)>&,
                                 ErrorPtr*));
  MOCK_METHOD4(WaitForDataBlocking,
               bool(AccessMode, base::TimeDelta, AccessMode*, ErrorPtr*));

 private:
  DISALLOW_COPY_AND_ASSIGN(MockStream);
};

}  // namespace brillo

#endif  // LIBBRILLO_BRILLO_STREAMS_MOCK_STREAM_H_
