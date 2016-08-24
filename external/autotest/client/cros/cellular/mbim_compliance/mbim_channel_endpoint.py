# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import Queue
import signal
import struct
import time
import numpy

from collections import namedtuple
from usb import core

import common
from autotest_lib.client.cros.cellular.mbim_compliance import mbim_errors


USBNotificationPacket = namedtuple(
        'USBNotificationPacket',
        ['bmRequestType', 'bNotificationCode', 'wValue', 'wIndex',
         'wLength'])


class MBIMChannelEndpoint(object):
    """
    An object dedicated to interacting with the MBIM capable USB device.

    This object interacts with the USB devies in a forever loop, servicing
    command requests from |MBIMChannel| as well as surfacing any notifications
    from the modem.

    """
    USB_PACKET_HEADER_FORMAT = '<BBHHH'
    # Sleeping for 0 seconds *may* hint for the schedular to relinquish CPU.
    QUIET_TIME_MS = 0
    INTERRUPT_READ_TIMEOUT_MS = 1  # We don't really want to wait.
    GET_ENCAPSULATED_RESPONSE_TIMEOUT_MS = 50
    SEND_ENCAPSULATED_REQUEST_TIMEOUT_MS = 50
    GET_ENCAPSULATED_RESPONSE_ARGS = {
            'bmRequestType' : 0b10100001,
            'bRequest' : 0b00000001,
            'wValue' : 0x0000}
    SEND_ENCAPSULATED_COMMAND_ARGS = {
            'bmRequestType' : 0b00100001,
            'bRequest' : 0b00000000,
            'wValue' : 0x0000}

    def __init__(self,
                 device,
                 interface_number,
                 interrupt_endpoint_address,
                 in_buffer_size,
                 request_queue,
                 response_queue,
                 stop_request_event,
                 strict=True):
        """
        @param device: Device handle returned by PyUSB for the modem to test.
        @param interface_number: |bInterfaceNumber| of the MBIM interface.
        @param interrupt_endpoint_address: |bEndpointAddress| for the usb
                INTERRUPT IN endpoint for notifications.
        @param in_buffer_size: The (fixed) buffer size to use for in control
                transfers.
        @param request_queue: A process safe queue where we expect commands
                to send be be enqueued.
        @param response_queue: A process safe queue where we enqueue
                non-notification responses from the device.
        @param strict: In strict mode (default), any unexpected error causes an
                abort. Otherwise, we merely warn.

        """
        self._device = device
        self._interface_number = interface_number
        self._interrupt_endpoint_address = interrupt_endpoint_address
        self._in_buffer_size = in_buffer_size
        self._request_queue = request_queue
        self._response_queue = response_queue
        self._stop_requested = stop_request_event
        self._strict = strict

        self._num_outstanding_responses = 0
        self._response_available_packet = USBNotificationPacket(
                bmRequestType=0b10100001,
                bNotificationCode=0b00000001,
                wValue=0x0000,
                wIndex=self._interface_number,
                wLength=0x0000)

        # SIGINT recieved by the parent process is forwarded to this process.
        # Exit graciously when that happens.
        signal.signal(signal.SIGINT,
                      lambda signum, frame: self._stop_requested.set())
        self.start()


    def start(self):
        """ Start the busy-loop that periodically interacts with the modem. """
        while not self._stop_requested.is_set():
            try:
                self._tick()
            except mbim_errors.MBIMComplianceChannelError as e:
                if self._strict:
                    raise

            time.sleep(self.QUIET_TIME_MS / 1000)


    def _tick(self):
        """ Work done in one time slice. """
        self._check_response()
        response = self._get_response()
        self._check_response()
        if response is not None:
            try:
                self._response_queue.put_nowait(response)
            except Queue.Full:
                mbim_errors.log_and_raise(
                        mbim_errors.MBIMComplianceChannelError,
                        'Response queue full.')

        self._check_response()
        try:
            request = self._request_queue.get_nowait()
            if request:
                self._send_request(request)
        except Queue.Empty:
            pass

        self._check_response()


    def _check_response(self):
        """
        Check if there is a response available.

        If a response is available, increment |outstanding_responses|.

        This method is kept separate from |_get_response| because interrupts are
        time critical. A separate method underscores this point. It also opens
        up the possibility of giving this method higher priority wherever
        possible.

        """
        try:
            in_data = self._device.read(
                    self._interrupt_endpoint_address,
                    struct.calcsize(self.USB_PACKET_HEADER_FORMAT),
                    self._interface_number,
                    self.INTERRUPT_READ_TIMEOUT_MS)
        except core.USBError:
            # If there is no response available, the modem will response with
            # STALL messages, and pyusb will raise an exception.
            return

        if len(in_data) != struct.calcsize(self.USB_PACKET_HEADER_FORMAT):
            mbim_errors.log_and_raise(
                    mbim_errors.MBIMComplianceChannelError,
                    'Received unexpected notification (%s) of length %d.' %
                    (in_data, len(in_data)))

        in_packet = USBNotificationPacket(
                *struct.unpack(self.USB_PACKET_HEADER_FORMAT, in_data))
        if in_packet != self._response_available_packet:
            mbim_errors.log_and_raise(
                    mbim_errors.MBIMComplianceChannelError,
                    'Received unexpected notification (%s).' % in_data)

        self._num_outstanding_responses += 1


    def _get_response(self):
        """
        Get the outstanding response from the device.

        @returns: The MBIM payload, if any. None otherwise.

        """
        if self._num_outstanding_responses == 0:
            return None

        # We count all failed cases also as an attempt.
        self._num_outstanding_responses -= 1
        response = self._device.ctrl_transfer(
                wIndex=self._interface_number,
                data_or_wLength=self._in_buffer_size,
                timeout=self.GET_ENCAPSULATED_RESPONSE_TIMEOUT_MS,
                **self.GET_ENCAPSULATED_RESPONSE_ARGS)
        numpy.set_printoptions(formatter={'int':lambda x: hex(int(x))},
                               linewidth=1000)
        logging.debug('Control Channel: Received %d bytes response. Payload:%s',
                      len(response), numpy.array(response))
        return response


    def _send_request(self, payload):
        """
        Send payload (one fragment) down to the device.

        @raises MBIMComplianceGenericError if the complete |payload| could not
                be sent.

        """
        actual_written = self._device.ctrl_transfer(
                wIndex=self._interface_number,
                data_or_wLength=payload,
                timeout=self.SEND_ENCAPSULATED_REQUEST_TIMEOUT_MS,
                **self.SEND_ENCAPSULATED_COMMAND_ARGS)
        numpy.set_printoptions(formatter={'int':lambda x: hex(int(x))},
                               linewidth=1000)
        logging.debug('Control Channel: Sent %d bytes out of %d bytes '
                      'requested. Payload:%s',
                      actual_written, len(payload), numpy.array(payload))
        if actual_written < len(payload):
            mbim_errors.log_and_raise(
                    mbim_errors.MBIMComplianceGenericError,
                    'Could not send the complete packet (%d/%d bytes sent)' %
                    actual_written, len(payload))
