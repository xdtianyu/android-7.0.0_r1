# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import json
import math
import os

from autotest_lib.server import test
from autotest_lib.server import utils

VIDEO_LIST = '__test_video_list'


class video_VDAStressSetup(test.test):
    """
    Setup for VDA stress test's server by coping a list of videos from a gs
    bucket to the server.
    """
    version = 1

    def run_once(self, gs_bucket, server_videos_dir, videos, shard_number,
                 shard_count):
        if not gs_bucket.endswith('/'):
            gs_bucket += '/'

        # Probably should not use os.path.join for gs:// paths.
        gs_video_list = '%s%s' % (gs_bucket, VIDEO_LIST)
        local_video_list = os.path.join(server_videos_dir, VIDEO_LIST)
        try:
            utils.system('gsutil cp %s %s' % (gs_video_list, local_video_list))
            videos.extend(json.load(open(local_video_list)))
        finally:
            os.remove(local_video_list)

        # Break test_video_list into equal sized shards numbered 0 and only
        # download shard_number.
        video_count = len(videos)
        shard_size = int(math.ceil(video_count / float(shard_count)))
        begin = shard_size * shard_number
        end = min(video_count, shard_size * (shard_number + 1))
        # Enforce sorting even if VIDEO_LIST file is not sorted.
        videos.sort()
        videos = videos[begin:end]

        for video in videos:
            file_name, _, _ = video.partition(':')
            utils.system('gsutil cp %s %s' %
                         ('%s%s' % (gs_bucket, file_name), server_videos_dir))
