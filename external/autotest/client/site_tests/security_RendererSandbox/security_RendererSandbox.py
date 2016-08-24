# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.


import os
import subprocess

from autotest_lib.client.bin import test, utils
from autotest_lib.client.common_lib import error

class security_RendererSandbox(test.test):
    version = 1
    renderer_pid = -1


    def _get_renderer_pid(self):
        """Query pgrep for the pid of the renderer. Since this function is
        passed as an argument to |utils.poll_for_condition()|, the return values
        are set to True/False depending on whether a pid has been found."""

        pgrep = subprocess.Popen(['pgrep', '-f', '-l', 'type=renderer'],
                                 stdout=subprocess.PIPE)
        procs = pgrep.communicate()[0].splitlines()
        pids = []
        # The fix for http://code.google.com/p/chromium/issues/detail?id=129884
        # adds '--ignored= --type=renderer' to the GPU process cmdline.
        # This makes 'pgrep' above return the pid of the GPU process,
        # which is not setuid sandboxed, as the pid of a renderer,
        # breaking the test.
        # Work around by removing processes with '--ignored= --type=renderer'
        # flags.
        for proc in procs:
            if '--ignored= --type=renderer' not in proc:
                pids.append(proc.split()[0])

        if pids:
            self.renderer_pid = pids[0]
            return True
        else:
            return False


    def _check_for_suid_sandbox(self, renderer_pid):
        """For the setuid sandbox, make sure there is no content in the CWD
        directory."""

        cwd_contents = os.listdir('/proc/%s/cwd' % self.renderer_pid)
        if len(cwd_contents) > 0:
            raise error.TestFail('Contents present in the CWD directory')


    def run_once(self, time_to_wait=20):
        """Wait until the page is loaded and poll for the renderer pid.
        If renderer pid is found, it is stored in |self.renderer_pid|."""

        utils.poll_for_condition(
            self._get_renderer_pid,
            error.TestFail('Timed out waiting to obtain pid of renderer'),
            time_to_wait)

        # Check if renderer is sandboxed.
        self._check_for_suid_sandbox(self.renderer_pid)
