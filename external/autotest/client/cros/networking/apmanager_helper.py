# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os.path

def apmanager_is_installed():
    """@return True iff apmanager is installed in this system."""
    return os.path.exists('/usr/bin/apmanager')
