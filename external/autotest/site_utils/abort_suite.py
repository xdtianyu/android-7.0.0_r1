#!/usr/bin/python -t
#
# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.


"""
Usage: ./abort_suite.py [-i and -s you passed to run_suite.py]

This code exists to allow buildbot to abort a HWTest run if another part of
the build fails while HWTesting is going on.  If we're going to fail the
build anyway, there's no point in continuing to run tests.

One can also pass just the build version to -i, to abort all boards running the
suite against that version. ie. |./abort_suite.py -i R28-3993.0.0 -s dummy|
would abort all boards running dummy on R28-3993.0.0.

To achieve better performance, this script aborts suite jobs and relies on
autotest scheduler to aborts its subjobs instead of directly aborting subjobs.
So only synchronous suites is supported.

"""


import argparse
import getpass
import logging
import os
import sys
from datetime import datetime

import common
from autotest_lib.client.common_lib.cros.graphite import autotest_stats
from autotest_lib.server import frontend
from autotest_lib.server import utils


LOG_NAME_TEMPLATE = 'abort_suite-%s.log'
SUITE_JOB_NAME_TEMPLATE = '%s-test_suites/control.%s'
_timer = autotest_stats.Timer('abort_suites')


def parse_args():
    """
    Parse the arguments to this script.

    @return The arguments to this script.

    """
    parser = argparse.ArgumentParser()
    parser.add_argument('-s', '--suite_name', dest='name')
    parser.add_argument('-i', '--build', dest='build')
    return parser.parse_args()


@_timer.decorate
def abort_suites(afe, substring):
    """
    Abort the suite.

    This method aborts the suite jobs whose name contains |substring|.
    Aborting a suite job will lead to all its child jobs to be aborted
    by autotest scheduler.

    @param afe: An instance of frontend.AFE to make RPCs with.
    @param substring: A string used to search for the jobs.

    """
    hqe_info = afe.run('abort_host_queue_entries',
            job__name__contains=substring, job__owner=getpass.getuser(),
            job__parent_job__isnull=True)
    if hqe_info:
        logging.info('The following suites have been aborted:\n%s', hqe_info)
    else:
        logging.info('No suites have been aborted. The suite jobs may have '
                     'already been aborted/completed? Note this script does '
                     'not support asynchronus suites.')


def main():
    """Main."""
    args = parse_args()

    log_dir = os.path.join(common.autotest_dir, 'logs')
    if not os.path.exists(log_dir):
        os.makedirs(log_dir)
    log_name = LOG_NAME_TEMPLATE % args.build.replace('/', '_')
    log_name = os.path.join(log_dir, log_name)

    utils.setup_logging(logfile=log_name, prefix=True)

    afe = frontend.AFE()
    name = SUITE_JOB_NAME_TEMPLATE % (args.build, args.name)
    abort_suites(afe, name)
    return 0


if __name__ == '__main__':
    sys.exit(main())
