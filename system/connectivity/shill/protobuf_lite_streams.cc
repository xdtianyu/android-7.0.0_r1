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

#include "shill/protobuf_lite_streams.h"

#include <fcntl.h>
#include <sys/types.h>

#include <base/files/file_util.h>
#include <base/posix/eintr_wrapper.h>

using google::protobuf::io::CopyingInputStream;
using google::protobuf::io::CopyingInputStreamAdaptor;
using std::string;

namespace shill {

CopyingInputStreamAdaptor* protobuf_lite_file_input_stream(
    const string& file_path) {
  int fd = HANDLE_EINTR(open(file_path.c_str(), O_RDONLY));
  if (fd == -1) {
    PLOG(ERROR) << __func__ << ": "
                << "Could not load protobuf file [" << file_path << "] ";
    return nullptr;
  }

  auto* file_stream(new ProtobufLiteCopyingFileInputStream(fd));
  auto* adaptor(new CopyingInputStreamAdaptor(
        static_cast<CopyingInputStream*>(file_stream)));
  // Pass ownership of |file_stream|.
  adaptor->SetOwnsCopyingStream(true);
  return adaptor;
}


ProtobufLiteCopyingFileInputStream::ProtobufLiteCopyingFileInputStream(int fd)
  : fd_(fd),
    scoped_fd_closer_(fd_),
    previous_seek_failed_(false) {}

ProtobufLiteCopyingFileInputStream::~ProtobufLiteCopyingFileInputStream() {}

int ProtobufLiteCopyingFileInputStream::Read(void* buffer, int size) {
  return HANDLE_EINTR(read(fd_, buffer, size));
}

int ProtobufLiteCopyingFileInputStream::Skip(int count) {
  if (!previous_seek_failed_ &&
      lseek(fd_, count, SEEK_CUR) != static_cast<off_t>(-1)) {
    // seek succeeded.
    return count;
  }
  // Let's not attempt to seek again later.
  previous_seek_failed_ = true;
  return CopyingInputStream::Skip(count);
}

}  // namespace shill
