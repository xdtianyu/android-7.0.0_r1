# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import numpy

import common
from autotest_lib.client.cros.cellular.mbim_compliance import mbim_errors


class MBIMDataChannel(object):
    """
    Provides access to the data channel of a MBIM modem.

    The object is used to send and receive MBIM frames to/from the modem.
    The object uses the BULK-IN endpoint exposed in the data interface for any
    reads from the modem to the host.
    The object uses the BULK-OUT endpoint exposed in the data interface for any
    writes from the host to the modem.
    The channel does not deaggregate/aggregate packets into MBIM frames. The
    caller is expected to validate/provide MBIM frames to the channel. The
    channel is just used to send raw bytes to the device and read raw bytes from
    the device.

    """
    _READ_TIMEOUT_MS = 10000
    _WRITE_TIMEOUT_MS = 10000

    def __init__(self,
                 device,
                 data_interface_number,
                 bulk_in_endpoint_address,
                 bulk_out_endpoint_address,
                 max_in_buffer_size):
        """
        @param device: Device handle returned by PyUSB for the modem to test.
        @param bulk_in_endpoint_address: |bEndpointAddress| for the usb
                BULK IN endpoint from the data interface.
        @param bulk_out_endpoint_address: |bEndpointAddress| for the usb
                BULK OUT endpoint from the data interface.
        @param max_in_buffer_size: The (fixed) buffer size to used for in
                data transfers.

        """
        self._device = device
        self._data_interface_number = data_interface_number
        self._bulk_in_endpoint_address = bulk_in_endpoint_address
        self._bulk_out_endpoint_address = bulk_out_endpoint_address
        self._max_in_buffer_size = max_in_buffer_size


    def send_ntb(self, ntb):
        """
        Send the specified payload down to the device using the bulk-out USB
        pipe.

        @param ntb: Byte array of complete MBIM NTB to be sent to the device.
        @raises MBIMComplianceDataTransferError if the complete |ntb| could not
                be sent.

        """
        ntb_length = len(ntb)
        written = self._device.write(endpoint=self._bulk_out_endpoint_address,
                                     data=ntb,
                                     timeout=self._WRITE_TIMEOUT_MS,
                                     interface=self._data_interface_number)
        numpy.set_printoptions(formatter={'int':lambda x: hex(int(x))},
                               linewidth=1000)
        logging.debug('Data Channel: Sent %d bytes out of %d bytes requested. '
                      'Payload: %s',
                       written, ntb_length, numpy.array(ntb))
        if written < ntb_length:
            mbim_errors.log_and_raise(
                    mbim_errors.MBIMComplianceDataTransferError,
                    'Could not send the complete NTB (%d/%d bytes sent)' %
                    written, ntb_length)


    def receive_ntb(self):
        """
        Receive a payload from the device using the bulk-in USB pipe.

        This API will return any data it receives from the device within
        |_READ_TIMEOUT_S| seconds. If nothing is received within this duration,
        it returns an empty byte array. The API returns only one MBIM NTB
        received per invocation.

        @returns Byte array of complete MBIM NTB received from the device. This
                could be empty if nothing is received from the device.

        """
        ntb = self._device.read(endpoint=self._bulk_in_endpoint_address,
                                size=self._max_in_buffer_size,
                                timeout=self._READ_TIMEOUT_MS,
                                interface=self._data_interface_number)
        ntb_length = len(ntb)
        numpy.set_printoptions(formatter={'int':lambda x: hex(int(x))},
                               linewidth=1000)
        logging.debug('Data Channel: Received %d bytes response. Payload: %s',
                       ntb_length, numpy.array(ntb))
        return ntb
