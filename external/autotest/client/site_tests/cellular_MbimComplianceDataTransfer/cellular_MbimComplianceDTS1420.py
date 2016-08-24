# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import common
from autotest_lib.client.cros.cellular.mbim_compliance import mbim_constants
from autotest_lib.client.cros.cellular.mbim_compliance import mbim_data_transfer
from autotest_lib.client.cros.cellular.mbim_compliance import mbim_dts_test_base
from autotest_lib.client.cros.cellular.mbim_compliance import mbim_errors
from autotest_lib.client.cros.cellular.mbim_compliance.sequences \
        import loopback_sequence


class cellular_MbimComplianceDTS1420(mbim_dts_test_base.MbimDtsTestBase):
    """
    Validation of dwSignature for IP Stream.

    This test validates 16/32-bit NCM Datagram Pointer signature for IP stream.

    Reference:
        [1] Universal Serial Bus Communication Class MBIM Compliance Testing: 33
        http://www.usb.org/developers/docs/devclass_docs/MBIM-Compliance-1.0.pdf

    """
    version = 1

    def run_internal(self, ntb_format):
        """
        Run DTS_14/DTS_20 test.

        @param ntb_format: Whether to send/receive an NTB16 or NTB32 frame.
                Possible values: NTB_FORMAT_16, NTB_FORMAT_32 (mbim_constants)

        """
        # Precondition
        _, _, _ = self.run_precondition(ntb_format)

        # Step 1
        loopback = loopback_sequence.LoopbackSequence(self.device_context)
        _, ndp, _, _ = loopback.run(ntb_format=ntb_format)

        # Step 2
        if ntb_format == mbim_constants.NTB_FORMAT_16:
            if ndp.signature != mbim_data_transfer.NDP_SIGNATURE_IPS_16:
                mbim_errors.log_and_raise(
                        mbim_errors.MBIMComplianceAssertionError,
                        'mbim1.0:7#1')
        else:
            if ndp.signature != mbim_data_transfer.NDP_SIGNATURE_IPS_32:
                mbim_errors.log_and_raise(
                        mbim_errors.MBIMComplianceAssertionError,
                        'mbim1.0:7#3')
