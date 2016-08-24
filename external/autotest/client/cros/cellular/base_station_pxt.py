# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import re
import time

import common
from autotest_lib.client.bin import utils
from autotest_lib.client.cros.cellular import air_state_verifier
from autotest_lib.client.cros.cellular import base_station_interface
from autotest_lib.client.cros.cellular import cellular
from autotest_lib.client.cros.cellular import cellular_logging
from autotest_lib.client.cros.cellular import cellular_system_error

POLL_SLEEP = 0.2

log = cellular_logging.SetupCellularLogging('base_station_pxt')


class BaseStationPxt(base_station_interface.BaseStationInterface):
    """Wrap an Agilent PXT"""

    def __init__(self, scpi_connection, no_initialization=False):
        """
        Creates a PXT call-box object.
        TODO(byronk): Make a factory that returns a call_box, of either
        a 8960 or a PXT, or a...

        @param scpi_connection:  The scpi port to send commands over.
        @param no_initialization: Don't do anything. Useful for unit testing
        and debugging when you don't want to run all the usual functions.
        """
        # TODO(byronk): Use variables longer then 1 char.
        self.c = scpi_connection
        if no_initialization:
            return
        self.checker_context = self.c.checker_context
        with self.checker_context:
            self._Verify()
            self._Reset()

    def _Verify(self):
        idn = self.c.Query('*IDN?')
        if 'E6621' not in idn:
            raise cellular_system_error.BadScpiCommand(
                'Not actually a E6621 PXT:  *IDN? says ' + idn)

    def _Reset(self):
        self.c.Reset()
        self.Stop()

    def _IsIdle(self):
        bs_active = self.c.Query('BSE:SIMULator?')
        return bs_active == 'STOP'

    def Close(self):
        self.c.Close()

    def GetAirStateVerifier(self):
        return air_state_verifier.AirStateVerifierBasestation(self)

    def GetDataCounters(self):
        raise NotImplementedError

    def GetRatUeDataStatus(self):
        """Get the radio-access-technology-specific status of the UE.

        Unlike GetUeDataStatus, below, this returns a status that depends
        on the RAT being used.
        """
        status = self.c.Query('BSE:STATus:ACELL?')
        rat = \
            ConfigDictionaries.FORMAT_TO_DATA_STATUS_TYPE[self.format][status]
        return rat

    def GetUeDataStatus(self):
        """Get the UeGenericDataStatus status of the device."""
        rat = self.GetRatUeDataStatus()
        return cellular.RatToGenericDataStatus[rat]

    def ResetDataCounters(self):
        # Keep this here until the implementation of this class
        # is done. If we never hit this, then we can safely remove it,
        # but we should think twice about that. The 8960 needs it,
        # it seems likely that the PXT should.
        raise NotImplementedError

    def ClearErrors(self):
        self.c.RetrieveErrors()

    def LogStats(self):
        # Keep this here until the implementation of this class
        # is done. If we never hit this, then we can safely remove it,
        # but we should think twice about that. The 8960 needs it,
        # it seems likely that the PXT should.
        raise NotImplementedError

    def SetBsIpV4(self, ip1, ip2):
        return  # TODO(byronk): Configure the PXT to find.crbug.com:/235643

    def SetBsNetmaskV4(self, netmask):
        return  # TODO(byronk): Configure the PXT to find. crbug.com:/235643

    def SetPlmn(self, mcc, mnc):
        raise NotImplementedError

    def SetPower(self, dbm):
        # TODO(byronk): Setting the RF output of the PXT. crbug.com/235655
        # and 8960 call boxes to OFF should be off
        if dbm <= cellular.Power.OFF:
            self.c.SendStanza(['AMPLitude:ALL -120'])
        else:
            self.c.SendStanza(['AMPLitude:ALL %s' % dbm])

    def SetTechnology(self, technology):
        # TODO(byronk):  The set technology step likely belongs in the
        # constructor.

        # Print out a helpful message on a key error.
        try:
            self.format = ConfigDictionaries.TECHNOLOGY_TO_FORMAT[technology]
        except KeyError:
            raise KeyError('%s not in %s ' % (
                technology,
                ConfigDictionaries.TECHNOLOGY_TO_FORMAT))
        self.technology = technology

    def SetUeDnsV4(self, dns1, dns2):
        """Set the DNS values provided to the UE.  Emulator must be stopped."""
        return  # TODO(byronk): Configure the PXT to find. crbug.com/235643
                # the data server

    def SetUeIpV4(self, ip1, ip2=None):
        """
        Set the IP addresses provided to the UE. Emulator must be stopped.
        """
        return  # TODO(byronk) crbug.com:/235643: Configure the PXT to find
                # the data server

    def Start(self):
        commands = [
            '*CLS',
            'STATus:PRESet',
            # Enable conn checks
            'BSE:CONFig:RRC:CTIMer:STATus ON',
            # Check freq (secs)
            'BSE:CONFig:RRC:CTIMer:LENGth 5',
            'SIGN:MODE BSE',
            'SCENArio:LOAD "FDD_Combined_v6.3.lbmf"',
            'BSE:CONF:PROF 20MH',
            'FREQ:BAND 13',
            'BSE:SIMULator RUN'
        ]
        self.c.SendStanza(commands)

    def Stop(self):
        self.c.SendStanza(['BSE:SIMULator STOP'])
        # Make sure the call status goes to idle before continuing.
        utils.poll_for_condition(
            self._IsIdle,
            timeout=cellular.DEFAULT_TIMEOUT,
            exception=cellular_system_error.BadState(
                'PXT did not enter IDLE state'))

    def SupportedTechnologies(self):
        return [cellular.Technology.LTE]

    def WaitForStatusChange(self,
                            interested=None,
                            timeout=cellular.DEFAULT_TIMEOUT):
        """When UE status changes (to a value in |interested|),
        return the value.

        Arguments:
            interested: if non-None, only transitions to these states will
              cause a return
            timeout: in seconds.
        Returns: state
        Raises: cellular_system_error.InstrumentTimeout
        """
        start = time.time()
        # TODO(byronk): consider utils.poll_for_condition()
        while time.time() - start <= timeout:
            state = self.GetUeDataStatus()
            if state in interested:
                return state
            time.sleep(POLL_SLEEP)

        state = self.GetUeDataStatus()
        if state in interested:
            return state

        raise cellular_system_error.InstrumentTimeout(
            'Timed out waiting for state in %s.  State was %s.' %
            (interested, state))


