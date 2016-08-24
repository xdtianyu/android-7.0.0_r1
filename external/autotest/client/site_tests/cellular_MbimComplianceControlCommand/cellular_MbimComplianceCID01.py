# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import common
from autotest_lib.client.cros.cellular.mbim_compliance import mbim_constants
from autotest_lib.client.cros.cellular.mbim_compliance import mbim_errors
from autotest_lib.client.cros.cellular.mbim_compliance import mbim_test_base
from autotest_lib.client.cros.cellular.mbim_compliance.sequences \
        import get_descriptors_sequence
from autotest_lib.client.cros.cellular.mbim_compliance.sequences \
        import mbim_cid_device_caps_sequence
from autotest_lib.client.cros.cellular.mbim_compliance.sequences \
        import mbim_open_generic_sequence


class cellular_MbimComplianceCID01(mbim_test_base.MbimTestBase):
    """
    Validation of IP flags for functions that support CDMA.

    This test verifies that a function that supports CDMA specifies at least
    one of the following IP flags: MBIMCtrlCapsCdmaMobileIP,
    MBIMCtrlCapsCdmaSimpleIP.

    Reference:
        [1] Universal Serial Bus Communication Class MBIM Compliance Testing: 52
        http://www.usb.org/developers/docs/devclass_docs/MBIM-Compliance-1.0.pdf

    """
    version = 1

    def run_internal(self):
        """ Run the CM_01 test. """
        # Precondition.
        desc_sequence = get_descriptors_sequence.GetDescriptorsSequence(
                self.device_context)
        descriptors = desc_sequence.run()
        self.device_context.update_descriptor_cache(descriptors)
        open_sequence = mbim_open_generic_sequence.MBIMOpenGenericSequence(
                self.device_context)
        open_sequence.run()
        caps_sequence = mbim_cid_device_caps_sequence.MBIMCIDDeviceCapsSequence(
                self.device_context)
        _, caps_response = caps_sequence.run()

        # Step1
        if (caps_response.cellular_class &
            mbim_constants.CELLULAR_CLASS_MASK_CDMA):
            if not ((caps_response.control_caps &
                     mbim_constants.CTRL_CAPS_MASK_CDMA_MOBILE_IP) or
                    (caps_response.control_caps &
                     mbim_constants.CTRL_CAPS_MASK_CDMA_SIMPLE_IP)):
                mbim_errors.log_and_raise(
                        mbim_errors.MBIMComplianceAssertionError,
                        'mbim1.0:10.5.1.3#1')
