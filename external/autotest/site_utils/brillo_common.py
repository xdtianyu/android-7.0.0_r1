# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import argparse
import logging
import os
import sys

import common
from autotest_lib.client.common_lib import error
from autotest_lib.server import hosts
from autotest_lib.server import utils
from autotest_lib.server.hosts import moblab_host
from autotest_lib.server.hosts import ssh_host


_LOGGING_FORMAT = '%(asctime)s - %(levelname)s - %(message)s'
_TEST_LAUNCH_SCRIPT = 'brillo_test_launcher.py'

# Running against a virtual machine has several intricacies that we need to
# adjust for. Namely SSH requires the use of 'localhost' while HTTP requires
# the use of '127.0.0.1'. Also because we are forwarding the ports from the VM
# to the host system, the ports to use for these services are also different
# from running on a physical machine.
_VIRT_MACHINE_SSH_ADDR = 'localhost:9222'
_VIRT_MACHINE_AFE_ADDR = '127.0.0.1:8888'
_VIRT_MACHINE_DEVSERVER_PORT = '7777'
_PHYS_MACHINE_DEVSERVER_PORT = '8080'
_MOBLAB_MIN_VERSION = 7569
_MOBLAB_IMAGE_DOWNLOAD_URL = ('https://storage.googleapis.com/chromeos-image-'
                              'archive/moblab_brillo_images/'
                              'moblab_brillo_%s.bin' % _MOBLAB_MIN_VERSION)


class BrilloTestError(Exception):
    """A general error while testing Brillo."""


class BrilloMoblabInitializationError(BrilloTestError):
    """An error during Moblab initialization or handling."""


def get_moblab_and_devserver_port(moblab_hostname):
    """Initializes and returns a MobLab Host Object.

    @params moblab_hostname: The Moblab hostname, None if using a local virtual
                             machine.

    @returns A pair consisting of a MoblabHost and a devserver port.

    @raise BrilloMoblabInitializationError: Failed to set up the Moblab.
    """
    if moblab_hostname:
        web_address = moblab_hostname
        devserver_port = _PHYS_MACHINE_DEVSERVER_PORT
        rpc_timeout_min = 2
    else:
        moblab_hostname = _VIRT_MACHINE_SSH_ADDR
        web_address = _VIRT_MACHINE_AFE_ADDR
        devserver_port = _VIRT_MACHINE_DEVSERVER_PORT
        rpc_timeout_min = 5

    try:
        host = hosts.create_host(moblab_hostname,
                                 host_class=moblab_host.MoblabHost,
                                 connectivity_class=ssh_host.SSHHost,
                                 web_address=web_address,
                                 retain_image_storage=True,
                                 rpc_timeout_min=rpc_timeout_min)
    except error.AutoservRunError as e:
        raise BrilloMoblabInitializationError(
                'Unable to connect to the MobLab: %s' % e)

    moblab_version = int(host.get_release_version().split('.')[0])
    if moblab_version < _MOBLAB_MIN_VERSION:
        raise BrilloMoblabInitializationError(
                'The Moblab version (%s) is older than the minimum required '
                '(%s). Download a current version from URL: %s' %
                (moblab_version, _MOBLAB_MIN_VERSION,
                _MOBLAB_IMAGE_DOWNLOAD_URL))

    try:
        host.afe.get_hosts()
    except Exception as e:
        raise BrilloMoblabInitializationError(
                "Unable to communicate with the MobLab's web frontend, "
                "please verify that it is up and running at http://%s/\n"
                "Error: %s" % (host.web_address, e))

    return host, devserver_port


