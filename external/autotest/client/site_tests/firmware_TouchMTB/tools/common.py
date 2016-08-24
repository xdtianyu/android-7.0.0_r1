# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""This module provides access to the namespace of the project home."""

import os, sys

cwd = os.path.dirname(sys.modules[__name__].__file__)
client_dir = os.path.abspath(os.path.join(cwd, '..'))
sys.path.insert(0, client_dir)
