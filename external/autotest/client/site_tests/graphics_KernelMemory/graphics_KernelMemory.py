# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
import time
from autotest_lib.client.bin import test
from autotest_lib.client.common_lib import error
from autotest_lib.client.cros.graphics import graphics_utils

class graphics_KernelMemory(test.test):
    """
    Reads from sysfs to determine kernel gem objects and memory info.
    """
    version = 1
    GSC = None

    def initialize(self):
        self.GSC = graphics_utils.GraphicsStateChecker()

    def run_once(self):
        # TODO(ihf): We want to give this test something well-defined to
        # measure. For now that will be the CrOS login-screen memory use.
        # We could also log into the machine using telemetry, but that is
        # still flaky. So for now we, lame as we are, just sleep a bit.
        time.sleep(10.0)

        keyvals = self.GSC.get_memory_keyvals()
        for key, val in keyvals.iteritems():
            self.output_perf_value(description=key, value=val,
                                   units='bytes', higher_is_better=False)
        self.GSC.finalize()
        self.write_perf_keyval(keyvals)
        # We should still be in the login screen and memory use > 0.
        if self.GSC.get_memory_access_errors() > 0:
            raise error.TestFail('Detected %d errors accessing graphics '
                                 'memory.' % self.GKM.num_errors)
