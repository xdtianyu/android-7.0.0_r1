#!/bin/bash -e
#
# Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
#
# Simple utility script for cleaning up old builds on the Dev Server. Should be
# run from the root of the archive directory.

declare -r NUM_BUILDS_KEPT=10
declare -r NUM_IMAGES_KEPT=3
declare -r IMAGE_NAME="chromiumos_test_image.bin"
declare -r BUILD_PATTERN="[0-9]*.[0-9]*.[0-9]*.[0-9]*"
declare -r NEW_BUILD_PATTERN="R[0-9]*-[0-9]*.[0-9]*.[0-9]*"
declare -r DEV_BUILD_PATTERN="[a-zA-Z]*-${BUILD_PATTERN}"

function cleanup_dir() {
  # First argument is the parent directory to look for builds under.
  local dirs=($(ls -d -t $1 2>/dev/null))
  # Second argument is the directory/build pattern to match against.
  local latest=$2
  latest=${latest:=2}

  for ((i=${latest}; i<${#dirs[@]}; i++)); do
    # delete those old ones.
    echo delete ${dirs[i]}
    rm -rf ${dirs[i]} || echo "Failed to remove ${dirs[i]}"
  done
}


for d in *; do
  if [ -d $d ]; then
    echo truncate ${d}
    # Cleanup stale image dirs.
    cleanup_dir "${d}/${BUILD_PATTERN}" ${NUM_BUILDS_KEPT}
    cleanup_dir "${d}/${DEV_BUILD_PATTERN}" ${NUM_BUILDS_KEPT}
    cleanup_dir "${d}/${NEW_BUILD_PATTERN}" ${NUM_BUILDS_KEPT}

    # Cleanup stale image files. Dev builds don't keep images.
    cleanup_dir "${d}/${BUILD_PATTERN}/${IMAGE_NAME}" ${NUM_IMAGES_KEPT}
    cleanup_dir "${d}/${NEW_BUILD_PATTERN}/${IMAGE_NAME}" ${NUM_IMAGES_KEPT}
  fi
done