class ConfigStanzas(object):
    # p 22 of http://cp.literature.agilent.com/litweb/pdf/5989-5932EN.pdf

    def _Parse( command_sequence):
        """Split and remove comments from a config stanza."""
        return [line for line in command_sequence.splitlines()
                if line and not line.startswith('#')]

    LTE = _Parse(""" """)

    # TODO(byronk): ConfigStanza should not be. These belong somewhere in
    # the PXT class.
    WCDMA_MAX = _Parse("""
# RAB3: 64 Up/384 down
# http://wireless.agilent.com/rfcomms/refdocs/
#        wcdma/wcdmala_hpib_call_service.html#CACBDEAH
CALL:UPLink:TXPower:LEVel:MAXimum 24
CALL:SERVICE:GPRS:RAB GPRSRAB3
""")

    # p 20 of http://cp.literature.agilent.com/litweb/pdf/5989-5932EN.pdf
    CDMA_2000_MAX = _Parse("""
CALL:SCHannel:FORWard:DRATe BPS153600
CALL:CELL:SOPTion:RCONfig3 SOFS33
""")

    # p 19 of http://cp.literature.agilent.com/litweb/pdf/5989-5932EN.pdf
    EVDO_1X_MAX = _Parse("""
CALL:CELL:CONTrol:CATTribute:ISTate:PCCCycle ATSP
# Default data application
CALL:APPLication:SESSion DPAPlication
# Give DUT 100% of channel
CALL:CELL:APPLication:ATDPackets 100
""")

    GPRS_MAX = _Parse("""
call:bch:scel gprs
call:pdtch:mslot:config d1u1
call:cell:tbflow:t3192 ms1500
""")

    EGPRS_MAX = _Parse("""
call:bch:scel egprs
call:pdtch:mslot:config d4u1
call:cell:tbflow:t3192 ms1500
""")

    CAT_08 = _Parse("""
call:pow:stat ON
call:ms:pow:targ 0
call:cell:rlc:rees OFF
call:hsdpa:ms:hsdschannel:cat:control:auto off
call:hsdpa:ms:hsdschannel:cat:man 8
call:hsdpa:service:psdata:hsdschannel:config cqiv
call:hsdpa:service:psdata:cqi 22
call:serv:gprs:rab PHSP
call:serv:rbt:rab HSDP12
call:serv:psd:srb:mapp UEDD
call:hsup:serv:psd:edpd:ccod:max T2T4
call:hsup:edch:tti MS10
call:hsup:serv:psd:ergc:inf:stat Off
""")

    CAT_10 = _Parse("""
call:pow:stat ON
call:ms:pow:targ 0
call:cell:rlc:rees OFF
call:hsdpa:ms:hsdschannel:cat:control:auto off
call:hsdpa:ms:hsdschannel:cat:man 10
call:serv:gprs:rab PHSP
call:serv:rbt:rab HSDP12
call:hsdpa:service:psdata:hsdschannel:config cqiv
call:hsdpa:service:psdata:cqi 22
call:serv:psd:srb:mapp UEDD
call:hsup:serv:psd:edpd:ccod:max T2T4
call:hsup:edch:tti MS2
call:hsup:serv:psd:ergc:inf:stat Off
""")


