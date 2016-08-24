# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os, time
from autotest_lib.client.bin import test
from autotest_lib.client.common_lib import error, utils

class platform_DaemonsRespawn(test.test):
  version = 1


  def run_once(self):
    utils.system_output(self.bindir + "/test_respawn.sh",
                        retain_output=True)
