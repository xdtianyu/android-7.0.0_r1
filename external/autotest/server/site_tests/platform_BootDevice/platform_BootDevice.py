# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
from autotest_lib.server import test, autotest

class platform_BootDevice(test.test):
   version = 1

   def run_once(self, host=None):
     self.client = host

     # Reboot the client
     logging.info('BootDevice: reboot %s' % self.client.hostname)
     self.client.reboot()
