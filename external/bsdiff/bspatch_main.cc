// Copyright 2015 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <err.h>
#include <stdlib.h>

#include "bspatch.h"

#define USAGE_TEMPLATE_STR                                          \
  "usage: %s oldfile newfile patchfile [old-extents new-extents]\n" \
  "with extents taking the form \"off_1:len_1,...,off_n:len_n\"\n"

int main(int argc, char* argv[]) {
  const char* old_extents = NULL;
  const char* new_extents = NULL;

  if ((argc != 6) && (argc != 4))
    errx(1, USAGE_TEMPLATE_STR, argv[0]);

  if (argc == 6) {
    old_extents = argv[4];
    new_extents = argv[5];
  }

  return bsdiff::bspatch(argv[1], argv[2], argv[3], old_extents, new_extents);
}