def parse_args(description, setup_parser=None, validate_args=None):
    """Parse command-line arguments.

    @param description: The script description in the help message.
    @param setup_parser: Function that takes a parser object and adds
                         script-specific options to it.
    @param validate_args: Function that takes a parser object and the parsed
                          arguments and validates the arguments. It should use
                          parser.error() to report errors.

    @return Parsed and validated arguments.
    """
    parser = argparse.ArgumentParser(description=description)
    if setup_parser:
        setup_parser(parser)

    # Add common options.
    parser.add_argument('-m', '--moblab_host',
                        help='MobLab hostname or IP to launch tests. If this '
                             'argument is not provided, the test launcher '
                             'will attempt to test a local virtual machine '
                             'instance of MobLab.')
    parser.add_argument('-a', '--adb_host',
                        help='Hostname or IP of the adb_host connected to the '
                             'Brillo DUT. Default is to assume it is connected '
                             'directly to the MobLab.')
    parser.add_argument('-n', '--no_quickmerge', dest='quickmerge',
                        action='store_false',
                        help='Do not update the Autotest code on the Moblab')
    parser.add_argument('-d', '--debug', action='store_true',
                        help='Print log statements.')

    args = parser.parse_args()

    # Configure the root logger.
    logging.getLogger().setLevel(logging.DEBUG if args.debug else logging.INFO)
    for log_handler in logging.getLogger().handlers:
        log_handler.setFormatter(logging.Formatter(fmt=_LOGGING_FORMAT))

    if validate_args:
        validate_args(parser, args)

    return args


def setup_test_action_parser(parser):
    """Add parser options related to test action.

    @param parser: argparse.ArgumentParser of the script.
    """
    launch_opts = parser.add_mutually_exclusive_group()
    launch_opts.add_argument('-A', '--print_args', action='store_true',
                             help='Print test arguments to stdout instead of '
                                  'launching the test.')
    launch_opts.add_argument('-C', '--print_command', action='store_true',
                             help='Print complete test launch command instead '
                                  'of launching the test.')


def _get_arg_strs(test_args):
    """Converts an argument dictionary into a list of 'arg=val' strings."""
    return ['%s=%s' % kv for kv in test_args.iteritems()]


def _get_command(moblab, test_name, test_args, do_quickmerge, do_quote):
    """Returns the test launch command.

    @param moblab: MoblabHost representing the MobLab being used for testing.
    @param test_name: The name of the test to run.
    @param test_args: Dictionary of test arguments.
    @param do_quickmerge: If False, pass the --no-quickmerge flag.
    @param do_quote: If True, add single-quotes around test arguments.

    @return Test launch command as a list of strings.
    """
    # pylint: disable=missing-docstring
    def quote(val):
        return "'%s'" % val if do_quote else val

    cmd = [os.path.join(os.path.dirname(__file__), _TEST_LAUNCH_SCRIPT),
           '-t', quote(test_name)]
    if not do_quickmerge:
        cmd.append('-n')
    if not moblab.hostname.startswith('localhost'):
           cmd += ['-m', quote(moblab.hostname)]
    for arg_str in _get_arg_strs(test_args):
        cmd += ['-A', quote(arg_str)]
    return cmd


def _print_args(test_args):
    """Prints the test arguments to stdout, one per line.

    @param test_args: Dictionary of test arguments.
    """
    print('\n'.join(_get_arg_strs(test_args)))


def _print_command(moblab, test_name, test_args, do_quickmerge):
    """Prints the test launch command to stdout with quoting.

    @param moblab: MoblabHost representing the MobLab being used for testing.
    @param test_name: The name of the test to run.
    @param test_args: Dictionary of test arguments.
    @param do_quickmerge: If False, pass the --no-quickmerge flag.
    """
    print(' '.join(
            _get_command(moblab, test_name, test_args, do_quickmerge, True)))


def _run_command(moblab, test_name, test_args, do_quickmerge):
    """Runs the test launch script.

    @param moblab: MoblabHost representing the MobLab being used for testing.
    @param test_name: The name of the test to run.
    @param test_args: Dictionary of test arguments.
    @param do_quickmerge: If False, pass the --no_quickmerge flag.
    """
    utils.run(_get_command(moblab, test_name, test_args, do_quickmerge, False),
              stdout_tee=sys.stdout, stderr_tee=sys.stderr)


def do_test_action(args, moblab, test_name, test_args):
    """Performs the desired action related to the test.

    @param args: Parsed arguments.
    @param moblab: MoblabHost representing the MobLab being used for testing.
    @param test_name: The name of the test to run.
    @param test_args: Dictionary of test arguments.
    """
    if args.print_args:
        logging.info('Printing test arguments')
        _print_args(test_args)
    elif args.print_command:
        logging.info('Printing test launch command')
        _print_command(moblab, test_name, test_args, args.quickmerge)
    else:
        logging.info('Launching test')
        _run_command(moblab, test_name, test_args, args.quickmerge)
