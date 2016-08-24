# Copyright (c) 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from native_Benchmarks_common import *
from webm import webm

class vp8(object):
    """Build webm codec (vpxenc/vpxdec) and run them on client"""
    def __init__(self, scratch_srv, scratch_cli, client, args):
        # Instantiating webm builds the codec.
        self.webm = webm(scratch_srv, scratch_cli, client, args)
        self.client = client
        self.scratch_cli = scratch_cli

        # download
        src = '%s/vp8.webm' % SERVER_TEST_ROOT
        dst = '%s/vp8.webm' % scratch_cli
        rcp_check(client, src, dst,
                  'Error occurred while sending vp8.webm to client.\n')

    def run(self):
        """Returns perf_value tuples"""
        # run decoder
        cmd = ('%s --summary %s/vp8.webm -o %s/vp8.yuv 2>&1' %
               (self.webm.vpxdec, self.scratch_cli, self.scratch_cli))
        declog = run_check(self.client, cmd, "Error occurred while running vp8")
        # run encoder
        cmd = (('%s %s/vp8.yuv -o /dev/null --codec=vp8 --i420 -w 1280' +
                ' -h 720 --good --cpu-used=0 --target-bitrate=2000 2>&1') %
               (self.webm.vpxenc, self.scratch_cli))
        enclog = run_check(self.client, cmd,
                           "Error occurred while running vp8enc")
        return self.parse(declog, enclog)

    def parse(self, dec, enc):
        """Translate logs into perf_values tuples.
        @param dec: logs from decoder
        @param enc: logs from encoder
        """
        return [{'description': 'VP8',
                 'graph': 'decode',
                 'value': dec.split()[-2][1:],
                 'units': 'fps'},
                {'description': 'VP8',
                 'graph': 'encode',
                 'value': enc.split()[-2][1:],
                 'units': 'fps'}]

    def __del__(self):
        run_check(self.client, 'rm -f %s/vp8.yuv' % self.scratch_cli,
                  "Error occurred while cleaning up")
