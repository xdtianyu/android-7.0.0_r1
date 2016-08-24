#!/usr/bin/python
# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import os
import sys

import common
try:
    # Ensure the chromite site-package is installed.
    from chromite.lib import *
except ImportError:
    import subprocess
    build_externals_path = os.path.join(
            os.path.dirname(os.path.dirname(os.path.realpath(__file__))),
            'utils', 'build_externals.py')
    subprocess.check_call([build_externals_path, 'chromiterepo'])
    # Restart the script so python now finds the autotest site-packages.
    sys.exit(os.execv(__file__, sys.argv))
from autotest_lib.site_utils import brillo_common


_TEST_NAME = 'brillo_RecoverFromBadImage'


def setup_parser(parser):
    """Add parser options.

    @param parser: argparse.ArgumentParser of the script.
    """
    parser.add_argument('-i', '--recovery_image', metavar='FILE', required=True,
                        help='Image file to use for recovery. This is a '
                             'mandatory input.')
    parser.add_argument('-p', '--partition', metavar='NAME',
                        help='Name of partition to recover. If the name ends '
                             'with "_X" then it will be substitued with the '
                             'currently active slot (e.g. "_a"). (default: '
                             'system_X)')
    parser.add_argument('-D', '--device', metavar='PATH',
                        help='Path of partition device. (default: infer from '
                             'name)')

    brillo_common.setup_test_action_parser(parser)


def main(args):
    """The main function."""
    args = brillo_common.parse_args(
            'Set up Moblab for running Brillo image recovery test, then launch '
            'the test (unless otherwise requested).',
            setup_parser=setup_parser)

    test_args = {}
    if args.partition:
        test_args['partition'] = args.partition
    if args.device:
        test_args['device'] = args.device

    moblab, _ = brillo_common.get_moblab_and_devserver_port(args.moblab_host)
    tmp_dir = moblab.make_tmp_dir()
    try:
        remote_recovery_image = os.path.join(
                tmp_dir, os.path.basename(args.recovery_image))
        moblab.send_file(args.recovery_image, remote_recovery_image)
        moblab.run('chown -R moblab:moblab %s' % tmp_dir)
        test_args['image_file'] = remote_recovery_image
        logging.info('Recovery image was staged')
        brillo_common.do_test_action(args, moblab, _TEST_NAME, test_args)
    finally:
        moblab.run('rm -rf %s' % tmp_dir)


if __name__ == '__main__':
    try:
        main(sys.argv)
        sys.exit(0)
    except brillo_common.BrilloTestError as e:
        logging.error('Error: %s', e)

    sys.exit(1)
