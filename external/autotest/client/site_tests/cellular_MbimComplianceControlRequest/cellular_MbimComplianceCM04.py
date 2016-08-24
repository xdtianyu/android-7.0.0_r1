# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import common
from autotest_lib.client.cros.cellular.mbim_compliance import mbim_errors
from autotest_lib.client.cros.cellular.mbim_compliance \
        import mbim_test_base
from autotest_lib.client.cros.cellular.mbim_compliance.sequences \
        import get_descriptors_sequence
from autotest_lib.client.cros.cellular.mbim_compliance.sequences \
        import mbim_cid_device_caps_sequence
from autotest_lib.client.cros.cellular.mbim_compliance.sequences \
        import mbim_open_generic_sequence


class cellular_MbimComplianceCM04(mbim_test_base.MbimTestBase):
    """
    CM_04 Validation of |transaction_id| in modem's response to MBIM_COMMAND_MSG

    This section contains tests that validate the specifics of MBIM_COMMAND_MSG
    request and the function's response.

    Reference:
        [1] Universal Serial Bus Communication Class MBIM Compliance Testing: 38
        http://www.usb.org/developers/docs/devclass_docs/MBIM-Compliance-1.0.pdf
    """
    version = 1

    def run_internal(self):
        """ Run CM_04 test. """
        # Precondition
        descriptors = get_descriptors_sequence.GetDescriptorsSequence(
                self.device_context).run()
        self.device_context.update_descriptor_cache(descriptors)
        mbim_open_generic_sequence.MBIMOpenGenericSequence(
                self.device_context).run()

        # Step 1
        command_message, response_message = (
                mbim_cid_device_caps_sequence.MBIMCIDDeviceCapsSequence(
                        self.device_context).run())
        # Validate |transaction_id| in the response to MBIM_COMMAND_MSG.
        if response_message.transaction_id != command_message.transaction_id:
            mbim_errors.log_and_raise(mbim_errors.MBIMComplianceAssertionError,
                                      'mbim1.0:9.4.3')
