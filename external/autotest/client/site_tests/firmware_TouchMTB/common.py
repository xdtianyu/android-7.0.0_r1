# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""This module provides access to autotest_lib.client.common_lib.cros namespace,
and fixes the environment variable for shared libraries (DLL).
"""

import os, sys

cwd = os.path.dirname(sys.modules[__name__].__file__)
client_dir = os.path.abspath(os.path.join(cwd, '../../common_lib/cros'))
sys.path.insert(0, client_dir)

DLL_PATH_ENV_NAME = 'LD_LIBRARY_PATH'
DLL_PATH = '/usr/local/lib:/usr/local/lib64'
if not os.environ.get(DLL_PATH_ENV_NAME):
    print 'Set up %s!' % DLL_PATH_ENV_NAME
    os.environ[DLL_PATH_ENV_NAME] = DLL_PATH
    try:
        # Note: It is required to restart the process since the linker/loader
        #       has kept a copy of the environment in its cache.
        print 'Restarting "%s"....' % sys.argv[0]
        os.execv(sys.argv[0], sys.argv)
    except Exception as e:
        print 'Error: Failed to restart %s' % sys.argv[0], e
        sys.exit(1)
