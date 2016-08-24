//
// Copyright (C) 2013 The Android Open Source Project
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

#include "shill/file_io.h"

#include <fcntl.h>
#include <unistd.h>

#include <base/posix/eintr_wrapper.h>

namespace shill {

namespace {

base::LazyInstance<FileIO> g_file_io = LAZY_INSTANCE_INITIALIZER;

}  // namespace

FileIO::FileIO() {}

FileIO::~FileIO() {}

// static
FileIO* FileIO::GetInstance() {
  return g_file_io.Pointer();
}

ssize_t FileIO::Write(int fd, const void* buf, size_t count) {
  return HANDLE_EINTR(write(fd, buf, count));
}

ssize_t FileIO::Read(int fd, void* buf, size_t count) {
  return HANDLE_EINTR(read(fd, buf, count));
}

int FileIO::Close(int fd) {
  return IGNORE_EINTR(close(fd));
}

int FileIO::SetFdNonBlocking(int fd) {
  const int flags = HANDLE_EINTR(fcntl(fd, F_GETFL)) | O_NONBLOCK;
  return HANDLE_EINTR(fcntl(fd, F_SETFL,  flags));
}

}  // namespace shill
