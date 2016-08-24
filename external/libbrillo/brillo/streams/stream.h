// Copyright 2015 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBBRILLO_BRILLO_STREAMS_STREAM_H_
#define LIBBRILLO_BRILLO_STREAMS_STREAM_H_

#include <cstdint>
#include <memory>

#include <base/callback.h>
#include <base/macros.h>
#include <base/memory/weak_ptr.h>
#include <base/time/time.h>
#include <brillo/brillo_export.h>
#include <brillo/errors/error.h>

namespace brillo {

// Stream is a base class that specific stream storage implementations must
// derive from to provide I/O facilities.
// The stream class provides general streaming I/O primitives to read, write and
// seek within a stream. It has methods for asynchronous (callback-based) as
// well as synchronous (both blocking and non-blocking) operations.
// The Stream class is abstract and cannot be created by itself.
// In order to construct a stream, you must use one of the derived classes'
// factory methods which return a stream smart pointer (StreamPtr):
//
//    StreamPtr input_stream = FileStream::Open(path, AccessMode::READ);
//    StreamPtr output_stream = MemoryStream::Create();
//    uint8_t buf[1000];
//    size_t read = 0;
//    while (input_stream->ReadBlocking(buf, sizeof(buf), &read, nullptr)) {
//      if (read == 0) break;
//      output_stream->WriteAllBlocking(buf, read, nullptr);
//    }
//
// NOTE ABOUT ASYNCHRONOUS OPERATIONS: Asynchronous I/O relies on a MessageLoop
// instance to be present on the current thread. Using Stream::ReadAsync(),
// Stream::WriteAsync() and similar will call MessageLoop::current() to access
// the current message loop and abort if there isn't one for the current thread.
// Also, only one outstanding asynchronous operation of particular kind (reading
// or writing) at a time is supported. Trying to call ReadAsync() while another
// asynchronous read operation is pending will fail with an error
// ("operation_not_supported").
//
// NOTE ABOUT READING FROM/WRITING TO STREAMS: In many cases underlying streams
// use buffered I/O. Using all read/write methods other than ReadAllAsync(),
// ReadAllBlocking(), WriteAllAsync(), WriteAllBlocking() will return
// immediately if there is any data available in the underlying buffer. That is,
// trying to read 1000 bytes while the internal buffer contains only 100 will
// return immediately with just those 100 bytes and no blocking or other I/O
// traffic will be incurred. This guarantee is important for efficient and
// correct implementation of duplex communication over pipes and sockets.
//
// NOTE TO IMPLEMENTERS: When creating new stream types, you must derive
// from this class and provide the implementation for its pure virtual methods.
// For operations that do not apply to your stream, make sure the corresponding
// methods return "false" and set the error to "operation_not_supported".
// You should use stream_utils::ErrorOperationNotSupported() for this. Also
// Make sure the stream capabilities functions like CanRead(), etc return
// correct values:
//
//    bool MyReadOnlyStream::CanRead() const { return true; }
//    bool MyReadOnlyStream::CanWrite() const { return false; }
//    bool MyReadOnlyStream::WriteBlocking(const void* buffer,
//                                         size_t size_to_write,
//                                         size_t* size_written,
//                                         ErrorPtr* error) {
//      return stream_utils::ErrorOperationNotSupported(error);
//    }
//
// The class should also provide a static factory methods to create/open
// a new stream:
//
//    static StreamPtr MyReadOnlyStream::Open(..., ErrorPtr* error) {
//      auto my_stream = std::make_unique<MyReadOnlyStream>(...);
//      if (!my_stream->Initialize(..., error))
//        my_stream.reset();
//      }
//      return my_stream;
//    }
//
class BRILLO_EXPORT Stream {
 public:
  // When seeking in streams, whence specifies the origin of the seek operation.
  enum class Whence { FROM_BEGIN, FROM_CURRENT, FROM_END };
  // Stream access mode for open operations (used in derived classes).
  enum class AccessMode { READ, WRITE, READ_WRITE };

  // Standard error callback for asynchronous operations.
  using ErrorCallback = base::Callback<void(const Error*)>;

  virtual ~Stream() = default;

  // == Stream capabilities ===================================================

  // Returns true while stream is open. Closing the last reference to the stream
  // will make this method return false.
  virtual bool IsOpen() const = 0;

