# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import os

from autotest_lib.server import autotest
from autotest_lib.server import hosts
from autotest_lib.server import test


class video_VDAStress(test.test):
    """
    VDA stress test run client video_VideoDecodeAccelerator tests on a list of
    videos.
    """
    version = 1

    def run_once(self, machine, server_videos_dir, videos):
        host = hosts.create_host(machine)
        host_at = autotest.Autotest(host)
        for video in videos:
            # Copy test vidoes from the server to the client.
            file_name, sep, video_arg = video.partition(':')
            file_path_at_server = os.path.join(server_videos_dir, file_name)
            file_path_at_client = '/tmp/%s' % file_name
            host.send_file(file_path_at_server, file_path_at_client)
            logging.info("Copied to the client: %s" % file_path_at_client)

            # Run the client test with the downloaded video.
            host_at.run_test('video_VideoDecodeAccelerator', videos=['%s%s%s' %
                             (file_path_at_client, sep, video_arg)],
                             use_cr_source_dir=False,
                             gtest_filter='DecodeVariations*\/0')
            host.run('rm %s' % file_path_at_client)
