// Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

int recbomb(int n);
void PrepareBelow(int argc, char *argv[]);
extern int DefeatTailOptimizationForCrasher();
int DefeatTailOptimizationForBomb() {
  return 0;
}

int main(int argc, char *argv[]) {
  PrepareBelow(argc, argv);
  return recbomb(16) + DefeatTailOptimizationForCrasher();
}

// Prepare for doing the crash, but do it below main so that main's
// line numbers remain stable.
void PrepareBelow(int argc, char *argv[]) {
  fprintf(stderr, "pid=%lld\n", static_cast<long long>(getpid()));
  fflush(stderr);
  if (argc == 2 && strcmp(argv[1], "--nocrash") == 0) {
    fprintf(stderr, "Doing normal exit\n");
    // Just exit with an error code if requested, to test that
    // CrashDumper cleanup does not cause troubles.
    exit(0);
  }
  fprintf(stderr, "Crashing as requested.\n");
}
