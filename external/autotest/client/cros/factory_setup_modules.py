# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Sets up the cros.factory module path.  This is necessary since there
# is already a cros directory, and we need to rejigger things so that
# cros.factory points to the correct path.

import imp, logging, os, sys

# If SYSROOT is present, also look in
# $SYSROOT/usr/local/factory/py_pkg (necessary during the build step).
sysroot = os.environ.get('SYSROOT')
extra_path = ([os.path.join(sysroot, 'usr/local/factory/py_pkg')]
              if sysroot else [])

# Try to import cros, or just create a dummy module if it doesn't
# exist.
try:
    import cros
except:
    cros = imp.load_module('cros', None, '', ('', '', imp.PKG_DIRECTORY))

# Load cros.factory, inserting it into the cros module.
cros.factory = imp.load_module(
    'cros.factory',
    *imp.find_module('cros/factory', sys.path + extra_path))
