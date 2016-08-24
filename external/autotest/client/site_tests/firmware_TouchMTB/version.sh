#!/bin/sh

# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Read command flags
. /usr/share/misc/shflags
DEFINE_string "remote" "" "remote machine IP address" "r"

PROG=$0
FLAGS_HELP=\
"USAGE: $PROG [flags]

Examples:
  # Generate the version info and scp it to the IP address.
  $ $PROG -r 100.20.300.123
"

FLAGS "$@" || exit 1
eval set -- "${FLAGS_ARGV}"
set -e


PROJ="firmware_TouchMTB"
TEST_DIR="/usr/local/autotest/tests/${PROJ}"
VERSION_FILE="/tmp/.version"

# Print an error message and exit.
die() {
  echo "$@" > /dev/stderr
  exit 1
}

if [ -z ${FLAGS_remote} ]; then
  die "Error: you need to provide the IP address of the test machine."
fi

create_version="`dirname $PROG`"/tools/create_version.py
if ! $create_version "$VERSION_FILE"; then
  die "Error: failed to create version info"
fi

expect_scp="`dirname $PROG`"/tools/expect_scp
if ! $expect_scp "root@${FLAGS_remote}:${TEST_DIR}" "$VERSION_FILE"; then
  die "Error: scp version file $VERSION_FILE to ${FLAGS_remote}:${TEST_DIR}"
fi

rm -fr "$VERSION_FILE"
