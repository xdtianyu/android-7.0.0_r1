# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os.path

from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.cros import chrome_binary_test
from autotest_lib.client.cros.graphics import graphics_utils

class graphics_GLES2ConformChrome(chrome_binary_test.ChromeBinaryTest):
    """
    Run the Khronos GLES2 Conformance test suite against the Chrome GPU command
    buffer.
    """
    version = 1
    GSC = None
    BINARY = 'gles2_conform_test'

    def initialize(self):
        super(graphics_GLES2ConformChrome, self).initialize()
        self.GSC = graphics_utils.GraphicsStateChecker()

    def cleanup(self):
        super(graphics_GLES2ConformChrome, self).cleanup()
        if self.GSC:
            keyvals = self.GSC.get_memory_keyvals()
            for key, val in keyvals.iteritems():
                self.output_perf_value(description=key, value=val,
                                       units='bytes', higher_is_better=False)
            self.GSC.finalize()
            self.write_perf_keyval(keyvals)

    def run_once(self):
        # TODO(ihf): Remove this once GLES2ConformChrome works on freon.
        if utils.is_freon():
            raise error.TestNAError(
                   'Test needs work on Freon. See crbug.com/484463.')

        if not os.path.exists(self.get_chrome_binary_path(self.BINARY)):
            raise error.TestFail('%s not found. Use internal Chrome sources!' %
                                 self.BINARY)

        self.run_chrome_test_binary(self.BINARY)
