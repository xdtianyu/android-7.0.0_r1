// Copyright 2015 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBBRILLO_BRILLO_STREAMS_MEMORY_STREAM_H_
#define LIBBRILLO_BRILLO_STREAMS_MEMORY_STREAM_H_

#include <string>
#include <vector>

#include <base/macros.h>
#include <base/memory/weak_ptr.h>
#include <brillo/brillo_export.h>
#include <brillo/streams/memory_containers.h>
#include <brillo/streams/stream.h>

namespace brillo {

// MemoryStream is a brillo::Stream implementation for memory buffer. A number
// of memory containers are supported, such as raw memory pointers, data stored
// in std::vector and std::string.
// MemoryStream offers support for constant read-only memory buffers as well as
// for writable buffers that can grow when needed.
// A memory stream is created by using the OpenNNN and CreateNNN factory methods
// to construct a read-only and writable streams respectively.
// The following factory methods overloads are provided:
//  - OpenRef    - overloads for constructing the stream on a constant read-only
//                 memory buffer that is not owned by the stream. The buffer
//                 pointer/reference must remain valid throughout the lifetime
//                 of the constructed stream object. The benefit of this is that
//                 no data copying is performed and the underlying container can
//                 be manipulated outside of the stream.
//  - OpenCopyOf - overloads to construct a stream that copies the data from the
//                 memory buffer and maintains the copied data until the stream
//                 is closed or destroyed. This makes it possible to construct
//                 a read-only streams on transient data or for cases where
//                 it is not possible or necessary to maintain the lifetime of
//                 the underlying memory buffer.
//  - Create     - creates a new internal memory buffer that can be written to
//                 or read from using the stream I/O interface.
//  - CreateRef  - constructs a read/write stream on a reference of data
//                 container such as std::vector or std::string which must
//                 remain valid throughout the lifetime of the memory stream.
//                 The data already stored in the container is maintained,
//                 however the stream pointer is set to the beginning of the
//                 data when the stream is created.
//  - CreateRefForAppend - similar to CreateRef except that it automatically
//                 positions the stream seek pointer at the end of the data,
//                 which makes it possible to append more data to the existing
//                 container.
class BRILLO_EXPORT MemoryStream : public Stream {
 public:
  // == Construction ==========================================================

  // Constructs a read-only stream on a generic memory buffer. The data
  // pointed to by |buffer| will be copied and owned by the stream object.
  static StreamPtr OpenCopyOf(const void* buffer, size_t size, ErrorPtr* error);
  static StreamPtr OpenCopyOf(std::string buffer, ErrorPtr* error);
  static StreamPtr OpenCopyOf(const char* buffer, ErrorPtr* error);
  // Only vectors of char and uint8_t are supported.
  template<typename T>
  inline static StreamPtr OpenCopyOf(std::vector<T> buffer, ErrorPtr* error) {
    std::unique_ptr<data_container::ReadOnlyVectorCopy<T>> container{
        new data_container::ReadOnlyVectorCopy<T>{std::move(buffer)}};
    return CreateEx(std::move(container), 0, error);
  }

  // Constructs a read-only stream on a generic memory buffer which is owned
  // by the caller.
  // ***WARNING***: The |buffer| pointer must be valid for as long as the stream
  // object is alive. The stream does not do any additional lifetime management
  // for the data pointed to by |buffer| and destroying that buffer before
  // the stream is closed will lead to unexpected behavior.
  static StreamPtr OpenRef(const void* buffer, size_t size, ErrorPtr* error);
  static StreamPtr OpenRef(const std::string& buffer, ErrorPtr* error);
  static StreamPtr OpenRef(const char* buffer, ErrorPtr* error);
  // Only vectors of char and uint8_t are supported.
  template<typename T>
  inline static StreamPtr OpenRef(const std::vector<T>& buffer,
                                  ErrorPtr* error) {
    std::unique_ptr<data_container::ReadOnlyVectorRef<T>> container{
        new data_container::ReadOnlyVectorRef<T>{buffer}};
    return CreateEx(std::move(container), 0, error);
  }

  ///------------------------------------------------------------------------
  // Creates new stream for reading/writing. This method creates an internal
  // memory buffer and maintains it until the stream is closed. |reserve_size|
  // parameter is a hint of the buffer size to pre-allocate. This does not
  // affect the memory buffer reported size. The buffer can grow past that
  // amount if needed.
  static StreamPtr Create(size_t reserve_size, ErrorPtr* error);

  inline static StreamPtr Create(ErrorPtr* error) { return Create(0, error); }

