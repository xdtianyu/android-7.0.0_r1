#!/usr/bin/python
#
# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.


import argparse, datetime, sys

import common
from autotest_lib.client.common_lib import mail
from autotest_lib.frontend import setup_django_readonly_environment

# Django and the models are only setup after
# the setup_django_readonly_environment module is imported.
from autotest_lib.frontend.tko import models as tko_models
from autotest_lib.frontend.health import utils


# Mark a test as failing too long if it has not passed in this many days
_DAYS_TO_BE_FAILING_TOO_LONG = 60
# Ignore any tests that have not ran in this many days
_DAYS_NOT_RUNNING_CUTOFF = 60
_MAIL_RESULTS_FROM = 'chromeos-test-health@google.com'
_MAIL_RESULTS_TO = 'chromeos-lab-infrastructure@google.com'


def is_valid_test_name(name):
    """
    Returns if a test name is valid or not.

    There is a bunch of entries in the tko_test table that are not actually
    test names. They are there as a side effect of how Autotest uses this
    table.

    Two examples of bad tests names are as follows:
    link-release/R29-4228.0.0/faft_ec/firmware_ECPowerG3_SERVER_JOB
    try_new_image-chormeos1-rack2-host2

    @param name: The candidate test names to check.
    @return True if name is a valid test name and false otherwise.

    """
    return not '/' in name and not name.startswith('try_new_image')


def prepare_last_passes(last_passes):
    """
    Fix up the last passes so they can be used by the system.

    This filters out invalid test names and converts the test names to utf8
    encoding.

    @param last_passes: The dictionary of test_name:last_pass pairs.

    @return: Valid entries in encoded as utf8 strings.
    """
    valid_test_names = filter(is_valid_test_name, last_passes)
    # The shelve module does not accept Unicode objects as keys but does
    # accept utf-8 strings.
    return {name.encode('utf8'): last_passes[name]
            for name in valid_test_names}


def get_recently_ran_test_names():
    """
    Get all the test names from the database that have been recently ran.

    @return a set of the recently ran tests.

    """
    cutoff_delta = datetime.timedelta(_DAYS_NOT_RUNNING_CUTOFF)
    cutoff_date = datetime.datetime.today() - cutoff_delta
    results = tko_models.Test.objects.filter(
        started_time__gte=cutoff_date).values('test').distinct()
    test_names = [test['test'] for test in results]
    valid_test_names = filter(is_valid_test_name, test_names)
    return {test.encode('utf8') for test in valid_test_names}


def get_tests_to_analyze(recent_test_names, last_pass_times):
    """
    Get all the recently ran tests as well as the last time they have passed.

    The minimum datetime is given as last pass time for tests that have never
    passed.

    @param recent_test_names: The set of the names of tests that have been
        recently ran.
    @param last_pass_times: The dictionary of test_name:last_pass_time pairs.

    @return the dict of test_name:last_finish_time pairs.

    """
    prepared_passes = prepare_last_passes(last_pass_times)

    running_passes = {}
    for test, pass_time in prepared_passes.items():
        if test in recent_test_names:
            running_passes[test] = pass_time

    failures_names = recent_test_names.difference(running_passes)
    always_failed = {test: datetime.datetime.min for test in failures_names}
    return dict(always_failed.items() + running_passes.items())


def email_about_test_failure(failed_tests, all_tests):
    """
    Send an email about all the tests that have failed if there are any.

    @param failed_tests: The list of failed tests. This will be sorted in this
        function.
    @param all_tests: All the names of tests that have been recently ran.

    """
    if failed_tests:
        failed_tests.sort()
        mail.send(_MAIL_RESULTS_FROM,
                  [_MAIL_RESULTS_TO],
                  [],
                  'Long Failing Tests',
                  '%d/%d tests have been failing for at least %d days.\n'
                  'They are the following:\n\n%s'
                  % (len(failed_tests), len(all_tests),
                     _DAYS_TO_BE_FAILING_TOO_LONG,
                     '\n'.join(failed_tests)))


def filter_out_good_tests(tests):
    """
    Remove all tests that have passed recently enough to be good.

    @param tests: The tests to filter on.

    @return: A list of tests that have not passed for a long time.

    """
    cutoff = (datetime.datetime.today() -
              datetime.timedelta(_DAYS_TO_BE_FAILING_TOO_LONG))
    return [name for name, last_pass in tests.items() if last_pass < cutoff]


def parse_options(args):
    """Parse the command line options."""

    description = ('Collects information about which tests have been '
                   'failing for a long time and creates an email summarizing '
                   'the results.')
    parser = argparse.ArgumentParser(description=description)
    parser.parse_args(args)


def main(args=None):
    """
    The script code.

    Allows other python code to import and run this code. This will be more
    important if a nice way to test this code can be determined.

    @param args: The command line arguments being passed in.

    """
    args = [] if args is None else args
    parse_options(args)
    all_test_names = get_recently_ran_test_names()
    last_passes = utils.get_last_pass_times()
    tests = get_tests_to_analyze(all_test_names, last_passes)
    failures = filter_out_good_tests(tests)
    email_about_test_failure(failures, all_test_names)



if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
