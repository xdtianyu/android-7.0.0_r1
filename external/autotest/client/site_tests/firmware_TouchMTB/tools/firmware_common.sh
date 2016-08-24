#!/bin/sh

# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# A die function to print the message and then exit
die() {
  echo -e "$@"
  exit 1
}

# Make a directory if it does not exist yet.
# Remove files in the directory if any.
make_empty_dir() {
  local dir="$1"
  mkdir -p "$dir"
  rm -fr "$dir"/*
}

# Source the cros common script
source_cros_common_script() {
  CROS_SCRIPT_ROOT="/usr/lib/crosutils"
  test -d "$CROS_SCRIPT_ROOT" || \
      die "Path $CROS_SCRIPT_ROOT not found. Make sure you are in chroot."
  . "${CROS_SCRIPT_ROOT}/common.sh"
}
