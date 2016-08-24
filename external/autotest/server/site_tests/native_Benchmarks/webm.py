# Copyright (c) 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os
import shutil

from autotest_lib.client.common_lib import error
from autotest_lib.server import utils

from native_Benchmarks_common import *

class webm(object):
    """Build and copy the codec to client."""

    def __init__(self, scratch_srv, scratch_cli, client, flags_additional):
        self.src = "%s/webm" % scratch_srv

        # unpack
        cmd = 'tar jxf %s/webm.tar.bz2 -C %s' % (SERVER_TEST_ROOT, scratch_srv)
        run_check(utils, cmd, 'Error occurred while unpacking webm')

        # build
        arch = client.get_arch()
        flags = {}
        def_flag(flags, 'LDFLAGS', '-static')
        options =  ' --disable-unit_tests'
        options += ' --disable-docs'
        options += ' --disable-runtime-cpu-detect'
        if arch == 'armv7l':
            def_flag(flags, 'CC', 'armv7a-cros-linux-gnueabi-gcc')
            def_flag(flags, 'CXX', 'armv7a-cros-linux-gnueabi-g++')
            def_flag(flags, 'LD', 'armv7a-cros-linux-gnueabi-g++')
            def_flag(flags, 'AR', 'armv7a-cros-linux-gnueabi-ar')
            def_flag(flags, 'AS', 'armv7a-cros-linux-gnueabi-as')
            options += ' --target=armv7-linux-gcc'
        elif arch == 'x86_64':
            def_flag(flags, 'CC', 'x86_64-cros-linux-gnu-gcc')
            def_flag(flags, 'CXX', 'x86_64-cros-linux-gnu-g++')
            def_flag(flags, 'LD', 'x86_64-cros-linux-gnu-g++')
            def_flag(flags, 'AR', 'x86_64-cros-linux-gnu-ar')
            options += ' --target=x86_64-linux-gcc'
        else:
            raise error.TestFail('Unknown cpu architecture: %s' % arch)
        for f, v in flags_additional.iteritems():
            def_flag(flags, f, v)
        envs = ' '.join('%s=%s' % (k, v) for k, v in flags.iteritems())
        cmd =  'mkdir -p %s/webm/out && ' % scratch_srv
        cmd += 'cd %s/webm/out && ' % scratch_srv
        cmd += ' %s ../configure %s && ' % (envs, options)
        cmd += 'make -j 40'

        run_check(utils, cmd, 'Error occurred building vpxenc')

        files = ['vpxenc', 'vpxdec']
        for v in files:
            if not os.path.isfile('%s/out/%s' % (self.src, v)):
                raise error.TestFail('Unknown error when building %s' % v)

        # copy
        for v in files:
            rcp_check(client, '%s/out/%s' % (self.src, v),
                      '%s/%s' % (scratch_cli, v),
                      'Error occurred while sending %s to client.' % v)
        self.vpxenc = '%s/vpxenc' % scratch_cli
        self.vpxdec = '%s/vpxdec' % scratch_cli

    def __del__(self):
        if os.path.isdir(self.src):
            shutil.rmtree(self.src)
