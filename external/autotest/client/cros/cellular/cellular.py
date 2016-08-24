# Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Constants, enums, and basic types for cellular base station emulation."""

DEFAULT_TIMEOUT = 10


def Enum(enum_name, items):
    """Build a class with a member for each item.

    Arguments:
      members: A list of items for the enum.  They must be valid python
       identifiers
    """
    class output(object):
        pass

    for item in items:
        setattr(output, item, enum_name + ':' + item)

    return output


Technology = Enum('Technology', [
    'GPRS',
    'EGPRS',
    'WCDMA',
    'HSDPA',
    'HSUPA',
    'HSDUPA',
    'HSPA_PLUS',
    'CDMA_2000',
    'EVDO_1X',
    'LTE'
])

TechnologyFamily = Enum('TechnologyFamily', [
    'UMTS',
    'CDMA',
    'LTE'
])

TechnologyToFamily = {
    Technology.GPRS: TechnologyFamily.UMTS,
    Technology.EGPRS: TechnologyFamily.UMTS,
    Technology.WCDMA: TechnologyFamily.UMTS,
    Technology.HSDPA: TechnologyFamily.UMTS,
    Technology.HSUPA: TechnologyFamily.UMTS,
    Technology.HSDUPA: TechnologyFamily.UMTS,
    Technology.HSPA_PLUS: TechnologyFamily.UMTS,
    Technology.CDMA_2000: TechnologyFamily.CDMA,
    Technology.EVDO_1X: TechnologyFamily.CDMA,
    Technology.LTE: TechnologyFamily.LTE,
}


UeGsmDataStatus = Enum('GsmDataStatus', [
    'NONE',
    'IDLE',
    'ATTACHING',
    'ATTACHED',
    'DETACHING',
    'PDP_ACTIVATING',
    'PDP_ACTIVE',
    'PDP_DEACTIVATING',
])

UeC2kDataStatus = Enum('C2kDataStatus', [
    'OFF',
    'DORMANT',
    'DATA_CONNECTED',
])

UeEvdoDataStatus = Enum('EvdoDataStatus', [
    'CONNECTION_CLOSING',
    'CONNECTION_NEGOTIATE',
    'CONNECTION_REQUEST',
    'DATA_CONNECTED',
    'DORMANT',
    'HANDOFF',
    'IDLE',
    'PAGING',
    'SESSION_CLOSING',
    'SESSION_NEGOTIATE',
    'SESSION_OPEN',
    'UATI_REQUEST',
])

# todo(byronk): Move this LTE specific data into the LTE call_box object
UeLteDataStatus = Enum('LteDataStatus', [
    'OFF',
    'IDLE',
    'CONNECTED',
    'REGISTERED',
    'LOOPBACK',
    'RELEASE',
    'UNAVAILABLE',
])

# Each cell technology has a different connection state machine.  For
# generic tests, we want to abstract that away.  UeGenericDataStatus
# is this abstraction, and RatToGenericDataStatus is a map from
# specific states to this generic status.


# TODO(rochberg):  Do we need connecting/disconnecting for this level of test?
UeGenericDataStatus = Enum('UeGenericDataStatus', [
    'NONE',             # UE not seen or in transition to/from REGISTERED
    'REGISTERED',       # Network knows about UE
    'CONNECTED',        # Data can be sent
    'CONNECTING',
    'DISCONNECTING',
])


RatToGenericDataStatus = {
    UeGsmDataStatus.NONE: UeGenericDataStatus.NONE,
    UeGsmDataStatus.IDLE: UeGenericDataStatus.NONE,
    UeGsmDataStatus.ATTACHING: UeGenericDataStatus.NONE, # Transition
    UeGsmDataStatus.ATTACHED: UeGenericDataStatus.REGISTERED,
    UeGsmDataStatus.DETACHING: UeGenericDataStatus.NONE, # Transition
    UeGsmDataStatus.PDP_ACTIVATING: UeGenericDataStatus.CONNECTING,
    UeGsmDataStatus.PDP_ACTIVE: UeGenericDataStatus.CONNECTED,
    UeGsmDataStatus.PDP_DEACTIVATING: UeGenericDataStatus.DISCONNECTING,

    UeC2kDataStatus.OFF: UeGenericDataStatus.NONE,
    UeC2kDataStatus.DORMANT: UeGenericDataStatus.CONNECTED,
    UeC2kDataStatus.DATA_CONNECTED: UeGenericDataStatus.CONNECTED,

    UeEvdoDataStatus.CONNECTION_CLOSING: UeGenericDataStatus.DISCONNECTING,
    UeEvdoDataStatus.CONNECTION_NEGOTIATE: UeGenericDataStatus.CONNECTING,
    UeEvdoDataStatus.CONNECTION_REQUEST: UeGenericDataStatus.CONNECTING,
    UeEvdoDataStatus.DATA_CONNECTED: UeGenericDataStatus.CONNECTED,
    UeEvdoDataStatus.DORMANT: UeGenericDataStatus.CONNECTED,
    UeEvdoDataStatus.HANDOFF: UeGenericDataStatus.CONNECTING,
    UeEvdoDataStatus.IDLE: UeGenericDataStatus.CONNECTED,
    UeEvdoDataStatus.PAGING: UeGenericDataStatus.CONNECTED,
    UeEvdoDataStatus.SESSION_CLOSING: UeGenericDataStatus.DISCONNECTING,
    UeEvdoDataStatus.SESSION_NEGOTIATE: UeGenericDataStatus.CONNECTING,
    UeEvdoDataStatus.SESSION_OPEN: UeGenericDataStatus.REGISTERED,
    UeEvdoDataStatus.UATI_REQUEST: UeGenericDataStatus.NONE,
    UeLteDataStatus.OFF: UeGenericDataStatus.NONE,
    UeLteDataStatus.IDLE: UeGenericDataStatus.NONE,
    UeLteDataStatus.CONNECTED: UeGenericDataStatus.CONNECTED,
    UeLteDataStatus.REGISTERED: UeGenericDataStatus.REGISTERED,
    UeLteDataStatus.LOOPBACK: UeGenericDataStatus.NONE,
    UeLteDataStatus.RELEASE: UeGenericDataStatus.DISCONNECTING,
    UeLteDataStatus.UNAVAILABLE: UeGenericDataStatus.NONE
}


class Power(object):
    """Useful power levels, in dBm."""
    OFF = -200
    DEFAULT = -35


class SmsAddress(object):
    def __init__(self, address, address_type='INAT', address_plan='ISDN'):
        """Constructs an SMS address.

        For expediency, the address type arguments come from the GPIB
        commands for the Agilent 8960.  See
        http://wireless.agilent.com/rfcomms/refdocs/
               gsmgprs/gprsla_hpib_sms.html#CIHDGBIH

        Arguments:
            address:  1-10 octets
            address_type:  INAT, NAT, NET, SUBS, ALPH, ABBR, RES
            address_plan:  ISDN, DATA, TEL, SCS1, SCS2, PRIV, NATional,
                           ERMes, RES
        """
        self.address = address
        self.address_type = address_type
        self.address_plan = address_plan


class TestEnvironment(object):
    def __init__(self, event_loop):
        pass

    def RequestBaseStations(self,
                            configuration,
                            requirements_list):
        """Requests a set of base stations that satisfy the given requirements.

        Arguments:
            configuration:  configuration dictionary
            requirements_list: A list of lists of technologies that must be
                               supported

        Returns: a list of base stations.
        """
        pass

    def TimedOut(self):
        """Called by base stations when an expected event hasn't occurred."""
        pass
