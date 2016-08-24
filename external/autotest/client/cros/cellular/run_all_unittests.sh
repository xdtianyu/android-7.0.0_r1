#!/bin/sh
# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

python -m unittest  discover -v --pattern=*test_noautorun.py > /dev/null
