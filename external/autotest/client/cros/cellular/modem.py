#!/usr/bin/python
# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os

from autotest_lib.client.cros.cellular import cellular
import dbus

MODEM_TIMEOUT=60

class Modem(object):
    """An object which talks to a ModemManager modem."""
    MODEM_INTERFACE = 'org.freedesktop.ModemManager.Modem'
    SIMPLE_MODEM_INTERFACE = 'org.freedesktop.ModemManager.Modem.Simple'
    CDMA_MODEM_INTERFACE = 'org.freedesktop.ModemManager.Modem.Cdma'
    GSM_MODEM_INTERFACE = 'org.freedesktop.ModemManager.Modem.Gsm'
    GOBI_MODEM_INTERFACE = 'org.chromium.ModemManager.Modem.Gobi'
    GSM_CARD_INTERFACE = 'org.freedesktop.ModemManager.Modem.Gsm.Card'
    GSM_SMS_INTERFACE = 'org.freedesktop.ModemManager.Modem.Gsm.SMS'
    GSM_NETWORK_INTERFACE = 'org.freedesktop.ModemManager.Modem.Gsm.Network'
    PROPERTIES_INTERFACE = 'org.freedesktop.DBus.Properties'

    GSM_MODEM = 1
    CDMA_MODEM = 2

    NETWORK_PREFERENCE_AUTOMATIC = 0
    NETWORK_PREFERENCE_CDMA_2000 = 1
    NETWORK_PREFERENCE_EVDO_1X = 2
    NETWORK_PREFERENCE_GSM = 3
    NETWORK_PREFERENCE_WCDMA = 4

    # MM_MODEM_GSM_ACCESS_TECH (not exported)
    # From /usr/include/mm/mm-modem.h
    _MM_MODEM_GSM_ACCESS_TECH_UNKNOWN = 0
    _MM_MODEM_GSM_ACCESS_TECH_GSM = 1
    _MM_MODEM_GSM_ACCESS_TECH_GSM_COMPACT = 2
    _MM_MODEM_GSM_ACCESS_TECH_GPRS = 3
    _MM_MODEM_GSM_ACCESS_TECH_EDGE = 4
    _MM_MODEM_GSM_ACCESS_TECH_UMTS = 5
    _MM_MODEM_GSM_ACCESS_TECH_HSDPA = 6
    _MM_MODEM_GSM_ACCESS_TECH_HSUPA = 7
    _MM_MODEM_GSM_ACCESS_TECH_HSPA = 8

    # MM_MODEM_STATE (not exported)
    # From /usr/include/mm/mm-modem.h
    _MM_MODEM_STATE_UNKNOWN = 0
    _MM_MODEM_STATE_DISABLED = 10
    _MM_MODEM_STATE_DISABLING = 20
    _MM_MODEM_STATE_ENABLING = 30
    _MM_MODEM_STATE_ENABLED = 40
    _MM_MODEM_STATE_SEARCHING = 50
    _MM_MODEM_STATE_REGISTERED = 60
    _MM_MODEM_STATE_DISCONNECTING = 70
    _MM_MODEM_STATE_CONNECTING = 80
    _MM_MODEM_STATE_CONNECTED = 90

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
        return dbus.Interface(obj, Modem.MODEM_INTERFACE)

    def SimpleModem(self):
        obj = self.bus.get_object(self.service, self.path)
        return dbus.Interface(obj, Modem.SIMPLE_MODEM_INTERFACE)

    def CdmaModem(self):
        obj = self.bus.get_object(self.service, self.path)
        return dbus.Interface(obj, Modem.CDMA_MODEM_INTERFACE)

    def GobiModem(self):
        obj = self.bus.get_object(self.service, self.path)
        return dbus.Interface(obj, Modem.GOBI_MODEM_INTERFACE)

    def GsmModem(self):
        obj = self.bus.get_object(self.service, self.path)
        return dbus.Interface(obj, Modem.GSM_MODEM_INTERFACE)

    def GsmCard(self):
        obj = self.bus.get_object(self.service, self.path)
        return dbus.Interface(obj, Modem.GSM_CARD_INTERFACE)

    def GsmSms(self):
        obj = self.bus.get_object(self.service, self.path)
        return dbus.Interface(obj, Modem.GSM_SMS_INTERFACE)

    def GsmNetwork(self):
        obj = self.bus.get_object(self.service, self.path)
        return dbus.Interface(obj, Modem.GSM_NETWORK_INTERFACE)

    def GetAll(self, iface):
        obj = self.bus.get_object(self.service, self.path)
        obj_iface = dbus.Interface(obj, Modem.PROPERTIES_INTERFACE)
        return obj_iface.GetAll(iface)

    def _GetModemInterfaces(self):
        return [
            Modem.MODEM_INTERFACE,
            Modem.SIMPLE_MODEM_INTERFACE,
            Modem.CDMA_MODEM_INTERFACE,
            Modem.GSM_MODEM_INTERFACE,
            Modem.GSM_NETWORK_INTERFACE,
            Modem.GOBI_MODEM_INTERFACE]


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

        status = self.SimpleModem().GetStatus()
        if 'meid' in status:
            props['Meid'] = status['meid']
        if 'imei' in status:
            props['Imei'] = status['imei']
        if 'imsi' in status:
            props['Imsi'] = status['imsi']
        if 'esn' in status:
            props['Esn'] = status['esn']

        # Operator information is not exposed through the properties interface.
        # Try to get it directly. This may fail on a disabled modem.
        try:
            network = self.GsmNetwork()
            _, operator_code, operator_name = network.GetRegistrationInfo()
            if operator_code:
                props['OperatorCode'] = operator_code
            if operator_name:
                props['OperatorName'] = operator_name
        except dbus.DBusException:
            pass

        return props

    def GetAccessTechnology(self):
        """Returns the modem access technology."""
        props = self.GetModemProperties()
        tech = props.get('AccessTechnology')
        return Modem._ACCESS_TECH_TO_TECHNOLOGY[tech]

    def GetCurrentTechnologyFamily(self):
        """Returns the modem technology family."""
        try:
            self.GetAll(Modem.GSM_CARD_INTERFACE)
            return cellular.TechnologyFamily.UMTS
        except dbus.exceptions.DBusException:
            return cellular.TechnologyFamily.CDMA

    def GetVersion(self):
        """Returns the modem version information."""
        return self.Modem().GetInfo()[2]

    def _GetRegistrationState(self):
        try:
            network = self.GsmNetwork()
            (status, unused_code, unused_name) = network.GetRegistrationInfo()
            # TODO(jglasgow): HOME - 1, ROAMING - 5
            return status == 1 or status == 5
        except dbus.exceptions.DBusException:
            pass

        cdma_modem = self.CdmaModem()
        try:
            cdma, evdo = cdma_modem.GetRegistrationState()
            return cdma > 0 or evdo > 0
        except dbus.exceptions.DBusException:
            pass

        return False

    def ModemIsRegistered(self):
        """Ensure that modem is registered on the network."""
        return self._GetRegistrationState()

    def ModemIsRegisteredUsing(self, technology):
        """Ensure that modem is registered on the network with a technology."""
        if not self.ModemIsRegistered():
            return False

        reported_tech = self.GetAccessTechnology()

        # TODO(jglasgow): Remove this mapping.  Basestation and
        # reported technology should be identical.
        BASESTATION_TO_REPORTED_TECHNOLOGY = {
            cellular.Technology.GPRS: cellular.Technology.GPRS,
            cellular.Technology.EGPRS: cellular.Technology.EGPRS,
            cellular.Technology.WCDMA: cellular.Technology.HSDUPA,
            cellular.Technology.HSDPA: cellular.Technology.HSDUPA,
            cellular.Technology.HSUPA: cellular.Technology.HSDUPA,
            cellular.Technology.HSDUPA: cellular.Technology.HSDUPA,
            cellular.Technology.HSPA_PLUS: cellular.Technology.HSPA_PLUS
        }

        return BASESTATION_TO_REPORTED_TECHNOLOGY[technology] == reported_tech

    def IsConnectingOrDisconnecting(self):
        props = self.GetAll(Modem.MODEM_INTERFACE)
        return props['State'] in [
            Modem._MM_MODEM_STATE_CONNECTING,
            Modem._MM_MODEM_STATE_DISCONNECTING
        ]

    def IsEnabled(self):
        props = self.GetAll(Modem.MODEM_INTERFACE)
        return props['Enabled']

    def IsDisabled(self):
        return not self.IsEnabled()

    def Enable(self, enable, **kwargs):
        self.Modem().Enable(enable, timeout=MODEM_TIMEOUT, **kwargs)

    def Connect(self, props):
        self.SimpleModem().Connect(props, timeout=MODEM_TIMEOUT)

    def Disconnect(self):
        self.Modem().Disconnect(timeout=MODEM_TIMEOUT)


class ModemManager(object):
    """An object which talks to a ModemManager service."""
    INTERFACE = 'org.freedesktop.ModemManager'

    def __init__(self, provider=None):
        self.bus = dbus.SystemBus()
        self.provider = provider or os.getenv('MMPROVIDER') or 'org.chromium'
        self.service = '%s.ModemManager' % self.provider
        self.path = '/%s/ModemManager' % (self.provider.replace('.', '/'))
        self.manager = dbus.Interface(
            self.bus.get_object(self.service, self.path),
            ModemManager.INTERFACE)

    def EnumerateDevices(self):
        return self.manager.EnumerateDevices()

    def GetModem(self, path):
        return Modem(self, path)

    def SetDebugLogging(self):
        self.manager.SetLogging('debug')
