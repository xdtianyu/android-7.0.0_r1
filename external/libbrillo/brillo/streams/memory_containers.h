// Copyright 2015 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBBRILLO_BRILLO_STREAMS_MEMORY_CONTAINERS_H_
#define LIBBRILLO_BRILLO_STREAMS_MEMORY_CONTAINERS_H_

#include <string>
#include <vector>

#include <brillo/brillo_export.h>
#include <brillo/errors/error.h>
#include <brillo/streams/stream.h>

namespace brillo {
namespace data_container {

// MemoryStream class relies on helper classes defined below to support data
// storage in various types of containers.
// A particular implementation of container type (e.g. based on raw memory
// buffers, std::vector, std::string or others) need to implement the container
// interface provided by data_container::DataContainerInterface.
// Low-level functionality such as reading data from and writing data to the
// container, getting and changing the buffer size, and so on, must be provided.
// Not all methods must be provided. For example, for read-only containers, only
// read operations can be provided.
class BRILLO_EXPORT DataContainerInterface {
 public:
  DataContainerInterface() = default;
  virtual ~DataContainerInterface() = default;

  // Read the data from the container into |buffer|. Up to |size_to_read| bytes
  // must be read at a time. The container can return fewer bytes. The actual
  // size of data read is provided in |size_read|.
  // If the read operation fails, the function must return false and provide
  // additional information about the error in |error| object.
  virtual bool Read(void* buffer,
                    size_t size_to_read,
                    size_t offset,
                    size_t* size_read,
                    ErrorPtr* error) = 0;

  // Writes |size_to_write| bytes of data from |buffer| into the container.
  // The container may accept fewer bytes of data. The actual size of data
  // written is provided in |size_written|.
  // If the read operation fails, the function must return false and provide
  // additional information about the error in |error| object.
  virtual bool Write(const void* buffer,
                     size_t size_to_write,
                     size_t offset,
                     size_t* size_written,
                     ErrorPtr* error) = 0;
  // Resizes the container to the new size specified in |new_size|.
  virtual bool Resize(size_t new_size, ErrorPtr* error) = 0;
  // Returns the current size of the container.
  virtual size_t GetSize() const = 0;
  // Returns true if the container is read-only.
  virtual bool IsReadOnly() const = 0;

 private:
  DISALLOW_COPY_AND_ASSIGN(DataContainerInterface);
};

// ContiguousBufferBase is a helper base class for memory containers that
// employ contiguous memory for all of their data. This class provides the
// default implementation for Read() and Write() functions and requires the
// implementations to provide GetBuffer() and/or ReadOnlyBuffer() functions.
class BRILLO_EXPORT ContiguousBufferBase : public DataContainerInterface {
 public:
  ContiguousBufferBase() = default;
  // Implementation of DataContainerInterface::Read().
  bool Read(void* buffer,
            size_t size_to_read,
            size_t offset,
            size_t* size_read,
            ErrorPtr* error) override;
  // Implementation of DataContainerInterface::Write().
  bool Write(const void* buffer,
             size_t size_to_write,
             size_t offset,
             size_t* size_written,
             ErrorPtr* error) override;

  // Overload to provide the pointer to the read-only data for the container at
  // the specified |offset|. In case of an error, this function must return
  // nullptr and provide error details in |error| object if provided.
  virtual const void* GetReadOnlyBuffer(size_t offset,
                                        ErrorPtr* error) const = 0;
  // Overload to provide the pointer to the read/write data for the container at
  // the specified |offset|. In case of an error, this function must return
  // nullptr and provide error details in |error| object if provided.
  virtual void* GetBuffer(size_t offset, ErrorPtr* error) = 0;

 protected:
  // Wrapper around memcpy which can be mocked out in tests.
  virtual void CopyMemoryBlock(void* dest, const void* src, size_t size) const;

 private:
  DISALLOW_COPY_AND_ASSIGN(ContiguousBufferBase);
};

// ContiguousReadOnlyBufferBase is a specialization of ContiguousBufferBase for
// read-only containers.
class BRILLO_EXPORT ContiguousReadOnlyBufferBase : public ContiguousBufferBase {
 public:
  ContiguousReadOnlyBufferBase() = default;
  // Fails with an error "operation_not_supported" (Stream is read-only) error.
  bool Write(const void* buffer,
             size_t size_to_write,
             size_t offset,
             size_t* size_written,
             ErrorPtr* error) override;
  // Fails with an error "operation_not_supported" (Stream is read-only) error.
  bool Resize(size_t new_size, ErrorPtr* error) override;
  // Fails with an error "operation_not_supported" (Stream is read-only) error.
  bool IsReadOnly() const override { return true; }
  // Fails with an error "operation_not_supported" (Stream is read-only) error.
  void* GetBuffer(size_t offset, ErrorPtr* error) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(ContiguousReadOnlyBufferBase);
};

// ReadOnlyBuffer implements a read-only container based on raw memory block.
class BRILLO_EXPORT ReadOnlyBuffer : public ContiguousReadOnlyBufferBase {
 public:
  // Constructs the container based at the pointer to memory |buffer| and its
  // |size|. The pointer to the memory must be valid throughout life-time of
  // the stream using this container.
  ReadOnlyBuffer(const void* buffer, size_t size)
      : buffer_(buffer), size_(size) {}

  // Returns the pointer to data at |offset|.
  const void* GetReadOnlyBuffer(size_t offset,
                                ErrorPtr* /* error */) const override {
    return reinterpret_cast<const uint8_t*>(buffer_) + offset;
  }
  // Returns the size of the container.
  size_t GetSize() const override { return size_; }

