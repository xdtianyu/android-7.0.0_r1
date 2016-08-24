# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import os
import time

from autotest_lib.client.bin import test
from autotest_lib.client.common_lib import error, utils
from autotest_lib.client.cros.graphics import graphics_utils

# to run this test manually on a test target
# ssh root@machine
# cd /usr/local/autotest/tests/graphics_GpuReset/src/
# stop ui
# ./gpureset
# start ui


class graphics_GpuReset(test.test):
  """
  Reset the GPU and check recovery mechanism.
  """
  version = 1
  preserve_srcdir = True
  loops = 1
  GSC = None

  def setup(self):
    os.chdir(self.srcdir)
    utils.make('clean')
    utils.make('all')

  def initialize(self):
    # GpuReset should pretty much be the only test where we don't want to raise
    # a test error when we detect a GPU hang.
    self.GSC = graphics_utils.GraphicsStateChecker(raise_error_on_hang=False)

  def cleanup(self):
    if self.GSC:
      self.GSC.finalize()

  def run_once(self, options=''):
    exefile = os.path.join(self.srcdir, 'gpureset')
    if not os.path.isfile(exefile):
      raise error.TestFail('Failed to locate gpureset executable (' +
                           exefile + '). Test setup error.')

    options = ''
    cmd = '%s %s' % (exefile, options)

    # If UI is running, we must stop it and restore later.
    need_restart_ui = False
    status_output = utils.system_output('initctl status ui')
    # If chrome is running, result will be similar to:
    #   ui start/running, process 11895
    logging.info('initctl status ui returns: %s', status_output)
    need_restart_ui = status_output.startswith('ui start')
    summary = ''

    # Run the gpureset test in a loop to stress the recovery.
    for i in range(1, self.loops + 1):
      summary += 'graphics_GpuReset iteration %d of %d\n' % (i, self.loops)
      if need_restart_ui:
        summary += 'initctl stop ui\n'
        utils.system('initctl stop ui', ignore_status=True)
        # TODO(ihf): Remove this code if no improvement for issue 409019.
        logging.info('Make sure chrome is dead before triggering hang.')
        utils.system('killall -9 chrome', ignore_status=True)
        time.sleep(3)
      try:
        summary += utils.system_output(cmd, retain_output=True)
        summary += '\n'
      finally:
        if need_restart_ui:
          summary += 'initctl start ui\n'
          utils.system('initctl start ui')

    # Write a copy of stdout to help debug failures.
    results_path = os.path.join(self.outputdir, 'summary.txt')
    f = open(results_path, 'w+')
    f.write('# need ui restart: %s\n' % need_restart_ui)
    f.write('# ---------------------------------------------------\n')
    f.write('# [' + cmd + ']\n')
    f.write(summary)
    f.write('\n# -------------------------------------------------\n')
    f.write('# [graphics_GpuReset.py postprocessing]\n')

    # Analyze the output. Sample:
    # [       OK ] graphics_GpuReset
    # [  FAILED  ] graphics_GpuReset
    results = summary.splitlines()
    if not results:
      f.close()
      raise error.TestFail('No output from test. Check /tmp/' +
                           'test_that_latest/graphics_GpuReset/summary.txt' +
                           ' for details.')
    # Analyze summary and count number of passes.
    pass_count = 0
    for line in results:
      if line.strip().startswith('[       OK ] graphics_GpuReset'):
        pass_count += 1
      if line.strip().startswith('[  FAILED  ] graphics_GpuReset'):
        msg = line.strip()[30:]
        failed_msg = 'Test failed with %s' % msg
        raise error.TestFail(failed_msg)
    f.close()

    # Final chance to fail.
    if pass_count != self.loops:
      failed_msg = 'Test failed with incomplete output. System hung? '
      failed_msg += '(pass_count=%d of %d)' % (pass_count, self.loops)
      raise error.TestFail(failed_msg)

    # We need to wait a bit for X to come back after the 'start ui'.
    time.sleep(5)

    keyvals = self.GSC.get_memory_keyvals()
    for key, val in keyvals.iteritems():
      self.output_perf_value(description=key, value=val, units='bytes',
                             higher_is_better=False)
