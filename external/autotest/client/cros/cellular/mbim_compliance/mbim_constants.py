# Copyright (c) 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""
This module defines the constants which are used in different components
within the MBIM compliance suite.
All the constants should match the values and names specified in MBIM
Specification[1].

Reference:
    [1] Universal Serial Bus Communications Class Subclass Specification for
        Mobile Broadband Interface Model
        http://www.usb.org/developers/docs/devclass_docs/
        MBIM10Errata1_073013.zip
"""
import uuid


# The following type values are defined for the MBIM control messages sent from
# the host to the device.
MBIM_OPEN_MSG = 0x00000001
MBIM_CLOSE_MSG = 0x00000002
MBIM_COMMAND_MSG = 0x00000003
MBIM_HOST_ERROR_MSG = 0x00000004

# The following type values are defined for the MBIM control messages sent from
# the device to the host.
MBIM_OPEN_DONE = 0x80000001
MBIM_CLOSE_DONE = 0x80000002
MBIM_COMMAND_DONE = 0x80000003
MBIM_FUNCTION_ERROR_MSG = 0x80000004
MBIM_INDICATE_STATUS_MSG = 0x80000007

# The following type values are defined for the MBIM status codes.
MBIM_STATUS_SUCCESS = 0x00000000
MBIM_STATUS_NO_DEVICE_SUPPORT = 0x00000009
MBIM_STATUS_CONTEXT_NOT_ACTIVATED = 0x00000010
MBIM_STATUS_INVALID_PARAMETERS = 0x00000015

# The following type values are defined for both MBIM_HOST_ERROR_MSG and
# MBIM_FUNCTION_ERROR_MSG.
MBIM_ERROR_TIMEOUT_FRAGMENT = 0x00000001
MBIM_ERROR_FRAGMENT_OUT_OF_SEQUENCE = 0x00000002
MBIM_ERROR_LENGTH_MISMATCH = 0x00000003
MBIM_ERROR_DUPLICATED_TID = 0x00000004
MBIM_ERROR_NOT_OPENED = 0x00000005
MBIM_ERROR_UNKNOWN = 0x00000006
MBIM_ERROR_CANCEL = 0x00000007
MBIM_ERROR_MAX_TRANSFER = 0x00000008

# The following command codes are defined for the MBIM command identifiers.
MBIM_CID_DEVICE_CAPS = 0x00000001
MBIM_CID_SUBSCRIBER_READY_STATUS = 0x00000002
MBIM_CID_RADIO_STATE = 0x00000003
MBIM_CID_PIN = 0x00000004
MBIM_CID_HOME_PROVIDER = 0x00000006
MBIM_CID_REGISTER_STATE = 0x00000009
MBIM_CID_PACKET_SERVICE = 0x0000000A
MBIM_CID_SIGNAL_STATE = 0x0000000B
MBIM_CID_CONNECT = 0x0000000C
MBIM_CID_SERVICE_ACTIVATION = 0x0000000E
MBIM_CID_IP_CONFIGURATION = 0x0000000F
MBIM_CID_DEVICE_SERVICES = 0x00000010

# The following command types are defined for the MBIM command message.
COMMAND_TYPE_QUERY = 0
COMMAND_TYPE_SET = 1

# The following UUID values are defined for the device service identifiers.
UUID_BASIC_CONNECT = uuid.UUID('A289CC33-BCBB-8B4F-B6B0-133EC2AAE6DF')

# The following UUID values are defined for the MBIM_CONTEXT_TYPES which are
# used in the |context_type| field of information structures.
MBIM_CONTEXT_TYPE_NONE = uuid.UUID('B43F758C-A560-4B46-B35E-C5869641FB54')
MBIM_CONTEXT_TYPE_INTERNET = uuid.UUID('7E5E2A7E-4E6F-7272-736B-656E7E5E2A7E')

# NTB formats
NTB_FORMAT_16 = 0
NTB_FORMAT_32 = 1

# MBIM Device Caps Cellular classes
CELLULAR_CLASS_MASK_GSM = 0x01
CELLULAR_CLASS_MASK_CDMA = 0x02

# MBIM CTRL Caps
CTRL_CAPS_MASK_NONE = 0x0000
CTRL_CAPS_MASK_REG_MANUAL = 0x0001
CTRL_CAPS_MASK_HW_RADIO_SWITCH = 0x0002
CTRL_CAPS_MASK_CDMA_MOBILE_IP = 0x0004
CTRL_CAPS_MASK_CDMA_SIMPLE_IP = 0x0008
CTRL_CAPS_MASK_MULTI_CARRIER = 0x0010
