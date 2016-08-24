# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.


import os, random, subprocess, time
import commands, logging, random, time, utils
from autotest_lib.client.bin import site_utils, test
from autotest_lib.client.common_lib import error

SUSPEND_START = '/tmp/power_state_cycle_begin'
SUSPEND_END = '/tmp/power_state_cycle_end'
CRYPTOHOMESTRESS_START = '/tmp/cryptohomestress_begin'
CRYPTOHOMESTRESS_END = '/tmp/cryptohomestress_end'

class platform_CryptohomeStress(test.test):
    version = 1
    def initialize(self):
        for signal_file in [SUSPEND_END, CRYPTOHOMESTRESS_END]:
            if os.path.exists(signal_file):
                logging.warning('removing existing stop file %s' % signal_file)
                os.unlink(signal_file)
    random.seed() # System time is fine.


    def run_once(self, runtime=300):
        # check that fio has started, waiting for up to TIMEOUT
        site_utils.poll_for_condition(
            lambda: os.path.exists(CRYPTOHOMESTRESS_START),
            error.TestFail('fiostress not triggered.'),
            timeout=30, sleep_interval=1)
        open(SUSPEND_START, 'w').close()
        # Pad disk stress runtime by 60s for safety.
        runtime = runtime + 60
        site_utils.poll_for_condition(
            lambda: os.path.exists(CRYPTOHOMESTRESS_END),
            error.TestFail('fiostress runtime exceeded.'),
            timeout=runtime, sleep_interval=10)

    def cleanup(self):
        open(SUSPEND_END, 'w').close()
        
