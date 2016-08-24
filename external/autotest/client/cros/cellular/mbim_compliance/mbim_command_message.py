# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""
This module implements the classes for information structures encapsulated in
either |MBIMCommandMessage| or |MBIMCommandDone|.

Reference:
    [1] Universal Serial Bus Communications Class Subclass Specification for
        Mobile Broadband Interface Model
        http://www.usb.org/developers/docs/devclass_docs/
        MBIM10Errata1_073013.zip
"""
from autotest_lib.client.cros.cellular.mbim_compliance import mbim_constants
from autotest_lib.client.cros.cellular.mbim_compliance \
        import mbim_message_request
from autotest_lib.client.cros.cellular.mbim_compliance \
        import mbim_message_response


class MBIMSetConnect(mbim_message_request.MBIMCommand):
    """ The class for MBIM_SET_CONNECT structure. """

    _FIELDS = (('I', 'session_id', ''),
               ('I', 'activation_command', ''),
               ('I', 'access_string_offset', ''),
               ('I', 'access_string_size', ''),
               ('I', 'user_name_offset', ''),
               ('I', 'user_name_size', ''),
               ('I', 'password_offset', ''),
               ('I', 'password_size', ''),
               ('I', 'compression', ''),
               ('I', 'auth_protocol', ''),
               ('I', 'ip_type', ''),
               ('16s', 'context_type', ''))
    _DEFAULTS = {'device_service_id' : mbim_constants.UUID_BASIC_CONNECT.bytes,
                 'cid' : mbim_constants.MBIM_CID_CONNECT,
                 'command_type' : mbim_constants.COMMAND_TYPE_SET}


class MBIMConnectQuery(mbim_message_request.MBIMCommand):
    """ The class for MBIM_CONNECT_QUERY structure. """

    _FIELDS = (('I', 'session_id', ''),
               ('I', 'activation_state', ''),
               ('I', 'voice_call_state', ''),
               ('I', 'ip_type', ''),
               ('16s', 'context_type', ''),
               ('I', 'nw_error', ''))
    _DEFAULTS = {'device_service_id' : mbim_constants.UUID_BASIC_CONNECT.bytes,
                 'cid' : mbim_constants.MBIM_CID_CONNECT,
                 'command_type' : mbim_constants.COMMAND_TYPE_QUERY,
                 'information_buffer_length' : 36,
                 'activation_state' : 0,
                 'voice_call_state' : 0,
                 'ip_type' : 0,
                 'context_type' : mbim_constants.MBIM_CONTEXT_TYPE_NONE.bytes,
                 'nw_error' : 0}


class MBIMConnectInfo(mbim_message_response.MBIMCommandDone):
    """ The class for MBIM_CONNECT_INFO structure. """

    _FIELDS = (('I', 'session_id', ''),
               ('I', 'activation_state', ''),
               ('I', 'voice_call_state', ''),
               ('I', 'ip_type', ''),
               ('16s', 'context_type', ''),
               ('I', 'nw_error', ''))
    _IDENTIFIERS = {
            'device_service_id' : mbim_constants.UUID_BASIC_CONNECT.bytes,
            'cid' : mbim_constants.MBIM_CID_CONNECT}


class MBIMDeviceCapsQuery(mbim_message_request.MBIMCommand):
    """ The class for MBIM_DEVICE_CAPS_QUERY structure. """

    _DEFAULTS = {'device_service_id' : mbim_constants.UUID_BASIC_CONNECT.bytes,
                 'cid' : mbim_constants.MBIM_CID_DEVICE_CAPS,
                 'command_type' : mbim_constants.COMMAND_TYPE_QUERY,
                 'information_buffer_length' : 0}


class MBIMDeviceCapsInfo(mbim_message_response.MBIMCommandDone):
    """ The class for MBIM_DEVICE_CAPS_INFO structure. """

    _FIELDS = (('I', 'device_type', ''),
               ('I', 'cellular_class', ''),
               ('I', 'voice_class', ''),
               ('I', 'sim_class', ''),
               ('I', 'data_class', ''),
               ('I', 'sms_caps', ''),
               ('I', 'control_caps', ''),
               ('I', 'max_sessions', ''),
               ('I', 'custom_data_class_offset', ''),
               ('I', 'custom_data_class_size', ''),
               ('I', 'device_id_offset', ''),
               ('I', 'device_id_size', ''),
               ('I', 'firmware_info_offset', ''),
               ('I', 'firmware_info_size', ''),
               ('I', 'hardware_info_offset', ''),
               ('I', 'hardware_info_size', ''))
    _IDENTIFIERS = {
            'device_service_id' : mbim_constants.UUID_BASIC_CONNECT.bytes,
            'cid' : mbim_constants.MBIM_CID_DEVICE_CAPS}


class MBIMDeviceServicesQuery(mbim_message_request.MBIMCommand):
    """ The class for MBIM_DEVICE_SERVICES_QUERY structure. """

    _DEFAULTS = {'device_service_id' : mbim_constants.UUID_BASIC_CONNECT.bytes,
                 'cid' : mbim_constants.MBIM_CID_DEVICE_SERVICES,
                 'command_type' : mbim_constants.COMMAND_TYPE_QUERY,
                 'information_buffer_length' : 0}


class MBIMDeviceServicesInfo(mbim_message_response.MBIMCommandDone):
    """ The class for MBIM_DEVICE_SERVICES_INFO structure. """

    # The length of |device_services_ref_list| depends on the value of
    # |device_services_count|.
    _FIELDS = (('I', 'device_services_count', ''),
               ('I', 'max_dss_sessions', ''))
               #('Q', 'device_services_ref_list', ''))
    _IDENTIFIERS = {
            'device_service_id' : mbim_constants.UUID_BASIC_CONNECT.bytes,
            'cid' : mbim_constants.MBIM_CID_DEVICE_SERVICES}


class MBIMRadioStateInfo(mbim_message_response.MBIMCommandDone):
    """ The class for MBIM_RADIO_STATE_INFO structure. """

    _FIELDS = (('I', 'hw_radio_state', ''),
               ('I', 'sw_radio_state', ''))


class MBIMIPConfigurationQuery(mbim_message_request.MBIMCommand):
    """ The class for MBIM_IP_CONFIGURATION_INFO structure. """

    _FIELDS = (('I', 'session_id', ''),
               ('I', 'ipv4_configuration_available', ''),
               ('I', 'ipv6_configuration_available', ''),
               ('I', 'ipv4_address_count', ''),
               ('I', 'ipv4_address_offset', ''),
               ('I', 'ipv6_address_count', ''),
               ('I', 'ipv6_address_offset', ''),
               ('I', 'ipv4_gateway_offset', ''),
               ('I', 'ipv6_gateway_offset', ''),
               ('I', 'ipv4_dns_server_count', ''),
               ('I', 'ipv4_dns_server_offset', ''),
               ('I', 'ipv6_dns_server_count', ''),
               ('I', 'ipv6_dns_server_offset', ''),
               ('I', 'ipv4_mtu', ''),
               ('I', 'ipv6_mtu', ''))
    _DEFAULTS = {'device_service_id' : mbim_constants.UUID_BASIC_CONNECT.bytes,
                 'cid' : mbim_constants.MBIM_CID_IP_CONFIGURATION,
                 'command_type' : mbim_constants.COMMAND_TYPE_QUERY,
                 'information_buffer_length' : 60,
                 'ipv4_configuration_available' : 0,
                 'ipv6_configuration_available' : 0,
                 'ipv4_address_count' : 0,
                 'ipv4_address_offset' : 0,
                 'ipv6_address_count' : 0,
                 'ipv6_address_offset' : 0,
                 'ipv4_gateway_offset' : 0,
                 'ipv6_gateway_offset' : 0,
                 'ipv4_dns_server_count' : 0,
                 'ipv4_dns_server_offset' : 0,
                 'ipv6_dns_server_count' : 0,
                 'ipv6_dns_server_offset' : 0,
                 'ipv4_mtu' : 0,
                 'ipv6_mtu' : 0}


class MBIMIPConfigurationInfo(mbim_message_response.MBIMCommandDone):
    """ The class for MBIM_IP_CONFIGURATION_INFO structure. """

    _FIELDS = (('I', 'session_id', ''),
               ('I', 'ipv4_configuration_available', ''),
               ('I', 'ipv6_configuration_available', ''),
               ('I', 'ipv4_address_count', ''),
               ('I', 'ipv4_address_offset', ''),
               ('I', 'ipv6_address_count', ''),
               ('I', 'ipv6_address_offset', ''),
               ('I', 'ipv4_gateway_offset', ''),
               ('I', 'ipv6_gateway_offset', ''),
               ('I', 'ipv4_dns_server_count', ''),
               ('I', 'ipv4_dns_server_offset', ''),
               ('I', 'ipv6_dns_server_count', ''),
               ('I', 'ipv6_dns_server_offset', ''),
               ('I', 'ipv4_mtu', ''),
               ('I', 'ipv6_mtu', ''))
    _IDENTIFIERS = {
            'device_service_id' : mbim_constants.UUID_BASIC_CONNECT.bytes,
            'cid' : mbim_constants.MBIM_CID_IP_CONFIGURATION}
