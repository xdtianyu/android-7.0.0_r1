#!/usr/bin/python
#pylint: disable-msg=C0111

import unittest
import common
#pylint: disable-msg=W0611
from autotest_lib.frontend import setup_django_lite_environment
from autotest_lib.frontend.afe import frontend_test_utils
from autotest_lib.frontend.afe import models

class JobTest(unittest.TestCase, frontend_test_utils.FrontendTestMixin):
    """
    Test that a jobs can be created when using django as set in
    setup_django_lite_environment.
    """
    def setUp(self):
        self._frontend_common_setup()

    def tearDown(self):
        self._frontend_common_teardown()

    def test_job_creation(self):
        self._create_job()
        self.assertEqual(1, models.Job.objects.all().count())


if __name__ == '__main__':
    unittest.main()
