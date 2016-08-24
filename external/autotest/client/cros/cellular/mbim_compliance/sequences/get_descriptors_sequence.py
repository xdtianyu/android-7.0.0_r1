# Copyright (c) 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""
Get Descriptor Sequence

Reference:
  [1] Universal Serial Bus Communication Class MBIM Compliance Testing: 18
      http://www.usb.org/developers/docs/devclass_docs/MBIM-Compliance-1.0.pdf
"""

from usb import control
from usb import util

import common
from autotest_lib.client.cros.cellular.mbim_compliance import mbim_errors
from autotest_lib.client.cros.cellular.mbim_compliance.sequences import sequence
from autotest_lib.client.cros.cellular.mbim_compliance.usb_descriptors import \
  DescriptorParser


class GetDescriptorsSequence(sequence.Sequence):
    """
    Implement the Get Descriptor Sequence.
    Given the vendor and product id for a USB device, obtains the USB
    descriptors for that device.
    """

    def run_internal(self):
        """
        Run the Get Descriptor Sequence.

        @returns a list of descriptor objects.

        """
        if self.device_context is None:
            mbim_errors.log_and_raise(mbim_errors.MBIMComplianceFrameworkError,
                                      'Test context not found')
        device = self.device_context.device
        if device is None:
            mbim_errors.log_and_raise(mbim_errors.MBIMComplianceFrameworkError,
                                      'Device %04X:%04X not found' %
                                      (self.device_context.id_vendor,
                                       self.device_context.id_product))

        configuration = device.get_active_configuration()

        # Get the actual wTotalLength by retrieving partial descriptor.
        # desc_index corresponds to the index of a configuration. Note that
        # index is of 0 base while configuration is of 1 base.
        descriptors_byte_stream = control.get_descriptor(
                dev=device,
                desc_size=9,
                desc_type=util.DESC_TYPE_CONFIG,
                desc_index=configuration.bConfigurationValue - 1,
                wIndex=0)
        if descriptors_byte_stream is None:
            mbim_errors.log_and_raise(
                    mbim_errors.MBIMComplianceSequenceError,
                    'Failed to find configuration descriptor '
                    'for active configuration of device '
                    '%04X:%04X' % (device.idVendor, device.idProduct))

        # Verify returned data is the requested size.
        descriptor_length = descriptors_byte_stream[0]
        if descriptor_length != 9:
            mbim_errors.log_and_raise(mbim_errors.MBIMComplianceSequenceError,
                                      'Expected bLength to be 9, got %d.' % (
                                      descriptor_length))

        descriptors_byte_stream = control.get_descriptor(
                dev=device,
                desc_size=descriptors_byte_stream[2],
                desc_type=util.DESC_TYPE_CONFIG,
                desc_index=configuration.bConfigurationValue - 1,
                wIndex=0)
        descriptors = [descriptor for descriptor
                                  in DescriptorParser(descriptors_byte_stream)]
        return descriptors
