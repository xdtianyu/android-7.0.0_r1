#!/usr/bin/python
#
# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import argparse, logging, os, subprocess, sys

THIS_DIR = os.path.dirname(__file__)
UTILS_DIR = os.path.abspath(os.path.join(THIS_DIR, os.pardir, os.pardir,
                                         'utils'))
TEST_IMPORTER = os.path.join(UTILS_DIR, 'test_importer.py')

# The list of scripts are passed as lists as this allows us to add arguments
# as the list is just passed to subprocess.call(). We expect the scripts to
# return 0 on success and a non-zero value on failure.

# Scripts that need to be ran first to do any preperation. E.g. update the
# database.
PREP_SCRIPTS = [
                    [TEST_IMPORTER]
               ]

COMPLETE_FAILURES = os.path.join(THIS_DIR, 'complete_failures.py')
PASSING_EXPERIMENTAL = os.path.join(THIS_DIR, 'passing_experimental.py')
# Scripts ran to do the analysis.
ANALYSIS_SCRIPTS = [
                        [COMPLETE_FAILURES],
                        [PASSING_EXPERIMENTAL]
                   ]


def run_prep_scripts(scripts):
    """
    Run the scripts that are required to be ran before the analysis.

    This stops and returns False at the first script failure.

    @param scripts: A list of lists. Where the inner list is the script name
        and arguments to be called (as subprocess.call() expects).

    @return True if all the scripts succeeded and False otherwise.

    """

    for script in scripts:
        logging.info('Running %s', ' '.join(script))
        return_code = subprocess.call(script)
        if return_code != 0:
            logging.error('\'%s\' failed with return code %d',
                          (' '.join(script), return_code))
            return False

    return True


def run_analysis_scripts(scripts):
    """
    Run the scripts that analyze the database.

    All scripts are ran even if one fails.

    @param scripts: A list of lists, where the inner list is the script name
        and arguments to be called (as subprocess.call() expects).

    @return True if all the scripts succeeded and False otherwise.

    """

    success = True

    for script in scripts:
        logging.info('Running %s', ' '.join(script))
        return_code = subprocess.call(script)
        if return_code != 0:
            logging.error('\'%s\' failed with return code %d',
                          (' '.join(script), return_code))
            success = False

    return success


def parse_options(args):
    """Parse the command line options."""

    description = ('Runs test health and preparation scripts.')
    parser = argparse.ArgumentParser(description=description)
    parser.parse_args(args)


def main(args=None):
    """
    The main function.

    This allows us to test this function by calling it in the unit test file.

    @param args: The command line arguments being passed in.

    @return 0 if everything succeeded and a non-zero integer otherwise.

    """
    args = [] if args is None else args
    parse_options(args)

    logging.getLogger().setLevel(logging.INFO)

    prep_success = run_prep_scripts(PREP_SCRIPTS)
    if not prep_success:
        return 1

    analysis_success = run_analysis_scripts(ANALYSIS_SCRIPTS)
    if not analysis_success:
        return 1

    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
