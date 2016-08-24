// Copyright 2015 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBBRILLO_BRILLO_STREAMS_FAKE_STREAM_H_
#define LIBBRILLO_BRILLO_STREAMS_FAKE_STREAM_H_

#include <queue>
#include <string>

#include <base/callback_forward.h>
#include <base/macros.h>
#include <base/time/clock.h>
#include <base/time/time.h>
#include <brillo/secure_blob.h>
#include <brillo/streams/stream.h>

namespace brillo {

// Fake stream implementation for testing.
// This class allows to provide data for the stream in tests that can be later
// read through the Stream interface. Also, data written into the stream can be
// later inspected and verified.
//
// NOTE: This class provides a fake implementation for streams with separate
// input and output channels. That is, read and write operations do not affect
// each other. Also, the stream implementation is sequential only (no seeking).
// Good examples of a use case for fake stream are:
//  - read-only sequential streams (file, memory, pipe, ...)
//  - write-only sequential streams (same as above)
//  - independent channel read-write streams (sockets, ...)
//
// For more complex read/write stream test scenarios using a real MemoryStream
// or temporary FileStream is probably a better choice.
class FakeStream : public Stream {
 public:
  // Construct a new instance of the fake stream.
  //   mode        - expected read/write mode supported by the stream.
  //   clock       - the clock to use to get the current time.
  FakeStream(Stream::AccessMode mode,
             base::Clock* clock);

  // Add data packets to the read queue of the stream.
  // Optional |delay| indicates that the data packet should be delayed.
  void AddReadPacketData(base::TimeDelta delay, const void* data, size_t size);
  void AddReadPacketData(base::TimeDelta delay, brillo::Blob data);
  void AddReadPacketString(base::TimeDelta delay, const std::string& data);

  // Schedule a read error by adding a special error packet to the queue.
  void QueueReadError(base::TimeDelta delay);
  void QueueReadErrorWithMessage(base::TimeDelta delay,
                                 const std::string& message);

  // Resets read queue and clears any input data buffers.
  void ClearReadQueue();

  // Add expectations for output data packets to be written by the stream.
  // Optional |delay| indicates that the initial write operation for the data in
  // the packet should be delayed.
  // ExpectWritePacketSize just limits the size of output packet while
  // ExpectWritePacketData also validates that the data matches that of |data|.
  void ExpectWritePacketSize(base::TimeDelta delay, size_t data_size);
  void ExpectWritePacketData(base::TimeDelta delay,
                             const void* data,
                             size_t size);
  void ExpectWritePacketData(base::TimeDelta delay, brillo::Blob data);
  void ExpectWritePacketString(base::TimeDelta delay, const std::string& data);

  // Schedule a write error by adding a special error packet to the queue.
  void QueueWriteError(base::TimeDelta delay);
  void QueueWriteErrorWithMessage(base::TimeDelta delay,
                                  const std::string& message);

  // Resets write queue and clears any output data buffers.
  void ClearWriteQueue();

  // Returns the output data accumulated so far by all complete write packets,
  // or explicitly flushed.
  const brillo::Blob& GetFlushedOutputData() const;
  std::string GetFlushedOutputDataAsString() const;

  // Overrides from brillo::Stream.
  bool IsOpen() const override { return is_open_; }
  bool CanRead() const override;
  bool CanWrite() const override;
  bool CanSeek() const override { return false; }
  bool CanGetSize() const override { return false; }
  uint64_t GetSize() const override { return 0; }
  bool SetSizeBlocking(uint64_t size, ErrorPtr* error) override;
  uint64_t GetRemainingSize() const override { return 0; }
  uint64_t GetPosition() const override { return 0; }
  bool Seek(int64_t offset,
            Whence whence,
            uint64_t* new_position,
            ErrorPtr* error) override;

  bool ReadNonBlocking(void* buffer,
                       size_t size_to_read,
                       size_t* size_read,
                       bool* end_of_stream,
                       ErrorPtr* error) override;
  bool WriteNonBlocking(const void* buffer,
                        size_t size_to_write,
                        size_t* size_written,
                        ErrorPtr* error) override;
  bool FlushBlocking(ErrorPtr* error) override;
  bool CloseBlocking(ErrorPtr* error) override;
  bool WaitForData(AccessMode mode,
                   const base::Callback<void(AccessMode)>& callback,
                   ErrorPtr* error) override;
  bool WaitForDataBlocking(AccessMode in_mode,
                           base::TimeDelta timeout,
                           AccessMode* out_mode,
                           ErrorPtr* error) override;

 private:
  // Input data packet to be placed on the read queue.
  struct InputDataPacket {
    brillo::Blob data;  // Data to be read.
    base::TimeDelta delay_before;  // Possible delay for the first read.
    bool read_error{false};  // Set to true if this packet generates an error.
  };

  // Output data packet to be placed on the write queue.
  struct OutputDataPacket {
    size_t expected_size{0};  // Output packet size
    brillo::Blob data;  // Possible data to verify the output with.
    base::TimeDelta delay_before;  // Possible delay for the first write.
    bool write_error{false};  // Set to true if this packet generates an error.
  };

  // Check if there is any pending read data in the input buffer.
  bool IsReadBufferEmpty() const;
  // Pops the next read packet from the queue and sets its data into the
  // internal input buffer.
  bool PopReadPacket();

  // Check if the output buffer is full.
  bool IsWriteBufferFull() const;

  // Moves the current full output buffer into |all_output_data_|, clears the
  // buffer, and pops the information about the next expected output packet
  // from the write queue.
  bool PopWritePacket();

  bool is_open_{true};
  Stream::AccessMode mode_;
  base::Clock* clock_;

  // Internal data for read operations.
  std::queue<InputDataPacket> incoming_queue_;
  base::Time delay_input_until_;
  brillo::Blob input_buffer_;
  size_t input_ptr_{0};
  bool report_read_error_{false};

  // Internal data for write operations.
  std::queue<OutputDataPacket> outgoing_queue_;
  base::Time delay_output_until_;
  brillo::Blob output_buffer_;
  brillo::Blob expected_output_data_;
  size_t max_output_buffer_size_{0};
  bool report_write_error_{false};
  brillo::Blob all_output_data_;

  DISALLOW_COPY_AND_ASSIGN(FakeStream);
};

}  // namespace brillo

#endif  // LIBBRILLO_BRILLO_STREAMS_FAKE_STREAM_H_
