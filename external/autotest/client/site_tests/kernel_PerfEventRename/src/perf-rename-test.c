/* Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/prctl.h>
#include <pthread.h>
#include <errno.h>

// little crc-like thing, compiler won't optimize it out
static int do_something(int seed, unsigned int loops) {
  int i;
  for (i = 0; i < loops; i++) {
    seed ^= i;
    seed = (seed << 1) ^ (i & 0x80000000 ? 0x04C11DB7 : 0);
  }
  return i;
}


int main(int argc, char* argv[]) {
  int loops;
  char *name;

  if (argc < 3) {
    fprintf(stderr, "usage: <name> <loops>\n");
    return 1;
  }

  name = argv[1];
  loops = strtoul(argv[2], NULL, 10);

  if (prctl(PR_SET_NAME, name) < 0) {
    perror("prctl(PR_SET_NAME)");
    return 1;
  }
  do_something(rand(), loops);

  return 0;
}
