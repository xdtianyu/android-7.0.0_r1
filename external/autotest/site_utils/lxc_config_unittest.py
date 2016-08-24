#!/usr/bin/env python
# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import json
import os
import unittest

import common

from autotest_lib.site_utils import lxc_config


class DeployConfigTest(unittest.TestCase):
    """Test DeployConfigManager.
    """

    def testValidate(self):
        """Test ssp_deploy_config.json can be validated.
        """
        global_deploy_config_file = os.path.join(
                common.autotest_dir, lxc_config.SSP_DEPLOY_CONFIG_FILE)
        with open(global_deploy_config_file) as f:
            deploy_configs = json.load(f)
        for config in deploy_configs:
            lxc_config.DeployConfigManager.validate(config)


if '__main__':
    unittest.main()