class ConfigDictionaries(object):
    TECHNOLOGY_TO_FORMAT_RAW = {
        cellular.Technology.GPRS: 'GSM/GPRS',
        cellular.Technology.EGPRS: 'GSM/GPRS',

        cellular.Technology.WCDMA: 'WCDMA',
        cellular.Technology.HSDPA: 'WCDMA',
        cellular.Technology.HSUPA: 'WCDMA',
        cellular.Technology.HSDUPA: 'WCDMA',
        cellular.Technology.HSPA_PLUS: 'WCDMA',

        cellular.Technology.CDMA_2000: 'IS-2000/IS-95/AMPS',

        cellular.Technology.EVDO_1X: 'IS-856',

        cellular.Technology.LTE: 'LTE',
    }

    # Put each value in "" marks to quote it for GPIB
    TECHNOLOGY_TO_FORMAT = dict([
        (x, '"%s"' % y) for
        x, y in TECHNOLOGY_TO_FORMAT_RAW.iteritems()])

    TECHNOLOGY_TO_CONFIG_STANZA = {
        cellular.Technology.CDMA_2000: ConfigStanzas.CDMA_2000_MAX,
        cellular.Technology.EVDO_1X: ConfigStanzas.EVDO_1X_MAX,
        cellular.Technology.GPRS: ConfigStanzas.GPRS_MAX,
        cellular.Technology.EGPRS: ConfigStanzas.EGPRS_MAX,
        cellular.Technology.WCDMA: ConfigStanzas.WCDMA_MAX,
        cellular.Technology.HSDPA: ConfigStanzas.CAT_08,
        cellular.Technology.HSUPA: ConfigStanzas.CAT_08,
        cellular.Technology.HSDUPA: ConfigStanzas.CAT_08,
        cellular.Technology.HSPA_PLUS: ConfigStanzas.CAT_10,
        cellular.Technology.LTE: ConfigStanzas.LTE,
    }
    # TODO(byronk): remove these. Not used for LTE. Check for external deps
    # http://wireless.agilent.com/rfcomms/refdocs/
    #        gsmgprs/prog_synch_callstategprs.html#CHDDFBAJ
    # NB:  We have elided a few states of the GSM state machine here.
    CALL_STATUS_DATA_TO_STATUS_GSM_GPRS = {
        'IDLE': cellular.UeGsmDataStatus.IDLE,
        'ATTG': cellular.UeGsmDataStatus.ATTACHING,
        'DET': cellular.UeGsmDataStatus.DETACHING,
        'ATT': cellular.UeGsmDataStatus.ATTACHED,
        'STAR': cellular.UeGsmDataStatus.ATTACHING,
        'END': cellular.UeGsmDataStatus.PDP_DEACTIVATING,
        'TRAN': cellular.UeGsmDataStatus.PDP_ACTIVE,
        'PDPAG': cellular.UeGsmDataStatus.PDP_ACTIVATING,
        'PDP': cellular.UeGsmDataStatus.PDP_ACTIVE,
        'PDPD': cellular.UeGsmDataStatus.PDP_DEACTIVATING,
        'DCON': cellular.UeGsmDataStatus.PDP_ACTIVE,
        'SUSP': cellular.UeGsmDataStatus.IDLE,
    }

    # http://wireless.agilent.com/rfcomms/refdocs/
    #        wcdma/wcdma_gen_call_proc_status.html#CJADGAHG
    CALL_STATUS_DATA_TO_STATUS_WCDMA = {
        'IDLE': cellular.UeGsmDataStatus.IDLE,
        'ATTG': cellular.UeGsmDataStatus.ATTACHING,
        'DET': cellular.UeGsmDataStatus.DETACHING,
        'OFF': cellular.UeGsmDataStatus.NONE,
        'PDPAG': cellular.UeGsmDataStatus.PDP_ACTIVATING,
        'PDP': cellular.UeGsmDataStatus.PDP_ACTIVE,
        'PDPD': cellular.UeGsmDataStatus.PDP_DEACTIVATING,
    }

    # http://wireless.agilent.com/rfcomms/refdocs/
    #        cdma2k/cdma2000_hpib_call_status.html#CJABGBCF
    CALL_STATUS_DATA_TO_STATUS_CDMA_2000 = {
        'OFF': cellular.UeC2kDataStatus.OFF,
        'DORM': cellular.UeC2kDataStatus.DORMANT,
        'DCON': cellular.UeC2kDataStatus.DATA_CONNECTED,
    }

    # http://wireless.agilent.com/rfcomms/refdocs/
    #        1xevdo/1xevdo_hpib_call_status.html#BABCGBCD
    CALL_STATUS_DATA_TO_STATUS_EVDO = {
        'CCL': cellular.UeEvdoDataStatus.CONNECTION_CLOSING,
        'CNEG': cellular.UeEvdoDataStatus.CONNECTION_NEGOTIATE,
        'CREQ': cellular.UeEvdoDataStatus.CONNECTION_REQUEST,
        'DCON': cellular.UeEvdoDataStatus.DATA_CONNECTED,
        'DORM': cellular.UeEvdoDataStatus.DORMANT,
        'HAND': cellular.UeEvdoDataStatus.HANDOFF,
        'IDLE': cellular.UeEvdoDataStatus.IDLE,
        'PAG': cellular.UeEvdoDataStatus.PAGING,
        'SCL': cellular.UeEvdoDataStatus.SESSION_CLOSING,
        'SNEG': cellular.UeEvdoDataStatus.SESSION_NEGOTIATE,
        'SOP': cellular.UeEvdoDataStatus.SESSION_OPEN,
        'UREQ': cellular.UeEvdoDataStatus.UATI_REQUEST,
    }

    #lte status from BSE:STATus:ACELL? on the PXT
    #OFF | IDLE | CON | REG |
    #LOOP | REL | UNAV

    CALL_STATUS_DATA_TO_STATUS_LTE = {
        'OFF': cellular.UeLteDataStatus.OFF,
        'IDLE': cellular.UeLteDataStatus.IDLE,
        'CON': cellular.UeLteDataStatus.CONNECTED,
        'REG': cellular.UeLteDataStatus.REGISTERED,
        'LOOP': cellular.UeLteDataStatus.LOOPBACK,
        'REL': cellular.UeLteDataStatus.RELEASE,
        'UNAV': cellular.UeLteDataStatus.UNAVAILABLE,
    }
    FORMAT_TO_DATA_STATUS_TYPE = {
        '"GSM/GPRS"': CALL_STATUS_DATA_TO_STATUS_GSM_GPRS,
        '"WCDMA"': CALL_STATUS_DATA_TO_STATUS_WCDMA,
        '"IS-2000/IS-95/AMPS"': CALL_STATUS_DATA_TO_STATUS_CDMA_2000,
        '"IS-856"': CALL_STATUS_DATA_TO_STATUS_EVDO,
        '"LTE"': CALL_STATUS_DATA_TO_STATUS_LTE,
    }
