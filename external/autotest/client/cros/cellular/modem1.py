#!/usr/bin/python
# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""Implement a modem proxy to talk to a ModemManager1 modem."""

from autotest_lib.client.common_lib import error
from autotest_lib.client.cros.cellular import cellular
from autotest_lib.client.cros.cellular import mm1
from autotest_lib.client.cros.cellular import mm1_constants
import dbus
import cellular_logging

log = cellular_logging.SetupCellularLogging('modem1')

MODEM_TIMEOUT = 60


class Modem(object):
    """An object which talks to a ModemManager1 modem."""
    # MM_MODEM_GSM_ACCESS_TECH (not exported)
    # From /usr/include/mm/mm-modem.h
    _MM_MODEM_GSM_ACCESS_TECH_UNKNOWN = 0
    _MM_MODEM_GSM_ACCESS_TECH_GSM = 1 << 1
    _MM_MODEM_GSM_ACCESS_TECH_GSM_COMPACT = 1 << 2
    _MM_MODEM_GSM_ACCESS_TECH_GPRS = 1 << 3
    _MM_MODEM_GSM_ACCESS_TECH_EDGE = 1 << 4
    _MM_MODEM_GSM_ACCESS_TECH_UMTS = 1 << 5
    _MM_MODEM_GSM_ACCESS_TECH_HSDPA = 1 << 6
    _MM_MODEM_GSM_ACCESS_TECH_HSUPA = 1 << 7
    _MM_MODEM_GSM_ACCESS_TECH_HSPA = 1 << 8

    # Mapping of modem technologies to cellular technologies
    _ACCESS_TECH_TO_TECHNOLOGY = {
        _MM_MODEM_GSM_ACCESS_TECH_GSM: cellular.Technology.WCDMA,
        _MM_MODEM_GSM_ACCESS_TECH_GSM_COMPACT: cellular.Technology.WCDMA,
        _MM_MODEM_GSM_ACCESS_TECH_GPRS: cellular.Technology.GPRS,
        _MM_MODEM_GSM_ACCESS_TECH_EDGE: cellular.Technology.EGPRS,
        _MM_MODEM_GSM_ACCESS_TECH_UMTS: cellular.Technology.WCDMA,
        _MM_MODEM_GSM_ACCESS_TECH_HSDPA: cellular.Technology.HSDPA,
        _MM_MODEM_GSM_ACCESS_TECH_HSUPA: cellular.Technology.HSUPA,
        _MM_MODEM_GSM_ACCESS_TECH_HSPA: cellular.Technology.HSDUPA,
    }

    def __init__(self, manager, path):
        self.manager = manager
        self.bus = manager.bus
        self.service = manager.service
        self.path = path

    def Modem(self):
        obj = self.bus.get_object(self.service, self.path)
        return dbus.Interface(obj, mm1.MODEM_INTERFACE)

    def SimpleModem(self):
        obj = self.bus.get_object(self.service, self.path)
        return dbus.Interface(obj, mm1.MODEM_SIMPLE_INTERFACE)

    def GsmModem(self):
        obj = self.bus.get_object(self.service, self.path)
        return dbus.Interface(obj, mm1.MODEM_MODEM3GPP_INTERFACE)

    def CdmaModem(self):
        obj = self.bus.get_object(self.service, self.path)
        return dbus.Interface(obj, mm1.MODEM_MODEMCDMA_INTERFACE)

    def Sim(self):
        obj = self.bus.get_object(self.service, self.path)
        return dbus.Interface(obj, mm1.SIM_INTERFACE)

    def PropertiesInterface(self):
        obj = self.bus.get_object(self.service, self.path)
        return dbus.Interface(obj, dbus.PROPERTIES_IFACE)

    def GetAll(self, iface):
        obj_iface = self.PropertiesInterface()
        return obj_iface.GetAll(iface)

    def _GetModemInterfaces(self):
        return [
            mm1.MODEM_INTERFACE,
            mm1.MODEM_SIMPLE_INTERFACE,
            mm1.MODEM_MODEM3GPP_INTERFACE,
            mm1.MODEM_MODEMCDMA_INTERFACE
            ]

    @staticmethod
    def _CopyPropertiesCheckUnique(src, dest):
        """Copies properties from |src| to |dest| and makes sure there are no
           duplicate properties that have different values."""
        for key, value in src.iteritems():
            if key in dest and value != dest[key]:
                raise KeyError('Duplicate property %s, different values '
                               '("%s", "%s")' % (key, value, dest[key]))
            dest[key] = value

    def GetModemProperties(self):
        """Returns all DBus Properties of all the modem interfaces."""
        props = dict()
        for iface in self._GetModemInterfaces():
            try:
                iface_props = self.GetAll(iface)
            except dbus.exceptions.DBusException:
                continue
            if iface_props:
                self._CopyPropertiesCheckUnique(iface_props, props)

        try:
            sim_obj = self.bus.get_object(self.service, props['Sim'])
            sim_props_iface = dbus.Interface(sim_obj, dbus.PROPERTIES_IFACE)
            sim_props = sim_props_iface.GetAll(mm1.SIM_INTERFACE)

            # SIM cards may store an empty operator name or store a value
            # different from the one obtained OTA. Rename the 'OperatorName'
            # property obtained from the SIM card to 'SimOperatorName' in
            # order to avoid a potential conflict with the 'OperatorName'
            # property obtained from the Modem3gpp interface.
            if 'OperatorName' in sim_props:
                sim_props['SimOperatorName'] = sim_props.pop('OperatorName')

            self._CopyPropertiesCheckUnique(sim_props, props)
        except dbus.exceptions.DBusException:
            pass

        return props

    def GetAccessTechnology(self):
        """Returns the modem access technology."""
        props = self.GetModemProperties()
        tech = props['AccessTechnologies']
        return Modem._ACCESS_TECH_TO_TECHNOLOGY[tech]

    def GetCurrentTechnologyFamily(self):
        """Returns the modem technology family."""
        props = self.GetAll(mm1.MODEM_INTERFACE)
        capabilities = props.get('SupportedCapabilities')
        if self._IsCDMAModem(capabilities):
            return cellular.TechnologyFamily.CDMA
        if self._Is3GPPModem(capabilities):
            return cellular.TechnologyFamily.UMTS
        raise error.TestError('Invalid modem type')

    def GetVersion(self):
        """Returns the modem version information."""
        return self.GetModemProperties()['Revision']

    def _IsCDMAModem(self, capabilities):
        return mm1_constants.MM_MODEM_CAPABILITY_CDMA_EVDO in capabilities

    def _Is3GPPModem(self, capabilities):
        for capability in capabilities:
            if (capability &
                    (mm1_constants.MM_MODEM_CAPABILITY_LTE |
                     mm1_constants.MM_MODEM_CAPABILITY_LTE_ADVANCED |
                     mm1_constants.MM_MODEM_CAPABILITY_GSM_UMTS)):
                return True
        return False

    def _CDMAModemIsRegistered(self):
        modem_status = self.SimpleModem().GetStatus()
        cdma1x_state = modem_status.get(
                'cdma-cdma1x-registration-state',
                mm1_constants.MM_MODEM_CDMA_REGISTRATION_STATE_UNKNOWN)
        evdo_state = modem_status.get(
                'cdma-evdo-registration-state',
                mm1_constants.MM_MODEM_CDMA_REGISTRATION_STATE_UNKNOWN)
        return (cdma1x_state !=
                mm1_constants.MM_MODEM_CDMA_REGISTRATION_STATE_UNKNOWN or
                evdo_state !=
                mm1_constants.MM_MODEM_CDMA_REGISTRATION_STATE_UNKNOWN)

    def _3GPPModemIsRegistered(self):
        modem_status = self.SimpleModem().GetStatus()
        state = modem_status.get('m3gpp-registration-state')
        return (state == mm1_constants.MM_MODEM_3GPP_REGISTRATION_STATE_HOME or
                state == mm1_constants.MM_MODEM_3GPP_REGISTRATION_STATE_ROAMING)

    def ModemIsRegistered(self):
        """Ensure that modem is registered on the network."""
        props = self.GetAll(mm1.MODEM_INTERFACE)
        capabilities = props.get('SupportedCapabilities')
        if self._IsCDMAModem(capabilities):
            return self._CDMAModemIsRegistered()
        elif self._Is3GPPModem(capabilities):
            return self._3GPPModemIsRegistered()
        else:
            raise error.TestError('Invalid modem type')

    def ModemIsRegisteredUsing(self, technology):
        """Ensure that modem is registered on the network with a technology."""
        if not self.ModemIsRegistered():
            return False

        reported_tech = self.GetAccessTechnology()

        # TODO(jglasgow): Remove this mapping.  Basestation and
        # reported technology should be identical.
        BASESTATION_TO_REPORTED_TECHNOLOGY = {
            cellular.Technology.GPRS: cellular.Technology.GPRS,
            cellular.Technology.EGPRS: cellular.Technology.GPRS,
            cellular.Technology.WCDMA: cellular.Technology.HSDUPA,
            cellular.Technology.HSDPA: cellular.Technology.HSDUPA,
            cellular.Technology.HSUPA: cellular.Technology.HSDUPA,
            cellular.Technology.HSDUPA: cellular.Technology.HSDUPA,
            cellular.Technology.HSPA_PLUS: cellular.Technology.HSPA_PLUS
        }

        return BASESTATION_TO_REPORTED_TECHNOLOGY[technology] == reported_tech

    def IsConnectingOrDisconnecting(self):
        props = self.GetAll(mm1.MODEM_INTERFACE)
        return props['State'] in [
            mm1.MM_MODEM_STATE_CONNECTING,
            mm1.MM_MODEM_STATE_DISCONNECTING
        ]

    def IsEnabled(self):
        props = self.GetAll(mm1.MODEM_INTERFACE)
        return props['State'] in [
            mm1.MM_MODEM_STATE_ENABLED,
            mm1.MM_MODEM_STATE_SEARCHING,
            mm1.MM_MODEM_STATE_REGISTERED,
            mm1.MM_MODEM_STATE_DISCONNECTING,
            mm1.MM_MODEM_STATE_CONNECTING,
            mm1.MM_MODEM_STATE_CONNECTED
        ]

    def IsDisabled(self):
        props = self.GetAll(mm1.MODEM_INTERFACE)
        return props['State'] == mm1.MM_MODEM_STATE_DISABLED

    def Enable(self, enable, **kwargs):
        self.Modem().Enable(enable, timeout=MODEM_TIMEOUT, **kwargs)

    def Connect(self, props):
        self.SimpleModem().Connect(props, timeout=MODEM_TIMEOUT)

    def Disconnect(self):
        self.SimpleModem().Disconnect('/', timeout=MODEM_TIMEOUT)


class ModemManager(object):
    """An object which talks to a ModemManager1 service."""

    def __init__(self):
        self.bus = dbus.SystemBus()
        self.service = mm1.MODEM_MANAGER_INTERFACE
        self.path = mm1.OMM
        self.manager = dbus.Interface(
            self.bus.get_object(self.service, self.path),
            mm1.MODEM_MANAGER_INTERFACE)
        self.objectmanager = dbus.Interface(
            self.bus.get_object(self.service, self.path), mm1.OFDOM)

    def EnumerateDevices(self):
        devices = self.objectmanager.GetManagedObjects()
        return devices.keys()

    def GetModem(self, path):
        return Modem(self, path)

    def SetDebugLogging(self):
        self.manager.SetLogging('DEBUG')
