#!/usr/bin/python
# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import argparse
import os
import sys

import logging
# Turn the logging level to INFO before importing other autotest
# code, to avoid having failed import logging messages confuse the
# test_droid user.
logging.basicConfig(level=logging.INFO)


import common
# Unfortunately, autotest depends on external packages for assorted
# functionality regardless of whether or not it is needed in a particular
# context.
# Since we can't depend on people to import these utilities in any principled
# way, we dynamically download code before any autotest imports.
try:
    import chromite.lib.terminal  # pylint: disable=unused-import
    import django.http  # pylint: disable=unused-import
except ImportError:
    # Ensure the chromite site-package is installed.
    import subprocess
    build_externals_path = os.path.join(
            os.path.dirname(os.path.dirname(os.path.realpath(__file__))),
            'utils', 'build_externals.py')
    subprocess.check_call([build_externals_path, 'chromiterepo', 'django'])
    # Restart the script so python now finds the autotest site-packages.
    sys.exit(os.execv(__file__, sys.argv))

from autotest_lib.site_utils import test_runner_utils
from autotest_lib.site_utils import tester_feedback


def parse_arguments(argv):
    """
    Parse command line arguments

    @param argv: argument list to parse

    @returns:    parsed arguments

    @raises SystemExit if arguments are malformed, or required arguments
            are not present.
    """
    return _parse_arguments_internal(argv)[0]


def _parse_arguments_internal(argv):
    """
    Parse command line arguments

    @param argv: argument list to parse

    @returns:    tuple of parsed arguments and argv suitable for remote runs

    @raises SystemExit if arguments are malformed, or required arguments
            are not present.
    """

    parser = argparse.ArgumentParser(description='Run remote tests.')

    parser.add_argument('serials', metavar='SERIALS',
                        help='Comma separate list of device serials under '
                             'test.')
    parser.add_argument('-r', '--remote', metavar='REMOTE',
                        default='localhost',
                        help='hostname[:port] if the ADB device is connected '
                             'to a remote machine. Ensure this workstation '
                             'is configured for passwordless ssh access as '
                             'users "root" or "adb"')
    parser.add_argument('-i', '--interactive', action='store_true',
                        help='Enable interactive feedback requests from tests.')
    test_runner_utils.add_common_args(parser)
    return parser.parse_args(argv)


def main(argv):
    """
    Entry point for test_droid script.

    @param argv: arguments list
    """
    arguments = _parse_arguments_internal(argv)

    results_directory = test_runner_utils.create_results_directory(
            arguments.results_dir)
    arguments.results_dir = results_directory

    autotest_path = os.path.dirname(os.path.dirname(
            os.path.realpath(__file__)))
    site_utils_path = os.path.join(autotest_path, 'site_utils')
    realpath = os.path.realpath(__file__)
    site_utils_path = os.path.realpath(site_utils_path)
    host_attributes = {'serials' : arguments.serials,
                       'os_type' : 'android'}

    fb_service = None
    try:
        # Start the feedback service if needed.
        if arguments.interactive:
            fb_service = tester_feedback.FeedbackService()
            fb_service.start()

            if arguments.args:
                arguments.args += ' '
            else:
                arguments.args = ''
            arguments.args += (
                    'feedback=interactive feedback_args=localhost:%d' %
                    fb_service.server_port)

        return test_runner_utils.perform_run_from_autotest_root(
                    autotest_path, argv, arguments.tests, arguments.remote,
                    args=arguments.args, ignore_deps=not arguments.enforce_deps,
                    results_directory=results_directory,
                    iterations=arguments.iterations,
                    fast_mode=arguments.fast_mode, debug=arguments.debug,
                    host_attributes=host_attributes)
    finally:
        if fb_service is not None:
            fb_service.stop()


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
