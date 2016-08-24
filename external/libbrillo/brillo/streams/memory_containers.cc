// Copyright 2015 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <brillo/streams/memory_containers.h>

#include <base/callback.h>
#include <brillo/streams/stream_errors.h>

namespace brillo {
namespace data_container {

namespace {

bool ErrorStreamReadOnly(const tracked_objects::Location& location,
                         ErrorPtr* error) {
  Error::AddTo(error,
               location,
               errors::stream::kDomain,
               errors::stream::kOperationNotSupported,
               "Stream is read-only");
  return false;
}

}  // anonymous namespace

void ContiguousBufferBase::CopyMemoryBlock(void* dest,
                                           const void* src,
                                           size_t size) const {
  memcpy(dest, src, size);
}

bool ContiguousBufferBase::Read(void* buffer,
                                size_t size_to_read,
                                size_t offset,
                                size_t* size_read,
                                ErrorPtr* error) {
  size_t buf_size = GetSize();
  if (offset < buf_size) {
    size_t remaining = buf_size - offset;
    if (size_to_read >= remaining) {
      size_to_read = remaining;
    }
    const void* src_buffer = GetReadOnlyBuffer(offset, error);
    if (!src_buffer)
      return false;

    CopyMemoryBlock(buffer, src_buffer, size_to_read);
  } else {
    size_to_read = 0;
  }
  if (size_read)
    *size_read = size_to_read;
  return true;
}

bool ContiguousBufferBase::Write(const void* buffer,
                                 size_t size_to_write,
                                 size_t offset,
                                 size_t* size_written,
                                 ErrorPtr* error) {
  if (size_to_write) {
    size_t new_size = offset + size_to_write;
    if (GetSize() < new_size && !Resize(new_size, error))
      return false;
    void* ptr = GetBuffer(offset, error);
    if (!ptr)
      return false;
    CopyMemoryBlock(ptr, buffer, size_to_write);
    if (size_written)
      *size_written = size_to_write;
  }
  return true;
}

bool ContiguousReadOnlyBufferBase::Write(const void* /* buffer */,
                                         size_t /* size_to_write */,
                                         size_t /* offset */,
                                         size_t* /* size_written */,
                                         ErrorPtr* error) {
  return ErrorStreamReadOnly(FROM_HERE, error);
}

bool ContiguousReadOnlyBufferBase::Resize(size_t /* new_size */,
                                          ErrorPtr* error) {
  return ErrorStreamReadOnly(FROM_HERE, error);
}

void* ContiguousReadOnlyBufferBase::GetBuffer(size_t /* offset */,
                                              ErrorPtr* error) {
  ErrorStreamReadOnly(FROM_HERE, error);
  return nullptr;
}

ByteBuffer::ByteBuffer(size_t reserve_size)
    : VectorPtr(new std::vector<uint8_t>()) {
  vector_ptr_->reserve(reserve_size);
}

ByteBuffer::~ByteBuffer() {
  delete vector_ptr_;
}

StringPtr::StringPtr(std::string* string) : string_ptr_(string) {}

bool StringPtr::Resize(size_t new_size, ErrorPtr* /* error */) {
  string_ptr_->resize(new_size);
  return true;
}

const void* StringPtr::GetReadOnlyBuffer(size_t offset,
                                         ErrorPtr* /* error */) const {
  return string_ptr_->data() + offset;
}

void* StringPtr::GetBuffer(size_t offset, ErrorPtr* /* error */) {
  return &(*string_ptr_)[offset];
}

ReadOnlyStringRef::ReadOnlyStringRef(const std::string& string)
    : string_ref_(string) {}

const void* ReadOnlyStringRef::GetReadOnlyBuffer(size_t offset,
                                                 ErrorPtr* /* error */) const {
  return string_ref_.data() + offset;
}

ReadOnlyStringCopy::ReadOnlyStringCopy(std::string string)
    : ReadOnlyStringRef(string_copy_), string_copy_(std::move(string)) {}

}  // namespace data_container
}  // namespace brillo
