# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import dbus

import dbus_std_ifaces
import utils

import common
from autotest_lib.client.cros.cellular import mm1_constants
from autotest_lib.client.cros.cellular import net_interface

class Bearer(dbus_std_ifaces.DBusProperties):
    """
    Fake implementation of the org.freedesktop.ModemManager1.Bearer
    interface. Bearer objects are owned and managed by specific Modem objects.
    A single Modem may expose one or more Bearer objects, which can then be
    used to get the modem into connected state.

    """

    count = 0

    def __init__(self, bus, properties, config=None):
        self._active = False
        self._bearer_props = properties
        path = '%s/Bearer/%d' % (mm1_constants.MM1, Bearer.count)
        Bearer.count += 1
        dbus_std_ifaces.DBusProperties.__init__(self, path, bus, config)


    def _InitializeProperties(self):
        props = {
            'Interface': net_interface.PseudoNetInterface.IFACE_NAME,
            'Connected': dbus.types.Boolean(False),
            'Suspended': dbus.types.Boolean(False),
            'Properties': self._bearer_props
        }
        return { mm1_constants.I_BEARER: props }


    def _AddProperty(self, property_key):
        self._properties[mm1_constants.I_BEARER][property_key] = None


    def _RemoveProperty(self, property_key):
        try:
            self._properties[mm1_constants.I_BEARER].pop(property_key)
        except KeyError:
            pass


    def IsActive(self):
        """
        @returns: True, if the bearer is currently active.

        """
        return self._active


    @property
    def bearer_properties(self):
        """
        @returns: The current bearer properties that were set during a call to
                org.freedesktop.ModemManager1.Modem.Simple.Connect.

        """
        return self._bearer_props


    @utils.log_dbus_method()
    @dbus.service.method(mm1_constants.I_BEARER)
    def Connect(self):
        """
        Requests activation of a packet data connection with the network using
        this bearer's properties. Upon successful activation, the modem can
        send and receive packet data and, depending on the addressing
        capability of the modem, a connection manager may need to start PPP,
        perform DHCP, or assign the IP address returned by the modem to the
        data interface. Upon successful return, the "Ip4Config" and/or
        "Ip6Config" properties become valid and may contain IP configuration
        information for the data interface associated with this bearer.

        Since this is a mock implementation, this bearer will not establish
        a real connection with the outside world. Since shill does not specify
        IP addressing information to the bearer, we do not need to populate
        these properties.

        """
        # Set the ip config property
        ip_family = self._bearer_props.get('ip-type', None)
        if ip_family and ip_family >= mm1_constants.MM_BEARER_IP_FAMILY_IPV6:
            config_prop = 'Ip6Config'
        else:
            config_prop = 'Ip4Config'

        self._AddProperty('Ip4Config')
        self.Set(mm1_constants.I_BEARER, config_prop, {
            'method': dbus.types.UInt32(mm1_constants.MM_BEARER_IP_METHOD_DHCP,
                                        variant_level=1)
        })
        self._active = True
        self.Set(mm1_constants.I_BEARER, 'Connected', dbus.types.Boolean(True))


    @utils.log_dbus_method()
    @dbus.service.method(mm1_constants.I_BEARER)
    def Disconnect(self):
        """
        Disconnect and deactivate this packet data connection. In a real bearer,
        any ongoing data session would be terminated and IP addresses would
        become invalid when this method is called, however, the fake
        implementation doesn't set the IP properties.

        """
        self._RemoveProperty('Ip4Config')
        self._RemoveProperty('Ip6Config')
        self._active = False
        self.Set(mm1_constants.I_BEARER, 'Connected',
                 dbus.types.Boolean(False))
