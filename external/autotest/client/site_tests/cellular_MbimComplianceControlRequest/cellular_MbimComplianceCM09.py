# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import common
from autotest_lib.client.cros.cellular.mbim_compliance import mbim_errors
from autotest_lib.client.cros.cellular.mbim_compliance \
        import mbim_test_base
from autotest_lib.client.cros.cellular.mbim_compliance.sequences \
        import connect_sequence
from autotest_lib.client.cros.cellular.mbim_compliance.sequences \
        import get_descriptors_sequence
from autotest_lib.client.cros.cellular.mbim_compliance.sequences \
        import mbim_open_generic_sequence


class cellular_MbimComplianceCM09(mbim_test_base.MbimTestBase):
    """
    CM_09 Validation of TransactionId for notifications received after connect
    sequence.

    This test verifies that TransactionId for notifications is zero.

    Reference:
        [1] Universal Serial Bus Communication Class MBIM Compliance Testing: 41
        http://www.usb.org/developers/docs/devclass_docs/MBIM-Compliance-1.0.pdf

    """
    version = 1

    def run_internal(self):
        """ Run CM_09 test. """
        # Precondition
        descriptors = get_descriptors_sequence.GetDescriptorsSequence(
                self.device_context).run()
        self.device_context.update_descriptor_cache(descriptors)
        mbim_open_generic_sequence.MBIMOpenGenericSequence(
                self.device_context).run()

        # Step 1
        _, _, notifications = (
                connect_sequence.ConnectSequence(self.device_context).run())

        # Step 2
        for notification in notifications:
            if notification.transaction_id != 0:
                mbim_errors.log_and_raise(
                        mbim_errors.MBIMComplianceAssertionError,
                        'mbim1.0:9.1#1')
