# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging, time

from autotest_lib.client.bin import test, utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.cros import perf
from autotest_lib.client.cros import service_stopper

class graphics_PerfControl(test.test):
  version = 1

  # None-init vars used by cleanup() here, in case setup() fails
  _services = None

  def initialize(self):
    self._services = service_stopper.ServiceStopper(['ui'])

  def cleanup(self):
    if self._services:
      self._services.restore_services()

  def run_once(self):
    logging.info(utils.get_board_with_frequency_and_memory())

    # If UI is running, we must stop it and restore later.
    self._services.stop_services()

    # Wrap the test run inside of a PerfControl instance to make machine
    # behavior more consistent.
    with perf.PerfControl() as pc:
      if not pc.verify_is_valid():
        raise error.TestError(pc.get_error_reason())
      # Do nothing for a short while so the PerfControl thread is collecting
      # real data.
      time.sleep(10)

      if not pc.verify_is_valid():
        raise error.TestError(pc.get_error_reason())
