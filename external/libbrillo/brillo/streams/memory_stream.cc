// Copyright 2015 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <brillo/streams/memory_stream.h>

#include <limits>

#include <base/bind.h>
#include <brillo/message_loops/message_loop.h>
#include <brillo/streams/stream_errors.h>
#include <brillo/streams/stream_utils.h>

namespace brillo {

MemoryStream::MemoryStream(
    std::unique_ptr<data_container::DataContainerInterface> container,
    size_t stream_position)
    : container_{std::move(container)}, stream_position_{stream_position} {}

StreamPtr MemoryStream::OpenRef(const void* buffer,
                                size_t size,
                                ErrorPtr* error) {
  std::unique_ptr<data_container::ReadOnlyBuffer> container{
      new data_container::ReadOnlyBuffer{buffer, size}};
  return CreateEx(std::move(container), 0, error);
}

StreamPtr MemoryStream::OpenCopyOf(const void* buffer,
                                   size_t size,
                                   ErrorPtr* error) {
  std::unique_ptr<data_container::ReadOnlyVectorCopy<uint8_t>> container{
      new data_container::ReadOnlyVectorCopy<uint8_t>{
          reinterpret_cast<const uint8_t*>(buffer), size}};
  return CreateEx(std::move(container), 0, error);
}

StreamPtr MemoryStream::OpenRef(const std::string& buffer, ErrorPtr* error) {
  std::unique_ptr<data_container::ReadOnlyStringRef> container{
      new data_container::ReadOnlyStringRef{buffer}};
  return CreateEx(std::move(container), 0, error);
}

StreamPtr MemoryStream::OpenCopyOf(std::string buffer, ErrorPtr* error) {
  std::unique_ptr<data_container::ReadOnlyStringCopy> container{
      new data_container::ReadOnlyStringCopy{std::move(buffer)}};
  return CreateEx(std::move(container), 0, error);
}

StreamPtr MemoryStream::OpenRef(const char* buffer, ErrorPtr* error) {
  return OpenRef(buffer, std::strlen(buffer), error);
}

StreamPtr MemoryStream::OpenCopyOf(const char* buffer, ErrorPtr* error) {
  return OpenCopyOf(buffer, std::strlen(buffer), error);
}

StreamPtr MemoryStream::Create(size_t reserve_size, ErrorPtr* error) {
  std::unique_ptr<data_container::ByteBuffer> container{
      new data_container::ByteBuffer{reserve_size}};
  return CreateEx(std::move(container), 0, error);
}

StreamPtr MemoryStream::CreateRef(std::string* buffer, ErrorPtr* error) {
  std::unique_ptr<data_container::StringPtr> container{
      new data_container::StringPtr{buffer}};
  return CreateEx(std::move(container), 0, error);
}

StreamPtr MemoryStream::CreateRefForAppend(std::string* buffer,
                                           ErrorPtr* error) {
  std::unique_ptr<data_container::StringPtr> container{
      new data_container::StringPtr{buffer}};
  return CreateEx(std::move(container), buffer->size(), error);
}

StreamPtr MemoryStream::CreateEx(
    std::unique_ptr<data_container::DataContainerInterface> container,
    size_t stream_position,
    ErrorPtr* error) {
  ignore_result(error);  // Unused.
  return StreamPtr{new MemoryStream(std::move(container), stream_position)};
}

bool MemoryStream::IsOpen() const { return container_ != nullptr; }
bool MemoryStream::CanRead() const { return IsOpen(); }

bool MemoryStream::CanWrite() const {
  return IsOpen() && !container_->IsReadOnly();
}

bool MemoryStream::CanSeek() const { return IsOpen(); }
bool MemoryStream::CanGetSize() const { return IsOpen(); }

uint64_t MemoryStream::GetSize() const {
  return IsOpen() ? container_->GetSize() : 0;
}

bool MemoryStream::SetSizeBlocking(uint64_t size, ErrorPtr* error) {
  if (!CheckContainer(error))
    return false;
  return container_->Resize(size, error);
}

uint64_t MemoryStream::GetRemainingSize() const {
  uint64_t pos = GetPosition();
  uint64_t size = GetSize();
  return (pos < size) ? size - pos : 0;
}

uint64_t MemoryStream::GetPosition() const {
  return IsOpen() ? stream_position_ : 0;
}

bool MemoryStream::Seek(int64_t offset,
                        Whence whence,
                        uint64_t* new_position,
                        ErrorPtr* error) {
  uint64_t pos = 0;
  if (!CheckContainer(error) ||
      !stream_utils::CalculateStreamPosition(FROM_HERE, offset, whence,
                                             stream_position_, GetSize(), &pos,
                                             error)) {
    return false;
  }
  if (pos > static_cast<uint64_t>(std::numeric_limits<size_t>::max())) {
    // This can only be the case on 32 bit systems.
    brillo::Error::AddTo(error, FROM_HERE, errors::stream::kDomain,
                         errors::stream::kInvalidParameter,
                         "Stream pointer position is outside allowed limits");
    return false;
  }

  stream_position_ = static_cast<size_t>(pos);
  if (new_position)
    *new_position = stream_position_;
  return true;
}

bool MemoryStream::ReadNonBlocking(void* buffer,
                                   size_t size_to_read,
                                   size_t* size_read,
                                   bool* end_of_stream,
                                   ErrorPtr* error) {
  if (!CheckContainer(error))
    return false;
  size_t read = 0;
  if (!container_->Read(buffer, size_to_read, stream_position_, &read, error))
    return false;
  stream_position_ += read;
  *size_read = read;
  if (end_of_stream)
    *end_of_stream = (read == 0) && (size_to_read != 0);
  return true;
}

bool MemoryStream::WriteNonBlocking(const void* buffer,
                                    size_t size_to_write,
                                    size_t* size_written,
                                    ErrorPtr* error) {
  if (!CheckContainer(error))
    return false;
  if (!container_->Write(buffer, size_to_write, stream_position_, size_written,
                         error)) {
    return false;
  }
  stream_position_ += *size_written;
  return true;
}

bool MemoryStream::FlushBlocking(ErrorPtr* error) {
  return CheckContainer(error);
}

bool MemoryStream::CloseBlocking(ErrorPtr* error) {
  ignore_result(error);  // Unused.
  container_.reset();
  return true;
}

bool MemoryStream::CheckContainer(ErrorPtr* error) const {
  return container_ || stream_utils::ErrorStreamClosed(FROM_HERE, error);
}

bool MemoryStream::WaitForData(AccessMode mode,
                               const base::Callback<void(AccessMode)>& callback,
                               ErrorPtr* /* error */) {
  MessageLoop::current()->PostTask(FROM_HERE, base::Bind(callback, mode));
  return true;
}

bool MemoryStream::WaitForDataBlocking(AccessMode in_mode,
                                       base::TimeDelta /* timeout */,
                                       AccessMode* out_mode,
                                       ErrorPtr* /* error */) {
  if (out_mode)
    *out_mode = in_mode;
  return true;
}

}  // namespace brillo
