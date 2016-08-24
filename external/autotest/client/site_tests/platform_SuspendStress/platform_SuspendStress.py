# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.


import os, random, subprocess, time
import commands, logging, random, time
from autotest_lib.client.bin import utils, test
from autotest_lib.client.common_lib import error
from autotest_lib.client.cros import rtc, sys_power


MIN_SLEEP_INTERVAL = 5
MIN_WORK_INTERVAL = 30
START_FILE = '/tmp/power_state_cycle_begin'
STOP_FILE = '/tmp/power_state_cycle_end'

class platform_SuspendStress(test.test):
    version = 1
    def initialize(self):
        random.seed() # System time is fine.
        if os.path.exists(STOP_FILE):
            logging.warning('removing existing stop file %s' % STOP_FILE)
            os.unlink(STOP_FILE)


    def suspend_and_resume(self, seconds=MIN_SLEEP_INTERVAL):
        """Suspends for N seconds."""
        sleep_seconds = min(seconds, MIN_SLEEP_INTERVAL)
        suspend_time = rtc.get_seconds()
        sys_power.do_suspend(sleep_seconds)
        logging.debug('and we\'re back... %ds elapsed.',
                      rtc.get_seconds() - suspend_time)


    def power_state_cycle(self, timeout=None):
        try:
            while not os.path.exists(STOP_FILE):
                if timeout and time.mktime(time.localtime()) > timeout:
                    raise error.TestFail('didn\'t find %s before timeout.' %
                                         STOP_FILE)
                self.suspend_and_resume(random.randint(MIN_SLEEP_INTERVAL, 15))
                time.sleep(random.randint(MIN_WORK_INTERVAL,
                                          MIN_WORK_INTERVAL+5))
        finally:
            # Ensure we disable the RTC alarm, leaving the original state
            rtc.set_wake_alarm(0)


    def run_once(self, auto_start=False, runtime=None):
        if auto_start:
            open(START_FILE, 'w').close()
        utils.poll_for_condition(lambda: os.path.exists(START_FILE),
                                 error.TestFail('startup not triggered.'),
                                 timeout=30, sleep_interval=1)
        logging.debug('Found %s, starting power state cycle.' % START_FILE)
        if runtime:
            runtime = time.mktime(time.localtime()) + runtime
        os.unlink(START_FILE)
        self.power_state_cycle(runtime)