 private:
  // Raw memory pointer to the data block and its size.
  const void* buffer_;
  size_t size_;

  DISALLOW_COPY_AND_ASSIGN(ReadOnlyBuffer);
};

// VectorPtr<T> is a read/write container based on a vector<T> pointer.
// This is a template class to allow usage of both vector<char> and
// vector<uint8_t> without duplicating the implementation.
template<typename T>
class VectorPtr : public ContiguousBufferBase {
 public:
  static_assert(sizeof(T) == 1, "Only char/byte is supported");
  explicit VectorPtr(std::vector<T>* vector) : vector_ptr_(vector) {}

  bool Resize(size_t new_size, ErrorPtr* /* error */) override {
    vector_ptr_->resize(new_size);
    return true;
  }
  size_t GetSize() const override { return vector_ptr_->size(); }
  bool IsReadOnly() const override { return false; }
  const void* GetReadOnlyBuffer(size_t offset,
                                ErrorPtr* /* error */) const override {
    return reinterpret_cast<const uint8_t*>(vector_ptr_->data()) + offset;
  }
  void* GetBuffer(size_t offset, ErrorPtr* /* error */) override {
    return reinterpret_cast<uint8_t*>(vector_ptr_->data()) + offset;
  }

 protected:
  std::vector<T>* vector_ptr_;

 private:
  DISALLOW_COPY_AND_ASSIGN(VectorPtr);
};

// ReadOnlyVectorRef<T> is a read-only container based on a vector<T> reference.
// This is a template class to allow usage of both vector<char> and
// vector<uint8_t> without duplicating the implementation.
template<typename T>
class ReadOnlyVectorRef : public ContiguousReadOnlyBufferBase {
 public:
  static_assert(sizeof(T) == 1, "Only char/byte is supported");
  explicit ReadOnlyVectorRef(const std::vector<T>& vector)
      : vector_ref_(vector) {}

  const void* GetReadOnlyBuffer(size_t offset,
                                ErrorPtr* /* error */) const override {
    return reinterpret_cast<const uint8_t*>(vector_ref_.data()) + offset;
  }
  size_t GetSize() const override { return vector_ref_.size(); }

 protected:
  const std::vector<T>& vector_ref_;

 private:
  DISALLOW_COPY_AND_ASSIGN(ReadOnlyVectorRef);
};

// ReadOnlyVectorCopy<T> is a read-only container based on a copy of vector<T>.
// This container actually owns the data stored in the vector.
// This is a template class to allow usage of both vector<char> and
// vector<uint8_t> without duplicating the implementation.
template<typename T>
class ReadOnlyVectorCopy : public ContiguousReadOnlyBufferBase {
 public:
  static_assert(sizeof(T) == 1, "Only char/byte is supported");
  explicit ReadOnlyVectorCopy(std::vector<T> vector)
      : vector_copy_(std::move(vector)) {}

  ReadOnlyVectorCopy(const T* buffer, size_t size)
      : vector_copy_(buffer, buffer + size) {}

  const void* GetReadOnlyBuffer(size_t offset,
                                ErrorPtr* /* error */) const override {
    return reinterpret_cast<const uint8_t*>(vector_copy_.data()) + offset;
  }
  size_t GetSize() const override { return vector_copy_.size(); }

 protected:
  std::vector<T> vector_copy_;

 private:
  DISALLOW_COPY_AND_ASSIGN(ReadOnlyVectorCopy);
};

// ByteBuffer is a read/write container that manages the data and underlying
// storage.
class BRILLO_EXPORT ByteBuffer : public VectorPtr<uint8_t> {
 public:
  explicit ByteBuffer(size_t reserve_size);
  ~ByteBuffer() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(ByteBuffer);
};

// StringPtr is a read/write container based on external std::string storage.
class BRILLO_EXPORT StringPtr : public ContiguousBufferBase {
 public:
  explicit StringPtr(std::string* string);

  bool Resize(size_t new_size, ErrorPtr* error) override;
  size_t GetSize() const override { return string_ptr_->size(); }
  bool IsReadOnly() const override { return false; }
  const void* GetReadOnlyBuffer(size_t offset, ErrorPtr* error) const override;
  void* GetBuffer(size_t offset, ErrorPtr* error) override;

 protected:
  std::string* string_ptr_;

 private:
  DISALLOW_COPY_AND_ASSIGN(StringPtr);
};

// ReadOnlyStringRef is a read-only container based on external std::string.
class BRILLO_EXPORT ReadOnlyStringRef : public ContiguousReadOnlyBufferBase {
 public:
  explicit ReadOnlyStringRef(const std::string& string);
  const void* GetReadOnlyBuffer(size_t offset, ErrorPtr* error) const override;
  size_t GetSize() const override { return string_ref_.size(); }

 protected:
  const std::string& string_ref_;

 private:
  DISALLOW_COPY_AND_ASSIGN(ReadOnlyStringRef);
};

// ReadOnlyStringCopy is a read-only container based on a copy of a std::string.
// This container actually owns the data stored in the string.
class BRILLO_EXPORT ReadOnlyStringCopy : public ReadOnlyStringRef {
 public:
  explicit ReadOnlyStringCopy(std::string string);

 protected:
  std::string string_copy_;

 private:
  DISALLOW_COPY_AND_ASSIGN(ReadOnlyStringCopy);
};

}  // namespace data_container
}  // namespace brillo

#endif  // LIBBRILLO_BRILLO_STREAMS_MEMORY_CONTAINERS_H_
