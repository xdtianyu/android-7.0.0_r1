/*
 * Copyright (c) 2015 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


static int write_marker(int fd, char *name)
{
  char buf[1024];
  struct timespec ts;
  int size, ret;
  unsigned long usec;

  ret = clock_gettime(CLOCK_MONOTONIC, &ts);
  if (ret < 0) {
    perror("clock_gettime");
    return 1;
  }

  // normalize nanoseconds down to microseconds
  // to make it easier to compare to the entry
  // timestamps
  usec = ts.tv_nsec / 1000;
  size = snprintf(buf, 1024, "%s: %lu.%06lu\n",
		  name, ts.tv_sec, usec);
  ret = write(fd, buf, size);
  if (ret < size) {
    perror("write");
    return 1;
  }

  return 0;
}
#define TRACE_PATH "/sys/kernel/debug/tracing/"

int main(int argc, char* argv[]) {
  int ret, fd;

  fd = open(TRACE_PATH "trace_marker", O_WRONLY);
  if (fd < 0) {
    perror("open");
    return 1;
  }
  ret = write_marker(fd, "start");
  if (ret)
    goto out;

  ret = write_marker(fd, "middle");
  if (ret)
    goto out;

  ret = write_marker(fd, "end");

 out:
  close(fd);
  return ret;
}
