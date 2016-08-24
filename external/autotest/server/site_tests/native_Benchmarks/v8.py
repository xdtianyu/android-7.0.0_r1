# Copyright (c) 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os
import shutil

from autotest_lib.client.common_lib import error
from autotest_lib.server import utils

from native_Benchmarks_common import *

class v8(object):
    """Build and copy the v8 engine to client."""

    def __init__(self, scratch_srv, scratch_cli, client, flags_additional):
        self.src = "%s/v8" % scratch_srv

        # unpack
        cmd = 'tar jxf %s/v8.tar.bz2 -C %s' % (SERVER_TEST_ROOT, scratch_srv)
        run_check(utils, cmd, 'Error occurred while unpacking v8')

        # build
        arch = client.get_arch()
        flags = {}
        def_flag(flags, 'LDFLAGS', '-static')
        options = '-C %s i18nsupport=off snapshot=off -j40' % self.src
        if arch == 'armv7l':
            def_flag(flags, 'CXX', 'armv7a-cros-linux-gnueabi-g++')
            def_flag(flags, 'LINK', 'armv7a-cros-linux-gnueabi-g++')
            options += ' arm.release'
            d8src = '%s/out/arm.release/d8' % self.src
        elif arch == 'x86_64':
            def_flag(flags, 'CXX', 'x86_64-cros-linux-gnu-g++')
            def_flag(flags, 'LINK', 'x86_64-cros-linux-gnu-g++')
            options += ' x64.release'
            d8src = '%s/out/x64.release/d8' % self.src
        else:
            raise error.TestFail('Unknown cpu architecture: %s' % arch)
        for f, v in flags_additional.iteritems():
            def_flag(flags, f, v)
        envs = ' '.join('%s=%s' % (k, v) for k, v in flags.iteritems())
        cmd = '%s make %s' % (envs, options)

        run_check(utils, cmd, 'Error occurred building v8')
        if not os.path.isfile(d8src):
            raise error.TestFail('Unknown error when building v8')

        # copy
        d8dst = '%s/d8' % scratch_cli
        rcp_check(client, d8src, d8dst,
                  'Error occurred while sending d8 to client.\n')
        self.executable = d8dst

    def __del__(self):
        if os.path.isdir(self.src):
            shutil.rmtree(self.src)
