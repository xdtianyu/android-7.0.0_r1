//
// Copyright (C) 2014 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

#ifndef SHILL_PROTOBUF_LITE_STREAMS_H_
#define SHILL_PROTOBUF_LITE_STREAMS_H_

#include <string>

#include <base/files/scoped_file.h>
#include <google/protobuf/io/zero_copy_stream_impl_lite.h>

// Some basic input/output streams are not implemented for protobuf-lite.

namespace shill {

// Attempts to create a |google::protobuf::io::CopyingInputStreamAdaptor| using
// a |shill::ProtobufLiteCopyingFileInputStream|. Returns a new instance on
// success. The caller owns the new instance, and must free it when done.
// Returns nullptr on failure.
google::protobuf::io::CopyingInputStreamAdaptor *
protobuf_lite_file_input_stream(const std::string& file_path);


class ProtobufLiteCopyingFileInputStream :
    public google::protobuf::io::CopyingInputStream {
 public:
  // Takes ownership of |fd| and closes it when the object is deleted.
  explicit ProtobufLiteCopyingFileInputStream(int fd);
  ~ProtobufLiteCopyingFileInputStream() override;
  int Read(void* buffer, int size) override;
  int Skip(int count) override;
 private:
  int fd_;
  base::ScopedFD scoped_fd_closer_;
  bool previous_seek_failed_;

  DISALLOW_COPY_AND_ASSIGN(ProtobufLiteCopyingFileInputStream);
};

}  // namespace shill

#endif  // SHILL_PROTOBUF_LITE_STREAMS_H_
