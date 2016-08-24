#!/usr/bin/python
#pylint: disable-msg=C0111
import unittest
import common
#pylint: disable-msg=W0611
from autotest_lib.frontend import setup_django_lite_environment
from autotest_lib.frontend.afe import direct_afe


class DirectAFETest(unittest.TestCase):
    def testEntryCreation(self):
        afe = direct_afe.directAFE()

        jobs = afe.get_jobs()
        self.assertEquals(len(jobs), 0)

        hosts = afe.get_hosts()
        self.assertEquals(len(hosts), 0)

        afe.create_host('a_host')
        hosts = afe.get_hosts()
        self.assertEquals(len(hosts), 1)

        afe.create_job('job_name', hosts=['a_host'])
        jobs = afe.get_jobs()
        self.assertEquals(len(jobs), 1)

if __name__ == '__main__':
    unittest.main()
