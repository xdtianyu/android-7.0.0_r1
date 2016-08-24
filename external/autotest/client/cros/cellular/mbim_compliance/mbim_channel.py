# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import multiprocessing
import Queue
import struct
import time

import common
from autotest_lib.client.bin import utils
from autotest_lib.client.cros.cellular.mbim_compliance \
        import mbim_channel_endpoint
from autotest_lib.client.cros.cellular.mbim_compliance import mbim_errors


class MBIMChannel(object):
    """
    Provide synchronous access to the modem with MBIM command level interaction.

    This object should simplify your interaction over the MBIM channel as
    follows:
    - Use |bidirectional_transaction| to send MBIM packets that are part of a
      transaction. This function will block until the transaction completes and
      return the MBIM packets received in response.
    - |bidirectional_transaction| will filter out packets that do not correspond
      to your transaction. This way, you don't have to worry about unsolicited
      notifications and/or stale packets when interacting with the modem.
    - All filtered out packets can be grabbed using the
      |get_outstanding_packets| function. Use this function to receive error
      notifications, status notifications, etc.
    - Use |unidirectional_transaction| to send MBIM packets for which you don't
      expect a response.
    - Use |flush| to clean out all pipes before starting a new transaction.

    Note that "MBIM packets" here really means MBIM fragments. This object does
    not (de)fragment packets for you. Out of necessity, it does check that
    received fragments are contiguous and in-order.

    So, this object houses the minimum information necessary about the MBIM
    fragments to provide you a comfortable synchronous packet level channel.

    """

    ENDPOINT_JOIN_TIMEOUT_S = 5
    FRAGMENT_TIMEOUT_S = 3
    # TODO(pprabhu) Consider allowing each transaction to specify its own
    # timeout.
    TRANSACTION_TIMEOUT_S = 5

    MESSAGE_HEADER_FORMAT = '<LLL'
    FRAGMENT_HEADER_FORMAT = '<LL'
    MBIM_FRAGMENTED_MESSAGES = [
            0x00000003,  # MBIM_COMMAND_MSG
            0x80000003,  # MBIM_COMMAND_DONE
            0x80000007]  # MBIM_INDICATE_STATUS

    def __init__(self,
                 device,
                 interface_number,
                 interrupt_endpoint_address,
                 in_buffer_size,
                 process_class=None):
        """
        @param device: Device handle returned by PyUSB for the modem to test.
        @param interface_number: |bInterfaceNumber| of the MBIM interface.
        @param interrupt_endpoint_address: |bEndpointAddress| for the usb
                INTERRUPT IN endpoint for notifications.
        @param in_buffer_size: The (fixed) buffer size to use for in control
                transfers.
        @param process_class: The class to instantiate to create a subprocess.
                This is used by tests only, to easily mock out the process
                ceation.

        """
        self._stop_request_event = multiprocessing.Event()
        self._request_queue = multiprocessing.Queue()
        self._response_queue = multiprocessing.Queue()
        self._outstanding_packets = []
        self._last_response = []
        self._stashed_first_fragment = None
        if process_class is None:
            process_class = multiprocessing.Process
        self._endpoint_process = process_class(
                target=mbim_channel_endpoint.MBIMChannelEndpoint,
                args=(device,
                      interface_number,
                      interrupt_endpoint_address,
                      in_buffer_size,
                      self._request_queue,
                      self._response_queue,
                      self._stop_request_event))
        self._endpoint_process.start()


    def __del__(self):
        """
        The destructor.

        Note that it is not guaranteed that |__del__| is called for objects that
        exist when the interpreter exits. It is recommended to call |close|
        explicitly.

        """
        self.close()


    def close(self):
        """
        Cleanly close the MBIMChannel.

        MBIMChannel forks a subprocess to communicate with the USB device. It is
        recommended that |close| be called explicitly.

        """
        if not self._endpoint_process:
            return

        if self._endpoint_process.is_alive():
            self._stop_request_event.set()
            self._endpoint_process.join(self.ENDPOINT_JOIN_TIMEOUT_S)
            if self._endpoint_process.is_alive():
                self._endpoint_process.terminate()

        self._endpoint_process = None


    def bidirectional_transaction(self, *args):
        """
        Execute a synchronous bidirectional transaction.

        @param *args: Fragments of a single MBIM transaction. An MBIM
                transaction may consist of multiple fragments - each fragment is
                the payload for a USB control message. It should be an
                |array.array| object.  It is your responsibility (and choice) to
                keep the fragments in-order, and to send all the fragments.
                For more details, see "Fragmentation of messages" in the MBIM
                spec.
        @returns: A list of fragments in the same order as received that
                correspond to the given transaction. If we receive less
                fragments than claimed, we will return what we get. If we
                receive non-contiguous / out-of-order fragments, we'll complain.
        @raises: MBIMComplianceChannelError if received fragments are
                out-of-order or non-contigouos.

        """
        self._verify_endpoint_open()
        if not args:
            mbim_errors.log_and_raise(
                    mbim_errors.MBIMComplianceChannelError,
                    'No data given to |bidirectional_transaction|.')

        transaction_id, _, _ = self._fragment_metadata(args[0])
        for fragment in args:
            self._request_queue.put_nowait(fragment)
        return self._get_response_fragments(transaction_id)


    def unidirectional_transaction(self, *args):
        """
        Execute a synchronous unidirectional transaction. No return value.

        @param *args: Fragments of a single MBIM transaction. An MBIM
                transaction may consist of multiple fragments - each fragment is
                the payload for a USB control message. It should be an
                |array.array| object.  It is your responsibility (and choice) to
                keep the fragments in-order, and to send all the fragments.
                For more details, see "Fragmentation of messages" in the MBIM
                spec.

        """
        self._verify_endpoint_open()
        if not args:
            mbim_errors.log_and_raise(
                    mbim_errors.MBIMComplianceChannelError,
                    'No data given to |unidirectional_transaction|.')

        for fragment in args:
            self._request_queue.put_nowait(fragment)


    def flush(self):
        """
        Clean out all queues.

        This waits till all outgoing packets have been sent, and then waits some
        more to give the channel time to settle down.

        @raises: MBIMComplianceChannelError if things don't settle down fast
                enough.
        """
        self._verify_endpoint_open()
        num_remaining_fragments = self._request_queue.qsize()
        try:
            timeout = self.FRAGMENT_TIMEOUT_S * num_remaining_fragments
            utils.poll_for_condition(lambda: self._request_queue.empty(),
                                     timeout=timeout)
        except utils.TimeoutError:
            mbim_errors.log_and_raise(
                    mbim_errors.MBIMComplianceChannelError,
                    'Could not flush request queue.')

        # Now wait for the response queue to settle down.
        # In the worst case, each request fragment that was remaining at the
        # time flush was called belonged to a different transaction, and each of
        # these transactions would serially timeout in |TRANSACTION_TIMEOUT_S|.
        # To avoid sleeping for long times, we cap this value arbitrarily to 5
        # transactions.
        num_remaining_transactions = min(5, num_remaining_fragments)
        time.sleep(num_remaining_fragments * self.TRANSACTION_TIMEOUT_S)
        extra_packets = self.get_outstanding_packets()
        for packet in extra_packets:
            logging.debug('flush: discarding packet: %s', packet)


    def get_outstanding_packets(self):
        """
        Get all received packets that were not part of an explicit transaction.

        @returns: A list of packets. Each packet is a list of fragments, so you
        perhaps want to do something like:
            for packet in channel.get_outstanding_packets():
                for fragment in packet:
                    # handle fragment.

        """
        self._verify_endpoint_open()
        # Try to get more packets from the response queue.
        # This can block forever if the modem keeps spewing trash at us.
        while True:
            packet = self._get_packet_fragments()
            if not packet:
                break
            self._outstanding_packets.append(packet)

        packets = self._outstanding_packets
        self._outstanding_packets = []
        return packets


    def _get_response_fragments(self, transaction_id):
        """
        Get response for the given |transaction_id|.

        @returns: A list of fragments.
        @raises: MBIMComplianceChannelError if response is not recieved.

        """
        def _poll_response():
            packet = self._get_packet_fragments()
            if not packet:
                return False
            first_fragment = packet[0]
            response_id, _, _ = self._fragment_metadata(first_fragment)
            if response_id == transaction_id:
                self._last_response = packet
                return True
            self._outstanding_packets.append(packet)
            return False

        try:
            utils.poll_for_condition(
                    _poll_response,
                    timeout=self.TRANSACTION_TIMEOUT_S)
        except utils.TimeoutError:
            mbim_errors.log_and_raise(
                    mbim_errors.MBIMComplianceChannelError,
                    'Did not receive timely reply to transaction %d' %
                    transaction_id)
        return self._last_response


    def _get_packet_fragments(self):
        """
        Get all fragements of the next packet from the modem.

        This function is responsible for putting together fragments of one
        packet, and checking that fragments are continguous and in-order.

        """
        fragments = []
        if self._stashed_first_fragment is not None:
            first_fragment = self._stashed_first_fragment
            self._stashed_first_fragment = None
        else:
            try:
                first_fragment = self._response_queue.get(
                        True, self.FRAGMENT_TIMEOUT_S)
            except Queue.Empty:
                # *Don't fail* Just return nothing.
                return fragments

        transaction_id, total_fragments, current_fragment = (
                self._fragment_metadata(first_fragment))
        if current_fragment != 0:
            mbim_errors.log_and_raise(
                    mbim_errors.MBIMComplianceChannelError,
                    'First fragment reports fragment number %d' %
                    current_fragment)

        fragments.append(first_fragment)

        last_fragment = 0
        while last_fragment < total_fragments - 1:
            try:
                fragment = self._response_queue.get(True,
                                                    self.FRAGMENT_TIMEOUT_S)
            except Queue.Empty:
                # *Don't fail* Just return the fragments we got so far.
                break

            fragment_id, fragment_total, fragment_current = (
                    self._fragment_metadata(fragment))
            if fragment_id != transaction_id:
                # *Don't fail* Treat a different transaction id as indicating
                # that the next packet has already arrived.
                logging.warning('Recieved only %d out of %d fragments for '
                                'transaction %d.',
                                last_fragment,
                                total_fragments,
                                transaction_id)
                self._stashed_first_fragment = fragment
                break

            if fragment_total != total_fragments:
                mbim_errors.log_and_raise(
                        mbim_errors.MBIMComplianceChannelError,
                        'Fragment number %d reports incorrect total (%d/%d)' %
                        (last_fragment + 1, fragment_total, total_fragments))

            if fragment_current != last_fragment + 1:
                mbim_errors.log_and_raise(
                        mbim_errors.MBIMComplianceChannelError,
                        'Received reordered fragments. Expected %d, got %d' %
                        (last_fragment + 1, fragment_current))

            last_fragment += 1
            fragments.append(fragment)

        return fragments


    def _fragment_metadata(self, fragment):
        """ This function houses all the MBIM packet knowledge. """
        # All packets have a message header.
        if len(fragment) < struct.calcsize(self.MESSAGE_HEADER_FORMAT):
            mbim_errors.log_and_raise(
                    mbim_errors.MBIMComplianceChannelError,
                    'Corrupted fragment |%s| does not have an MBIM header.' %
                    fragment)

        message_type, _, transaction_id = struct.unpack_from(
                self.MESSAGE_HEADER_FORMAT,
                fragment)

        if message_type in self.MBIM_FRAGMENTED_MESSAGES:
            fragment = fragment[struct.calcsize(self.MESSAGE_HEADER_FORMAT):]
            if len(fragment) < struct.calcsize(self.FRAGMENT_HEADER_FORMAT):
                mbim_errors.log_and_raise(
                        mbim_errors.MBIMComplianceChannelError,
                        'Corrupted fragment |%s| does not have a fragment '
                        'header. ' %
                        fragment)

            total_fragments, current_fragment = struct.unpack_from(
                    self.FRAGMENT_HEADER_FORMAT,
                    fragment)
        else:
            # For other types, there is only one 'fragment'.
            total_fragments = 1
            current_fragment = 0

        return transaction_id, total_fragments, current_fragment


    def _verify_endpoint_open(self):
        if not self._endpoint_process.is_alive():
            mbim_errors.log_and_raise(
                    mbim_errors.MBIMComplianceChannelError,
                    'MBIMChannelEndpoint died unexpectedly. '
                    'The actual exception can be found in log entries from the '
                    'subprocess.')
