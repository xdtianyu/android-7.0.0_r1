# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import os
import re

from autotest_lib.client.bin import test, utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.cros import service_stopper
from autotest_lib.client.cros.graphics import graphics_utils


class graphics_SanAngeles(test.test):
    """
    Benchmark OpenGL object rendering.
    """
    version = 2
    preserve_srcdir = True
    GSC = None

    def setup(self):
        os.chdir(self.srcdir)
        utils.make('clean')
        utils.make('all')

    def initialize(self):
        self.GSC = graphics_utils.GraphicsStateChecker()
        if utils.is_freon():
            # If UI is running, we must stop it and restore later.
            self._services = service_stopper.ServiceStopper(['ui'])
            self._services.stop_services()

    def cleanup(self):
        if utils.is_freon() and self._services:
            self._services.restore_services()
        if self.GSC:
            keyvals = self.GSC.get_memory_keyvals()
            for key, val in keyvals.iteritems():
                self.output_perf_value(description=key, value=val,
                                       units='bytes', higher_is_better=False)
            self.GSC.finalize()
            self.write_perf_keyval(keyvals)

    def run_once(self):
        cmd_gl = os.path.join(self.srcdir, 'SanOGL')
        cmd_gles = os.path.join(self.srcdir, 'SanOGLES')
        cmd_gles_s = os.path.join(self.srcdir, 'SanOGLES_S')
        if os.path.isfile(cmd_gl):
            cmd = cmd_gl
        elif os.path.isfile(cmd_gles):
            cmd = cmd_gles
        elif os.path.isfile(cmd_gles_s):
            cmd = cmd_gles_s
        else:
            raise error.TestFail('Failed to locate SanAngeles executable: '
                                 '%s, %s or %s.  Test setup error.'
                                 % (cmd_gl, cmd_gles, cmd_gles_s))

        cmd += ' ' + utils.graphics_platform()
        cmd = graphics_utils.xcommand(cmd)
        result = utils.run(cmd,
                           stderr_is_expected=False,
                           stdout_tee=utils.TEE_TO_LOGS,
                           stderr_tee=utils.TEE_TO_LOGS,
                           ignore_status=True)

        report = re.findall(r'frame_rate = ([0-9.]+)', result.stdout)
        if not report:
            raise error.TestFail('Could not find frame_rate in stdout (' +
                                 result.stdout + ') ' + result.stderr)

        frame_rate = float(report[0])
        logging.info('frame_rate = %.1f', frame_rate)
        self.write_perf_keyval(
            {'frames_per_sec_rate_san_angeles': frame_rate})
        self.output_perf_value(description='fps', value=frame_rate,
                               units='fps', higher_is_better=True)
        if 'error' in result.stderr.lower():
            raise error.TestFail('Error on stderr while running SanAngeles: ' +
                                 result.stderr + ' (' + report[0] + ')')
