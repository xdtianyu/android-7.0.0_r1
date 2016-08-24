// Copyright 2015 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBBRILLO_BRILLO_STREAMS_FILE_STREAM_H_
#define LIBBRILLO_BRILLO_STREAMS_FILE_STREAM_H_

#include <base/files/file_path.h>
#include <base/macros.h>
#include <brillo/brillo_export.h>
#include <brillo/streams/stream.h>

namespace brillo {

// FileStream class provides the implementation of brillo::Stream for files
// and file-descriptor-based streams, such as pipes and sockets.
// The FileStream class cannot be instantiated by clients directly. However
// they should use the static factory methods such as:
//  - FileStream::Open(): to open a file by name.
//  - FileStream::CreateTemporary(): to create a temporary file stream.
//  - FileStream::FromFileDescriptor(): to create a stream using an existing
//    file descriptor.
class BRILLO_EXPORT FileStream : public Stream {
 public:
  // See comments for FileStream::Open() for detailed description of this enum.
  enum class Disposition {
    OPEN_EXISTING,  // Open existing file only. Fail if doesn't exist.
    CREATE_ALWAYS,  // Create empty file, possibly overwriting existing file.
    CREATE_NEW_ONLY,  // Create new file if doesn't exist already.
    TRUNCATE_EXISTING,  // Open/truncate existing file. Fail if doesn't exist.
  };

  // Simple interface to wrap native library calls so that they can be mocked
  // out for testing.
  struct FileDescriptorInterface {
    using DataCallback = base::Callback<void(Stream::AccessMode)>;

    virtual ~FileDescriptorInterface() = default;

    virtual bool IsOpen() const = 0;
    virtual ssize_t Read(void* buf, size_t nbyte) = 0;
    virtual ssize_t Write(const void* buf, size_t nbyte) = 0;
    virtual off64_t Seek(off64_t offset, int whence) = 0;
    virtual mode_t GetFileMode() const = 0;
    virtual uint64_t GetSize() const = 0;
    virtual int Truncate(off64_t length) const = 0;
    virtual int Close() = 0;
    virtual bool WaitForData(AccessMode mode,
                             const DataCallback& data_callback,
                             ErrorPtr* error) = 0;
    virtual int WaitForDataBlocking(AccessMode in_mode,
                                    base::TimeDelta timeout,
                                    AccessMode* out_mode) = 0;
    virtual void CancelPendingAsyncOperations() = 0;
  };

  // == Construction ==========================================================

  // Opens a file at specified |path| for reading, writing or both as indicated
  // by |mode|. The |disposition| specifies how the file must be opened/created:
  //  - OPEN_EXISTING   - opens the existing file and keeps its content intact.
  //                      The seek pointer is at the beginning of the file.
  //  - CREATE_ALWAYS   - creates the file always. If it exists, the file is
  //                      truncated.
  //  - CREATE_NEW_ONLY - creates a new file only if it doesn't exist. Fails
  //                      otherwise. This can be useful for creating lock files.
  //  - TRUNCATE_EXISTING - opens existing file and truncates it to zero length.
  //                       Fails if the file doesn't already exist.
  // If successful, the open file stream is returned. Otherwise returns the
  // stream pointer containing nullptr and fills in the details of the error
  // in |error| object, if provided.
  static StreamPtr Open(const base::FilePath& path,
                        AccessMode mode,
                        Disposition disposition,
                        ErrorPtr* error);

  // Creates a temporary unnamed file and returns a stream to it. The file will
  // be deleted when the stream is destroyed.
  static StreamPtr CreateTemporary(ErrorPtr* error);

  // Creates a file stream based on existing file descriptor. The file
  // descriptor will be set into non-blocking mode and will be owned by the
  // resulting stream (and closed when the stream is destroyed).
  // If the function fails, returns a null stream pointer and sets the error
  // details to |error| object. Also note that it is the caller's responsibility
  // to close the file descriptor if this function fails, since the stream
  // hasn't been created yet and didn't take ownership of the file descriptor.
  // |own_descriptor| indicates whether the stream must close the underlying
  // file descriptor when its CloseBlocking() method is called. This should be
  // set to false for file descriptors that shouldn't be closed (e.g. stdin).
  static StreamPtr FromFileDescriptor(int file_descriptor,
                                      bool own_descriptor,
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

  // Override for Stream::WaitForData to start watching the associated file
  // descriptor for non-blocking read/write operations.
  bool WaitForData(AccessMode mode,
                   const base::Callback<void(AccessMode)>& callback,
                   ErrorPtr* error) override;

  // Runs select() on the file descriptor to wait until we can do non-blocking
  // I/O on it.
  bool WaitForDataBlocking(AccessMode in_mode,
                           base::TimeDelta timeout,
                           AccessMode* out_mode,
                           ErrorPtr* error) override;

  // Cancels pending asynchronous read/write operations.
  void CancelPendingAsyncOperations() override;

 private:
  friend class FileStreamTest;

  // Internal constructor used by the factory methods Open(), CreateTemporary(),
  // and FromFileDescriptor().
  FileStream(std::unique_ptr<FileDescriptorInterface> fd_interface,
             AccessMode mode);

  // Wrapper for the file descriptor. Used in testing to mock out the real
  // file system APIs.
  std::unique_ptr<FileDescriptorInterface> fd_interface_;

  // The access mode this stream is open with.
  AccessMode access_mode_{AccessMode::READ_WRITE};

  // Set to false for streams that are guaranteed non-seekable.
  bool seekable_{true};

  // Set to false for streams that have unknown size.
  bool can_get_size_{false};

  DISALLOW_COPY_AND_ASSIGN(FileStream);
};

}  // namespace brillo

#endif  // LIBBRILLO_BRILLO_STREAMS_FILE_STREAM_H_
