#!/usr/bin/python

# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Module used sync and deploy infrastructure changes for the lab team.

This is the main utility for syncing and deploying changes to the autotest
infrastructure defined by the autotest config.

Usage:
  lab_deploy.py (sync,restart,print) (devservers, drones, scheduler)+.
"""
import logging
import os
import sys

import common
from autotest_lib.client.common_lib import global_config
from autotest_lib.client.common_lib import utils


import common_util

CONFIG = global_config.global_config

_AUTOTEST_PATH = '/usr/local/autotest'
_HELPER_PATH = 'site_utils/deploy_autotest'


def autotest_master():
    """Returns the autotest master that has the infrastructure configuration."""
    autotest_master = CONFIG.get_config_value('SERVER', 'hostname', type=str,
                                              default=None)
    return autotest_master


def autotest_user():
    """Returns the valid list of autotest users we can 'become' on the master.
    """
    autotest_user = CONFIG.get_config_value('CROS', 'infrastructure_user',
                                              type=str)
    return autotest_user


def main(argv):
    common_util.setup_logging()
    # We parse even though we don't use args here to ensure our args are valid
    # before we ssh anywhere.
    common_util.parse_args(argv)
    master = autotest_master()
    # Take the first user.
    main_user = autotest_user()
    remote_path = os.path.join(_AUTOTEST_PATH, _HELPER_PATH)
    command = ('ssh %(master)s -- become %(user)s -- '
               '%(remote_path)s/lab_deploy_helper.py %(argv)s' %
               dict(remote_path=remote_path, master=master,
                    user=main_user, argv=' '.join(argv)))
    logging.info('Running %s' % command)
    utils.run(command, stderr_tee=sys.stderr, stdout_tee=sys.stdout)


if __name__ == '__main__':
    main(sys.argv[1:])
