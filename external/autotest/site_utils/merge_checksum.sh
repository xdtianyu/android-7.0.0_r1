#!/bin/bash

# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# This script takes a checksum file and merges it into the packages
# checksum file in ../packages/packages.checksum.

# This script is thread-safe.

set -e

function main () {
  local merge_file="$1"
  local packages_dir="$(dirname $0)/../packages"
  local checksum_file="${packages_dir}/packages.checksum"

  # Preparatory work.
  mkdir -p "${packages_dir}"
  touch ${checksum_file}

  if [ ! -f "${merge_file}" ]; then
    return
  fi

  # This operation is performed using an flock on the packages dir
  # to allow it to run concurrently.
  flock "${packages_dir}" \
    -c "sort -k2,2 -u ${merge_file} ${checksum_file} -o ${checksum_file}"
}

if [ $# != 1 ]; then
  echo "Not enough arguments."
  exit 1
fi

main $1
