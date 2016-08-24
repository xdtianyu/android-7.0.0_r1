# Copyright (c) 2009 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from autotest_lib.client.common_lib.cros import tpm_utils
from autotest_lib.server import test, autotest


class enterprise_RemoraRequisitionServer(test.test):
    """A test that runs enterprise_RemoraRequisition and clears the TPM as
    necessary."""
    version = 1

    def run_once(self, host=None):
        self.client = host

        tpm_utils.ClearTPMOwnerRequest(self.client)
        autotest.Autotest(self.client).run_test('enterprise_RemoraRequisition')
        tpm_utils.ClearTPMOwnerRequest(self.client)
