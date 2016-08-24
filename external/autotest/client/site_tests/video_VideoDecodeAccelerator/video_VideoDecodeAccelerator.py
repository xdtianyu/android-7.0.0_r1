# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import os
from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.cros import chrome_binary_test

class video_VideoDecodeAccelerator(chrome_binary_test.ChromeBinaryTest):
    """
    This test is a wrapper of the chrome unittest binary:
    video_decode_accelerator_unittest.
    """

    version = 1
    binary = 'video_decode_accelerator_unittest'


    @chrome_binary_test.nuke_chrome
    def run_once(self, videos, use_cr_source_dir=True, gtest_filter=''):
        """
        Runs video_decode_accelerator_unittest on the videos.

        @param videos: The test videos for video_decode_accelerator_unittest.
        @param use_cr_source_dir:  Videos are under chrome source directory.
        @param gtest_filter: gtest_filter parameter for the unittest.

        @raises: error.TestFail for video_decode_accelerator_unittest failures.
        """
        logging.debug('Starting video_VideoDecodeAccelerator: %s', videos)

        if use_cr_source_dir:
            path = os.path.join(self.cr_source_dir, 'media', 'test', 'data', '')
        else:
            path = ''

        last_test_failure = None
        for video in videos:
            cmd_line = ('--test_video_data="%s%s"' % (path, video))

            if gtest_filter:
              cmd_line = '%s --gtest_filter=%s' % (cmd_line, gtest_filter)

            if utils.is_freon():
                cmd_line += ' --ozone-platform=gbm'

            try:
                self.run_chrome_test_binary(self.binary, cmd_line)
            except error.TestFail as test_failure:
                # Continue to run the remaining test videos and raise
                # the last failure after finishing all videos.
                logging.error('%s: %s', video, test_failure.message)
                last_test_failure = test_failure

        if last_test_failure:
            raise last_test_failure
