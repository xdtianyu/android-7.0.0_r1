# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Looks up all python files in this folder at runtime, and sets them to
# be loaded by the `import *` below.
import glob
import os
__all__ = [os.path.basename(f)[:-3] for f in
           glob.glob(os.path.dirname(__file__)+"/*.py")]

# Import all the files in this folder so that _registry gets filled in and
# force the import order to make sure registry, which is imported by all, is
# imported first.
import common
import registry
from . import *
