// Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <asm/unistd.h>

void usage(const char *comm) {
  fprintf(stderr, "Usage: %s <access mode>\n", comm);
  fprintf(stderr, "\tAccess mode: 0-O_RDONLY, 1-O_WRONLY, 2-O_RDWR\n");
  return;
}

int main(int argc, char **argv) {
  if (argc < 2) {
    usage(argv[0]);
    return 1;
  }

  unsigned int access_mode = strtoul(argv[1], NULL, 0);
  if (access_mode < 0 || access_mode > 2) {
    usage(argv[0]);
    return 1;
  }

  char *path;
  int flags;

  switch (access_mode) {
    case 0:
      path = "/dev/zero";
      flags = O_RDONLY;
      break;
    case 1:
      path = "/dev/null";
      flags = O_WRONLY;
      break;
    case 2:
      path = "/dev/null";
      flags = O_RDWR;
      break;
    default:
      usage(argv[0]);
      return 1;
  }

  int fd = syscall(__NR_open, path, flags);
  syscall(__NR_close, fd);
  syscall(__NR_exit, 0);
}
