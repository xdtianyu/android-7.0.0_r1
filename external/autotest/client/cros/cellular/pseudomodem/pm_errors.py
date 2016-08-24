# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""
This module provides exception classes for pseudomodem.

"""

import dbus.exceptions

import common
from autotest_lib.client.cros.cellular import mm1_constants

class MMError(dbus.exceptions.DBusException):
    """
    Generic DBusException subclass that serves as the base class for
    ModemManager errors.

    """

    def __init__(self, errno, *args, **kwargs):
        super(MMError, self).__init__(self, args, kwargs)
        self.include_traceback = False
        self._error_name_base = None
        self._error_name_map = None
        self._Setup()
        self._dbus_error_name = (self._error_name_base +
            self._error_name_map[errno])

    def _Setup(self):
        raise NotImplementedError()


class MMConnectionError(MMError):
    """ DBusException wrapper for MMConnectionError values. """

    UNKNOWN = 0
    NO_CARRIER = 1
    NO_DIALTONE = 2
    BUSY = 3
    NO_ANSWER = 4

    def _Setup(self):
        self._error_name_base = mm1_constants.MM1_ERROR_PREFIX + '.Connection'
        self._error_name_map = {
            self.UNKNOWN : '.Unknown',
            self.NO_CARRIER : '.NoCarrier',
            self.NO_DIALTONE : '.NoDialtone',
            self.BUSY : '.Busy',
            self.NO_ANSWER : '.NoAnswer'
        }


class MMCoreError(MMError):
    """
    DBusException wrapper for MMCoreError values.

    """

    FAILED = 0
    CANCELLED = 1
    ABORTED = 2
    UNSUPPORTED = 3
    NO_PLUGINS = 4
    UNAUTHORIZED = 5
    INVALID_ARGS = 6
    IN_PROGRESS = 7
    WRONG_STATE = 8
    CONNECTED = 9
    TOO_MANY = 10
    NOT_FOUND = 11
    RETRY = 12
    EXISTS = 13

    def _Setup(self):
        self._error_name_base = mm1_constants.MM1_ERROR_PREFIX + '.Core'
        self._error_name_map = {
            self.FAILED : '.Failed',
            self.CANCELLED : '.Cancelled',
            self.ABORTED : '.Aborted',
            self.UNSUPPORTED : '.Unsupported',
            self.NO_PLUGINS : '.NoPlugins',
            self.UNAUTHORIZED : '.Unauthorized',
            self.INVALID_ARGS : '.InvalidArgs',
            self.IN_PROGRESS : '.InProgress',
            self.WRONG_STATE : '.WrongState',
            self.CONNECTED : '.Connected',
            self.TOO_MANY : '.TooMany',
            self.NOT_FOUND : '.NotFound',
            self.RETRY : '.Retry',
            self.EXISTS : '.Exists'
        }


class MMMessageError(MMError):
    """ DBusException wrapper for MMMessageError values. """

    ME_FAILURE = 300
    SMS_SERVICE_RESERVED = 301
    NOT_ALLOWED = 302
    NOT_SUPPORTED = 303
    INVALID_PDU_PARAMETER = 304
    INVALID_TEXT_PARAMETER = 305
    SIM_NOT_INSERTED = 310
    SIM_PIN = 311
    PH_SIM_PIN = 312
    SIM_FAILURE = 313
    SIM_BUSY = 314
    SIM_WRONG = 315
    SIM_PUK = 316
    SIM_PIN2 = 317
    SIM_PUK2 = 318
    MEMORY_FAILURE = 320
    INVALID_INDEX = 321
    MEMORY_FULL = 322
    SMSC_ADDRESS_UNKNOWN = 330
    NO_NETWORK = 331
    NETWORK_TIMEOUT = 332
    NO_CNMA_ACK_EXPECTED = 340
    UNKNOWN = 500

    def _Setup(self):
        self._error_name_base = mm1_constants.MM1_ERROR_PREFIX + '.Message'
        self._error_name_map = {
            self.ME_FAILURE : '.MeFailure ',
            self.SMS_SERVICE_RESERVED : '.SmsServiceReserved',
            self.NOT_ALLOWED : '.NotAllowed',
            self.NOT_SUPPORTED : '.NotSupported',
            self.INVALID_PDU_PARAMETER :
                    '.InvalidPduParameter',
            self.INVALID_TEXT_PARAMETER :
                    '.InvalidTextParameter',
            self.SIM_NOT_INSERTED : '.SimNotInserted',
            self.SIM_PIN : '.SimPin',
            self.PH_SIM_PIN : '.PhSimPin',
            self.SIM_FAILURE : '.SimFailure',
            self.SIM_BUSY : '.SimBusy',
            self.SIM_WRONG : '.SimWrong',
            self.SIM_PUK : '.SimPuk',
            self.SIM_PIN2 : '.SimPin2',
            self.SIM_PUK2 : '.SimPuk2',
            self.MEMORY_FAILURE : '.MemoryFailure',
            self.INVALID_INDEX : '.InvalidIndex',
            self.MEMORY_FULL : '.MemoryFull',
            self.SMSC_ADDRESS_UNKNOWN : '.SmscAddressUnknown',
            self.NO_NETWORK : '.NoNetwork',
            self.NETWORK_TIMEOUT : '.NetworkTimeout',
            self.NO_CNMA_ACK_EXPECTED : '.NoCnmaAckExpected',
            self.UNKNOWN : '.Unknown'
        }


class MMMobileEquipmentError(MMError):
    """ DBusException wrapper for MMMobileEquipmentError values. """

    PHONE_FAILURE = 0
    NO_CONNECTION = 1
    LINK_RESERVED = 2
    NOT_ALLOWED = 3
    NOT_SUPPORTED = 4
    PH_SIM_PIN = 5
    PH_FSIM_PIN = 6
    PH_FSIM_PUK = 7
    SIM_NOT_INSERTED = 10
    SIM_PIN = 11
    SIM_PUK = 12
    SIM_FAILURE = 13
    SIM_BUSY = 14
    SIM_WRONG = 15
    INCORRECT_PASSWORD = 16
    SIM_PIN2 = 17
    SIM_PUK2 = 18
    MEMORY_FULL = 20
    INVALID_INDEX = 21
    NOT_FOUND = 22
    MEMORY_FAILURE = 23
    TEXT_TOO_LONG = 24
    INVALID_CHARS = 25
    DIAL_STRING_TOO_LONG = 26
    DIAL_STRING_INVALID = 27
    NO_NETWORK = 30
    NETWORK_TIMEOUT = 31
    NETWORK_NOT_ALLOWED = 32
    NETWORK_PIN = 40
    NETWORK_PUK = 41
    NETWORK_SUBSET_PIN = 42
    NETWORK_SUBSET_PUK = 43
    SERVICE_PIN = 44
    SERVICE_PUK = 45
    CORP_PIN = 46
    CORP_PUK = 47
    UNKNOWN = 100
    # GPRS related errors
    GPRS_ILLEGAL_MS = 103
    GPRS_ILLEGAL_ME = 106
    GPRS_SERVICE_NOT_ALLOWED = 107
    GPRS_PLMN_NOT_ALLOWED = 111
    GPRS_LOCATION_NOT_ALLOWED = 112
    GPRS_ROAMING_NOT_ALLOWED = 113
    GPRS_SERVICE_OPTION_NOT_SUPPORTED = 132
    GPRS_SERVICE_OPTION_NOT_SUBSCRIBED = 133
    GPRS_SERVICE_OPTION_OUT_OF_ORDER = 134
    GPRS_UNKNOWN = 148
    GPRS_PDP_AUTH_FAILURE = 149
    GPRS_INVALID_MOBILE_CLASS = 150

    def _Setup(self):
        self._error_name_base = \
            mm1_constants.MM1_ERROR_PREFIX + '.MobileEquipment'
        self._error_name_map = {
            self.PHONE_FAILURE : '.PhoneFailure',
            self.NO_CONNECTION : '.NoConnection',
            self.LINK_RESERVED : '.LinkReserved',
            self.NOT_ALLOWED : '.NotAllowed',
            self.NOT_SUPPORTED : '.NotSupported',
            self.PH_SIM_PIN : '.PhSimPin',
            self.PH_FSIM_PIN : '.PhFsimPin',
            self.PH_FSIM_PUK : '.PhFsimPuk',
            self.SIM_NOT_INSERTED : '.SimNotInserted',
            self.SIM_PIN : '.SimPin',
            self.SIM_PUK : '.SimPuk',
            self.SIM_FAILURE : '.SimFailure',
            self.SIM_BUSY : '.SimBusy',
            self.SIM_WRONG : '.SimWrong',
            self.INCORRECT_PASSWORD :
                '.IncorrectPassword',
            self.SIM_PIN2 : '.SimPin2',
            self.SIM_PUK2 : '.SimPuk2',
            self.MEMORY_FULL : '.MemoryFull',
            self.INVALID_INDEX : '.InvalidIndex',
            self.NOT_FOUND : '.NotFound',
            self.MEMORY_FAILURE : '.MemoryFailure',
            self.TEXT_TOO_LONG : '.TextTooLong',
            self.INVALID_CHARS : '.InvalidChars',
            self.DIAL_STRING_TOO_LONG :
                '.DialStringTooLong',
            self.DIAL_STRING_INVALID :
                '.DialStringInvalid',
            self.NO_NETWORK : '.NoNetwork',
            self.NETWORK_TIMEOUT : '.NetworkTimeout',
            self.NETWORK_NOT_ALLOWED :
                '.NetworkNotAllowed',
            self.NETWORK_PIN : '.NetworkPin',
            self.NETWORK_PUK : '.NetworkPuk',
            self.NETWORK_SUBSET_PIN :
                '.NetworkSubsetPin',
            self.NETWORK_SUBSET_PUK :
                '.NetworkSubsetPuk',
            self.SERVICE_PIN : '.ServicePin',
            self.SERVICE_PUK : '.ServicePuk',
            self.CORP_PIN : '.CorpPin',
            self.CORP_PUK : '.CorpPuk',
            self.UNKNOWN : '.Unknown',
            self.GPRS_ILLEGAL_MS : '.Gprs.IllegalMs',
            self.GPRS_ILLEGAL_ME : '.Gprs.IllegalMe',
            self.GPRS_SERVICE_NOT_ALLOWED :
                '.Gprs.ServiceNotAllowed',
            self.GPRS_PLMN_NOT_ALLOWED :
                '.Gprs.PlmnNotAllowed',
            self.GPRS_LOCATION_NOT_ALLOWED :
                '.Gprs.LocationNotAllowed',
            self.GPRS_ROAMING_NOT_ALLOWED :
                '.Gprs.RoamingNotAllowed',
            self.GPRS_SERVICE_OPTION_NOT_SUPPORTED :
                '.Gprs.ServiceOptionNotSupported',
            self.GPRS_SERVICE_OPTION_NOT_SUBSCRIBED :
                '.Gprs.ServiceOptionNotSubscribed',
            self.GPRS_SERVICE_OPTION_OUT_OF_ORDER :
                '.Gprs.ServiceOptionOutOfOrder',
            self.GPRS_UNKNOWN :
                '.Gprs.Unknown',
            self.GPRS_PDP_AUTH_FAILURE :
                '.Gprs.PdpAuthFailure',
            self.GPRS_INVALID_MOBILE_CLASS :
                '.Gprs.InvalidMobileClass'
        }


class MMSerialError(MMError):
    """ DBusException wrapper for MMSerialError values. """

    UNKNOWN = 0
    OPEN_FAILED = 1
    SEND_FAILED = 2
    RESPONSE_TIMEOUT = 3
    OPEN_FAILED_NO_DEVICE = 4
    FLASH_FAILED = 5
    NOT_OPEN = 6

    def _Setup(self):
        self._error_name_base = mm1_constants.MM1_ERROR_PREFIX + '.Serial'
        self._error_name_map = {
            self.UNKNOWN : '.Unknown',
            self.OPEN_FAILED : '.OpenFailed',
            self.SEND_FAILED : '.SendFailed',
            self.RESPONSE_TIMEOUT : '.ResponseTimeout',
            self.OPEN_FAILED_NO_DEVICE : '.OpenFailedNoDevice',
            self.FLASH_FAILED : '.FlashFailed',
            self.NOT_OPEN : '.NotOpen'
        }


class MMCdmaActivationError(MMError):
    """ DBusException wrapper for MMCdmaActivationError values. """

    NONE = 0
    UNKNOWN = 1
    ROAMING = 2
    WRONG_RADIO_INTERFACE = 3
    COULD_NOT_CONNECT = 4
    SECURITY_AUTHENTICATION_FAILED = 5
    PROVISIONING_FAILED = 6
    NO_SIGNAL = 7
    TIMED_OUT = 8
    START_FAILED = 9

    def _Setup(self):
        self._error_name_base = \
            mm1_constants.MM1_ERROR_PREFIX + '.CdmaActivation'
        self._error_name_map = {
            self.NONE : '.None',
            self.UNKNOWN :
                '.Unknown',
            self.ROAMING :
                '.Roaming',
            self.WRONG_RADIO_INTERFACE :
                '.WrongRadioInterface',
            self.COULD_NOT_CONNECT :
                '.CouldNotConnect',
            self.SECURITY_AUTHENTICATION_FAILED :
                '.SecurityAuthenticationFailed',
            self.PROVISIONING_FAILED :
                '.ProvisioningFailed',
            self.NO_SIGNAL :
                '.NoSignal',
            self.TIMED_OUT :
                '.TimedOut',
            self.START_FAILED :
                '.StartFailed'
        }


class TestError(dbus.exceptions.DBusException):
    """
    Raised by the test interface of Pseudomodem.

    This is not a core ModemManager error, and is raised only on the test
    interface mostly to notify the user of invalid requests or misconfiguration
    of pseudomodem.

    """
    pass
