// Copyright 2015 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <brillo/streams/input_stream_set.h>

#include <base/bind.h>
#include <brillo/message_loops/message_loop.h>
#include <brillo/streams/stream_errors.h>
#include <brillo/streams/stream_utils.h>

namespace brillo {

InputStreamSet::InputStreamSet(
    std::vector<Stream*> source_streams,
    std::vector<StreamPtr> owned_source_streams,
    uint64_t initial_stream_size)
    : source_streams_{std::move(source_streams)},
      owned_source_streams_{std::move(owned_source_streams)},
      initial_stream_size_{initial_stream_size} {}

StreamPtr InputStreamSet::Create(std::vector<Stream*> source_streams,
                                 std::vector<StreamPtr> owned_source_streams,
                                 ErrorPtr* error) {
  StreamPtr stream;

  if (source_streams.empty()) {
    Error::AddTo(error, FROM_HERE, errors::stream::kDomain,
                 errors::stream::kInvalidParameter,
                 "Source stream list is empty");
    return stream;
  }

  // Make sure we have only readable streams.
  for (Stream* src_stream : source_streams) {
    if (!src_stream->CanRead()) {
      Error::AddTo(error, FROM_HERE, errors::stream::kDomain,
                   errors::stream::kInvalidParameter,
                   "The stream list must contain only readable streams");
      return stream;
    }
  }

  // We are using remaining size here because the multiplexed stream is not
  // seekable and the bytes already read are essentially "lost" as far as this
  // stream is concerned.
  uint64_t initial_stream_size = 0;
  for (const Stream* stream : source_streams)
    initial_stream_size += stream->GetRemainingSize();

  stream.reset(new InputStreamSet{std::move(source_streams),
                                  std::move(owned_source_streams),
                                  initial_stream_size});
  return stream;
}

StreamPtr InputStreamSet::Create(std::vector<Stream*> source_streams,
                                 ErrorPtr* error) {
  return Create(std::move(source_streams), {}, error);
}

StreamPtr InputStreamSet::Create(std::vector<StreamPtr> owned_source_streams,
                                 ErrorPtr* error) {
  std::vector<Stream*> source_streams;
  source_streams.reserve(owned_source_streams.size());
  for (const StreamPtr& stream : owned_source_streams)
    source_streams.push_back(stream.get());
  return Create(std::move(source_streams), std::move(owned_source_streams),
                error);
}

bool InputStreamSet::IsOpen() const {
  return !closed_;
}

bool InputStreamSet::CanGetSize() const {
  bool can_get_size = IsOpen();
  for (const Stream* stream : source_streams_) {
    if (!stream->CanGetSize()) {
      can_get_size = false;
      break;
    }
  }
  return can_get_size;
}

uint64_t InputStreamSet::GetSize() const {
  return initial_stream_size_;
}

bool InputStreamSet::SetSizeBlocking(uint64_t /* size */, ErrorPtr* error) {
  return stream_utils::ErrorOperationNotSupported(FROM_HERE, error);
}

uint64_t InputStreamSet::GetRemainingSize() const {
  uint64_t size = 0;
  for (const Stream* stream : source_streams_)
    size += stream->GetRemainingSize();
  return size;
}

bool InputStreamSet::Seek(int64_t /* offset */,
                          Whence /* whence */,
                          uint64_t* /* new_position */,
                          ErrorPtr* error) {
  return stream_utils::ErrorOperationNotSupported(FROM_HERE, error);
}

bool InputStreamSet::ReadNonBlocking(void* buffer,
                                     size_t size_to_read,
                                     size_t* size_read,
                                     bool* end_of_stream,
                                     ErrorPtr* error) {
  if (!IsOpen())
    return stream_utils::ErrorStreamClosed(FROM_HERE, error);

  while (!source_streams_.empty()) {
    Stream* stream = source_streams_.front();
    bool eos = false;
    if (!stream->ReadNonBlocking(buffer, size_to_read, size_read, &eos, error))
      return false;

    if (*size_read > 0 || !eos) {
      if (end_of_stream)
        *end_of_stream = false;
      return true;
    }

    source_streams_.erase(source_streams_.begin());
  }
  *size_read = 0;
  if (end_of_stream)
    *end_of_stream = true;
  return true;
}

bool InputStreamSet::WriteNonBlocking(const void* /* buffer */,
                                      size_t /* size_to_write */,
                                      size_t* /* size_written */,
                                      ErrorPtr* error) {
  return stream_utils::ErrorOperationNotSupported(FROM_HERE, error);
}

bool InputStreamSet::CloseBlocking(ErrorPtr* error) {
  bool success = true;
  // We want to close only the owned streams.
  for (StreamPtr& stream_ptr : owned_source_streams_) {
    if (!stream_ptr->CloseBlocking(error))
      success = false;  // Keep going for other streams...
  }
  owned_source_streams_.clear();
  source_streams_.clear();
  initial_stream_size_ = 0;
  closed_ = true;
  return success;
}

bool InputStreamSet::WaitForData(
    AccessMode mode,
    const base::Callback<void(AccessMode)>& callback,
    ErrorPtr* error) {
  if (!IsOpen())
    return stream_utils::ErrorStreamClosed(FROM_HERE, error);

  if (stream_utils::IsWriteAccessMode(mode))
    return stream_utils::ErrorOperationNotSupported(FROM_HERE, error);

  if (!source_streams_.empty()) {
    Stream* stream = source_streams_.front();
    return stream->WaitForData(mode, callback, error);
  }

  MessageLoop::current()->PostTask(FROM_HERE, base::Bind(callback, mode));
  return true;
}

bool InputStreamSet::WaitForDataBlocking(AccessMode in_mode,
                                         base::TimeDelta timeout,
                                         AccessMode* out_mode,
                                         ErrorPtr* error) {
  if (!IsOpen())
    return stream_utils::ErrorStreamClosed(FROM_HERE, error);

  if (stream_utils::IsWriteAccessMode(in_mode))
    return stream_utils::ErrorOperationNotSupported(FROM_HERE, error);

  if (!source_streams_.empty()) {
    Stream* stream = source_streams_.front();
    return stream->WaitForDataBlocking(in_mode, timeout, out_mode, error);
  }

  if (out_mode)
    *out_mode = in_mode;
  return true;
}

void InputStreamSet::CancelPendingAsyncOperations() {
  if (IsOpen() && !source_streams_.empty()) {
    Stream* stream = source_streams_.front();
    stream->CancelPendingAsyncOperations();
  }
  Stream::CancelPendingAsyncOperations();
}

}  // namespace brillo