  // Called to determine if read operations are supported on the stream (stream
  // is readable). This method does not check if there is actually any data to
  // read, only the fact that the stream is open in read mode and can be read
  // from in general.
  // If CanRead() returns false, it is guaranteed that the stream can't be
  // read from. However, if it returns true, there is no guarantee that the
  // subsequent read operation will actually succeed (for example, the stream
  // position could be at the end of the data stream, or the access mode of
  // the stream is unknown beforehand).
  virtual bool CanRead() const = 0;

  // Called to determine if write operations are supported on the stream (stream
  // is writable).
  // If CanWrite() returns false, it is guaranteed that the stream can't be
  // written to. However, if it returns true, the subsequent write operation
  // is not guaranteed to succeed (e.g. the output media could be out of free
  // space or a transport error could occur).
  virtual bool CanWrite() const = 0;

  // Called to determine if random access I/O operations are supported on
  // the stream. Sequential streams should return false.
  // If CanSeek() returns false, it is guaranteed that the stream can't use
  // Seek(). However, if it returns true, it might be possible to seek, but this
  // is not guaranteed since the actual underlying stream capabilities might
  // not be known.
  // Note that non-seekable streams might still maintain the current stream
  // position and GetPosition method might still be used even if CanSeek()
  // returns false. However SetPosition() will almost always fail in such
  // a case.
  virtual bool CanSeek() const = 0;

  // Called to determine if the size of the stream is known. Size of some
  // sequential streams (e.g. based on pipes) is unknown beforehand, so this
  // method can be used to check how reliable a call to GetSize() is.
  virtual bool CanGetSize() const = 0;

  // == Stream size operations ================================================

  // Returns the size of stream data.
  // If the stream size is unavailable/unknown, it returns 0.
  virtual uint64_t GetSize() const = 0;

  // Resizes the stream storage to |size|. Stream must be writable and support
  // this operation.
  virtual bool SetSizeBlocking(uint64_t size, ErrorPtr* error) = 0;

  // Truncates the stream at the current stream pointer.
  // Calls SetSizeBlocking(GetPosition(), ...).
  bool TruncateBlocking(ErrorPtr* error);

  // Returns the amount of data remaining in the stream. If the size of the
  // stream is unknown, or if the stream pointer is at or past the end of the
  // stream, the function returns 0.
  virtual uint64_t GetRemainingSize() const = 0;

  // == Seek operations =======================================================

  // Gets the position of the stream I/O pointer from the beginning of the
  // stream. If the stream position is unavailable/unknown, it returns 0.
  virtual uint64_t GetPosition() const = 0;

  // Moves the stream pointer to the specified position, relative to the
  // beginning of the stream. This calls Seek(position, Whence::FROM_BEGIN),
  // however it also provides proper |position| validation to ensure that
  // it doesn't overflow the range of signed int64_t used by Seek.
  bool SetPosition(uint64_t position, ErrorPtr* error);

  // Moves the stream pointer by |offset| bytes relative to |whence|.
  // When successful, returns true and sets the new pointer position from the
  // beginning of the stream to |new_position|. If |new_position| is nullptr,
  // new stream position is not returned.
  // On error, returns false and specifies additional details in |error| if it
  // is not nullptr.
  virtual bool Seek(int64_t offset,
                    Whence whence,
                    uint64_t* new_position,
                    ErrorPtr* error) = 0;

  // == Read operations =======================================================

  // -- Asynchronous ----------------------------------------------------------

  // Reads up to |size_to_read| bytes from the stream asynchronously. It is not
  // guaranteed that all requested data will be read. It is not an error for
  // this function to read fewer bytes than requested. If the function reads
  // zero bytes, it means that the end of stream is reached.
  // Upon successful read, the |success_callback| will be invoked with the
  // actual number of bytes read.
  // If an error occurs during the asynchronous operation, the |error_callback|
  // is invoked with the error details. The error object pointer passed in as a
  // parameter to the |error_callback| is valid only for the duration of that
  // callback.
  // If this function successfully schedules an asynchronous operation, it
  // returns true. If it fails immediately, it will return false and set the
  // error details to |error| object and will not call the success or error
  // callbacks.
  // The |buffer| must be at least |size_to_read| in size and must remain
  // valid for the duration of the asynchronous operation (until either
  // |success_callback| or |error_callback| is called).
  // Only one asynchronous operation at a time is allowed on the stream (read
  // and/or write)
  // Uses ReadNonBlocking() and MonitorDataAvailable().
  virtual bool ReadAsync(void* buffer,
                         size_t size_to_read,
                         const base::Callback<void(size_t)>& success_callback,
                         const ErrorCallback& error_callback,
                         ErrorPtr* error);

