# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import common
from autotest_lib.client.cros.cellular.mbim_compliance \
        import mbim_command_message
from autotest_lib.client.cros.cellular.mbim_compliance import mbim_errors
from autotest_lib.client.cros.cellular.mbim_compliance import mbim_test_base
from autotest_lib.client.cros.cellular.mbim_compliance.sequences \
        import get_descriptors_sequence
from autotest_lib.client.cros.cellular.mbim_compliance.sequences \
        import mbim_cid_device_caps_sequence
from autotest_lib.client.cros.cellular.mbim_compliance.sequences \
        import mbim_open_generic_sequence


class cellular_MbimComplianceCM15(mbim_test_base.MbimTestBase):
    """
    CM_15 Validation of message fragmentation ability.

    This test verifies that the function follows the rules of control message
    fragmentation.

    Reference:
        [1] Universal Serial Bus Communication Class MBIM Compliance Testing: 43
        http://www.usb.org/developers/docs/devclass_docs/MBIM-Compliance-1.0.pdf

    """
    version = 1

    def run_internal(self):
        """ Run CM_15 test. """
        # Precondition
        descriptors = get_descriptors_sequence.GetDescriptorsSequence(
                self.device_context).run()
        self.device_context.update_descriptor_cache(descriptors)
        open_sequence = mbim_open_generic_sequence.MBIMOpenGenericSequence(
                self.device_context)
        open_sequence.run(max_control_transfer_size=64)

        # Step 1
        caps_sequence = mbim_cid_device_caps_sequence.MBIMCIDDeviceCapsSequence(
                self.device_context)
        _, response_message = caps_sequence.run()
        if not isinstance(response_message,
                          mbim_command_message.MBIMDeviceCapsInfo):
            mbim_errors.log_and_raise(mbim_errors.MBIMComplianceAssertionError,
                                      'mbim1.0:9.2')
