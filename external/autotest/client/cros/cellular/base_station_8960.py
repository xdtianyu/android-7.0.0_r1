# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import re
import time
import common
from autotest_lib.client.cros.cellular import cellular_logging
from autotest_lib.client.cros.cellular import cellular_system_error
from autotest_lib.client.cros.cellular import air_state_verifier
from autotest_lib.client.cros.cellular import base_station_interface
from autotest_lib.client.cros.cellular import cellular
from autotest_lib.client.bin import utils

POLL_SLEEP = 0.2

log = cellular_logging.SetupCellularLogging('base_station_8960')

class BaseStation8960(base_station_interface.BaseStationInterface):
    """Wrap an Agilent 8960 Series 10."""

    def __init__(self, scpi_connection, no_initialization=False):
        """
        Creates an 8960 call-box object.
        TODO (byrok): make a factory that returns a call_box, of either
        a 8960 or a PXT, or a...

        @param scpi_connection:  The scpi port to send commands over
        @param no_initialization: Don't do anything. Useful for unit testing
        and debugging when you don't want to run all the usual functions.
        """
        self.c = scpi_connection
        if no_initialization:
            return
        self.checker_context = self.c.checker_context
        with self.checker_context:
            self._Verify()
            self._Reset()
            self.SetPower(cellular.Power.DEFAULT)

    def _Verify(self):
        idn = self.c.Query('*IDN?')
        if '8960 Series 10 E5515C' not in idn:
            raise cellular_system_error.BadState(
                'Not actually an 8960:  *IDN? says ' + idn)

    def _Reset(self):
        self.c.Reset()
        self.Stop()
        # Perform a partial reset to workaround a problem with the 8960
        # failing to accept CDMA connections after switching from a
        # GSM technology.
        self.c.SendStanza(['SYSTEM:PRESet3'])

    def _IsIdle(self):
        call_state = self.c.Query('CALL:STATus?')
        data_state = self.c.Query('CALL:STATus:DATa?')
        return call_state == 'IDLE' and data_state in ['IDLE', 'OFF']

    def Close(self):
        self.c.Close()

    def GetAirStateVerifier(self):
        return air_state_verifier.AirStateVerifierBasestation(self)

    def GetDataCounters(self):
        output = {}
        for counter in ['OTATx', 'OTARx', 'IPTX', 'IPRX']:
            result_text = self.c.Query('CALL:COUNT:DTMonitor:%s:DRATe?' %
                                       counter)
            result = [float(x) for x in result_text.rstrip().split(',')]
            output[counter] = dict(zip(['Mean', 'Current', 'Max', 'Total'],
                                       result))
        logging.info('Data counters: %s', output)
        return output

    def GetRatUeDataStatus(self):
        """Get the radio-access-technology-specific status of the UE.

        Unlike GetUeDataStatus, below, this returns a status that depends
        on the RAT being used.
        """
        status = self.c.Query('CALL:STATus:DATa?')
        rat = \
            ConfigDictionaries.FORMAT_TO_DATA_STATUS_TYPE[self.format][status]
        return rat

    def GetUeDataStatus(self):
        """Get the UeGenericDataStatus status of the device."""
        rat = self.GetRatUeDataStatus()
        return cellular.RatToGenericDataStatus[rat]

    def ResetDataCounters(self):
        self.c.SendStanza(['CALL:COUNt:DTMonitor:CLEar'])

    def ClearErrors(self):
        self.c.RetrieveErrors()

    def LogStats(self):
        self.c.Query("CALL:HSDPa:SERVice:PSData:HSDSchannel:CONFig?")

        # Category reported by UE
        self.c.Query("CALL:HSDPa:MS:REPorted:HSDSChannel:CATegory?")
        # The category in use
        self.c.Query("CALL:STATUS:MS:HSDSChannel:CATegory?")
        self.c.Query("CALL:HSDPA:SERV:PSD:CQI?")

    def SetBsIpV4(self, ip1, ip2):
        self.c.SendStanza([
            'SYSTem:COMMunicate:LAN:SELF:ADDRess:IP4 "%s"' % ip1,
            'SYSTem:COMMunicate:LAN:SELF:ADDRess2:IP4 "%s"' % ip2,])

    def SetBsNetmaskV4(self, netmask):
        self.c.SendStanza([
            'SYSTem:COMMunicate:LAN:SELF:SMASk:IP4 "%s"' % netmask,])

    def SetPlmn(self, mcc, mnc):
        # Doing this appears to set the WCDMa versions as well
        self.c.SendStanza([
            'CALL:MCCode %s' % mcc,
            'CALL:MNCode %s' % mnc,])

    def SetPower(self, dbm):
        if dbm <= cellular.Power.OFF :
            self.c.SendStanza([
                'CALL:CELL:POWer:STATe off',])
        else:
            self.c.SendStanza([
                'CALL:CELL:POWer %s' % dbm,])

    def SetTechnology(self, technology):
        # TODO(rochberg): Check that we're not already in chosen tech for
        # speed boost

        # Print out a helpful message on a key error.
        try:
            self.format = ConfigDictionaries.TECHNOLOGY_TO_FORMAT[technology]
        except KeyError:
            raise KeyError('%s not in %s ' %
                           (technology,
                            ConfigDictionaries.TECHNOLOGY_TO_FORMAT))
        self.technology = technology

        self.c.SimpleVerify('SYSTem:APPLication:FORMat', self.format)
        # Setting the format will start the call box, we need to stop it so we
        # can configure the new format.
        self.Stop()
        self.c.SendStanza(
            ConfigDictionaries.TECHNOLOGY_TO_CONFIG_STANZA.get(technology, []))

    def SetUeDnsV4(self, dns1, dns2):
        """Set the DNS values provided to the UE.  Emulator must be stopped."""
        stanza = ['CALL:MS:DNSServer:PRIMary:IP:ADDRess "%s"' % dns1]
        if dns2:
            stanza.append('CALL:MS:DNSServer:SECondary:IP:ADDRess "%s"' % dns2)
        self.c.SendStanza(stanza)

    def SetUeIpV4(self, ip1, ip2=None):
        """
        Set the IP addresses provided to the UE.  Emulator must be stopped.
        """
        stanza = ['CALL:MS:IP:ADDRess1 "%s"' % ip1]
        if ip2:
            stanza.append('CALL:MS:IP:ADDRess2 "%s"' % ip2)
        self.c.SendStanza(stanza)

    def Start(self):
        self.c.SendStanza(['CALL:OPERating:MODE CALL'])

    def Stop(self):
        self.c.SendStanza(['CALL:OPERating:MODE OFF'])
        # Make sure the call status goes to idle before continuing.
        utils.poll_for_condition(
            self._IsIdle,
            timeout=cellular.DEFAULT_TIMEOUT,
            exception=cellular_system_error.BadState(
                '8960 did not enter IDLE state'))

    def SupportedTechnologies(self):
        return [
            cellular.Technology.GPRS,
            cellular.Technology.EGPRS,
            cellular.Technology.WCDMA,
            cellular.Technology.HSDPA,
            cellular.Technology.HSUPA,
            cellular.Technology.HSDUPA,
            cellular.Technology.HSPA_PLUS,
            cellular.Technology.CDMA_2000,
            cellular.Technology.EVDO_1X,
        ]

    def WaitForStatusChange(self,
                            interested=None,
                            timeout=cellular.DEFAULT_TIMEOUT):
        """When UE status changes (to a value in interested), return the value.

        Arguments:
            interested: if non-None, only transitions to these states will
              cause a return
            timeout: in seconds.
        Returns: state
        Raises:  cellular_system_error.InstrumentTimeout
        """
        start = time.time()
        while time.time() - start <= timeout:
            state = self.GetUeDataStatus()
            if state in interested:
                return state
            time.sleep(POLL_SLEEP)

        state = self.GetUeDataStatus()
        if state in interested:
            return state

        raise cellular_system_error.InstrumentTimeout(
            'Timed out waiting for state in %s.  State was %s' %
                      (interested, state))

def _Parse(command_sequence):
    """Split and remove comments from a config stanza."""
    uncommented = [re.sub(r'\s*#.*', '', line)
                   for line in command_sequence.split('\n')]

    # Return only nonempty lines
    return [line for line in uncommented if line]


class ConfigStanzas(object):
    # p 22 of http://cp.literature.agilent.com/litweb/pdf/5989-5932EN.pdf
    WCDMA_MAX = _Parse("""
# RAB3: 64 Up/384 down
# http://wireless.agilent.com/rfcomms/refdocs/wcdma/wcdmala_hpib_call_service.html#CACBDEAH
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
    }

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

    FORMAT_TO_DATA_STATUS_TYPE = {
        '"GSM/GPRS"': CALL_STATUS_DATA_TO_STATUS_GSM_GPRS,
        '"WCDMA"': CALL_STATUS_DATA_TO_STATUS_WCDMA,
        '"IS-2000/IS-95/AMPS"': CALL_STATUS_DATA_TO_STATUS_CDMA_2000,
        '"IS-856"': CALL_STATUS_DATA_TO_STATUS_EVDO,
    }