  // Creates new stream for reading/writing stored in a string. The string
  // |buffer| must remain valid during the lifetime of the stream.
  // The stream pointer will be at the beginning of the string and the string's
  // content is preserved.
  static StreamPtr CreateRef(std::string* buffer, ErrorPtr* error);

  // Creates new stream for reading/writing stored in a vector. The vector
  // |buffer| must remain valid during the lifetime of the stream.
  // The stream pointer will be at the beginning of the data and the vector's
  // content is preserved.
  // Only vectors of char and uint8_t are supported.
  template<typename T>
  static StreamPtr CreateRef(std::vector<T>* buffer, ErrorPtr* error) {
    std::unique_ptr<data_container::VectorPtr<T>> container{
        new data_container::VectorPtr<T>{buffer}};
    return CreateEx(std::move(container), 0, error);
  }

  // Creates new stream for reading/writing stored in a string. The string
  // |buffer| must remain valid during the lifetime of the stream.
  // The stream pointer will be at the end of the string and the string's
  // content is preserved.
  static StreamPtr CreateRefForAppend(std::string* buffer, ErrorPtr* error);

  // Creates new stream for reading/writing stored in a vector. The vector
  // |buffer| must remain valid during the lifetime of the stream.
  // The stream pointer will be at the end of the data and the vector's
  // content is preserved.
  // Only vectors of char and uint8_t are supported.
  template<typename T>
  static StreamPtr CreateRefForAppend(std::vector<T>* buffer, ErrorPtr* error) {
    std::unique_ptr<data_container::VectorPtr<T>> container{
        new data_container::VectorPtr<T>{buffer}};
    return CreateEx(std::move(container), buffer->size() * sizeof(T), error);
  }

  ///------------------------------------------------------------------------
  // Generic stream creation on a data container. Takes an arbitrary |container|
  // and constructs a stream using it. The container determines the traits of
  // the stream (e.g. whether it is read-only, what operations are supported
  // and so on). |stream_position| is the current stream pointer position at
  // creation time.
  static StreamPtr CreateEx(
      std::unique_ptr<data_container::DataContainerInterface> container,
      size_t stream_position,
      ErrorPtr* error);

  // == Stream capabilities ===================================================
  bool IsOpen() const override;
  bool CanRead() const override;
  bool CanWrite() const override;
  bool CanSeek() const override;
  bool CanGetSize() const override;

  // == Stream size operations ================================================
  uint64_t GetSize() const override;
  bool SetSizeBlocking(uint64_t size, ErrorPtr* error) override;
  uint64_t GetRemainingSize() const override;

  // == Seek operations =======================================================
  uint64_t GetPosition() const override;
  bool Seek(int64_t offset,
            Whence whence,
            uint64_t* new_position,
            ErrorPtr* error) override;

  // == Read operations =======================================================
  bool ReadNonBlocking(void* buffer,
                       size_t size_to_read,
                       size_t* size_read,
                       bool* end_of_stream,
                       ErrorPtr* error) override;

  // == Write operations ======================================================
  bool WriteNonBlocking(const void* buffer,
                        size_t size_to_write,
                        size_t* size_written,
                        ErrorPtr* error) override;

  // == Finalizing/closing streams  ===========================================
  bool FlushBlocking(ErrorPtr* error) override;
  bool CloseBlocking(ErrorPtr* error) override;

  // == Data availability monitoring ==========================================
  bool WaitForData(AccessMode mode,
                   const base::Callback<void(AccessMode)>& callback,
                   ErrorPtr* error) override;

  bool WaitForDataBlocking(AccessMode in_mode,
                           base::TimeDelta timeout,
                           AccessMode* out_mode,
                           ErrorPtr* error) override;

 private:
  friend class MemoryStreamTest;

  // Private constructor used by MemoryStream::OpenNNNN() and
  // MemoryStream::CreateNNNN() factory methods.
  MemoryStream(
      std::unique_ptr<data_container::DataContainerInterface> container,
      size_t stream_position);

  // Checks if the stream has a valid container.
  bool CheckContainer(ErrorPtr* error) const;

  // Data container the stream is using to write and/or read data.
  std::unique_ptr<data_container::DataContainerInterface> container_;

  // The current stream pointer position.
  size_t stream_position_{0};

  DISALLOW_COPY_AND_ASSIGN(MemoryStream);
};

}  // namespace brillo

#endif  // LIBBRILLO_BRILLO_STREAMS_MEMORY_STREAM_H_
