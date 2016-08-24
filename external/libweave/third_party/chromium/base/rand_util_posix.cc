// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/rand_util.h"

#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdint.h>
#include <unistd.h>

#include "base/logging.h"
#include "base/posix/eintr_wrapper.h"

namespace {

class URandomFd {
 public:
  URandomFd() : fd_(open("/dev/urandom", O_RDONLY)) {
    DCHECK_GE(fd_, 0) << "Cannot open /dev/urandom: " << errno;
  }

  ~URandomFd() { close(fd_); }

  int fd() const { return fd_; }

 private:
  const int fd_;
};

bool ReadFromFD(int fd, char* buffer, size_t bytes) {
  size_t total_read = 0;
  while (total_read < bytes) {
    ssize_t bytes_read =
        HANDLE_EINTR(read(fd, buffer + total_read, bytes - total_read));
    if (bytes_read <= 0)
      break;
    total_read += bytes_read;
  }
  return total_read == bytes;
}

}  // namespace

namespace base {

// NOTE: This function must be cryptographically secure. http://crbug.com/140076
uint64_t RandUint64() {
  uint64_t number;
  RandBytes(&number, sizeof(number));
  return number;
}

void RandBytes(void* output, size_t output_length) {
  URandomFd urandom_fd;
  const bool success =
      ReadFromFD(urandom_fd.fd(), static_cast<char*>(output), output_length);
  CHECK(success);
}

}  // namespace base
