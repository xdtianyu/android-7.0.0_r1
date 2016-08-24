# Copyright (c) 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os

from autotest_lib.client.cros import chrome_binary_test

BINARY = 'media_unittests'

class video_MediaUnittests(chrome_binary_test.ChromeBinaryTest):
    """
    This test is a wrapper of the chrome unittest binary: media_unittests.
    """

    version = 1


    def run_once(self):
        """
        Runs media_unittests.
        """
        cmd_line = '--brave-new-test-launcher --test-launcher-bot-mode'
        output = os.path.join(self.resultsdir, 'test-launcher-summary')
        cmd_line = '%s --test-launcher-summary-output=%s' % (cmd_line, output)
        self.run_chrome_test_binary(BINARY, cmd_line)
