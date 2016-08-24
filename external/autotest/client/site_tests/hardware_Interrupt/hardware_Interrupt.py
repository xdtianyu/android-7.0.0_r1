# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.


# DESCRIPTION :
#
# This is a hardware test for interrupts. The test reloads kernel module
# to check if the hardware issues interrupts back to the host.


import re
import time
import logging

from autotest_lib.client.bin import test, utils
from autotest_lib.client.common_lib import error


class ProcInterrupts(object):
    """
    Parse /proc/interrupts and provide interfaces to access number of
    interrupts per module or per interrupt number/name
    """

    INTERRUPT_FILE='/proc/interrupts'

    def __init__(self):
        self._int_count = {}
        with open(self.INTERRUPT_FILE) as interrupts_file:
            lines = interrupts_file.readlines()

            # First line indicates CPUs in system
            num_cpus = len(lines.pop(0).split())

            for line in lines:
                fields = line.split()
                count = sum(map(int, fields[1:1 + num_cpus]))
                interrupt = fields[0].strip().split(':')[0]
                if interrupt.isdigit():
                    self._int_count[fields[-1]] = count
                    logging.debug('int[%s] = %d', fields[-1], count)
                    interrupt = int(interrupt)
                self._int_count[interrupt] = count
                logging.debug('int[%s] = %d', interrupt, count)

    def get(self, interrupt):
        if interrupt in self._int_count:
            logging.debug('got int[%s] = %d', interrupt,
                          self._int_count[interrupt])
            return self._int_count[interrupt]
        return 0


class hardware_Interrupt(test.test):
    version = 1

    def run_once(self,
                 interrupt=None,
                 reload_module=None,
                 min_count=1):
        proc_int = ProcInterrupts()
        count = proc_int.get(interrupt)

        if reload_module:
            utils.system('rmmod %s' % reload_module)
            utils.system('modprobe %s' % reload_module)
            # Wait for procfs update
            time.sleep(1)
            proc_int = ProcInterrupts()
            count = proc_int.get(interrupt) - count

        if count < min_count:
            raise error.TestError('Interrupt test failed: int[%s] = %d < '
                                  'min_count %d' % (interrupt, count,
                                  min_count))

