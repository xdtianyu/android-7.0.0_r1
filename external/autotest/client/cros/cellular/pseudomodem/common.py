# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# setup the environment so that autotest_lib
# can be imported when this file is run as an
# executable

import os, sys

dirname = os.path.dirname(sys.modules[__name__].__file__)
client_dir = os.path.abspath(os.path.join(dirname, "..", "..", ".."))
sys.path.insert(0, client_dir)

import setup_modules

sys.path.pop(0)
setup_modules.setup(base_path=client_dir,
                    root_module_name="autotest_lib.client")
