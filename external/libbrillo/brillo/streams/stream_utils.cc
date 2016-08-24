// Copyright 2015 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <brillo/streams/stream_utils.h>

#include <limits>

#include <base/bind.h>
#include <brillo/message_loops/message_loop.h>
#include <brillo/streams/stream_errors.h>

namespace brillo {
namespace stream_utils {

namespace {

// Status of asynchronous CopyData operation.
struct CopyDataState {
  brillo::StreamPtr in_stream;
  brillo::StreamPtr out_stream;
  std::vector<uint8_t> buffer;
  uint64_t remaining_to_copy;
  uint64_t size_copied;
  CopyDataSuccessCallback success_callback;
  CopyDataErrorCallback error_callback;
};

// Async CopyData I/O error callback.
void OnCopyDataError(const std::shared_ptr<CopyDataState>& state,
                     const brillo::Error* error) {
  state->error_callback.Run(std::move(state->in_stream),
                            std::move(state->out_stream), error);
}

// Forward declaration.
void PerformRead(const std::shared_ptr<CopyDataState>& state);

// Callback from read operation for CopyData. Writes the read data to the output
// stream and invokes PerformRead when done to restart the copy cycle.
void PerformWrite(const std::shared_ptr<CopyDataState>& state, size_t size) {
  if (size == 0) {
    state->success_callback.Run(std::move(state->in_stream),
                                std::move(state->out_stream),
                                state->size_copied);
    return;
  }
  state->size_copied += size;
  CHECK_GE(state->remaining_to_copy, size);
  state->remaining_to_copy -= size;

  brillo::ErrorPtr error;
  bool success = state->out_stream->WriteAllAsync(
      state->buffer.data(), size, base::Bind(&PerformRead, state),
      base::Bind(&OnCopyDataError, state), &error);

  if (!success)
    OnCopyDataError(state, error.get());
}

// Performs the read part of asynchronous CopyData operation. Reads the data
// from input stream and invokes PerformWrite when done to write the data to
// the output stream.
void PerformRead(const std::shared_ptr<CopyDataState>& state) {
  brillo::ErrorPtr error;
  const uint64_t buffer_size = state->buffer.size();
  // |buffer_size| is guaranteed to fit in size_t, so |size_to_read| value will
  // also not overflow size_t, so the static_cast below is safe.
  size_t size_to_read =
      static_cast<size_t>(std::min(buffer_size, state->remaining_to_copy));
  if (size_to_read == 0)
    return PerformWrite(state, 0);  // Nothing more to read. Finish operation.
  bool success = state->in_stream->ReadAsync(
      state->buffer.data(), size_to_read, base::Bind(PerformWrite, state),
      base::Bind(OnCopyDataError, state), &error);

  if (!success)
    OnCopyDataError(state, error.get());
}

}  // anonymous namespace

bool ErrorStreamClosed(const tracked_objects::Location& location,
                       ErrorPtr* error) {
  Error::AddTo(error,
               location,
               errors::stream::kDomain,
               errors::stream::kStreamClosed,
               "Stream is closed");
  return false;
}

bool ErrorOperationNotSupported(const tracked_objects::Location& location,
                                ErrorPtr* error) {
  Error::AddTo(error,
               location,
               errors::stream::kDomain,
               errors::stream::kOperationNotSupported,
               "Stream operation not supported");
  return false;
}

bool ErrorReadPastEndOfStream(const tracked_objects::Location& location,
                              ErrorPtr* error) {
  Error::AddTo(error,
               location,
               errors::stream::kDomain,
               errors::stream::kPartialData,
               "Reading past the end of stream");
  return false;
}

bool ErrorOperationTimeout(const tracked_objects::Location& location,
                           ErrorPtr* error) {
  Error::AddTo(error,
               location,
               errors::stream::kDomain,
               errors::stream::kTimeout,
               "Operation timed out");
  return false;
}

bool CheckInt64Overflow(const tracked_objects::Location& location,
                        uint64_t position,
                        int64_t offset,
                        ErrorPtr* error) {
  if (offset < 0) {
    // Subtracting the offset. Make sure we do not underflow.
    uint64_t unsigned_offset = static_cast<uint64_t>(-offset);
    if (position >= unsigned_offset)
      return true;
  } else {
    // Adding the offset. Make sure we do not overflow unsigned 64 bits first.
    if (position <= std::numeric_limits<uint64_t>::max() - offset) {
      // We definitely will not overflow the unsigned 64 bit integer.
      // Now check that we end up within the limits of signed 64 bit integer.
      uint64_t new_position = position + offset;
      uint64_t max = std::numeric_limits<int64_t>::max();
      if (new_position <= max)
        return true;
    }
  }
  Error::AddTo(error,
               location,
               errors::stream::kDomain,
               errors::stream::kInvalidParameter,
               "The stream offset value is out of range");
  return false;
}

bool CalculateStreamPosition(const tracked_objects::Location& location,
                             int64_t offset,
                             Stream::Whence whence,
                             uint64_t current_position,
                             uint64_t stream_size,
                             uint64_t* new_position,
                             ErrorPtr* error) {
  uint64_t pos = 0;
  switch (whence) {
    case Stream::Whence::FROM_BEGIN:
      pos = 0;
      break;

    case Stream::Whence::FROM_CURRENT:
      pos = current_position;
      break;

    case Stream::Whence::FROM_END:
      pos = stream_size;
      break;

    default:
      Error::AddTo(error,
                   location,
                   errors::stream::kDomain,
                   errors::stream::kInvalidParameter,
                   "Invalid stream position whence");
      return false;
  }

  if (!CheckInt64Overflow(location, pos, offset, error))
    return false;

  *new_position = static_cast<uint64_t>(pos + offset);
  return true;
}

void CopyData(StreamPtr in_stream,
              StreamPtr out_stream,
              const CopyDataSuccessCallback& success_callback,
              const CopyDataErrorCallback& error_callback) {
  CopyData(std::move(in_stream), std::move(out_stream),
           std::numeric_limits<uint64_t>::max(), 4096, success_callback,
           error_callback);
}

void CopyData(StreamPtr in_stream,
              StreamPtr out_stream,
              uint64_t max_size_to_copy,
              size_t buffer_size,
              const CopyDataSuccessCallback& success_callback,
              const CopyDataErrorCallback& error_callback) {
  auto state = std::make_shared<CopyDataState>();
  state->in_stream = std::move(in_stream);
  state->out_stream = std::move(out_stream);
  state->buffer.resize(buffer_size);
  state->remaining_to_copy = max_size_to_copy;
  state->size_copied = 0;
  state->success_callback = success_callback;
  state->error_callback = error_callback;
  brillo::MessageLoop::current()->PostTask(FROM_HERE,
                                             base::Bind(&PerformRead, state));
}

}  // namespace stream_utils
}  // namespace brillo
