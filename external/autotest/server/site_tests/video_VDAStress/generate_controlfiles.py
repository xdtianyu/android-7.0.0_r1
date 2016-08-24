#!/usr/bin/env python

"""
This file generates all video_VDAStress control files from a master list.
"""

import collections

Testdata = collections.namedtuple('Testdata', 'gs, decoder, access')

TESTS = [
    Testdata('gs://chromeos-test-assets-private/VDA/', 'h264', 'private'),
    Testdata('gs://chromeos-test-assets-private/VDA/', 'vp8', 'private'),
    # TODO(ihf): Populate public bucket with test videos.
    #Testdata('gs://chromiumos-test-assets-public/VDA/', 'h264', 'public'),
    #Testdata('gs://chromiumos-test-assets-public/VDA/', 'vp8', 'public'),
    #Testdata('gs://chromiumos-test-assets-public/VDA/', 'vp9', 'public'),
]

CONTROLFILE_TEMPLATE = (
"""# Copyright 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

AUTHOR = 'Chrome OS Team, chromeos-video@google.com'
NAME = 'video_VDAStress.{1}.{2}.{3}'
ATTRIBUTES = 'suite:video'
SUITE = 'video'
TIME = 'LENGTHY'
TEST_CATEGORY = 'Stress'
TEST_CLASS = 'video'
TEST_TYPE = 'server'
DEPENDENCIES = 'hw_video_acc_{1}'

DOC = \"\"\"
VDA stress test to download and run with {1} test videos from cloud storage.
\"\"\"

import shutil
import tempfile

# Download the test videos from the gs bucket to the server.
server_videos_dir = tempfile.mkdtemp(dir=job.tmpdir)
videos = []
job.run_test(
    'video_VDAStressSetup',
    gs_bucket='{0}{1}/',
    server_videos_dir=server_videos_dir,
    videos=videos,
    shard_number={3},
    shard_count={4})


def run(machine):
    job.run_test('video_VDAStress',
                 machine=machine,
                 server_videos_dir=server_videos_dir,
                 videos=videos)


job.parallel_on_machines(run, machines)
shutil.rmtree(server_videos_dir)""")


shard_count = 10
for test in TESTS:
  for shard_number in xrange(0, shard_count):
    filename = 'control.%s.%s.%d' % (test.decoder, test.access, shard_number)
    with open(filename, 'w+') as f:
      content = (
          CONTROLFILE_TEMPLATE.format(
              test.gs, test.decoder, test.access,
              shard_number, shard_count))
      f.write(content)
