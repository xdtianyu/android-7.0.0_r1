# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging, re

from autotest_lib.client.common_lib import error
from autotest_lib.server import test

class kernel_EmptyLines(test.test):
    version = 1

    def run_once(self, host=None):
        self.client = host

        # Reboot the client
        logging.info('kernel_EmptyLines: reboot %s' % self.client.hostname)
        self.client.reboot()

        # Get dmesg since boot and check for empty printk lines.
        # Format is from start of line: '[   x.yyyyyy] ' where x.y is
        # the timestamp.
        #
        # A typical example for an error:
        # [ 3.799802] device-mapper: init: foo bar
        # [ 3.799807]
        # [ 3.799921] device-mapper: done

        result = self.client.run('dmesg')
        match = re.search('^\[[\s0-9\.]+\]\s*$', result.stdout, re.M)

        lines = result.stdout.count('\n')

        if match:
            raise error.TestFail("Found an empty line in dmesg: '%s'" %
                                 match.group(0))
        elif lines < 5:
            raise error.TestFail("Only got %d lines of dmesg" % lines)
        else:
            logging.info('kernel_EmptyLines: checked %d lines' % lines)
