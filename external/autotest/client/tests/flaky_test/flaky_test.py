#pylint: disable-msg=C0111
import time
import random

from autotest_lib.client.bin import test
from autotest_lib.client.common_lib import error

class flaky_test(test.test):
    version = 1

    def run_once(self, seconds=1):
        if random.randint(0,1):
            raise error.TestFailRetry('Flaky test failed intentionally.')

        time.sleep(seconds)
