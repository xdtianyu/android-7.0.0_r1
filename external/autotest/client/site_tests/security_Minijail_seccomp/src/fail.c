// Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <asm/unistd.h>

#define SIZE 1024

int main(int argc, char **argv) {
  char buf[SIZE];
  int fd_z = syscall(__NR_open, "/dev/zero", O_RDONLY);
  int fd_n = syscall(__NR_open, "/dev/null", O_RDONLY);
  int nr = syscall(__NR_read, fd_z, buf, SIZE);
  int nw = syscall(__NR_write, fd_n, buf, SIZE);
  syscall(__NR_close, fd_z);
  syscall(__NR_close, fd_n);
  syscall(__NR_exit, 0);
}
