# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from autotest_lib.client.common_lib import error
from autotest_lib.server import autotest, test
import time

class platform_CompromisedStatefulPartition(test.test):
    """Tests how the system recovers with the corrupted stateful partition.
    """
    version = 2

    CMD_CORRUPT = 'rm -fr /mnt/stateful_partition/*.*'
    OOBE_FILE = '/home/chronos/.oobe_completed'
    _WAIT_DELAY = 2
    FILES_LIST = [
        '/mnt/stateful_partition/dev_image',
        '/mnt/stateful_partition/encrypted.key',
        '/mnt/stateful_partition/encrypted.block',
        '/mnt/stateful_partition/unencrypted',
    ]


    def run_once(self, host, client_autotest):
        """This test verify that user should get OOBE after booting
        the device with corrupted statefull partition.
        Test fails if not able to recover the device with corrupted
        stateful partition.
        """
        if host.get_board_type() == 'OTHER':
            raise error.TestNAError('Test can not processed on OTHER board type devices')
        autotest_client = autotest.Autotest(host)
        host.reboot()
        autotest_client.run_test(client_autotest,
                                 exit_without_logout=True)
        if not host.run(self.CMD_CORRUPT,
                        ignore_status=True).exit_status == 0:
             raise error.TestFail('Unable to corrupt stateful partition')
        host.run('sync', ignore_status=True)
        time.sleep(self._WAIT_DELAY)
        host.reboot()
        host.run('sync', ignore_status=True)
        time.sleep(self._WAIT_DELAY)
        if host.path_exists(self.OOBE_FILE):
            raise error.TestFail('Did not get OOBE screen after '
                                 'rebooting the device with '
                                 'corrupted statefull partition')
        autotest_client.run_test(client_autotest,
                                 exit_without_logout=True)
        time.sleep(self._WAIT_DELAY)
        for new_file in self.FILES_LIST:
            if not host.path_exists(new_file):
                raise error.TestFail('%s is missing after rebooting '
                                     'the device with corrupted '
                                     'statefull partition' % new_file)