  // Similar to ReadAsync() operation above but reads exactly |size_to_read|
  // bytes from the stream into the |buffer|. Attempt to read past the end of
  // the stream is considered an error in this case and will trigger the
  // |error_callback|. The rest of restrictions and conditions of ReadAsync()
  // method applies to ReadAllAsync() as well.
  // Uses ReadNonBlocking() and MonitorDataAvailable().
  virtual bool ReadAllAsync(void* buffer,
                            size_t size_to_read,
                            const base::Closure& success_callback,
                            const ErrorCallback& error_callback,
                            ErrorPtr* error);

  // -- Synchronous non-blocking ----------------------------------------------

  // Reads up to |size_to_read| bytes from the stream without blocking.
  // The |buffer| must be at least |size_to_read| in size. It is not an error
  // for this function to return without reading all (or any) the data.
  // The actual amount of data read (which could be 0 bytes) is returned in
  // |size_read|.
  // On error, the function returns false and specifies additional error details
  // in |error|.
  // If end of stream is reached or if no data is currently available to be read
  // without blocking, |size_read| will contain 0 and the function will still
  // return true (success). In case of end-of-stream scenario, |end_of_stream|
  // will also be set to true to indicate that no more data is available.
  virtual bool ReadNonBlocking(void* buffer,
                               size_t size_to_read,
                               size_t* size_read,
                               bool* end_of_stream,
                               ErrorPtr* error) = 0;

  // -- Synchronous blocking --------------------------------------------------

  // Reads up to |size_to_read| bytes from the stream. This function will block
  // until at least one byte is read or the end of stream is reached or until
  // the stream is closed.
  // The |buffer| must be at least |size_to_read| in size. It is not an error
  // for this function to return without reading all the data. The actual amount
  // of data read (which could be 0 bytes) is returned in |size_read|.
  // On error, the function returns false and specifies additional error details
  // in |error|. In this case, the state of the stream pointer is undefined,
  // since some bytes might have been read successfully (and the pointer moved)
  // before the error has occurred and |size_read| is not updated.
  // If end of stream is reached, |size_read| will contain 0 and the function
  // will still return true (success).
  virtual bool ReadBlocking(void* buffer,
                            size_t size_to_read,
                            size_t* size_read,
                            ErrorPtr* error);

  // Reads exactly |size_to_read| bytes to |buffer|. Returns false on error
  // (reading fewer than requested bytes is treated as an error as well).
  // Calls ReadAllBlocking() repeatedly until all the data is read.
  virtual bool ReadAllBlocking(void* buffer,
                               size_t size_to_read,
                               ErrorPtr* error);

  // == Write operations ======================================================

  // -- Asynchronous ----------------------------------------------------------

  // Writes up to |size_to_write| bytes from |buffer| to the stream
  // asynchronously. It is not guaranteed that all requested data will be
  // written. It is not an error for this function to write fewer bytes than
  // requested.
  // Upon successful write, the |success_callback| will be invoked with the
  // actual number of bytes written.
  // If an error occurs during the asynchronous operation, the |error_callback|
  // is invoked with the error details. The error object pointer is valid only
  // for the duration of the error callback.
  // If this function successfully schedules an asynchronous operation, it
  // returns true. If it fails immediately, it will return false and set the
  // error details to |error| object and will not call the success or error
  // callbacks.
  // The |buffer| must be at least |size_to_write| in size and must remain
  // valid for the duration of the asynchronous operation (until either
  // |success_callback| or |error_callback| is called).
  // Only one asynchronous operation at a time is allowed on the stream (read
  // and/or write).
  // Uses WriteNonBlocking() and MonitorDataAvailable().
  virtual bool WriteAsync(const void* buffer,
                          size_t size_to_write,
                          const base::Callback<void(size_t)>& success_callback,
                          const ErrorCallback& error_callback,
                          ErrorPtr* error);

