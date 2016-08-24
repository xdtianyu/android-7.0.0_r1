// Copyright 2014 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <brillo/unittest_utils.h>

#include <fcntl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <base/logging.h>
#include <gtest/gtest.h>

namespace brillo {

const int ScopedPipe::kPipeSize = 4096;

ScopedPipe::ScopedPipe() {
  int fds[2];
  if (pipe(fds) != 0) {
    PLOG(FATAL) << "Creating a pipe()";
  }
  reader = fds[0];
  writer = fds[1];
  EXPECT_EQ(kPipeSize, fcntl(writer, F_SETPIPE_SZ, kPipeSize));
}

ScopedPipe::~ScopedPipe() {
  if (reader != -1)
    close(reader);
  if (writer != -1)
    close(writer);
}


ScopedSocketPair::ScopedSocketPair() {
  int fds[2];
  if (socketpair(PF_LOCAL, SOCK_STREAM, 0, fds) != 0) {
    PLOG(FATAL) << "Creating a socketpair()";
  }
  left = fds[0];
  right = fds[1];
}

ScopedSocketPair::~ScopedSocketPair() {
  if (left != -1)
    close(left);
  if (right != -1)
    close(right);
}

}  // namespace brillo
