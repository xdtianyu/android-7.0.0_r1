# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os
import sys

# Parent passes test file path as first argument.
test_file = sys.argv[1]

# We entered a mount namespace where |test_file| should not be accessible.
if os.access(test_file, os.F_OK):
    sys.exit(1)
