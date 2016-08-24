// Copyright 2015 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBBRILLO_BRILLO_STREAMS_TLS_STREAM_H_
#define LIBBRILLO_BRILLO_STREAMS_TLS_STREAM_H_

#include <memory>
#include <string>

#include <base/macros.h>
#include <brillo/brillo_export.h>
#include <brillo/errors/error.h>
#include <brillo/streams/stream.h>

namespace brillo {

// This class provides client-side TLS stream that performs handshake with the
// server and established a secure communication channel which can be used
// by performing read/write operations on this stream. Both synchronous and
// asynchronous I/O is supported.
// The underlying socket stream must already be created and connected to the
// destination server and passed in TlsStream::Connect() method as |socket|.
class BRILLO_EXPORT TlsStream : public Stream {
 public:
  ~TlsStream() override;

  // Perform a TLS handshake and establish secure connection over |socket|.
  // Calls |callback| when successful and passes the instance of TlsStream
  // as an argument. In case of an error, |error_callback| is called.
  // |host| must specify the expected remote host (server) name.
  static void Connect(
      StreamPtr socket,
      const std::string& host,
      const base::Callback<void(StreamPtr)>& success_callback,
      const Stream::ErrorCallback& error_callback);

  // Overrides from Stream:
  bool IsOpen() const override;
  bool CanRead() const override { return true; }
  bool CanWrite() const override { return true; }
  bool CanSeek() const override { return false; }
  bool CanGetSize() const override { return false; }
  uint64_t GetSize() const override { return 0; }
  bool SetSizeBlocking(uint64_t size, ErrorPtr* error) override;
  uint64_t GetRemainingSize() const override { return 0; }
  uint64_t GetPosition() const override { return 0; }
  bool Seek(int64_t offset,
            Whence whence,
            uint64_t* new_position,
            ErrorPtr* error) override;
  bool ReadNonBlocking(void* buffer,
                       size_t size_to_read,
                       size_t* size_read,
                       bool* end_of_stream,
                       ErrorPtr* error) override;
  bool WriteNonBlocking(const void* buffer,
                        size_t size_to_write,
                        size_t* size_written,
                        ErrorPtr* error) override;
  bool FlushBlocking(ErrorPtr* error) override;
  bool CloseBlocking(ErrorPtr* error) override;
  bool WaitForData(AccessMode mode,
                   const base::Callback<void(AccessMode)>& callback,
                   ErrorPtr* error) override;
  bool WaitForDataBlocking(AccessMode in_mode,
                           base::TimeDelta timeout,
                           AccessMode* out_mode,
                           ErrorPtr* error) override;
  void CancelPendingAsyncOperations() override;

 private:
  class TlsStreamImpl;

  // Private constructor called from TlsStream::Connect() factory method.
  explicit TlsStream(std::unique_ptr<TlsStreamImpl> impl);

  std::unique_ptr<TlsStreamImpl> impl_;
  DISALLOW_COPY_AND_ASSIGN(TlsStream);
};

}  // namespace brillo

#endif  // LIBBRILLO_BRILLO_STREAMS_TLS_STREAM_H_
