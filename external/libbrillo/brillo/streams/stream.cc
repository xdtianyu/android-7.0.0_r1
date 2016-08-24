// Copyright 2015 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <brillo/streams/stream.h>

#include <algorithm>

#include <base/bind.h>
#include <brillo/message_loops/message_loop.h>
#include <brillo/pointer_utils.h>
#include <brillo/streams/stream_errors.h>
#include <brillo/streams/stream_utils.h>

namespace brillo {

bool Stream::TruncateBlocking(ErrorPtr* error) {
  return SetSizeBlocking(GetPosition(), error);
}

bool Stream::SetPosition(uint64_t position, ErrorPtr* error) {
  if (!stream_utils::CheckInt64Overflow(FROM_HERE, position, 0, error))
    return false;
  return Seek(position, Whence::FROM_BEGIN, nullptr, error);
}

bool Stream::ReadAsync(void* buffer,
                       size_t size_to_read,
                       const base::Callback<void(size_t)>& success_callback,
                       const ErrorCallback& error_callback,
                       ErrorPtr* error) {
  if (is_async_read_pending_) {
    Error::AddTo(error, FROM_HERE, errors::stream::kDomain,
                 errors::stream::kOperationNotSupported,
                 "Another asynchronous operation is still pending");
    return false;
  }

  auto callback = base::Bind(&Stream::IgnoreEOSCallback, success_callback);
  // If we can read some data right away non-blocking we should still run the
  // callback from the main loop, so we pass true here for force_async_callback.
  return ReadAsyncImpl(buffer, size_to_read, callback, error_callback, error,
                       true);
}

bool Stream::ReadAllAsync(void* buffer,
                          size_t size_to_read,
                          const base::Closure& success_callback,
                          const ErrorCallback& error_callback,
                          ErrorPtr* error) {
  if (is_async_read_pending_) {
    Error::AddTo(error, FROM_HERE, errors::stream::kDomain,
                 errors::stream::kOperationNotSupported,
                 "Another asynchronous operation is still pending");
    return false;
  }

  auto callback = base::Bind(&Stream::ReadAllAsyncCallback,
                             weak_ptr_factory_.GetWeakPtr(), buffer,
                             size_to_read, success_callback, error_callback);
  return ReadAsyncImpl(buffer, size_to_read, callback, error_callback, error,
                       true);
}

bool Stream::ReadBlocking(void* buffer,
                          size_t size_to_read,
                          size_t* size_read,
                          ErrorPtr* error) {
  for (;;) {
    bool eos = false;
    if (!ReadNonBlocking(buffer, size_to_read, size_read, &eos, error))
      return false;

    if (*size_read > 0 || eos)
      break;

    if (!WaitForDataBlocking(AccessMode::READ, base::TimeDelta::Max(), nullptr,
                             error)) {
      return false;
    }
  }
  return true;
}

bool Stream::ReadAllBlocking(void* buffer,
                             size_t size_to_read,
                             ErrorPtr* error) {
  while (size_to_read > 0) {
    size_t size_read = 0;
    if (!ReadBlocking(buffer, size_to_read, &size_read, error))
      return false;

    if (size_read == 0)
      return stream_utils::ErrorReadPastEndOfStream(FROM_HERE, error);

    size_to_read -= size_read;
    buffer = AdvancePointer(buffer, size_read);
  }
  return true;
}

bool Stream::WriteAsync(const void* buffer,
                        size_t size_to_write,
                        const base::Callback<void(size_t)>& success_callback,
                        const ErrorCallback& error_callback,
                        ErrorPtr* error) {
  if (is_async_write_pending_) {
    Error::AddTo(error, FROM_HERE, errors::stream::kDomain,
                 errors::stream::kOperationNotSupported,
                 "Another asynchronous operation is still pending");
    return false;
  }
  // If we can read some data right away non-blocking we should still run the
  // callback from the main loop, so we pass true here for force_async_callback.
  return WriteAsyncImpl(buffer, size_to_write, success_callback, error_callback,
                        error, true);
}

bool Stream::WriteAllAsync(const void* buffer,
                           size_t size_to_write,
                           const base::Closure& success_callback,
                           const ErrorCallback& error_callback,
                           ErrorPtr* error) {
  if (is_async_write_pending_) {
    Error::AddTo(error, FROM_HERE, errors::stream::kDomain,
                 errors::stream::kOperationNotSupported,
                 "Another asynchronous operation is still pending");
    return false;
  }

  auto callback = base::Bind(&Stream::WriteAllAsyncCallback,
                             weak_ptr_factory_.GetWeakPtr(), buffer,
                             size_to_write, success_callback, error_callback);
  return WriteAsyncImpl(buffer, size_to_write, callback, error_callback, error,
                        true);
}

bool Stream::WriteBlocking(const void* buffer,
                           size_t size_to_write,
                           size_t* size_written,
                           ErrorPtr* error) {
  for (;;) {
    if (!WriteNonBlocking(buffer, size_to_write, size_written, error))
      return false;

    if (*size_written > 0 || size_to_write == 0)
      break;

    if (!WaitForDataBlocking(AccessMode::WRITE, base::TimeDelta::Max(), nullptr,
                             error)) {
      return false;
    }
  }
  return true;
}

bool Stream::WriteAllBlocking(const void* buffer,
                              size_t size_to_write,
                              ErrorPtr* error) {
  while (size_to_write > 0) {
    size_t size_written = 0;
    if (!WriteBlocking(buffer, size_to_write, &size_written, error))
      return false;

    if (size_written == 0) {
      Error::AddTo(error, FROM_HERE, errors::stream::kDomain,
                   errors::stream::kPartialData,
                   "Failed to write all the data");
      return false;
    }
    size_to_write -= size_written;
    buffer = AdvancePointer(buffer, size_written);
  }
  return true;
}

bool Stream::FlushAsync(const base::Closure& success_callback,
                        const ErrorCallback& error_callback,
                        ErrorPtr* /* error */) {
  auto callback = base::Bind(&Stream::FlushAsyncCallback,
                             weak_ptr_factory_.GetWeakPtr(),
                             success_callback, error_callback);
  MessageLoop::current()->PostTask(FROM_HERE, callback);
  return true;
}

void Stream::IgnoreEOSCallback(
    const base::Callback<void(size_t)>& success_callback,
    size_t bytes,
    bool /* eos */) {
  success_callback.Run(bytes);
}

bool Stream::ReadAsyncImpl(
    void* buffer,
    size_t size_to_read,
    const base::Callback<void(size_t, bool)>& success_callback,
    const ErrorCallback& error_callback,
    ErrorPtr* error,
    bool force_async_callback) {
  CHECK(!is_async_read_pending_);
  // We set this value to true early in the function so calling others will
  // prevent us from calling WaitForData() to make calls to
  // ReadAsync() fail while we run WaitForData().
  is_async_read_pending_ = true;

  size_t read = 0;
  bool eos = false;
  if (!ReadNonBlocking(buffer, size_to_read, &read, &eos, error))
    return false;

  if (read > 0 || eos) {
    if (force_async_callback) {
      MessageLoop::current()->PostTask(
          FROM_HERE,
          base::Bind(&Stream::OnReadAsyncDone, weak_ptr_factory_.GetWeakPtr(),
                     success_callback, read, eos));
    } else {
      is_async_read_pending_ = false;
      success_callback.Run(read, eos);
    }
    return true;
  }

  is_async_read_pending_ = WaitForData(
      AccessMode::READ,
      base::Bind(&Stream::OnReadAvailable, weak_ptr_factory_.GetWeakPtr(),
                 buffer, size_to_read, success_callback, error_callback),
      error);
  return is_async_read_pending_;
}

void Stream::OnReadAsyncDone(
    const base::Callback<void(size_t, bool)>& success_callback,
    size_t bytes_read,
    bool eos) {
  is_async_read_pending_ = false;
  success_callback.Run(bytes_read, eos);
}

void Stream::OnReadAvailable(
    void* buffer,
    size_t size_to_read,
    const base::Callback<void(size_t, bool)>& success_callback,
    const ErrorCallback& error_callback,
    AccessMode mode) {
  CHECK(stream_utils::IsReadAccessMode(mode));
  CHECK(is_async_read_pending_);
  is_async_read_pending_ = false;
  ErrorPtr error;
  // Just reschedule the read operation but don't need to run the callback from
  // the main loop since we are already running on a callback.
  if (!ReadAsyncImpl(buffer, size_to_read, success_callback, error_callback,
                     &error, false)) {
    error_callback.Run(error.get());
  }
}

bool Stream::WriteAsyncImpl(
    const void* buffer,
    size_t size_to_write,
    const base::Callback<void(size_t)>& success_callback,
    const ErrorCallback& error_callback,
    ErrorPtr* error,
    bool force_async_callback) {
  CHECK(!is_async_write_pending_);
  // We set this value to true early in the function so calling others will
  // prevent us from calling WaitForData() to make calls to
  // ReadAsync() fail while we run WaitForData().
  is_async_write_pending_ = true;

  size_t written = 0;
  if (!WriteNonBlocking(buffer, size_to_write, &written, error))
    return false;

  if (written > 0) {
    if (force_async_callback) {
      MessageLoop::current()->PostTask(
          FROM_HERE,
          base::Bind(&Stream::OnWriteAsyncDone, weak_ptr_factory_.GetWeakPtr(),
                     success_callback, written));
    } else {
      is_async_write_pending_ = false;
      success_callback.Run(written);
    }
    return true;
  }
  is_async_write_pending_ = WaitForData(
      AccessMode::WRITE,
      base::Bind(&Stream::OnWriteAvailable, weak_ptr_factory_.GetWeakPtr(),
                 buffer, size_to_write, success_callback, error_callback),
      error);
  return is_async_write_pending_;
}

void Stream::OnWriteAsyncDone(
    const base::Callback<void(size_t)>& success_callback,
    size_t size_written) {
  is_async_write_pending_ = false;
  success_callback.Run(size_written);
}

void Stream::OnWriteAvailable(
    const void* buffer,
    size_t size,
    const base::Callback<void(size_t)>& success_callback,
    const ErrorCallback& error_callback,
    AccessMode mode) {
  CHECK(stream_utils::IsWriteAccessMode(mode));
  CHECK(is_async_write_pending_);
  is_async_write_pending_ = false;
  ErrorPtr error;
  // Just reschedule the read operation but don't need to run the callback from
  // the main loop since we are already running on a callback.
  if (!WriteAsyncImpl(buffer, size, success_callback, error_callback, &error,
                      false)) {
    error_callback.Run(error.get());
  }
}

void Stream::ReadAllAsyncCallback(void* buffer,
                                  size_t size_to_read,
                                  const base::Closure& success_callback,
                                  const ErrorCallback& error_callback,
                                  size_t size_read,
                                  bool eos) {
  ErrorPtr error;
  size_to_read -= size_read;
  if (size_to_read != 0 && eos) {
    stream_utils::ErrorReadPastEndOfStream(FROM_HERE, &error);
    error_callback.Run(error.get());
    return;
  }

  if (size_to_read) {
    buffer = AdvancePointer(buffer, size_read);
    auto callback = base::Bind(&Stream::ReadAllAsyncCallback,
                               weak_ptr_factory_.GetWeakPtr(), buffer,
                               size_to_read, success_callback, error_callback);
    if (!ReadAsyncImpl(buffer, size_to_read, callback, error_callback, &error,
                       false)) {
      error_callback.Run(error.get());
    }
  } else {
    success_callback.Run();
  }
}

void Stream::WriteAllAsyncCallback(const void* buffer,
                                   size_t size_to_write,
                                   const base::Closure& success_callback,
                                   const ErrorCallback& error_callback,
                                   size_t size_written) {
  ErrorPtr error;
  if (size_to_write != 0 && size_written == 0) {
    Error::AddTo(&error, FROM_HERE, errors::stream::kDomain,
                 errors::stream::kPartialData, "Failed to write all the data");
    error_callback.Run(error.get());
    return;
  }
  size_to_write -= size_written;
  if (size_to_write) {
    buffer = AdvancePointer(buffer, size_written);
    auto callback = base::Bind(&Stream::WriteAllAsyncCallback,
                               weak_ptr_factory_.GetWeakPtr(), buffer,
                               size_to_write, success_callback, error_callback);
    if (!WriteAsyncImpl(buffer, size_to_write, callback, error_callback, &error,
                        false)) {
      error_callback.Run(error.get());
    }
  } else {
    success_callback.Run();
  }
}

void Stream::FlushAsyncCallback(const base::Closure& success_callback,
                                const ErrorCallback& error_callback) {
  ErrorPtr error;
  if (FlushBlocking(&error)) {
    success_callback.Run();
  } else {
    error_callback.Run(error.get());
  }
}

void Stream::CancelPendingAsyncOperations() {
  weak_ptr_factory_.InvalidateWeakPtrs();
  is_async_read_pending_ = false;
  is_async_write_pending_ = false;
}

}  // namespace brillo
