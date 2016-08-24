#!/bin/bash
# Copyright 2015 The Weave Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Instead of this script, try running "make all -j" and "make testall".
# TODO: Delete this file after 15-feb-2016.

DIR=$(cd -P -- "$(dirname -- "$0")" && pwd -P)
ROOT_DIR=$(cd -P -- "$(dirname -- "$0")/.." && pwd -P)

cd $ROOT_DIR

make all -j
