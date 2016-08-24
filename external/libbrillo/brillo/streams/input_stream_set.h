// Copyright 2015 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBBRILLO_BRILLO_STREAMS_INPUT_STREAM_SET_H_
#define LIBBRILLO_BRILLO_STREAMS_INPUT_STREAM_SET_H_

#include <vector>

#include <base/macros.h>
#include <brillo/brillo_export.h>
#include <brillo/streams/stream.h>

namespace brillo {

// Multiplexer stream allows to bundle a bunch of secondary streams in one
// logical stream and simulate a read operation across data concatenated from
// all those source streams.
//
// When created on a set of source streams like stream1, stream2, stream3, etc.,
// reading from the multiplexer stream will read all the data from stream1 until
// end-of-stream is reached, then keep reading from stream2, stream3 and so on.
//
// InputStreamSet has an option of owning the underlying source streams
// or just referencing them. Owned streams are passed to InputStreamSet
// with exclusive ownership transfer (using StreamPtr) and those streams will
// be closed/destroyed when InputStreamSet is closed/destroyed.
// Referenced source streams' life time is maintained elsewhere and they must
// be valid for the duration of InputStreamSet's life. Closing the
// muliplexer stream does not close the referenced streams.
class BRILLO_EXPORT InputStreamSet : public Stream {
 public:
  // == Construction ==========================================================

  // Generic method that constructs a multiplexer stream on a list of source
  // streams. |source_streams| is the list of all source stream references
  // in the order they need to be read from. |owned_source_streams| is a list
  // of source stream instances that the multiplexer stream will own.
  // Note that the streams from |owned_source_streams| should still be
  // referenced in |source_streams| if you need their data to be read from.
  // |owned_source_streams| could be empty (in which case none of the source
  // streams are not owned), or contain fewer items than in |source_streams|.
  static StreamPtr Create(std::vector<Stream*> source_streams,
                          std::vector<StreamPtr> owned_source_streams,
                          ErrorPtr* error);

  // Simple helper method to create a multiplexer stream with a list of
  // referenced streams. None of the streams will be owned.
  // Effectively calls Create(source_streams, {}, error);
  static StreamPtr Create(std::vector<Stream*> source_streams, ErrorPtr* error);

  // Simple helper method to create a multiplexer stream with a list of
  // referenced streams. None of the streams will be owned.
  // Effectively calls Create(source_streams, owned_source_streams, error)
  // with |source_streams| containing pointers to the streams from
  // |owned_source_streams| list.
  static StreamPtr Create(std::vector<StreamPtr> owned_source_streams,
                          ErrorPtr* error);

  // == Stream capabilities ===================================================
  bool IsOpen() const override;
  bool CanRead() const override { return true; }
  bool CanWrite() const override { return false; }
  bool CanSeek() const override { return false; }
  bool CanGetSize() const override;

  // == Stream size operations ================================================
  uint64_t GetSize() const override;
  bool SetSizeBlocking(uint64_t size, ErrorPtr* error) override;
  uint64_t GetRemainingSize() const override;

  // == Seek operations =======================================================
  uint64_t GetPosition() const override { return 0; }
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
  bool FlushBlocking(ErrorPtr* /* error */) override { return true; }
  bool CloseBlocking(ErrorPtr* error) override;

  // == Data availability monitoring ==========================================
  bool WaitForData(AccessMode mode,
                   const base::Callback<void(AccessMode)>& callback,
                   ErrorPtr* error) override;

  bool WaitForDataBlocking(AccessMode in_mode,
                           base::TimeDelta timeout,
                           AccessMode* out_mode,
                           ErrorPtr* error) override;

  void CancelPendingAsyncOperations() override;

 private:
  friend class InputStreamSetTest;

  // Internal constructor used by the Create() factory methods.
  InputStreamSet(std::vector<Stream*> source_streams,
                 std::vector<StreamPtr> owned_source_streams,
                 uint64_t initial_stream_size);

  // List of streams to read data from.
  std::vector<Stream*> source_streams_;

  // List of source streams this stream owns. Owned source streams will be
  // closed when InputStreamSet::CloseBlocking() is called and will be
  // destroyed when this stream is destroyed.
  std::vector<StreamPtr> owned_source_streams_;

  uint64_t initial_stream_size_{0};
  bool closed_{false};

  DISALLOW_COPY_AND_ASSIGN(InputStreamSet);
};

}  // namespace brillo

#endif  // LIBBRILLO_BRILLO_STREAMS_INPUT_STREAM_SET_H_
