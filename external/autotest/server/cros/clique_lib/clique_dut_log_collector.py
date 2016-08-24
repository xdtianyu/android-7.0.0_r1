# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import multiprocessing

import common
from autotest_lib.server import test
from autotest_lib.server import hosts
from autotest_lib.server import crashcollect


def log_collector_dut_worker(dut, job):
    """Worker function to collect logs from each DUT in the pool.

    The method called by multiprocessing worker pool for collecting DUT
    logs. This function is the function which is repeatedly scheduled for each
    DUT through the multiprocessing worker. This has to be defined outside
    the class because it needs to be pickleable.

    @param dut: DUTObject representing the DUT.
    @param job: Autotest job object.
    """
    host = dut.host
    # Set the job on the host object for log collection.
    host.job = job
    logging.info("Collecting logs from: %s", host.hostname)
    crashcollect.get_crashinfo(host, 0)


class CliqueDUTLogCollector(object):
    """CliqueDUTLogCollector is responsible for collecting the relevant logs
    from all the DUT's in the DUT pool after the test is executed.
    """

    def collect_logs(self, dut_objects, job):
        """Collects logs from all tall the DUT's in the pool to a provided
        folder.

        @param dut_objects: An array of DUTObjects corresponding to all the
                            DUT's in the DUT pool.
        @param job: Autotest job object.
        """
        tasks = []
        for dut in dut_objects:
            # Schedule the log collection for this DUT to the log process
            # pool.
            task = multiprocessing.Process(
                    target=log_collector_dut_worker,
                    args=(dut, job))
            tasks.append(task)
        # Run the log collections in parallel.
        for task in tasks:
            task.start()
        for task in tasks:
            task.join()
