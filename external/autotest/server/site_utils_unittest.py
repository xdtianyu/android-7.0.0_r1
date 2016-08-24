# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import unittest

import common
from autotest_lib.frontend import setup_django_lite_environment
from autotest_lib.frontend.afe import site_rpc_interface
from autotest_lib.server import site_utils
from autotest_lib.server.cros.dynamic_suite import tools


class SiteUtilsUnittests(unittest.TestCase):
    """Test functions in site_utils.py
    """

    def testParseJobName(self):
        """Test method parse_job_name.
        """
        trybot_paladin_build = 'trybot-lumpy-paladin/R27-3837.0.0-b123'
        trybot_release_build = 'trybot-lumpy-release/R27-3837.0.0-b456'
        release_build = 'lumpy-release/R27-3837.0.0'
        paladin_build = 'lumpy-paladin/R27-3878.0.0-rc7'

        builds = [trybot_paladin_build, trybot_release_build, release_build,
                  paladin_build]
        test_name = 'login_LoginSuccess'
        board = 'lumpy'
        suite = 'bvt'
        for build in builds:
            expected_info = {'board': board,
                             'suite': suite,
                             'build': build,
                             'build_version': build.split('/')[1]}
            suite_job_name = ('%s-%s' %
                    (build, site_rpc_interface.canonicalize_suite_name(suite)))
            info = site_utils.parse_job_name(suite_job_name)
            self.assertEqual(info, expected_info, '%s failed to be parsed to '
                             '%s' % (suite_job_name, expected_info))
            test_job_name = tools.create_job_name(build, suite, test_name)
            info = site_utils.parse_job_name(test_job_name)
            self.assertEqual(info, expected_info, '%s failed to be parsed to '
                             '%s' % (test_job_name, expected_info))


if __name__ == '__main__':
    unittest.main()
