# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
#
# GLMark outputs a final performance score, and it checks the performance score
# against minimum requirement if min_score is set.

import logging
import os
import re
import string

from autotest_lib.client.bin import test, utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.cros import service_stopper
from autotest_lib.client.cros.graphics import graphics_utils

GLMARK2_TEST_RE = (
    r'^\[(?P<scene>.*)\] (?P<options>.*): FPS: (?P<fps>\d+) FrameTime: '
    r'(?P<frametime>\d+.\d+) ms$')
GLMARK2_SCORE_RE = r'glmark2 Score: (\d+)'

# perf value description strings may only contain letters, numbers, periods,
# dashes and underscores.
# But glmark2 test names are usually in the form:
#   scene-name:opt=val:opt=v1,v2;v3,v4 or scene:<default>
# which we convert to:
#   scene-name.opt_val.opt_v1-v2_v3-v4 or scene.default
description_table = string.maketrans(':,=;', '.-__')
description_delete = '<>'


class graphics_GLMark2(test.test):
    """Runs glmark2, which benchmarks only calls compatible with OpenGL ES 2.0"""
    version = 1
    preserve_srcdir = True
    _services = None
    GSC = None

    def setup(self):
        self.job.setup_dep(['glmark2'])

    def initialize(self):
        self.GSC = graphics_utils.GraphicsStateChecker()
        # If UI is running, we must stop it and restore later.
        self._services = service_stopper.ServiceStopper(['ui'])
        self._services.stop_services()

    def cleanup(self):
        if self._services:
            self._services.restore_services()
        if self.GSC:
            keyvals = self.GSC.get_memory_keyvals()
            for key, val in keyvals.iteritems():
                self.output_perf_value(description=key, value=val,
                                       units='bytes', higher_is_better=False)
            self.GSC.finalize()
            self.write_perf_keyval(keyvals)

    def run_once(self, size='800x600', hasty=False, min_score=None):
        dep = 'glmark2'
        dep_dir = os.path.join(self.autodir, 'deps', dep)
        self.job.install_pkg(dep, 'dep', dep_dir)

        glmark2 = os.path.join(self.autodir, 'deps/glmark2/glmark2')
        if not os.path.exists(glmark2):
            raise error.TestFail('Could not find test binary. Setup error.')

        glmark2_data = os.path.join(self.autodir, 'deps/glmark2/data')

        options = []
        options.append('--data-path %s' % glmark2_data)
        options.append('--size %s' % size)
        options.append('--annotate')
        if hasty:
            options.append('-b :duration=0.2')
        else:
            options.append('-b :duration=2')
        cmd = glmark2 + ' ' + ' '.join(options)
        if not utils.is_freon():
            cmd = 'X :1 vt1 & sleep 1; chvt 1 && DISPLAY=:1 ' + cmd

        if os.environ.get('CROS_FACTORY'):
            from autotest_lib.client.cros import factory_setup_modules
            from cros.factory.test import ui
            ui.start_reposition_thread('^glmark')

        # TODO(ihf): Switch this test to use perf.PerfControl like
        #            graphics_GLBench once it is stable. crbug.com/344766.
        if not hasty:
            if not utils.wait_for_idle_cpu(60.0, 0.1):
                if not utils.wait_for_idle_cpu(20.0, 0.2):
                    raise error.TestFail('Could not get idle CPU.')
            if not utils.wait_for_cool_machine():
                raise error.TestFail('Could not get cold machine.')

        try:
            result = utils.run(cmd,
                               stderr_is_expected=False,
                               stdout_tee=utils.TEE_TO_LOGS,
                               stderr_tee=utils.TEE_TO_LOGS)
        finally:
            # Just sending SIGTERM to X is not enough; we must wait for it to
            # really die before we start a new X server (ie start ui).
            if not utils.is_freon():
                utils.ensure_processes_are_dead_by_name('^X$')

        logging.info(result)
        for line in result.stderr.splitlines():
            if line.startswith('Error:'):
                raise error.TestFail(line)

        # Numbers in hasty mode are not as reliable, so don't send them to
        # the dashboard etc.
        if not hasty:
            keyvals = {}
            score = None
            test_re = re.compile(GLMARK2_TEST_RE)
            for line in result.stdout.splitlines():
                match = test_re.match(line)
                if match:
                    test = '%s.%s' % (match.group('scene'),
                                      match.group('options'))
                    test = test.translate(description_table,
                                          description_delete)
                    frame_time = match.group('frametime')
                    keyvals[test] = frame_time
                    self.output_perf_value(description=test, value=frame_time,
                                           units='ms', higher_is_better=False)
                else:
                    # glmark2 output the final performance score as:
                    #  glmark2 Score: 530
                    match = re.findall(GLMARK2_SCORE_RE, line)
                    if match:
                        score = int(match[0])
            if score is None:
                raise error.TestFail('Unable to read benchmark score')
            # Output numbers for plotting by harness.
            logging.info('GLMark2 score: %d', score)
            if os.environ.get('CROS_FACTORY'):
                from autotest_lib.client.cros import factory_setup_modules
                from cros.factory.event_log import EventLog
                EventLog('graphics_GLMark2').Log('glmark2_score', score=score)
            keyvals['glmark2_score'] = score
            self.write_perf_keyval(keyvals)
            self.output_perf_value(description='Score', value=score,
                                   units='score', higher_is_better=True)

            if min_score is not None and score < min_score:
                raise error.TestFail('Benchmark score %d < %d (minimum score '
                                     'requirement)' % (score, min_score))
