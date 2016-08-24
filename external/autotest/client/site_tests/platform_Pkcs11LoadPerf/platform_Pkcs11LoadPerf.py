# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import re
from autotest_lib.client.bin import test, utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.cros import pkcs11

class platform_Pkcs11LoadPerf(test.test):
    """This tests the performance of loading a PKCS #11 token."""

    version = 1

    def run_once(self):
        pkcs11.setup_p11_test_token(True)
        pkcs11.load_p11_test_token()
        # Prepare the token with a key.
        utils.system('p11_replay --inject')
        pkcs11.unload_p11_test_token()
        pkcs11.load_p11_test_token()
        # List the objects and gather timing data.
        output = utils.system_output('p11_replay --list_objects')
        # The output will have multiple lines like 'Elapsed: 25ms'. We are
        # expecting at least three:
        # 1) How long it took to open a session.
        # 2) How long it took to list public objects.
        # 3) How long it took to list private objects.
        # The following extracts the numeric value from each timing statement.
        time_list = [int(match.group(1)) for match in
            re.finditer(r'Elapsed: (\d+)ms', output, flags=re.MULTILINE)]
        if len(time_list) < 3:
            error.TestFail('Expected output not found.')
        self.output_perf_value(description='Key_Ready',
                               value=(time_list[0] + time_list[1] + time_list[2]),
                               units='ms', higher_is_better=False)
        self.write_perf_keyval(
            {'cert_ready_ms': time_list[0] + time_list[1],
             'key_ready_ms': time_list[0] + time_list[1] + time_list[2]})
        pkcs11.cleanup_p11_test_token()