  // Similar to WriteAsync() operation above but writes exactly |size_to_write|
  // bytes from |buffet| to the stream. When all the data is written
  // successfully, the |success_callback| is invoked.
  // The rest of restrictions and conditions of WriteAsync() method applies to
  // WriteAllAsync() as well.
  // Uses WriteNonBlocking() and MonitorDataAvailable().
  virtual bool WriteAllAsync(const void* buffer,
                             size_t size_to_write,
                             const base::Closure& success_callback,
                             const ErrorCallback& error_callback,
                             ErrorPtr* error);

  // -- Synchronous non-blocking ----------------------------------------------

  // Writes up to |size_to_write| bytes to the stream. The |buffer| must be at
  // least |size_to_write| in size. It is not an error for this function to
  // return without writing all the data requested (or any data at all).
  // The actual amount of data written is returned in |size_written|.
  // On error, the function returns false and specifies additional error details
  // in |error|.
  virtual bool WriteNonBlocking(const void* buffer,
                                size_t size_to_write,
                                size_t* size_written,
                                ErrorPtr* error) = 0;

  // -- Synchronous blocking --------------------------------------------------

  // Writes up to |size_to_write| bytes to the stream. The |buffer| must be at
  // least |size_to_write| in size. It is not an error for this function to
  // return without writing all the data requested. The actual amount of data
  // written is returned in |size_written|.
  // On error, the function returns false and specifies additional error details
  // in |error|.
  virtual bool WriteBlocking(const void* buffer,
                             size_t size_to_write,
                             size_t* size_written,
                             ErrorPtr* error);

  // Writes exactly |size_to_write| bytes to |buffer|. Returns false on error
  // (writing fewer than requested bytes is treated as an error as well).
  // Calls WriteBlocking() repeatedly until all the data is written.
  virtual bool WriteAllBlocking(const void* buffer,
                                size_t size_to_write,
                                ErrorPtr* error);

  // == Finalizing/closing streams  ===========================================

  // Flushes all the user-space data from cache output buffers to storage
  // medium. For read-only streams this is a no-op, however it is still valid
  // to call this method on read-only streams.
  // If an error occurs, the function returns false and specifies additional
  // error details in |error|.
  virtual bool FlushBlocking(ErrorPtr* error) = 0;

  // Flushes all the user-space data from the cache output buffer
  // asynchronously. When all the data is successfully flushed, the
  // |success_callback| is invoked. If an error occurs while flushing, partial
  // data might be flushed and |error_callback| is invoked. If there's an error
  // scheduling the flush operation, it returns false and neither callback will
  // be called.
  virtual bool FlushAsync(const base::Closure& success_callback,
                          const ErrorCallback& error_callback,
                          ErrorPtr* error);

  // Closes the underlying stream. The stream is also automatically closed
  // when the stream object is destroyed, but since closing a stream is
  // an operation that may fail, in situations when it is important to detect
  // the failure to close the stream, CloseBlocking() should be used explicitly
  // before destroying the stream object.
  virtual bool CloseBlocking(ErrorPtr* error) = 0;

  // == Data availability monitoring ==========================================

  // Overloaded by derived classes to provide stream monitoring for read/write
  // data availability for the stream. Calls |callback| when data can be read
  // and/or written without blocking.
  // |mode| specifies the type of operation to monitor for (read, write, both).
  virtual bool WaitForData(AccessMode mode,
                           const base::Callback<void(AccessMode)>& callback,
                           ErrorPtr* error) = 0;

  // Helper function for implementing blocking I/O. Blocks until the
  // non-blocking operation specified by |in_mode| can be performed.
  // If |out_mode| is not nullptr, it receives the actual operation that can be
  // performed. For example, watching a stream for READ_WRITE while only
  // READ can be performed, |out_mode| would contain READ even though |in_mode|
  // was set to READ_WRITE.
  // |timeout| is the maximum amount of time to wait. Set it to TimeDelta::Max()
  // to wait indefinitely.
  virtual bool WaitForDataBlocking(AccessMode in_mode,
                                   base::TimeDelta timeout,
                                   AccessMode* out_mode,
                                   ErrorPtr* error) = 0;

  // Cancels pending asynchronous read/write operations.
  virtual void CancelPendingAsyncOperations();

 protected:
  Stream() = default;

 private:
  // Simple wrapper to call the externally exposed |success_callback| that only
  // receives a size_t.
  BRILLO_PRIVATE static void IgnoreEOSCallback(
      const base::Callback<void(size_t)>& success_callback,
      size_t read,
      bool eos);

