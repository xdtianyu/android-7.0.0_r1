# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import common
from autotest_lib.client.cros.cellular.mbim_compliance import mbim_constants
from autotest_lib.client.cros.cellular.mbim_compliance import mbim_dts_test_base
from autotest_lib.client.cros.cellular.mbim_compliance import mbim_errors
from autotest_lib.client.cros.cellular.mbim_compliance.sequences \
        import loopback_sequence


class cellular_MbimComplianceDTS1622(mbim_dts_test_base.MbimDtsTestBase):
    """
    Validation of wDatagramIndex[0].

    This test validates the value in wDatagramIndex[0] field of NDP16/32.

    Reference:
        [1] Universal Serial Bus Communication Class MBIM Compliance Testing: 33
        http://www.usb.org/developers/docs/devclass_docs/MBIM-Compliance-1.0.pdf

    """
    version = 1

    def run_internal(self, ntb_format):
        """
        Run DTS_16/DTS_22 test.

        @param ntb_format: Whether to send/receive an NTB16 or NTB32 frame.
                Possible values: NTB_FORMAT_16, NTB_FORMAT_32 (mbim_constants)

        """
        # Precondition
        _, _, _ = self.run_precondition(ntb_format)

        # Step 1
        loopback = loopback_sequence.LoopbackSequence(self.device_context)
        _, _, ndp_entries, _ = loopback.run(ntb_format=ntb_format)

        # Step 2
        if ntb_format == mbim_constants.NTB_FORMAT_16:
            if ndp_entries[0].datagram_index < 12:
                mbim_errors.log_and_raise(
                        mbim_errors.MBIMComplianceAssertionError,
                        'ncm1.0:3.3.1#2')
        else:
            if ndp_entries[0].datagram_index < 16:
                mbim_errors.log_and_raise(
                        mbim_errors.MBIMComplianceAssertionError,
                        'ncm1.0:3.3.2#2')
