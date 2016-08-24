# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Common to set up import path."""

import os, sys
dirname = os.path.dirname(sys.modules[__name__].__file__)
cros_dir = os.path.abspath(os.path.join(dirname, "..", ".."))
sys.path.insert(0, cros_dir)