  // The internal implementation of ReadAsync() and ReadAllAsync().
  // Calls ReadNonBlocking and if there's no data available waits for it calling
  // WaitForData(). The extra |force_async_callback| tell whether the success
  // callback should be called from the main loop instead of directly from this
  // method. This method only calls WaitForData() if ReadNonBlocking() returns a
  // situation in which it would block (bytes_read = 0 and eos = false),
  // preventing us from calling WaitForData() on streams that don't support such
  // feature.
  BRILLO_PRIVATE bool ReadAsyncImpl(
      void* buffer,
      size_t size_to_read,
      const base::Callback<void(size_t, bool)>& success_callback,
      const ErrorCallback& error_callback,
      ErrorPtr* error,
      bool force_async_callback);

  // Called from the main loop when the ReadAsyncImpl finished right away
  // without waiting for data. We use this callback to call the
  // |sucess_callback| but invalidate the callback if the Stream is destroyed
  // while this call is waiting in the main loop.
  BRILLO_PRIVATE void OnReadAsyncDone(
      const base::Callback<void(size_t, bool)>& success_callback,
      size_t bytes_read,
      bool eos);

  // Called from WaitForData() when read operations can be performed
  // without blocking (the type of operation is provided in |mode|).
  BRILLO_PRIVATE void OnReadAvailable(
      void* buffer,
      size_t size_to_read,
      const base::Callback<void(size_t, bool)>& success_callback,
      const ErrorCallback& error_callback,
      AccessMode mode);

  // The internal implementation of WriteAsync() and WriteAllAsync().
  // Calls WriteNonBlocking and if the write would block for it to not block
  // calling WaitForData(). The extra |force_async_callback| tell whether the
  // success callback should be called from the main loop instead of directly
  // from this method. This method only calls WaitForData() if
  // WriteNonBlocking() returns a situation in which it would block
  // (size_written = 0 and eos = false), preventing us from calling
  // WaitForData() on streams that don't support such feature.
  BRILLO_PRIVATE bool WriteAsyncImpl(
      const void* buffer,
      size_t size_to_write,
      const base::Callback<void(size_t)>& success_callback,
      const ErrorCallback& error_callback,
      ErrorPtr* error,
      bool force_async_callback);

  // Called from the main loop when the WriteAsyncImpl finished right away
  // without waiting for data. We use this callback to call the
  // |sucess_callback| but invalidate the callback if the Stream is destroyed
  // while this call is waiting in the main loop.
  BRILLO_PRIVATE void OnWriteAsyncDone(
      const base::Callback<void(size_t)>& success_callback,
      size_t size_written);

  // Called from WaitForData() when write operations can be performed
  // without blocking (the type of operation is provided in |mode|).
  BRILLO_PRIVATE void OnWriteAvailable(
      const void* buffer,
      size_t size,
      const base::Callback<void(size_t)>& success_callback,
      const ErrorCallback& error_callback,
      AccessMode mode);

  // Helper callbacks to implement ReadAllAsync/WriteAllAsync.
  BRILLO_PRIVATE void ReadAllAsyncCallback(
      void* buffer,
      size_t size_to_read,
      const base::Closure& success_callback,
      const ErrorCallback& error_callback,
      size_t size_read,
      bool eos);
  BRILLO_PRIVATE void WriteAllAsyncCallback(
      const void* buffer,
      size_t size_to_write,
      const base::Closure& success_callback,
      const ErrorCallback& error_callback,
      size_t size_written);

  // Helper callbacks to implement FlushAsync().
  BRILLO_PRIVATE void FlushAsyncCallback(
      const base::Closure& success_callback,
      const ErrorCallback& error_callback);

  // Data members for asynchronous read operations.
  bool is_async_read_pending_{false};

  // Data members for asynchronous write operations.
  bool is_async_write_pending_{false};

  base::WeakPtrFactory<Stream> weak_ptr_factory_{this};
  DISALLOW_COPY_AND_ASSIGN(Stream);
};

// A smart pointer to the stream used to pass the stream object around.
using StreamPtr = std::unique_ptr<Stream>;

}  // namespace brillo

#endif  // LIBBRILLO_BRILLO_STREAMS_STREAM_H_
