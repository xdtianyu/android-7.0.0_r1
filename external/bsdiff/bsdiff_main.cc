// Copyright 2015 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <err.h>

#include "bsdiff.h"

int main(int argc, char* argv[]) {
  if (argc != 4)
    errx(1, "usage: %s oldfile newfile patchfile\n", argv[0]);

  return bsdiff::bsdiff(argv[1], argv[2], argv[3]);
}
