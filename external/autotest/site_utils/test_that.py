#!/usr/bin/python
# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import argparse
import os
import pipes
import signal
import subprocess
import sys

import logging
# Turn the logging level to INFO before importing other autotest
# code, to avoid having failed import logging messages confuse the
# test_that user.
logging.basicConfig(level=logging.INFO)

import common
from autotest_lib.client.common_lib import error, logging_manager
from autotest_lib.server import server_logging_config
from autotest_lib.server.cros.dynamic_suite import constants
from autotest_lib.server.hosts import factory
from autotest_lib.site_utils import test_runner_utils


try:
    from chromite.lib import cros_build_lib
except ImportError:
    print 'Unable to import chromite.'
    print 'This script must be either:'
    print '  - Be run in the chroot.'
    print '  - (not yet supported) be run after running '
    print '    ../utils/build_externals.py'

_QUICKMERGE_SCRIPTNAME = '/mnt/host/source/chromite/bin/autotest_quickmerge'


def _get_board_from_host(remote):
    """Get the board of the remote host.

    @param remote: string representing the IP of the remote host.

    @return: A string representing the board of the remote host.
    """
    logging.info('Board unspecified, attempting to determine board from host.')
    host = factory.create_host(remote)
    try:
        board = host.get_board().replace(constants.BOARD_PREFIX, '')
    except error.AutoservRunError:
        raise test_runner_utils.TestThatRunError(
                'Cannot determine board, please specify a --board option.')
    logging.info('Detected host board: %s', board)
    return board


def validate_arguments(arguments):
    """
    Validates parsed arguments.

    @param arguments: arguments object, as parsed by ParseArguments
    @raises: ValueError if arguments were invalid.
    """
    if arguments.remote == ':lab:':
        if arguments.args:
            raise ValueError('--args flag not supported when running against '
                             ':lab:')
        if arguments.pretend:
            raise ValueError('--pretend flag not supported when running '
                             'against :lab:')
        if arguments.ssh_verbosity:
            raise ValueError('--ssh_verbosity flag not supported when running '
                             'against :lab:')
        if not arguments.board or not arguments.build:
            raise ValueError('--board and --build are both required when '
                             'running against :lab:')
    else:
        if arguments.build is None:
            arguments.build = test_runner_utils.NO_BUILD
        if arguments.web:
            raise ValueError('--web flag not supported when running locally')


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
    local_parser, remote_argv = parse_local_arguments(argv)

    parser = argparse.ArgumentParser(description='Run remote tests.',
                                     parents=[local_parser])

    parser.add_argument('remote', metavar='REMOTE',
                        help='hostname[:port] for remote device. Specify '
                             ':lab: to run in test lab. When tests are run in '
                             'the lab, test_that will use the client autotest '
                             'package for the build specified with --build, '
                             'and the lab server code rather than local '
                             'changes.')
    test_runner_utils.add_common_args(parser)
    default_board = cros_build_lib.GetDefaultBoard()
    parser.add_argument('-b', '--board', metavar='BOARD', default=default_board,
                        action='store',
                        help='Board for which the test will run. Default: %s' %
                             (default_board or 'Not configured'))
    parser.add_argument('-i', '--build', metavar='BUILD',
                        default=test_runner_utils.NO_BUILD,
                        help='Build to test. Device will be reimaged if '
                             'necessary. Omit flag to skip reimage and test '
                             'against already installed DUT image. Examples: '
                             'link-paladin/R34-5222.0.0-rc2, '
                             'lumpy-release/R34-5205.0.0')
    parser.add_argument('-p', '--pool', metavar='POOL', default='suites',
                        help='Pool to use when running tests in the lab. '
                             'Default is "suites"')
    parser.add_argument('--autotest_dir', metavar='AUTOTEST_DIR',
                        help='Use AUTOTEST_DIR instead of normal board sysroot '
                             'copy of autotest, and skip the quickmerge step.')
    parser.add_argument('--no-quickmerge', action='store_true', default=False,
                        dest='no_quickmerge',
                        help='Skip the quickmerge step and use the sysroot '
                             'as it currently is. May result in un-merged '
                             'source tree changes not being reflected in the '
                             'run. If using --autotest_dir, this flag is '
                             'automatically applied.')
    parser.add_argument('--whitelist-chrome-crashes', action='store_true',
                        default=False, dest='whitelist_chrome_crashes',
                        help='Ignore chrome crashes when producing test '
                             'report. This flag gets passed along to the '
                             'report generation tool.')
    parser.add_argument('--ssh_private_key', action='store',
                        default=test_runner_utils.TEST_KEY_PATH,
                        help='Path to the private ssh key.')
    return parser.parse_args(argv), remote_argv


def parse_local_arguments(argv):
    """
    Strips out arguments that are not to be passed through to runs.

    Add any arguments that should not be passed to remote test_that runs here.

    @param argv: argument list to parse.
    @returns: tuple of local argument parser and remaining argv.
    """
    parser = argparse.ArgumentParser(add_help=False)
    parser.add_argument('-w', '--web', dest='web', default=None,
                        help='Address of a webserver to receive test requests.')
    _, remaining_argv = parser.parse_known_args(argv)
    return parser, remaining_argv


