# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from autotest_lib.client.common_lib import error
from autotest_lib.server import test

class platform_UReadAheadServer(test.test):
    """Test to check whether ureadahead pack files are created on reboot.

    The test first deletes the existing pack files, reboots and then checks
    that new pack files are created on reboot.
    """
    version = 1


    def run_once(self, host=None):
        # First delete the pack files.
        host.run('rm -rf /var/lib/ureadahead')

        # Reboot the client
        host.reboot()

        # Check if the ureadahead pack files were created on reboot.
        try:
            host.run('ls /var/lib/ureadahead/*pack')
        except error.AutoservRunError:
            raise error.TestFail('"ureadahead" pack files were not created on '
                                 'reboot')
