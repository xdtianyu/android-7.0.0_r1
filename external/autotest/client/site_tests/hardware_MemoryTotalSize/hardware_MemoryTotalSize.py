# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging

from autotest_lib.client.bin import test, utils
from autotest_lib.client.common_lib import error

class hardware_MemoryTotalSize(test.test):
    version = 1

    def run_once(self):
        # TODO(zmo@): this may not get total physical memory size on ARM
        #             or some x86 machines.
        mem_size = utils.memtotal()
        gb = mem_size / 1024.0 / 1024.0
        self.write_perf_keyval({"gb_memory_total": gb})
        logging.info("MemTotal: %.3f GB" % gb)

        # x86 and ARM SDRAM configurations differ significantly from each other.
        # Use a value specific to the architecture.
        # On x86, I see 1.85GiB (2GiB - reserved memory).
        # On ARM, I see 0.72GiB (1GiB - 256MiB carveout).
        cpuType = utils.get_cpu_arch()
        limit = 1.65
        if cpuType == "arm":
            limit = 0.65

        if gb <= limit:
            raise error.TestFail("total system memory size < %.3f GB" % limit);
