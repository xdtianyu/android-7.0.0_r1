# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import traceback

import common
from autotest_lib.client.common_lib import error


class MBIMComplianceError(error.TestFail):
    """ Base class for all errors overtly raised in the suite. """
    pass


class MBIMComplianceFrameworkError(MBIMComplianceError):
    """
    Errors raised by any of the framework code.

    These errors are raised by code that is not part of a test / sequence /
    assertion.

    """
    pass


class MBIMComplianceChannelError(MBIMComplianceError):
    """ Errors raised in the MBIM communication channel. """
    pass


class MBIMComplianceControlMessageError(MBIMComplianceError):
    """ Errors raised in the MBIM control module. """
    pass


class MBIMComplianceDataTransferError(MBIMComplianceError):
    """ Errors raised in the MBIM data transfer module. """
    pass


class MBIMComplianceNtbError(MBIMComplianceError):
    """ Errors raised in the MBIM NTB module. """
    pass


class MBIMComplianceTestError(MBIMComplianceError):
    """ Errors raised by compliance suite tests. """
    pass


class MBIMComplianceSequenceError(MBIMComplianceError):
    """ Errors raised by compliance suite sequences. """
    pass


class MBIMComplianceAssertionError(MBIMComplianceError):
    """ Errors raised by compliance suite assertions. """

    MBIM_ASSERTIONS = {
            # This key should not be used directly.
            # Raise |MBIMComplianceGenericAssertionError| instead.
            'no_code': '',

            # Assertion group: 3.x.x#x
            'mbim1.0:3.2.1#1': 'Functions that implement both NCM 1.0 and MBIM '
                               'shall provide two alternate settings for the '
                               'Communication Interface.',
            'mbim1.0:3.2.1#2': 'For alternate setting 0 of the Communication '
                               'Interface of an NCM/MBIM function: interface, '
                               'functional and endpoint descriptors shall be '
                               'constructed according to the rules given in '
                               '[USBNCM10].',
            'mbim1.0:3.2.1#3': 'For alternate setting 1 of the Communication '
                               'Interface of an NCM/MBIM function: interface, '
                               'functional and endpoint descriptors shall be '
                               'constructed according to the rules given in '
                               '[MBIM 1.0] section 6.',
            'mbim1.0:3.2.1#4': 'When alternate setting 0 of the Communiation'
                               'Interface of an NCM/MBIM function is selected, '
                               'the function shall operator according to the '
                               'NCM rules given in [USBNCM10].',
            'mbim1.0:3.2.1#5': 'When alternate setting 1 of the Communiation'
                               'Interface of an NCM/MBIM function is selected, '
                               'the function shall operator according to the '
                               'MBIM rules given in [MBIM1.0].',
            'mbim1.0:3.2.2.1#1': 'If an Interface Association Descriptor is '
                                 'used to form an NCM/MBIM function, its '
                                 'interface class, subclass, and protocol '
                                 'codes shall match those given in alternate '
                                 'setting 0 of the Communication Interface. ',
            'mbim1.0:3.2.2.4#1': 'Functions that implement both NCM 1.0 and '
                                 'MBIM (an "NCM/MBIM function") shall provide '
                                 'three alternate settings for the Data '
                                 'Interface.',
            'mbim1.0:3.2.2.4#2': 'For an NCM/MBIM function, the Data Interface '
                                 'descriptors for alternate settings 0 and 1 '
                                 'must have bInterfaceSubClass == 00h, and '
                                 'bInterfaceProtocol == 01h.',
            'mbim1.0:3.2.2.4#3': 'For an NCM/MBIM function, the Data Interface '
                                 'descriptor for alternate setting 2 must have '
                                 'bInterfaceSubClass == 00h, and '
                                 'bInterfaceProtocol == 02h.',
            'mbim1.0:3.2.2.4#4': 'For an NCM/MBIM function there must be no '
                                 'endpoints for alternate setting 0 of the '
                                 'Data Interface. For each of the other two '
                                 'alternate settings (1 and 2) there must be '
                                 'exactly two endpoints: one Bulk IN and one '
                                 'Bulk OUT.',

            # Assertion group: 6.x#x
            'mbim1.0:6.1#1': 'If an Interface Association Descriptor (IAD) is '
                             'provided for the MBIM function, the IAD and the '
                             'mandatory CDC Union Functional Descriptor '
                             'specified for the MBIM function shall group '
                             'together the same interfaces.',
            'mbim1.0:6.1#2': 'If an Interface Association Descriptor (IAD) is '
                             'provided for the MBIM only function, its '
                             'interface class, subclass, and protocol codes '
                             'shall match those given in the Communication '
                             'Interface descriptor.',
            'mbim1.0:6.3#1': 'The descriptor for alternate setting 0 of the '
                             'Communication Interface of an MBIM only function '
                             'shall have bInterfaceClass == 02h, '
                             'bInterfaceSubClass == 0Eh, and '
                             'bInterfaceProtocol == 00h.',
            'mbim1.0:6.3#2': 'MBIM Communication Interface description shall '
                             'include the following functional descriptors: '
                             'CDC Header Functional Descriptor, CDC Union '
                             'Functional Descriptor, and MBIM Functional '
                             'Descriptor. Refer to Table 6.2 of [USBMBIM10].',
            'mbim1.0:6.3#3': 'CDC Header Functional Descriptor shall appear '
                             'before CDC Union Functional Descriptor and '
                             'before MBIM Functional Descriptor.',
            'mbim1.0:6.3#4': 'CDC Union Functional Descriptor for an MBIM '
                             'function shall group together the MBIM '
                             'Communication Interface and the MBIM Data '
                             'Interface.',
            'mbim1.0:6.3#5': 'The class-specific descriptors must be followed '
                             'by an Interrupt IN endpoint descriptor.',
            'mbim1.0:6.4#1': 'Field wMaxControlMessage of MBIM Functional '
                             'Descriptor must not be smaller than 64.',
            'mbim1.0:6.4#2': 'Field bNumberFilters of MBIM Functional '
                             'Descriptor must not be smaller than 16.',
            'mbim1.0:6.4#3': 'Field bMaxFilterSize of MBIM Functional '
                             'Descriptor must not exceed 192.',
            'mbim1.0:6.4#4': 'Field wMaxSegmentSize of MBIM Functional '
                             'Descriptor must not be smaller than 2048.',
            'mbim1.0:6.4#5': 'Field bFunctionLength of MBIM Functional '
                             'Descriptor must be 12 representing the size of '
                             'the descriptor.',
            'mbim1.0:6.4#6': 'Field bcdMBIMVersion of MBIM Functional '
                             'Descriptor must be 0x0100 in little endian '
                             'format.',
            'mbim1.0:6.4#7': 'Field bmNetworkCapabilities of MBIM Functional '
                             'Descriptor should have the following bits set to '
                             'zero: D0, D1, D2, D4, D6 and D7.',
            'mbim1.0:6.5#1': 'If MBIM Extended Functional Descriptor is '
                             'provided, it must appear after MBIM Functional '
                             'Descriptor.',
            'mbim1.0:6.5#2': 'Field bFunctionLength of MBIM Extended '
                             'Functional Descriptor must be 8 representing the '
                             'size of the descriptor.',
            'mbim1.0:6.5#3': 'Field bcdMBIMEFDVersion of MBIM Extended '
                             'Functional Descriptor must be 0x0100 in little '
                             'endian format.',
            'mbim1.0:6.5#4': 'Field bMaxOutstandingCommandMessages of MBIM '
                             'Extended Functional Descriptor shall be greater '
                             'than 0.',
            'mbim1.0:6.6#1': 'The Data Interface for an MBIM only function '
                             'shall provide two alternate settings.',
            'mbim1.0:6.6#2': 'The first alternate setting for the Data '
                             'Interface of an MBIM only function (the default '
                             'interface setting, alternate setting 0) shall '
                             'include no endpoints.',
            'mbim1.0:6.6#3': 'The second alternate setting for the Data '
                             'Interface of an MBIM only function (alternate '
                             'setting 1) is used for normal operation, and '
                             'shall include one Bulk IN endpoint and one Bulk '
                             'OUT endpoint.',
            'mbim1.0:6.6#4': 'For an MBIM only function the Data Interface '
                             'descriptors for alternate settings 0 and 1 must '
                             'have bInterfaceSubClass == 00h, and '
                             'bInterfaceProtocol == 02h. Refer to Table 6.4 of '
                             '[USBMBIM10].',

            # Assertion Groups: 7.x.x#x
            'mbim1.0:7#1':   'To distinguish among the data streams, the last '
                             'character of the dwSignature in the NDP16 header '
                             'shall be coded with the index SessionId specified'
                             ' by the host in the MBIM_CID_CONNECT. The first '
                             'three symbols are encoded as ASCII characters in '
                             'little-endian form plus a last byte in HEX '
                             '(binary) format: "IPS"<SessionId>.',
            'mbim1.0:7#3':   'To distinguish among the data streams, the last '
                             'character of the dwSignature in the NDP32 header '
                             'shall be coded with the index SessionId specified'
                             ' by the host in the MBIM_CID_CONNECT. The first '
                             'three symbols are encoded as ASCII characters in '
                             'little-endian form plus a last byte in HEX '
                             '(binary) format: "ips"<SessionId>.',

            # Assertion Groups: 8.x.x#x
            'mbim1.0:8.1.2#2': 'The function must use a separate '
                               'GET_ENCAPSULATED_RESPONSE transfer for each '
                               'control message it has to send to the host.',
            'mbim1.0:8.1.2#3': 'The function must send a RESPONSE_AVAILABLE '
                               'notification for each available fragment of '
                               'ENCAPSULATED_RESPONSE to be read from the '
                               'default pipe.',

            # Assertion Groups: 9.x#x, 9.x.x and 9.x.x#x
            'mbim1.0:9.1#1':   'For notifications, the TransactionId must be '
                               'set to 0 by the function.',
            'mbim1.0:9.1#2':   'MessageLength in MBIM_MESSAGE_HEADER must be >='
                               ' 0x0C.',
            'mbim1.0:9.2':     'Function should fragment responses based on '
                               'MaxControlTransfer value from MBIM_OPEN_MSG.',
            'mbim1.0:9.3.1#1': 'In case MBIM_OPEN_MSG message is sent to a '
                               'function that is already opened, the function '
                               'shall interpret this as that the host and the '
                               'function are out of synchronization. The '
                               'function shall then perform the actions '
                               'dictated by the MBIM_CLOSE_MSG before it '
                               'performs the actions dictated by this '
                               'command.The function shall not send the '
                               'MBIM_CLOSE_DONE when the transition to the '
                               'Closed state has been completed. Only the '
                               ' MBIM_OPEN_DONE message is sent upon '
                               'successful completion of this message.',
            'mbim1.0:9.3.2#1': 'Between the host\'s sending MBIM_CLOSE_MSG '
                               'message and the function\'s completing the '
                               'request (acknowledged with MBIM_CLOSE_DONE), '
                               'the function shall ignore any MBIM control '
                               'messages it receives on the control plane or '
                               'the data on the bulk pipes.',
            'mbim1.0:9.3.2#2': 'The function shall not send any MBIM control '
                               'messages on the control plane or data on the '
                               'bulk pipes after completing '
                               'MBIM_CLOSE_MSG message (acknowledging it with '
                               'the MBIM_CLOSE_DONE message) with one '
                               'exception and that is MBIM_ERROR_NOT_OPENED.',
            'mbim1.0:9.3.2#3': 'On MBIM_CLOSE_MSG, any active context between '
                               'the function and the host shall be terminated ',
            'mbim1.0:9.3.4#2': 'An MBIM_FUNCTION_ERROR_MSG shall not make use '
                               'of a DataBuffer, so it cannot send any data '
                               'payload.',
            'mbim1.0:9.3.4#3': 'MBIM_ERROR_FRAGMENT_OUT_OF_SEQUENCE shall be '
                               'sent by the function if it detects a fragmented'
                               ' message out of sequence.',
            'mbim1.0:9.3.4.2#2': 'For MBIM_ERROR_FRAGMENT_OUT_OF_SEQUENCE, the'
                               ' TransactionId of the responding message must '
                               'match the TransactionId in the faulty '
                               'fragmented sequence.',
            'mbim1.0:9.3.4.2#3': 'In case of an out of a sequence error, the '
                               'function shall discard all the packets with '
                               'the same TransactionId as the faulty message '
                               'sequence.',
            'mbim1.0:9.3.4.2#4': 'If the function gets one more message that '
                               'is out of order for the same TransactionId, it '
                               'shall send a new error message with the same '
                               'TransactionId once more.',
            'mbim1.0:9.4.1#1': 'The function shall respond to the '
                               'MBIM_OPEN_MSG message with an MBIM_OPEN_DONE '
                               'message in which the TransactionId must match '
                               'the TransactionId in the MBIM_OPEN_MSG.',
            'mbim1.0:9.4.1#2': 'The Status field of MBIM_OPEN_DONE shall be '
                               'set to MBIM_STATUS_SUCCESS if the function '
                               'initialized successfully.',
            'mbim1.0:9.4.2#1': 'The function shall respond to the '
                               'MBIM_CLOSE_MSG message with an '
                               'MBIM_CLOSE_DONE message in which the '
                               'TransactionId must match the TransactionId in '
                               'the MBIM_CLOSE_MSG.',
            'mbim1.0:9.4.2#2': 'The Status field of MBIM_CLOSE_DONE shall '
                               'always be set to MBIM_STATUS_SUCCESS.',
            'mbim1.0:9.4.3': 'The function shall respond to '
                             'the MBIM_COMMAND_MSG message with an '
                             'MBIM_COMMAND_DONE message in which the '
                             'TransactionId must match the TransactionId in '
                             'the MBIM_COMMAND_MSG.',
            'mbim1.0:9.4.5#1': 'If the CID is successful, the function shall '
                               'set the Status field to MBIM_STATUS_SUCCESS '
                               'in the MBIM_COMMAND_DONE.',
            'mbim1.0:9.4.5#2': 'If the function does not implement the CID, '
                               'then the function shall fail the request with '
                               'MBIM_STATUS_NO_DEVICE_SUPPORT.',
            'mbim1.0:9.4.5#3': 'If the Status field returned to the host is '
                               'not equal to MBIM_STATUS_SUCCESS, the function '
                               'must set the Information BufferLength to 0, '
                               'indicating an empty InformationBuffer except '
                               'the following CIDs: MBIM_CID_REGISTER_STATE, '
                               'MBIM_CID_PACKET_SERVICE, MBIM_CID_CONNECT, '
                               'MBIM_CID_SERVICE_ACTIVATION.',
            'mbim1.0:9.5#1':   'Function should transmit fragmented message to '
                               'host without intermixing fragments from other '
                               'messages.',
            'mbim1.0:10.3#2':  'The function shall reject incoming messages '
                               'that dont follow the rules for variable-length'
                               ' encoding by setting '
                               'MBIM_STATUS_INVALID_PARAMETERS as the status '
                               'code in the MBIM_COMMAND_DONE message.',
            'mbim1.0:10.5.1.3#1': 'Functions that support CDMA must specify '
                               'MBIMCtrlCapsCdmaMobileIP or '
                               'MBIMCtrlCapsCdmaSimpleIP or both flags to '
                               'inform the host about the type of IP that the '
                               'function supports.',

            # NCM Assertion group: 3.x.x#x
            'ncm1.0:3.2.1#1':  'The first four bytes in NTH16 shall be '
                               '0x484D434E in little-endian format ("NCMH").',
            'ncm1.0:3.2.1#2':  'wHeaderLength value in NTH16 shall be 0x000C.',
            'ncm1.0:3.2.1#3':  'wSequence in NTH16 shall be set to zero by the '
                               'function in the first NTB transferred after '
                               'every "function reset" event.',
            'ncm1.0:3.2.1#4':  'wSequence value in NTH16 shall be incremented '
                               'for every NTB subsequent transfer.',
            'ncm1.0:3.2.1#5':  'NTB size (IN) shall not exceed dwNtbInMaxSize.',
            'ncm1.0:3.2.1#6':  'wNdpIndex value in NTH16 must be a multiple of '
                               '4, and must be >= 0x000C, in little endian.',
            'ncm1.0:3.2.2#1':  'The first four bytes in NTH32 shall be '
                               '0x686D636E in little-endian format ("ncmh").',
            'ncm1.0:3.2.2#2':  'wHeaderLength value in NTH32 shall be 0x0010.',
            'ncm1.0:3.2.2#3':  'wSequence in NTH32 shall be set to zero by the '
                               'function in the first NTB transferred after '
                               'every "function reset" event.',
            'ncm1.0:3.2.2#4':  'wSequence value in NTH32 shall be incremented '
                               'for every NTB subsequent transfer.',
            'ncm1.0:3.2.2#5':  'NTB size (IN) shall not exceed dwNtbInMaxSize.',
            'ncm1.0:3.2.2#6':  'dwNdpIndex value in NTH32 must be a multiple of'
                               ' 4, and must be >= 0x0010, in little endian.',
            'ncm1.0:3.3.1#1':  'wLength value in the NDP16 must be a multiple '
                               'of 4, and must be at least 16d (0x0010).',
            'ncm1.0:3.3.1#2':  'wDatagramIndex[0] value in NDP16 must be >= '
                               '0x000C (because it must point past the NTH16).',
            'ncm1.0:3.3.1#3':  'wDatagramLength[0] value in NDP16 must be >= '
                               '20d if datagram payload is IPv4 and >= 40d if '
                               'datagram payload is IPv6.',
            'ncm1.0:3.3.1#4':  'wDatagramIndex[(wLength-8)/4 - 1] value in '
                               'NDP16 must be zero.',
            'ncm1.0:3.3.1#5':  'wDatagramLength[(wLength-8)/4 - 1] value in '
                               'NDP16 must be zero.',
            'ncm1.0:3.3.2#1':  'wLength value in the NDP32 must be a multiple '
                               'of 8, and must be at least 16d (0x0020).',
            'ncm1.0:3.3.2#2':  'dwDatagramIndex[0] value in NDP32 must be >= '
                               '0x0010 (because it must point past the NTH32).',
            'ncm1.0:3.3.2#3':  'dwDatagramLength[0] value in NDP32 must be >= '
                               '20d if datagram payload is IPv4 and >= 40d if '
                               'datagram payload is IPv6.',
            'ncm1.0:3.3.2#4':  'dwDatagramIndex[(wLength-8)/4 - 1] value in '
                               'NDP32 must be zero.',
            'ncm1.0:3.3.2#5':  'dwDatagramLength[(wLength-8)/4 - 1] value in '
                               'NDP32 must be zero.',
    }

    def __init__(self, assertion_id, error_string=None):
        """
        @param assertion_id: A str that must be a key in the MBIM_ASSERTIONS map
                defined in this class.
        @param error_string: An optional str to be appended to the error
                description.

        For example,
            MBIMComplianceAssertionError('mbim1.0:3.2.1#1')
            raises an error associated with assertion [MBIM 1.0]-3.2.1#1

        """
        if assertion_id not in self.MBIM_ASSERTIONS:
            log_and_raise(MBIMComplianceFrameworkError,
                          'Unknown assertion id "%s"' % assertion_id)

        message = '[%s]: %s' % (assertion_id,
                                self.MBIM_ASSERTIONS[assertion_id])
        if error_string:
            message += ': %s' % error_string

        super(MBIMComplianceAssertionError, self).__init__(message)


class MBIMComplianceGenericAssertionError(MBIMComplianceAssertionError):
    """ Assertion errors that don't map directly to an MBIM assertion. """
    def __init__(self, error_string):
        """
        @param error_string: A description of the error.
        """
        super(MBIMComplianceGenericAssertionError, self).__init__(
                'no_code',
                error_string)


def log_and_raise(error_class, *args):
    """
    Log and raise an error.

    This function should be used to raise all errors.

    @param error_class: An Exception subclass to raise.
    @param *args: Arguments to be passed to the error class constructor.
    @raises: |error_class|.

    """
    error_object = error_class(*args)
    logging.error(error_object)
    trace = traceback.format_stack()
    # Get rid of the current frame from trace
    trace = trace[:len(trace)-1]
    logging.error('Traceback:\n' + ''.join(trace))
    raise error_object
