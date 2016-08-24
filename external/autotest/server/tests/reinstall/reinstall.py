import common
from autotest_lib.client.common_lib import error
from autotest_lib.server import afe_utils
from autotest_lib.server import test

class reinstall(test.test):
    version = 1

    def execute(self, host):
        try:
            afe_utils.machine_install_and_update_labels(host)
        except Exception, e:
            raise error.TestFail(str(e))
