# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os.path

from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.cros import chrome_binary_test
from autotest_lib.client.cros.graphics import graphics_utils

class graphics_KhronosGLCTSChrome(chrome_binary_test.ChromeBinaryTest):
    """
    Run the Khronos GL-CTS test suite against the Chrome GPU command
    buffer.
    """
    version = 1
    GSC = None
    BINARY = 'khronos_glcts_test'

    def initialize(self):
        super(graphics_KhronosGLCTSChrome, self).initialize()
        self.GSC = graphics_utils.GraphicsStateChecker()

    def cleanup(self):
        super(graphics_KhronosGLCTSChrome, self).cleanup()
        if self.GSC:
            self.GSC.finalize()

    def run_once(self):
        # TODO(ihf): Remove this once KhronosGLCTSChrome works on freon.
        if utils.is_freon():
            raise error.TestNAError(
                'Test needs work on Freon. See crbug.com/484467.')

        if not os.path.exists(self.get_chrome_binary_path(self.BINARY)):
            raise error.TestFail('%s not found. Use internal Chrome sources!' %
                                 self.BINARY)

        log_file = os.path.join(self.resultsdir, self.BINARY + ".xml")
        bin_args = '--gtest_output=xml:%s %s' % (log_file, self.resultsdir)

        self.run_chrome_test_binary(self.BINARY, bin_args)
