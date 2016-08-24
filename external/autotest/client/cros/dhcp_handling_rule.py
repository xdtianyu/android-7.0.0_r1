# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""
DHCP handling rules are ways to record expectations for a DhcpTestServer.

When a handling rule reaches the front of the DhcpTestServer handling rule
queue, the server begins to ask the rule what it should do with each incoming
DHCP packet (in the form of a DhcpPacket).  The handle() method is expected to
return a tuple (response, action) where response indicates whether the packet
should be ignored or responded to and whether the test failed, succeeded, or is
continuing.  The action part of the tuple refers to whether or not the rule
should be be removed from the test server's handling rule queue.
"""

import logging
import time

from autotest_lib.client.cros import dhcp_packet

# Drops the packet and acts like it never happened.
RESPONSE_NO_ACTION = 0
# Signals that the handler wishes to send a packet.
RESPONSE_HAVE_RESPONSE = 1 << 0
# Signals that the handler wishes to be removed from the handling queue.
# The handler will be asked to generate a packet first if the handler signalled
# that it wished to do so with RESPONSE_HAVE_RESPONSE.
RESPONSE_POP_HANDLER = 1 << 1
# Signals that the handler wants to end the test on a failure.
RESPONSE_TEST_FAILED = 1 << 2
# Signals that the handler wants to end the test because it succeeded.
# Note that the failure bit has precedence over the success bit.
RESPONSE_TEST_SUCCEEDED = 1 << 3

class DhcpHandlingRule(object):
    """
    DhcpHandlingRule defines an interface between the DhcpTestServer and
    subclasses of DhcpHandlingRule.  A handling rule at the front of the
    DhcpTestServer rule queue is first asked what should be done with a packet
    via handle().  handle() returns a bitfield as described above.  If the
    response from handle() indicates that a packet should be sent in response,
    the server asks the handling rule to construct a response packet via
    respond().
    """

    def __init__(self, message_type, additional_options, custom_fields):
        """
        |message_type| should be a MessageType, from DhcpPacket.
        |additional_options| should be a dictionary that maps from
        dhcp_packet.OPTION_* to values.  For instance:

        {dhcp_packet.OPTION_SERVER_ID : "10.10.10.1"}

        These options are injected into response packets if the client requests
        it.  See inject_options().
        """
        super(DhcpHandlingRule, self).__init__()
        self._is_final_handler = False
        self._logger = logging.getLogger("dhcp.handling_rule")
        self._options = additional_options
        self._fields = custom_fields
        self._target_time_seconds = None
        self._allowable_time_delta_seconds = 0.5
        self._force_reply_options = []
        self._message_type = message_type
        self._last_warning = None

    def __str__(self):
        if self._last_warning:
            return '%s (%s)' % (self.__class__.__name__, self._last_warning)
        else:
            return self.__class__.__name__

    @property
    def logger(self):
        return self._logger

    @property
    def is_final_handler(self):
        return self._is_final_handler

    @is_final_handler.setter
    def is_final_handler(self, value):
        self._is_final_handler = value

    @property
    def options(self):
        """
        Returns a dictionary that maps from DhcpPacket options to their values.
        """
        return self._options

    @property
    def fields(self):
        """
        Returns a dictionary that maps from DhcpPacket fields to their values.
        """
        return self._fields

    @property
    def target_time_seconds(self):
        """
        If this is not None, packets will be rejected if they don't fall within
        |self.allowable_time_delta_seconds| seconds of
        |self.target_time_seconds|.  A value of None will cause this handler to
        ignore the target packet time.

        Defaults to None.
        """
        return self._target_time_seconds

    @target_time_seconds.setter
    def target_time_seconds(self, value):
        self._target_time_seconds = value

    @property
    def allowable_time_delta_seconds(self):
        """
        A configurable fudge factor for |self.target_time_seconds|.  If a packet
        comes in at time T and:

        delta = abs(T - |self.target_time_seconds|)

        Then if delta < |self.allowable_time_delta_seconds|, we accept the
        packet.  Otherwise we either fail the test or ignore the packet,
        depending on whether this packet is before or after the window.

        Defaults to 0.5 seconds.
        """
        return self._allowable_time_delta_seconds

    @allowable_time_delta_seconds.setter
    def allowable_time_delta_seconds(self, value):
        self._allowable_time_delta_seconds = value

    @property
    def packet_is_too_late(self):
        if self.target_time_seconds is None:
            return False
        delta = time.time() - self.target_time_seconds
        logging.debug("Handler received packet %0.2f seconds from target time.",
                      delta)
        if delta > self._allowable_time_delta_seconds:
            logging.info("Packet was too late for handling (+%0.2f seconds)",
                         delta - self._allowable_time_delta_seconds)
            return True
        logging.info("Packet was not too late for handling.")
        return False

    @property
    def packet_is_too_soon(self):
        if self.target_time_seconds is None:
            return False
        delta = time.time() - self.target_time_seconds
        logging.debug("Handler received packet %0.2f seconds from target time.",
                      delta)
        if -delta > self._allowable_time_delta_seconds:
            logging.info("Packet arrived too soon for handling: "
                         "(-%0.2f seconds)",
                         -delta - self._allowable_time_delta_seconds)
            return True
        logging.info("Packet was not too soon for handling.")
        return False

    @property
    def force_reply_options(self):
        return self._force_reply_options

    @force_reply_options.setter
    def force_reply_options(self, value):
        self._force_reply_options = value

    @property
    def response_packet_count(self):
        return 1

    def emit_warning(self, warning):
        """
        Log a warning, and retain that warning as |_last_warning|.

        @param warning: The warning message
        """
        self.logger.warning(warning)
        self._last_warning = warning

    def handle(self, query_packet):
        """
        The DhcpTestServer will call this method to ask a handling rule whether
        it wants to take some action in response to a packet.  The handler
        should return some combination of RESPONSE_* bits as described above.

        |packet| is a valid DHCP packet, but the values of fields and presence
        of options is not guaranteed.
        """
        if self.packet_is_too_late:
            return RESPONSE_TEST_FAILED
        if self.packet_is_too_soon:
            return RESPONSE_NO_ACTION
        return self.handle_impl(query_packet)

    def handle_impl(self, query_packet):
        logging.error("DhcpHandlingRule.handle_impl() called.")
        return RESPONSE_TEST_FAILED

    def respond(self, query_packet):
        """
        Called by the DhcpTestServer to generate a packet to send back to the
        client.  This method is called if and only if the response returned from
        handle() had RESPONSE_HAVE_RESPONSE set.
        """
        return None

    def inject_options(self, packet, requested_parameters):
        """
        Adds options listed in the intersection of |requested_parameters| and
        |self.options| to |packet|.  Also include the options in the
        intersection of |self.force_reply_options| and |self.options|.

        |packet| is a DhcpPacket.

        |requested_parameters| is a list of options numbers as you would find in
        a DHCP_DISCOVER or DHCP_REQUEST packet after being parsed by DhcpPacket
        (e.g. [1, 121, 33, 3, 6, 12]).

        Subclassed handling rules may call this to inject options into response
        packets to the client.  This process emulates a real DHCP server which
        would have a pool of configuration settings to hand out to DHCP clients
        upon request.
        """
        for option, value in self.options.items():
            if (option.number in requested_parameters or
                option in self.force_reply_options):
                packet.set_option(option, value)

    def inject_fields(self, packet):
        """
        Adds fields listed in |self.fields| to |packet|.

        |packet| is a DhcpPacket.

        Subclassed handling rules may call this to inject fields into response
        packets to the client.  This process emulates a real DHCP server which
        would have a pool of configuration settings to hand out to DHCP clients
        upon request.
        """
        for field, value in self.fields.items():
            packet.set_field(field, value)

    def is_our_message_type(self, packet):
        """
        Checks if the Message Type DHCP Option in |packet| matches the message
        type handled by this rule. Logs a warning if the types do not match.

        @param packet: a DhcpPacket

        @returns True or False
        """
        if packet.message_type == self._message_type:
            return True
        else:
            self.emit_warning("Packet's message type was %s, not %s." % (
                              packet.message_type.name,
                              self._message_type.name))
            return False


class DhcpHandlingRule_RespondToDiscovery(DhcpHandlingRule):
    """
    This handler will accept any DISCOVER packet received by the server. In
    response to such a packet, the handler will construct an OFFER packet
    offering |intended_ip| from a server at |server_ip| (from the constructor).
    """
    def __init__(self,
                 intended_ip,
                 server_ip,
                 additional_options,
                 custom_fields,
                 should_respond=True):
        """
        |intended_ip| is an IPv4 address string like "192.168.1.100".

        |server_ip| is an IPv4 address string like "192.168.1.1".

        |additional_options| is handled as explained by DhcpHandlingRule.
        """
        super(DhcpHandlingRule_RespondToDiscovery, self).__init__(
                dhcp_packet.MESSAGE_TYPE_DISCOVERY, additional_options,
                custom_fields)
        self._intended_ip = intended_ip
        self._server_ip = server_ip
        self._should_respond = should_respond

    def handle_impl(self, query_packet):
        if not self.is_our_message_type(query_packet):
            return RESPONSE_NO_ACTION

        self.logger.info("Received valid DISCOVERY packet.  Processing.")
        ret = RESPONSE_POP_HANDLER
        if self.is_final_handler:
            ret |= RESPONSE_TEST_SUCCEEDED
        if self._should_respond:
            ret |= RESPONSE_HAVE_RESPONSE
        return ret

    def respond(self, query_packet):
        if not self.is_our_message_type(query_packet):
            return None

        self.logger.info("Responding to DISCOVERY packet.")
        response_packet = dhcp_packet.DhcpPacket.create_offer_packet(
                query_packet.transaction_id,
                query_packet.client_hw_address,
                self._intended_ip,
                self._server_ip)
        requested_parameters = query_packet.get_option(
                dhcp_packet.OPTION_PARAMETER_REQUEST_LIST)
        if requested_parameters is not None:
            self.inject_options(response_packet, requested_parameters)
        self.inject_fields(response_packet)
        return response_packet


class DhcpHandlingRule_RejectRequest(DhcpHandlingRule):
    """
    This handler receives a REQUEST packet, and responds with a NAK.
    """
    def __init__(self):
        super(DhcpHandlingRule_RejectRequest, self).__init__(
                dhcp_packet.MESSAGE_TYPE_REQUEST, {}, {})
        self._should_respond = True

    def handle_impl(self, query_packet):
        if not self.is_our_message_type(query_packet):
            return RESPONSE_NO_ACTION

        ret = RESPONSE_POP_HANDLER
        if self.is_final_handler:
            ret |= RESPONSE_TEST_SUCCEEDED
        if self._should_respond:
            ret |= RESPONSE_HAVE_RESPONSE
        return ret

    def respond(self, query_packet):
        if not self.is_our_message_type(query_packet):
            return None

        self.logger.info("NAKing the REQUEST packet.")
        response_packet = dhcp_packet.DhcpPacket.create_nak_packet(
            query_packet.transaction_id, query_packet.client_hw_address)
        return response_packet


class DhcpHandlingRule_RespondToRequest(DhcpHandlingRule):
    """
    This handler accepts any REQUEST packet that contains options for SERVER_ID
    and REQUESTED_IP that match |expected_server_ip| and |expected_requested_ip|
    respectively.  It responds with an ACKNOWLEDGEMENT packet from a DHCP server
    at |response_server_ip| granting |response_granted_ip| to a client at the
    address given in the REQUEST packet.  If |response_server_ip| or
    |response_granted_ip| are not given, then they default to
    |expected_server_ip| and |expected_requested_ip| respectively.
    """
    def __init__(self,
                 expected_requested_ip,
                 expected_server_ip,
                 additional_options,
                 custom_fields,
                 should_respond=True,
                 response_server_ip=None,
                 response_granted_ip=None,
                 expect_server_ip_set=True):
        """
        All *_ip arguments are IPv4 address strings like "192.168.1.101".

        |additional_options| is handled as explained by DhcpHandlingRule.
        """
        super(DhcpHandlingRule_RespondToRequest, self).__init__(
                dhcp_packet.MESSAGE_TYPE_REQUEST, additional_options,
                custom_fields)
        self._expected_requested_ip = expected_requested_ip
        self._expected_server_ip = expected_server_ip
        self._should_respond = should_respond
        self._granted_ip = response_granted_ip
        self._server_ip = response_server_ip
        self._expect_server_ip_set = expect_server_ip_set
        if self._granted_ip is None:
            self._granted_ip = self._expected_requested_ip
        if self._server_ip is None:
            self._server_ip = self._expected_server_ip

    def handle_impl(self, query_packet):
        if not self.is_our_message_type(query_packet):
            return RESPONSE_NO_ACTION

        self.logger.info("Received REQUEST packet, checking fields...")
        server_ip = query_packet.get_option(dhcp_packet.OPTION_SERVER_ID)
        requested_ip = query_packet.get_option(dhcp_packet.OPTION_REQUESTED_IP)
        server_ip_provided = server_ip is not None
        if ((server_ip_provided != self._expect_server_ip_set) or
            (requested_ip is None)):
            self.logger.info("REQUEST packet did not have the expected "
                             "options, discarding.")
            return RESPONSE_NO_ACTION

        if server_ip_provided and server_ip != self._expected_server_ip:
            self.emit_warning("REQUEST packet's server ip did not match our "
                              "expectations; expected %s but got %s" %
                              (self._expected_server_ip, server_ip))
            return RESPONSE_NO_ACTION

        if requested_ip != self._expected_requested_ip:
            self.emit_warning("REQUEST packet's requested IP did not match "
                              "our expectations; expected %s but got %s" %
                              (self._expected_requested_ip, requested_ip))
            return RESPONSE_NO_ACTION

        self.logger.info("Received valid REQUEST packet, processing")
        ret = RESPONSE_POP_HANDLER
        if self.is_final_handler:
            ret |= RESPONSE_TEST_SUCCEEDED
        if self._should_respond:
            ret |= RESPONSE_HAVE_RESPONSE
        return ret

    def respond(self, query_packet):
        if not self.is_our_message_type(query_packet):
            return None

        self.logger.info("Responding to REQUEST packet.")
        response_packet = dhcp_packet.DhcpPacket.create_acknowledgement_packet(
                query_packet.transaction_id,
                query_packet.client_hw_address,
                self._granted_ip,
                self._server_ip)
        requested_parameters = query_packet.get_option(
                dhcp_packet.OPTION_PARAMETER_REQUEST_LIST)
        if requested_parameters is not None:
            self.inject_options(response_packet, requested_parameters)
        self.inject_fields(response_packet)
        return response_packet


class DhcpHandlingRule_RespondToPostT2Request(
        DhcpHandlingRule_RespondToRequest):
    """
    This handler is a lot like DhcpHandlingRule_RespondToRequest except that it
    expects request packets like those sent after the T2 deadline (see RFC
    2131).  This is the only time that you can find a request packet without the
    SERVER_ID option.  It responds to packets in exactly the same way.
    """
    def __init__(self,
                 expected_requested_ip,
                 response_server_ip,
                 additional_options,
                 custom_fields,
                 should_respond=True,
                 response_granted_ip=None):
        """
        All *_ip arguments are IPv4 address strings like "192.168.1.101".

        |additional_options| is handled as explained by DhcpHandlingRule.
        """
        super(DhcpHandlingRule_RespondToPostT2Request, self).__init__(
                expected_requested_ip,
                None,
                additional_options,
                custom_fields,
                should_respond=should_respond,
                response_server_ip=response_server_ip,
                response_granted_ip=response_granted_ip)

    def handle_impl(self, query_packet):
        if not self.is_our_message_type(query_packet):
            return RESPONSE_NO_ACTION

        self.logger.info("Received REQUEST packet, checking fields...")
        if query_packet.get_option(dhcp_packet.OPTION_SERVER_ID) is not None:
            self.logger.info("REQUEST packet had a SERVER_ID option, which it "
                             "is not expected to have, discarding.")
            return RESPONSE_NO_ACTION

        requested_ip = query_packet.get_option(dhcp_packet.OPTION_REQUESTED_IP)
        if requested_ip is None:
            self.logger.info("REQUEST packet did not have the expected "
                             "request ip option at all, discarding.")
            return RESPONSE_NO_ACTION

        if requested_ip != self._expected_requested_ip:
            self.emit_warning("REQUEST packet's requested IP did not match "
                              "our expectations; expected %s but got %s" %
                              (self._expected_requested_ip, requested_ip))
            return RESPONSE_NO_ACTION

        self.logger.info("Received valid post T2 REQUEST packet, processing")
        ret = RESPONSE_POP_HANDLER
        if self.is_final_handler:
            ret |= RESPONSE_TEST_SUCCEEDED
        if self._should_respond:
            ret |= RESPONSE_HAVE_RESPONSE
        return ret


class DhcpHandlingRule_AcceptRelease(DhcpHandlingRule):
    """
    This handler accepts any RELEASE packet that contains an option for
    SERVER_ID matches |expected_server_ip|.  There is no response to this
    packet.
    """
    def __init__(self,
                 expected_server_ip,
                 additional_options,
                 custom_fields):
        """
        All *_ip arguments are IPv4 address strings like "192.168.1.101".

        |additional_options| is handled as explained by DhcpHandlingRule.
        """
        super(DhcpHandlingRule_AcceptRelease, self).__init__(
                dhcp_packet.MESSAGE_TYPE_RELEASE, additional_options,
                custom_fields)
        self._expected_server_ip = expected_server_ip

    def handle_impl(self, query_packet):
        if not self.is_our_message_type(query_packet):
            return RESPONSE_NO_ACTION

        self.logger.info("Received RELEASE packet, checking fields...")
        server_ip = query_packet.get_option(dhcp_packet.OPTION_SERVER_ID)
        if server_ip is None:
            self.logger.info("RELEASE packet did not have the expected "
                             "options, discarding.")
            return RESPONSE_NO_ACTION

        if server_ip != self._expected_server_ip:
            self.emit_warning("RELEASE packet's server ip did not match our "
                                "expectations; expected %s but got %s" %
                                (self._expected_server_ip, server_ip))
            return RESPONSE_NO_ACTION

        self.logger.info("Received valid RELEASE packet, processing")
        ret = RESPONSE_POP_HANDLER
        if self.is_final_handler:
            ret |= RESPONSE_TEST_SUCCEEDED
        return ret


class DhcpHandlingRule_RejectAndRespondToRequest(
        DhcpHandlingRule_RespondToRequest):
    """
    This handler accepts any REQUEST packet that contains options for SERVER_ID
    and REQUESTED_IP that match |expected_server_ip| and |expected_requested_ip|
    respectively.  It responds with both an ACKNOWLEDGEMENT packet from a DHCP
    server as well as a NAK, in order to simulate a network with two conflicting
    servers.
    """
    def __init__(self,
                 expected_requested_ip,
                 expected_server_ip,
                 additional_options,
                 custom_fields,
                 send_nak_before_ack):
        super(DhcpHandlingRule_RejectAndRespondToRequest, self).__init__(
                expected_requested_ip,
                expected_server_ip,
                additional_options,
                custom_fields)
        self._send_nak_before_ack = send_nak_before_ack
        self._response_counter = 0

    @property
    def response_packet_count(self):
        return 2

    def respond(self, query_packet):
        """ Respond to |query_packet| with a NAK then ACK or ACK then NAK. """
        if ((self._response_counter == 0 and self._send_nak_before_ack) or
            (self._response_counter != 0 and not self._send_nak_before_ack)):
            response_packet = dhcp_packet.DhcpPacket.create_nak_packet(
                query_packet.transaction_id, query_packet.client_hw_address)
        else:
            response_packet = super(DhcpHandlingRule_RejectAndRespondToRequest,
                                    self).respond(query_packet)
        self._response_counter += 1
        return response_packet


class DhcpHandlingRule_AcceptDecline(DhcpHandlingRule):
    """
    This handler accepts any DECLINE packet that contains an option for
    SERVER_ID matches |expected_server_ip|.  There is no response to this
    packet.
    """
    def __init__(self,
                 expected_server_ip,
                 additional_options,
                 custom_fields):
        """
        All *_ip arguments are IPv4 address strings like "192.168.1.101".

        |additional_options| is handled as explained by DhcpHandlingRule.
        """
        super(DhcpHandlingRule_AcceptDecline, self).__init__(
                dhcp_packet.MESSAGE_TYPE_DECLINE, additional_options,
                custom_fields)
        self._expected_server_ip = expected_server_ip

    def handle_impl(self, query_packet):
        if not self.is_our_message_type(query_packet):
            return RESPONSE_NO_ACTION

        self.logger.info("Received DECLINE packet, checking fields...")
        server_ip = query_packet.get_option(dhcp_packet.OPTION_SERVER_ID)
        if server_ip is None:
            self.logger.info("DECLINE packet did not have the expected "
                             "options, discarding.")
            return RESPONSE_NO_ACTION

        if server_ip != self._expected_server_ip:
            self.emit_warning("DECLINE packet's server ip did not match our "
                                "expectations; expected %s but got %s" %
                                (self._expected_server_ip, server_ip))
            return RESPONSE_NO_ACTION

        self.logger.info("Received valid DECLINE packet, processing")
        ret = RESPONSE_POP_HANDLER
        if self.is_final_handler:
            ret |= RESPONSE_TEST_SUCCEEDED
        return ret
