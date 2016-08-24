import os
from autotest_lib.client.bin import test, utils


class fio(test.test):
    """Simple test that runs fio."""
    version = 2

    def run_once(self, args = '', user = 'root'):
        log = os.path.join(self.resultsdir, 'fio-mixed.log')
        job = os.path.join(self.bindir, 'fio-mixed.job')
        utils.system(' '.join(['fio', '--output', log, job]))
