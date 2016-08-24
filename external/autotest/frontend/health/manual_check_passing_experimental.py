#!/usr/bin/python
#
# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import argparse, datetime, sys

import common
# This must come before the import of complete_failures in order to use the
# in memory database.
from autotest_lib.frontend import setup_django_readonly_environment
from autotest_lib.frontend import setup_test_environment
from autotest_lib.frontend.afe import models as afe_models
from autotest_lib.frontend.health import passing_experimental
from autotest_lib.frontend.tko import models as tko_models

GOOD_STATUS_IDX = 6


def parse_options(args):
    """Parse the command line options."""
    description = ('Sets up a fake database and then runs '
                   'passing_experimental.py main() function to simulate '
                   'running the script to test bug filing. Manually checking '
                   'will be required to verify that bugs have been submitted '
                   'correctly. Remember to set up the shadow_config.ini file '
                   'to point to the autotest-bug-filing-test dummy project.')
    parser = argparse.ArgumentParser(description=description)
    parser.parse_args(args)


def main(args):
    """
    Run passing_experimental.py to check bug filing for it.

    This sets the fake database up so a bug is guranteed to be filed. However,
    it requires manually verifying that the bug was filed and deduped.

    @param args: The arguments passed in from the commandline.

    """
    args = [] if args is None else args
    parse_options(args)

    setup_test_environment.set_up()

    afe_models.Test(name='test_dedupe', test_type=0, path='test_dedupe',
                    experimental=True).save()

    tko_models.Status(status_idx=6, word='GOOD').save()

    job = tko_models.Job(job_idx=1)
    kernel = tko_models.Kernel(kernel_idx=1)
    machine = tko_models.Machine(machine_idx=1)
    success_status = tko_models.Status(status_idx=GOOD_STATUS_IDX)

    tko_dedupe = tko_models.Test(job=job, status=success_status,
                                 kernel=kernel, machine=machine,
                                 test='test_dedupe',
                                 started_time=datetime.datetime.today())
    tko_dedupe.save()

    passing_experimental.main()

    # We assume that the user is using the dummy tracker when using this script.
    print ('Now check the bug tracker to make sure this was properly deduped.\n'
           'https://code.google.com/p/autotest-bug-filing-test/issues/list?'
           'q=PassingExperimental')


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