def perform_bootstrap_into_autotest_root(arguments, autotest_path, argv):
    """
    Perfoms a bootstrap to run test_that from the |autotest_path|.

    This function is to be called from test_that's main() script, when
    test_that is executed from the source tree location. It runs
    autotest_quickmerge to update the sysroot unless arguments.no_quickmerge
    is set. It then executes and waits on the version of test_that.py
    in |autotest_path|.

    @param arguments: A parsed arguments object, as returned from
                      test_that.parse_arguments(...).
    @param autotest_path: Full absolute path to the autotest root directory.
    @param argv: The arguments list, as passed to main(...)

    @returns: The return code of the test_that script that was executed in
              |autotest_path|.
    """
    logging_manager.configure_logging(
            server_logging_config.ServerLoggingConfig(),
            use_console=True,
            verbose=arguments.debug)
    if arguments.no_quickmerge:
        logging.info('Skipping quickmerge step.')
    else:
        logging.info('Running autotest_quickmerge step.')
        command = [_QUICKMERGE_SCRIPTNAME, '--board='+arguments.board]
        s = subprocess.Popen(command,
                             stdout=subprocess.PIPE,
                             stderr=subprocess.STDOUT)
        for message in iter(s.stdout.readline, b''):
            logging.info('quickmerge| %s', message.strip())
        return_code = s.wait()
        if return_code:
            raise test_runner_utils.TestThatRunError(
                    'autotest_quickmerge failed with error code %s.' %
                    return_code)

    logging.info('Re-running test_that script in %s copy of autotest.',
                 autotest_path)
    script_command = os.path.join(autotest_path, 'site_utils',
                                  'test_that.py')
    if not os.path.exists(script_command):
        raise test_runner_utils.TestThatRunError(
            'Unable to bootstrap to autotest root, %s not found.' %
            script_command)
    proc = None
    def resend_sig(signum, stack_frame):
        #pylint: disable-msg=C0111
        if proc:
            proc.send_signal(signum)
    signal.signal(signal.SIGINT, resend_sig)
    signal.signal(signal.SIGTERM, resend_sig)

    proc = subprocess.Popen([script_command] + argv)

    return proc.wait()


def _main_for_local_run(argv, arguments):
    """
    Effective entry point for local test_that runs.

    @param argv: Script command line arguments.
    @param arguments: Parsed command line arguments.
    """
    if not cros_build_lib.IsInsideChroot():
        print >> sys.stderr, 'For local runs, script must be run inside chroot.'
        return 1

    results_directory = test_runner_utils.create_results_directory(
            arguments.results_dir)
    test_runner_utils.add_ssh_identity(results_directory,
                                       arguments.ssh_private_key)
    arguments.results_dir = results_directory

    # If the board has not been specified through --board, and is not set in the
    # default_board file, determine the board by ssh-ing into the host. Also
    # prepend it to argv so we can re-use it when we run test_that from the
    # sysroot.
    if arguments.board is None:
        arguments.board = _get_board_from_host(arguments.remote)
        argv = ['--board', arguments.board] + argv

    if arguments.autotest_dir:
        autotest_path = arguments.autotest_dir
        arguments.no_quickmerge = True
    else:
        sysroot_path = os.path.join('/build', arguments.board, '')

        if not os.path.exists(sysroot_path):
            print >> sys.stderr, ('%s does not exist. Have you run '
                                  'setup_board?' % sysroot_path)
            return 1

        path_ending = 'usr/local/build/autotest'
        autotest_path = os.path.join(sysroot_path, path_ending)

    site_utils_path = os.path.join(autotest_path, 'site_utils')

    if not os.path.exists(autotest_path):
        print >> sys.stderr, ('%s does not exist. Have you run '
                              'build_packages? Or if you are using '
                              '--autotest-dir, make sure it points to '
                              'a valid autotest directory.' % autotest_path)
        return 1

    realpath = os.path.realpath(__file__)
    site_utils_path = os.path.realpath(site_utils_path)

    # If we are not running the sysroot version of script, perform
    # a quickmerge if necessary and then re-execute
    # the sysroot version of script with the same arguments.
    if os.path.dirname(realpath) != site_utils_path:
        return perform_bootstrap_into_autotest_root(
                arguments, autotest_path, argv)
    else:
        return test_runner_utils.perform_run_from_autotest_root(
                autotest_path, argv, arguments.tests, arguments.remote,
                build=arguments.build, board=arguments.board,
                args=arguments.args, ignore_deps=not arguments.enforce_deps,
                results_directory=results_directory,
                ssh_verbosity=arguments.ssh_verbosity,
                ssh_options=arguments.ssh_options,
                iterations=arguments.iterations,
                fast_mode=arguments.fast_mode, debug=arguments.debug,
                whitelist_chrome_crashes=arguments.whitelist_chrome_crashes)


def _main_for_lab_run(argv, arguments):
    """
    Effective entry point for lab test_that runs.

    @param argv: Script command line arguments.
    @param arguments: Parsed command line arguments.
    """
    autotest_path = os.path.realpath(os.path.join(os.path.dirname(__file__),
                                                  '..'))
    flattened_argv = ' '.join([pipes.quote(item) for item in argv])
    command = [os.path.join(autotest_path, 'site_utils',
                            'run_suite.py'),
               '--board', arguments.board,
               '--build', arguments.build,
               '--suite_name', 'test_that_wrapper',
               '--pool', arguments.pool,
               '--suite_args', flattened_argv]
    if arguments.web:
        command.extend(['--web', arguments.web])
    logging.info('About to start lab suite with command %s.', command)
    return subprocess.call(command)


def main(argv):
    """
    Entry point for test_that script.

    @param argv: arguments list
    """
    arguments, remote_argv = _parse_arguments_internal(argv)
    try:
        validate_arguments(arguments)
    except ValueError as err:
        print >> sys.stderr, ('Invalid arguments. %s' % err.message)
        return 1

    if arguments.remote == ':lab:':
        return _main_for_lab_run(remote_argv, arguments)
    else:
        return _main_for_local_run(argv, arguments)


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
