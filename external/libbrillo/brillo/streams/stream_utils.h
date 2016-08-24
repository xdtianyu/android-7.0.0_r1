// Copyright 2015 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBBRILLO_BRILLO_STREAMS_STREAM_UTILS_H_
#define LIBBRILLO_BRILLO_STREAMS_STREAM_UTILS_H_

#include <base/location.h>
#include <brillo/brillo_export.h>
#include <brillo/streams/stream.h>

namespace brillo {
namespace stream_utils {

// Generates "Stream closed" error and returns false.
BRILLO_EXPORT bool ErrorStreamClosed(
    const tracked_objects::Location& location, ErrorPtr* error);

// Generates "Not supported" error and returns false.
BRILLO_EXPORT bool ErrorOperationNotSupported(
    const tracked_objects::Location& location, ErrorPtr* error);

// Generates "Read past end of stream" error and returns false.
BRILLO_EXPORT bool ErrorReadPastEndOfStream(
    const tracked_objects::Location& location, ErrorPtr* error);

// Generates "Operation time out" error and returns false.
BRILLO_EXPORT bool ErrorOperationTimeout(
    const tracked_objects::Location& location, ErrorPtr* error);

// Checks if |position| + |offset| fit within the constraint of positive
// signed int64_t type. We use uint64_t for absolute stream pointer positions,
// however many implementations, including file-descriptor-based I/O do not
// support the full extent of unsigned 64 bit numbers. So we restrict the file
// positions to what can fit in the signed 64 bit value (that is, we support
// "only" up to 9 exabytes, instead of the possible 18).
// The |location| parameter will be used to report the origin of the error
// if one is generated/triggered.
BRILLO_EXPORT bool CheckInt64Overflow(
    const tracked_objects::Location& location,
    uint64_t position,
    int64_t offset,
    ErrorPtr* error);

// Helper function to calculate the stream position based on the current
// stream position and offset. Returns true and the new calculated stream
// position in |new_position| if successful. In case of invalid stream
// position (negative values or out of range of signed 64 bit values), returns
// false and "invalid_parameter" |error|.
// The |location| parameter will be used to report the origin of the error
// if one is generated/triggered.
BRILLO_EXPORT bool CalculateStreamPosition(
    const tracked_objects::Location& location,
    int64_t offset,
    Stream::Whence whence,
    uint64_t current_position,
    uint64_t stream_size,
    uint64_t* new_position,
    ErrorPtr* error);

// Checks if |mode| allows read access.
inline bool IsReadAccessMode(Stream::AccessMode mode) {
  return mode == Stream::AccessMode::READ ||
         mode == Stream::AccessMode::READ_WRITE;
}

// Checks if |mode| allows write access.
inline bool IsWriteAccessMode(Stream::AccessMode mode) {
  return mode == Stream::AccessMode::WRITE ||
         mode == Stream::AccessMode::READ_WRITE;
}

// Make the access mode based on read/write rights requested.
inline Stream::AccessMode MakeAccessMode(bool read, bool write) {
  CHECK(read || write);  // Either read or write (or both) must be specified.
  if (read && write)
    return Stream::AccessMode::READ_WRITE;
  return write ? Stream::AccessMode::WRITE : Stream::AccessMode::READ;
}

using CopyDataSuccessCallback =
    base::Callback<void(StreamPtr, StreamPtr, uint64_t)>;
using CopyDataErrorCallback =
    base::Callback<void(StreamPtr, StreamPtr, const brillo::Error*)>;

// Asynchronously copies data from input stream to output stream until all the
// data from the input stream is read. The function takes ownership of both
// streams for the duration of the operation and then gives them back when
// either the |success_callback| or |error_callback| is called.
// |success_callback| also provides the number of bytes actually copied.
// This variant of CopyData uses internal buffer of 4 KiB for the operation.
BRILLO_EXPORT void CopyData(StreamPtr in_stream,
                            StreamPtr out_stream,
                            const CopyDataSuccessCallback& success_callback,
                            const CopyDataErrorCallback& error_callback);

// Asynchronously copies data from input stream to output stream until the
// maximum amount of data specified in |max_size_to_copy| is copied or the end
// of the input stream is encountered. The function takes ownership of both
// streams for the duration of the operation and then gives them back when
// either the |success_callback| or |error_callback| is called.
// |success_callback| also provides the number of bytes actually copied.
// |buffer_size| specifies the size of the read buffer to use for the operation.
BRILLO_EXPORT void CopyData(StreamPtr in_stream,
                            StreamPtr out_stream,
                            uint64_t max_size_to_copy,
                            size_t buffer_size,
                            const CopyDataSuccessCallback& success_callback,
                            const CopyDataErrorCallback& error_callback);

}  // namespace stream_utils
}  // namespace brillo

#endif  // LIBBRILLO_BRILLO_STREAMS_STREAM_UTILS_H_
