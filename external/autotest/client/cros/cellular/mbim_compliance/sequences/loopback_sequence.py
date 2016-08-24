# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""
Loopback NTB-16/32 Sequence

Reference:
    [1] Universal Serial Bus Communication Class MBIM Compliance Testing: 20
        http://www.usb.org/developers/docs/devclass_docs/MBIM-Compliance-1.0.pdf
"""
import array

import common
from autotest_lib.client.cros.cellular.mbim_compliance import mbim_data_transfer
from autotest_lib.client.cros.cellular.mbim_compliance.sequences \
        import sequence

class LoopbackSequence(sequence.Sequence):
    """
    Data loopback sequence used for data transfer testing.

    In this sequence, we send out an IPv4 ping packet to the device which is
    in |connected| state and fetch the repsonse packet received from the device.

    """
    # Payload to be used for our test. This is an IPv4 ICMP ping packet
    DATA_PAYLOAD = array.array('B', [0x45, 0x00, 0x00, 0x54, 0xB4, 0x5A, 0x00,
                                     0x00, 0x40, 0x01, 0x42, 0xF2, 0xC0, 0xA8,
                                     0x01, 0x0B, 0xC0, 0xA8, 0x01, 0x01, 0x00,
                                     0x00, 0x54, 0xC0, 0x3C, 0xD7, 0x00, 0x08,
                                     0x6A, 0x6F, 0xB5, 0x54, 0x00, 0x00, 0x00,
                                     0x00, 0x8B, 0xC9, 0x04, 0x00, 0x00, 0x00,
                                     0x00, 0x00, 0x10, 0x11, 0x12, 0x13, 0x14,
                                     0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B,
                                     0x1C, 0x1D, 0x1E, 0x1F, 0x20, 0x21, 0x22,
                                     0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29,
                                     0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f, 0x30,
                                     0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37])

    def run_internal(self, ntb_format):
        """
        Run the MBIM Loopback Sequence.

        Need to run the |connect| sequence before invoking this loopback
        sequence.

        @param ntb_format: Whether to send/receive an NTB16 or NTB32 frame.
                Possible values: NTB_FORMAT_16, NTB_FORMAT_32 (mbim_constants)
        @returns tuple of (nth, ndp, ndp_entries, payload) where,
                nth - NTH header object received.
                ndp - NDP header object received.
                ndp_entries - Array of NDP entry header objects.
                payload - Array of packets where each packet is a byte array.

        """
        # Step 1 is to run |connect| sequence which is expected to be run
        # before calling this to avoid calling sequences within another
        # sequence.

        # Step 2
        data_transfer = mbim_data_transfer.MBIMDataTransfer(self.device_context)
        data_transfer.send_data_packets(ntb_format, [self.DATA_PAYLOAD])

        # Step 3
        nth, ndp, ndp_entries, payload = data_transfer.receive_data_packets(
                ntb_format)

        return (nth, ndp, ndp_entries, payload)
