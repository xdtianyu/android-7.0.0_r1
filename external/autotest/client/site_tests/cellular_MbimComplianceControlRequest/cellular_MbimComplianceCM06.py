# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import common
from autotest_lib.client.cros.cellular.mbim_compliance import mbim_constants
from autotest_lib.client.cros.cellular.mbim_compliance import mbim_errors
from autotest_lib.client.cros.cellular.mbim_compliance \
        import mbim_test_base
from autotest_lib.client.cros.cellular.mbim_compliance.sequences \
        import get_descriptors_sequence
from autotest_lib.client.cros.cellular.mbim_compliance.sequences \
        import mbim_cid_device_caps_sequence
from autotest_lib.client.cros.cellular.mbim_compliance.sequences \
        import mbim_open_generic_sequence

class cellular_MbimComplianceCM06(mbim_test_base.MbimTestBase):
    """
    CM_06 Validation of |status_codes| in modem's response to MBIM_COMMAND_MSG.

    This test verifies that the function returns MBIM_STATUS_SUCCESS in Status
    field of MBIM_COMMAND_DONE response in case of a successfully executed
    command.

    Reference:
        [1] Universal Serial Bus Communication Class MBIM Compliance Testing: 39
        http://www.usb.org/developers/docs/devclass_docs/MBIM-Compliance-1.0.pdf
    """
    version = 1

    def run_internal(self):
        """ Run CM_06 test. """
        # Precondition
        descriptors = get_descriptors_sequence.GetDescriptorsSequence(
                self.device_context).run()
        self.device_context.update_descriptor_cache(descriptors)
        mbim_open_generic_sequence.MBIMOpenGenericSequence(
                self.device_context).run()

        # Step 1
        _, response_message = (
                mbim_cid_device_caps_sequence.MBIMCIDDeviceCapsSequence(
                        self.device_context).run())
        if response_message.status_codes != mbim_constants.MBIM_STATUS_SUCCESS:
            mbim_errors.log_and_raise(mbim_errors.MBIMComplianceAssertionError,
                                      'mbim1.0:9.4.5#1')
