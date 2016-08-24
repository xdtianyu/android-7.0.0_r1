#!/usr/bin/python
#
# Copyright (c) 2010 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

__author__ = 'kdlucas@chromium.org (Kelly Lucas)'

import logging, re

from autotest_lib.client.bin import utils, test
from autotest_lib.client.common_lib import error


class platform_MemCheck(test.test):
    """
    Verify memory usage looks correct.
    """
    version = 1
    swap_disksize_file = '/sys/block/zram0/disksize'

    def run_once(self):
        errors = 0
        keyval = dict()
        # The total memory will shrink if the system bios grabs more of the
        # reserved memory. We derived the value below by giving a small
        # cushion to allow for more system BIOS usage of ram. The memref value
        # is driven by the supported netbook model with the least amount of
        # total memory.  ARM and x86 values differ considerably.
        cpuType = utils.get_cpu_arch()
        memref = 986392
        vmemref = 102400
        if cpuType == "arm":
            memref = 700000
            vmemref = 210000

        speedref = 1333
        os_reserve = 600000

        # size reported in /sys/block/zram0/disksize is in byte
        swapref = int(utils.read_one_line(self.swap_disksize_file)) / 1024

        less_refs = ['MemTotal', 'MemFree', 'VmallocTotal']
        approx_refs = ['SwapTotal']

        # read physical HW size from mosys and adjust memref if need
        cmd = 'mosys memory spd print geometry -s size_mb'
        phy_size_run = utils.run(cmd)
        phy_size = 0
        for line in phy_size_run.stdout.split():
            phy_size += int(line)
        # memref is in KB but phy_size is in MB
        phy_size *= 1024
        keyval['PhysicalSize'] = phy_size
        memref = max(memref, phy_size - os_reserve)
        freeref = memref / 2

        # Special rule for free memory size for parrot and butterfly
        board = utils.get_board()
        if board.startswith('parrot'):
            freeref = 100 * 1024
        elif board.startswith('butterfly'):
            freeref = freeref - 400 * 1024
        elif board.startswith('rambi') or board.startswith('expresso'):
            logging.info('Skipping test on rambi and expresso, '
                         'see crbug.com/411401')
            return

        ref = {'MemTotal': memref,
               'MemFree': freeref,
               'SwapTotal': swapref,
               'VmallocTotal': vmemref,
              }

        logging.info('board: %s, phy_size: %d memref: %d freeref: %d',
                      board, phy_size, memref, freeref)

        error_list = []

        for k in ref:
            value = utils.read_from_meminfo(k)
            keyval[k] = value
            if k in less_refs:
                if value < ref[k]:
                    logging.warning('%s is %d', k, value)
                    logging.warning('%s should be at least %d', k, ref[k])
                    errors += 1
                    error_list += [k]
            elif k in approx_refs:
                if value < ref[k] * 0.9 or ref[k] * 1.1 < value:
                    logging.warning('%s is %d', k, value)
                    logging.warning('%s should be within 10%% of %d', k, ref[k])
                    errors += 1
                    error_list += [k]

        # read spd timings
        cmd = 'mosys memory spd print timings -s speeds'
        # result example
        # DDR3-800, DDR3-1066, DDR3-1333, DDR3-1600
        pattern = '[A-Z]*DDR([3-9]|[1-9]\d+)[A-Z]*-(?P<speed>\d+)'
        timing_run = utils.run(cmd)

        keyval['speedref'] = speedref
        for dimm, line in enumerate(timing_run.stdout.split('\n')):
            if not line:
                continue
            max_timing = line.split(', ')[-1]
            keyval['timing_dimm_%d' % dimm] = max_timing
            m = re.match(pattern, max_timing)
            if not m:
                logging.warning('Error parsing timings for dimm #%d (%s)',
                             dimm, max_timing)
                errors += 1
                continue
            logging.info('dimm #%d timings: %s', dimm, max_timing)
            max_speed = int(m.group('speed'))
            keyval['speed_dimm_%d' % dimm] = max_speed
            if max_speed < speedref:
                logging.warning('ram speed is %s', max_timing)
                logging.warning('ram speed should be at least %d', speedref)
                error_list += ['speed_dimm_%d' % dimm]
                errors += 1

        # If self.error is not zero, there were errors.
        if errors > 0:
            error_list_str = ', '.join(error_list)
            raise error.TestFail('Found incorrect values: %s' % error_list_str)

        self.write_perf_keyval(keyval)
