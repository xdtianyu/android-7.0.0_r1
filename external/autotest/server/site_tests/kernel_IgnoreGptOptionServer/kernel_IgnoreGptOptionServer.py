# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from autotest_lib.client.common_lib import error
from autotest_lib.server import test

class kernel_IgnoreGptOptionServer(test.test):
    """Test to check that the kernel is ignoring the cmd line option 'gpt'.
    """
    version = 1


    def run_once(self, host=None):
        # Check if gpt option is present on the command line.
        try:
            host.run('cat /proc/cmdline | grep -E "( gpt)|(gpt )"')
        except error.AutoservRunError:
            raise error.TestNAError('No need to check that "gpt" is ignored '
                                    'by the kernel on this device.')

        # Reboot the client
        host.reboot()

        try:
            msg = 'Not forcing GPT even though \'gpt\' specified on cmd line.'
            host.run ('dmesg | grep "%s"' % msg)
        except error.AutoservRunError:
            raise error.TestFail('The option "gpt" not ignored by the kernel.')
