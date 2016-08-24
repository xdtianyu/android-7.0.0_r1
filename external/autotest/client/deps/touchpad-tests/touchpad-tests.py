# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import os

# Setup autotest_lib path by importing common.
import common
from autotest_lib.client.bin import utils


version = 1


def setup(setup_dir):
    """Stores a copy of the chromite cbuildbot source code for use on a DUT.

    @param setup_dir: the target directory

    """
    logging.info('setup(%s)', setup_dir)

pwd = os.getcwd()
utils.update_version(os.getcwd(), True, version, setup, pwd)
