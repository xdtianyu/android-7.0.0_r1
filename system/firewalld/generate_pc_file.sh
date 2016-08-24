#!/bin/bash

# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

set -e

OUT=$1
shift
PC_IN=$1
shift
INCLUDE_DIR=$1
shift

sed \
  -e "s|@INCLUDE_DIR@|${INCLUDE_DIR}|g" \
  "${PC_IN}.pc.in" > "${OUT}/${PC_IN}.pc"
