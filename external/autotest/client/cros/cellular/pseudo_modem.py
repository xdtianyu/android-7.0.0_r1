#!/usr/bin/env python

# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""An implementation of the ModemManager1 DBUS interface.

This modem mimics a GSM (eventually LTE & CDMA) modem and allows a
user to test shill and UI behaviors when a supported SIM is inserted
into the device.  Invoked with the proper flags it can test that SMS
messages are deliver to the UI.

This program creates a virtual network interface to simulate the
network interface of a modem.  It depends on modemmanager-next to
set the dbus permissions properly.

TODO:
   * Use more appropriate values for many of the properties
   * Support all ModemManager1 interfaces
   * implement LTE modems
   * implement CDMA modems
"""

from optparse import OptionParser
import logging
import os
import signal
import string
import subprocess
import sys
import time

import dbus
from dbus.exceptions import DBusException
import dbus.mainloop.glib
import dbus.service
from dbus.types import Int32
from dbus.types import ObjectPath
from dbus.types import Struct
from dbus.types import UInt32
import glib
import gobject
import mm1


# Miscellaneous delays to simulate a modem
DEFAULT_CONNECT_DELAY_MS = 1500

DEFAULT_CARRIER = 'att'


class DBusObjectWithProperties(dbus.service.Object):
    """Implements the org.freedesktop.DBus.Properties interface.

    Implements the org.freedesktop.DBus.Properties interface, specifically
    the Get and GetAll methods.  Class which inherit from this class must
    implement the InterfacesAndProperties function which will return a
    dictionary of all interfaces and the properties defined on those interfaces.
    """

    def __init__(self, bus, path):
        dbus.service.Object.__init__(self, bus, path)

    @dbus.service.method(dbus.PROPERTIES_IFACE,
                         in_signature='ss', out_signature='v')
    def Get(self, interface, property_name, *args, **kwargs):
        """Returns: The value of property_name on interface."""
        logging.info('%s: Get %s, %s', self.path, interface, property_name)
        interfaces = self.InterfacesAndProperties()
        properties = interfaces.get(interface, None)
        if property_name in properties:
            return properties[property_name]
        raise dbus.exceptions.DBusException(
            mm1.MODEM_MANAGER_INTERFACE + '.UnknownProperty',
            'Property %s not defined for interface %s' %
            (property_name, interface))

    @dbus.service.method(dbus.PROPERTIES_IFACE,
                         in_signature='s', out_signature='a{sv}')
    def GetAll(self, interface, *args, **kwargs):
        """Returns: A dictionary. The properties on interface."""
        logging.info('%s: GetAll %s', self.path, interface)
        interfaces = self.InterfacesAndProperties()
        properties = interfaces.get(interface, None)
        if properties is not None:
            return properties
        raise dbus.exceptions.DBusException(
            mm1.MODEM_MANAGER_INTERFACE + '.UnknownInterface',
            'Object does not implement the %s interface' % interface)

    def InterfacesAndProperties(self):
        """Subclasses must implement this function.

        Returns:
            A dictionary of interfaces where the values are dictionaries
            of dbus properties.
        """
        pass


class SIM(DBusObjectWithProperties):
    """SIM Object.

       Mock SIM Card and the typical information it might contain.
       SIM cards of different carriers can be created by providing
       the MCC, MNC, operator name, imsi, and msin.  SIM objects are
       passed to the Modem during Modem initialization.
    """

    DEFAULT_MCC = '310'
    DEFAULT_MNC = '090'
    DEFAULT_OPERATOR = 'AT&T'
    DEFAULT_MSIN = '1234567890'
    DEFAULT_IMSI = '888999111'
    MCC_LIST = {
        'us': '310',
        'de': '262',
        'es': '214',
        'fr': '208',
        'gb': '234',
        'it': '222',
        'nl': '204',
    }
    CARRIERS = {
        'att': ('us', '090', 'AT&T'),
        'tmobile': ('us', '026', 'T-Mobile'),
        'simyo': ('de', '03', 'simyo'),
        'movistar': ('es', '07', 'Movistar'),
        'sfr': ('fr', '10', 'SFR'),
        'three': ('gb', '20', '3'),
        'threeita': ('it', '99', '3ITA'),
        'kpn': ('nl', '08', 'KPN')
        }

    def __init__(self,
                 manager,
                 mcc_country='us',
                 mnc=DEFAULT_MNC,
                 operator_name=DEFAULT_OPERATOR,
                 msin=DEFAULT_MSIN,
                 imsi=None,
                 mcc=None,
                 name='/Sim/0'):
        self.manager = manager
        self.name = name
        self.path = manager.path + name
        self.mcc = mcc or SIM.MCC_LIST.get(mcc_country, '000')
        self.mnc = mnc
        self.operator_name = operator_name
        self.msin = msin
        self.imsi = imsi or (self.mcc + self.mnc + SIM.DEFAULT_IMSI)
        DBusObjectWithProperties.__init__(self, manager.bus, self.path)

    @staticmethod
    def FromCarrier(carrier, manager):
        """Creates a SIM card object for a given carrier."""
        args = SIM.CARRIERS.get(carrier, [])
        return SIM(manager, *args)

    def Properties(self):
        return {
            'SimIdentifier': self.msin,
            'Imsi': self.imsi,
            'OperatorIdentifier': self.mcc + self.mnc,
            'OperatorName': self.operator_name
            }

    def InterfacesAndProperties(self):
        return {mm1.SIM_INTERFACE: self.Properties()}

class SMS(DBusObjectWithProperties):
    """SMS Object.

       Mock SMS message.
    """

    def __init__(self, manager, name='/SMS/0', text='test',
                 number='123', timestamp='12:00', smsc=''):
        self.manager = manager
        self.name = name
        self.path = manager.path + name
        self.text = text or 'test sms at %s' % name
        self.number = number
        self.timestamp = timestamp
        self.smsc = smsc
        DBusObjectWithProperties.__init__(self, manager.bus, self.path)

    def Properties(self):
        # TODO(jglasgow): State, Validity, Class, Storage are also defined
        return {
            'Text': self.text,
            'Number': self.number,
            'Timestamp': self.timestamp,
            'SMSC': self.smsc
            }

    def InterfacesAndProperties(self):
        return {mm1.SMS_INTERFACE: self.Properties()}


class PseudoNetworkInterface(object):
    """A Pseudo network interface.

    This uses a pair of network interfaces and dnsmasq to simulate the
    network device normally associated with a modem.
    """

    def __init__(self, interface, base):
        self.interface = interface
        self.peer = self.interface + 'p'
        self.base = base
        self.lease_file = '/tmp/dnsmasq.%s.leases' % self.interface
        self.dnsmasq = None

    def __enter__(self):
        """Make usable with "with" statement."""
        self.CreateInterface()
        return self

    def __exit__(self, exception, value, traceback):
        """Make usable with "with" statement."""
        self.DestroyInterface()
        return False

    def CreateInterface(self):
        """Creates a virtual interface.

        Creates the virtual interface self.interface as well as a peer
        interface.  Runs dnsmasq on the peer interface so that a DHCP
        service can offer ip addresses to the virtual interface.
        """
        os.system('ip link add name %s type veth peer name %s' % (
            self.interface, self.peer))

        os.system('ifconfig %s %s.1/24' % (self.peer, self.base))
        os.system('ifconfig %s up' % self.peer)

        os.system('ifconfig %s up' % self.interface)
        os.system('route add -host 255.255.255.255 dev %s' % self.peer)
        os.close(os.open(self.lease_file, os.O_CREAT | os.O_TRUNC))
        self.dnsmasq = subprocess.Popen(
            ['/usr/local/sbin/dnsmasq',
             '--pid-file',
             '-k',
             '--dhcp-leasefile=%s' % self.lease_file,
             '--dhcp-range=%s.2,%s.254' % (self.base, self.base),
             '--port=0',
             '--interface=%s' % self.peer,
             '--bind-interfaces'
            ])
        # iptables rejects packets on a newly defined interface.  Fix that.
        os.system('iptables -I INPUT -i %s -j ACCEPT' % self.peer)
        os.system('iptables -I INPUT -i %s -j ACCEPT' % self.interface)

    def DestroyInterface(self):
        """Destroys the virtual interface.

        Stops dnsmasq and cleans up all on disk state.
        """
        if self.dnsmasq:
            self.dnsmasq.terminate()
        try:
            os.system('route del -host 255.255.255.255')
        except:
            pass
        try:
            os.system('ip link del %s' % self.interface)
        except:
            pass
        os.system('iptables -D INPUT -i %s -j ACCEPT' % self.peer)
        os.system('iptables -D INPUT -i %s -j ACCEPT' % self.interface)
        if os.path.exists(self.lease_file):
            os.remove(self.lease_file)


class Modem(DBusObjectWithProperties):
    """A Modem object that implements the ModemManager DBUS API."""

    def __init__(self, manager, name='/Modem/0',
                 device='pseudomodem0',
                 mdn='0000001234',
                 meid='A100000DCE2CA0',
                 carrier='CrCarrier',
                 esn='EDD1EDD1',
                 sim=None):
        """Instantiates a Modem with some options.

        Args:
            manager: a ModemManager object.
            name: string, a dbus path name.
            device: string, the network device to use.
            mdn: string, the mobile directory number.
            meid: string, the mobile equipment id (CDMA only?).
            carrier: string, the name of the carrier.
            esn: string, the electronic serial number.
            sim: a SIM object.
        """
        self.state = mm1.MM_MODEM_STATE_DISABLED
        self.manager = manager
        self.name = name
        self.path = manager.path + name
        self.device = device
        self.mdn = mdn
        self.meid = meid
        self.carrier = carrier
        self.operator_name = carrier
        self.operator_code = '123'
        self.esn = esn
        self.registration_state = mm1.MM_MODEM_3GPP_REGISTRATION_STATE_IDLE
        self.sim = sim
        DBusObjectWithProperties.__init__(self, manager.bus, self.path)
        self.pseudo_interface = PseudoNetworkInterface(self.device, '192.168.7')
        self.smses = {}

    def __enter__(self):
        """Make usable with "with" statement."""
        self.pseudo_interface.__enter__()
        # Add the device to the manager only after the pseudo
        # interface has been created.
        self.manager.Add(self)
        return self

    def __exit__(self, exception, value, traceback):
        """Make usable with "with" statement."""
        self.manager.Remove(self)
        return self.pseudo_interface.__exit__(exception, value, traceback)

    def DiscardModem(self):
        """Discard this DBUS Object.

        Send a message that a modem has disappeared and deregister from DBUS.
        """
        logging.info('DiscardModem')
        self.remove_from_connection()
        self.manager.Remove(self)

    def ModemProperties(self):
        """Return the properties of the modem object."""
        properties = {
            # 'Sim': type='o'
            'ModemCapabilities': UInt32(0),
            'CurrentCapabilities': UInt32(0),
            'MaxBearers': UInt32(2),
            'MaxActiveBearers': UInt32(2),
            'Manufacturer': 'Foo Electronics',
            'Model': 'Super Foo Modem',
            'Revision': '1.0',
            'DeviceIdentifier': '123456789',
            'Device': self.device,
            'Driver': 'fake',
            'Plugin': 'Foo Plugin',
            'EquipmentIdentifier': self.meid,
            'UnlockRequired': UInt32(0),
            #'UnlockRetries' type='a{uu}'
            mm1.MM_MODEM_PROPERTY_STATE: Int32(self.state),
            'AccessTechnologies': UInt32(self.state),
            'SignalQuality': Struct([UInt32(90), True], signature='ub'),
            'OwnNumbers': ['6175551212'],
            'SupportedModes': UInt32(0),
            'AllowedModes': UInt32(0),
            'PreferredMode': UInt32(0),
            'SupportedBands': [UInt32(0)],
            'Bands': [UInt32(0)]
            }
        if self.sim:
            properties['Sim'] = ObjectPath(self.sim.path)
        return properties

    def InterfacesAndProperties(self):
        """Return all supported interfaces and their properties."""
        return {
            mm1.MODEM_INTERFACE: self.ModemProperties(),
            }

    def ChangeState(self, new_state,
                    why=mm1.MM_MODEM_STATE_CHANGE_REASON_UNKNOWN):
        logging.info('Change state from %s to %s', self.state, new_state)
        self.StateChanged(Int32(self.state), Int32(new_state), UInt32(why))
        self.PropertiesChanged(mm1.MODEM_INTERFACE,
                               {mm1.MM_MODEM_PROPERTY_STATE: Int32(new_state)},
                               [])
        self.state = new_state

    @dbus.service.method(mm1.MODEM_INTERFACE,
                         in_signature='b', out_signature='')
    def Enable(self, on, *args, **kwargs):
        """Enables the Modem."""
        logging.info('Modem: Enable %s', str(on))
        if on:
            if self.state <= mm1.MM_MODEM_STATE_ENABLING:
                self.ChangeState(mm1.MM_MODEM_STATE_ENABLING)
            if self.state <= mm1.MM_MODEM_STATE_ENABLED:
                self.ChangeState(mm1.MM_MODEM_STATE_ENABLED)
            if self.state <= mm1.MM_MODEM_STATE_SEARCHING:
                self.ChangeState(mm1.MM_MODEM_STATE_SEARCHING)
            glib.timeout_add(250, self.OnRegistered)
        else:
            if self.state >= mm1.MM_MODEM_STATE_DISABLING:
                self.ChangeState(mm1.MM_MODEM_STATE_DISABLING)
            if self.state >= mm1.MM_MODEM_STATE_DISABLED:
                self.ChangeState(mm1.MM_MODEM_STATE_DISABLED)
                self.ChangeRegistrationState(
                    mm1.MM_MODEM_3GPP_REGISTRATION_STATE_IDLE)
        return None

    def ChangeRegistrationState(self, new_state):
        """Updates the registration state of the modem.

        Updates the registration state of the modem and broadcasts a
        DBUS signal.

        Args:
          new_state: the new registation state of the modem.
        """
        if new_state != self.registration_state:
            self.registration_state = new_state
            self.PropertiesChanged(
                mm1.MODEM_MODEM3GPP_INTERFACE,
                {mm1.MM_MODEM3GPP_PROPERTY_REGISTRATION_STATE:
                     UInt32(new_state)},
                [])

    def OnRegistered(self):
        """Called when the Modem is Registered."""
        if (self.state >= mm1.MM_MODEM_STATE_ENABLED and
            self.state <= mm1.MM_MODEM_STATE_REGISTERED):
            logging.info('Modem: Marking Registered')
            self.ChangeRegistrationState(
                mm1.MM_MODEM_3GPP_REGISTRATION_STATE_HOME)
            self.ChangeState(mm1.MM_MODEM_STATE_REGISTERED)

    @dbus.service.method(mm1.MODEM_SIMPLE_INTERFACE, in_signature='',
                         out_signature='a{sv}')
    def GetStatus(self, *args, **kwargs):
        """Gets the general modem status.

        Returns:
            A dictionary of properties.
        """
        logging.info('Modem: GetStatus')
        properties = {
            'state': UInt32(self.state),
            'signal-quality': UInt32(99),
            'bands': self.carrier,
            'access-technology': UInt32(0),
            'm3gpp-registration-state': UInt32(self.registration_state),
            'm3gpp-operator-code': '123',
            'm3gpp-operator-name': '123',
            'cdma-cdma1x-registration-state': UInt32(99),
            'cdma-evdo-registration-state': UInt32(99),
            'cdma-sid': '123',
            'cdma-nid': '123',
            }
        if self.state >= mm1.MM_MODEM_STATE_ENABLED:
            properties['carrier'] = 'Test Network'
        return properties

    @dbus.service.signal(mm1.MODEM_INTERFACE, signature='iiu')
    def StateChanged(self, old_state, new_state, why):
        pass

    @dbus.service.method(mm1.MODEM_SIMPLE_INTERFACE, in_signature='a{sv}',
                         out_signature='o',
                         async_callbacks=('return_cb', 'raise_cb'))
    def Connect(self, unused_props, return_cb, raise_cb, **kwargs):
        """Connect the modem to the network.

        Args:
            unused_props: connection properties. See ModemManager documentation.
            return_cb: function to call to return result asynchronously.
            raise_cb: function to call to raise an error asynchronously.
        """

        def ConnectDone(new, why):
            logging.info('Modem: ConnectDone %s -> %s because %s',
                         str(self.state), str(new), str(why))
            if self.state == mm1.MM_MODEM_STATE_CONNECTING:
                self.ChangeState(new, why)
            # TODO(jglasgow): implement a bearer object
                bearer_path = '/Bearer/0'
                return_cb(bearer_path)
            else:
                raise_cb(mm1.ConnectionUnknownError())

        logging.info('Modem: Connect')
        if self.state != mm1.MM_MODEM_STATE_REGISTERED:
            logging.info(
                'Modem: Connect fails on unregistered modem.  State = %s',
                self.state)
            raise mm1.NoNetworkError()
        delay_ms = kwargs.get('connect_delay_ms', DEFAULT_CONNECT_DELAY_MS)
        time.sleep(delay_ms / 1000.0)
        self.ChangeState(mm1.MM_MODEM_STATE_CONNECTING)
        glib.timeout_add(50, lambda: ConnectDone(
            mm1.MM_MODEM_STATE_CONNECTED,
            mm1.MM_MODEM_STATE_CHANGE_REASON_USER_REQUESTED))

    @dbus.service.method(mm1.MODEM_SIMPLE_INTERFACE, in_signature='o',
                         async_callbacks=('return_cb', 'raise_cb'))
    def Disconnect(self, bearer, return_cb, raise_cb, **kwargs):
        """Disconnect the modem from the network."""

        def DisconnectDone(old, new, why):
            logging.info('Modem: DisconnectDone %s -> %s because %s',
                         str(old), str(new), str(why))
            if self.state == mm1.MM_MODEM_STATE_DISCONNECTING:
                logging.info('Modem: State is DISCONNECTING, changing to %s',
                             str(new))
                self.ChangeState(new)
                return_cb()
            elif self.state == mm1.MM_MODEM_STATE_DISABLED:
                logging.info('Modem: State is DISABLED, not changing state')
                return_cb()
            else:
                raise_cb(mm1.ConnectionUnknownError())

        logging.info('Modem: Disconnect')
        self.ChangeState(mm1.MM_MODEM_STATE_DISCONNECTING)
        glib.timeout_add(
            500,
            lambda: DisconnectDone(
                self.state,
                mm1.MM_MODEM_STATE_REGISTERED,
                mm1.MM_MODEM_STATE_CHANGE_REASON_USER_REQUESTED))

    @dbus.service.signal(dbus.PROPERTIES_IFACE, signature='sa{sv}as')
    def PropertiesChanged(self, interface, changed_properties,
                          invalidated_properties):
        pass

    def AddSMS(self, sms):
        logging.info('Adding SMS %s to list', sms.path)
        self.smses[sms.path] = sms
        self.Added(self.path, True)

    @dbus.service.method(mm1.MODEM_MESSAGING_INTERFACE, in_signature='',
                         out_signature='ao')
    def List(self, *args, **kwargs):
        logging.info('Modem.Messaging: List: %s',
                     ', '.join(self.smses.keys()))
        return self.smses.keys()

    @dbus.service.method(mm1.MODEM_MESSAGING_INTERFACE, in_signature='o',
                         out_signature='')
    def Delete(self, sms_path, *args, **kwargs):
        logging.info('Modem.Messaging: Delete %s', sms_path)
        del self.smses[sms_path]

    @dbus.service.signal(mm1.MODEM_MESSAGING_INTERFACE, signature='ob')
    def Added(self, sms_path, complete):
        pass


class GSMModem(Modem):
    """A GSMModem implements the mm1.MODEM_MODEM3GPP_INTERFACE interface."""

    def __init__(self, manager, imei='00112342342', **kwargs):
        self.imei = imei
        Modem.__init__(self, manager, **kwargs)

    @dbus.service.method(mm1.MODEM_MODEM3GPP_INTERFACE,
                         in_signature='s', out_signature='')
    def Register(self, operator_id, *args, **kwargs):
        """Register the modem on the network."""
        pass

    def Modem3GPPProperties(self):
        """Return the 3GPP Properties of the modem object."""
        return {
            'Imei': self.imei,
            mm1.MM_MODEM3GPP_PROPERTY_REGISTRATION_STATE:
                UInt32(self.registration_state),
            'OperatorCode': self.operator_code,
            'OperatorName': self.operator_name,
            'EnabledFacilityLocks': UInt32(0)
            }

    def InterfacesAndProperties(self):
        """Return all supported interfaces and their properties."""
        return {
            mm1.MODEM_INTERFACE: self.ModemProperties(),
            mm1.MODEM_MODEM3GPP_INTERFACE: self.Modem3GPPProperties()
            }

    @dbus.service.method(mm1.MODEM_MODEM3GPP_INTERFACE, in_signature='',
                         out_signature='aa{sv}')
    def Scan(self, *args, **kwargs):
        """Scan for networks."""
        raise mm1.CoreUnsupportedError()


class ModemManager(dbus.service.Object):
    """Implements the org.freedesktop.DBus.ObjectManager interface."""

    def __init__(self, bus, path):
        self.devices = []
        self.bus = bus
        self.path = path
        dbus.service.Object.__init__(self, bus, path)

    def Add(self, device):
        """Adds a modem device to the list of devices that are managed."""
        logging.info('ModemManager: add %s', device.name)
        self.devices.append(device)
        interfaces = device.InterfacesAndProperties()
        logging.info('Add: %s', interfaces)
        self.InterfacesAdded(device.path, interfaces)

    def Remove(self, device):
        """Removes a modem device from the list of managed devices."""
        logging.info('ModemManager: remove %s', device.name)
        self.devices.remove(device)
        interfaces = device.InterfacesAndProperties().keys()
        self.InterfacesRemoved(device.path, interfaces)

    @dbus.service.method(mm1.OFDOM, out_signature='a{oa{sa{sv}}}')
    def GetManagedObjects(self):
        """Returns the list of managed objects and their properties."""
        results = {}
        for device in self.devices:
            results[device.path] = device.InterfacesAndProperties()
        logging.info('GetManagedObjects: %s', ', '.join(results.keys()))
        return results

    @dbus.service.signal(mm1.OFDOM, signature='oa{sa{sv}}')
    def InterfacesAdded(self, object_path, interfaces_and_properties):
        pass

    @dbus.service.signal(mm1.OFDOM, signature='oas')
    def InterfacesRemoved(self, object_path, interfaces):
        pass


def main():
    usage = """
Run pseudo_modem to simulate a GSM modem using the modemmanager-next
DBUS interfaces.  This can be used for the following:
  - to simpilify the verification process of UI features that use of
    overseas SIM cards
  - to test shill on a virtual machine without a physical modem
  - to test that Chrome property displays SMS messages

To use on a test image you use test_that to run
network_3GModemControl which will cause pseudo_modem.py to be
installed in /usr/local/autotests/cros/cellular.  Then stop
modemmanager and start the pseudo modem with the commands:

  stop modemmanager
  /usr/local/autotest/cros/cellular/pseudo_modem.py

When done, use Control-C to stop the process and restart modem manager:
  start modemmanager

Additional help documentation is available by invoking pseudo_modem.py
--help.

SMS testing can be accomnplished by supplying the -s flag to simulate
the receipt of a number of SMS messages.  The message text can be
specified with the --text option on the command line, or read from a
file by using the --file option.  If the messages are located in a
file, then each line corresponds to a single SMS message.

Chrome should display the SMS messages as soon as a user logs in to
the Chromebook, or if the user is already logged in, then shortly
after the pseudo modem is recognized by shill.
"""
    parser = OptionParser(usage=usage)
    parser.add_option('-c', '--carrier', dest='carrier_name',
                      metavar='<carrier name>',
                      help='<carrier name> := %s' % ' | '.join(
                          SIM.CARRIERS.keys()))
    parser.add_option('-s', '--smscount', dest='sms_count',
                      default=0,
                      metavar='<smscount>',
                      help='<smscount> := integer')
    parser.add_option('-l', '--logfile', dest='logfile',
                      default='',
                      metavar='<filename>',
                      help='<filename> := filename for logging output')
    parser.add_option('-t', '--text', dest='sms_text',
                      default=None,
                      metavar='<text>',
                      help='<text> := text for sms messages')
    parser.add_option('-f', '--file', dest='filename',
                      default=None,
                      metavar='<filename>',
                      help='<filename> := file with text for sms messages')

    (options, args) = parser.parse_args()

    kwargs = {}
    if options.logfile:
        kwargs['filename'] = options.logfile
    logging.basicConfig(format='%(asctime)-15s %(message)s',
                        level=logging.DEBUG,
                        **kwargs)

    if not options.carrier_name:
        options.carrier_name = DEFAULT_CARRIER

    dbus.mainloop.glib.DBusGMainLoop(set_as_default=True)
    bus = dbus.SystemBus()
    name = dbus.service.BusName(mm1.MODEM_MANAGER_INTERFACE, bus)
    manager = ModemManager(bus, mm1.OMM)
    sim_card = SIM.FromCarrier(string.lower(options.carrier_name),
                               manager)
    with GSMModem(manager, sim=sim_card) as modem:
        if options.filename:
            f = open(options.filename, 'r')
            for index, line in enumerate(f.readlines()):
                line = line.strip()
                if line:
                    sms = SMS(manager, name='/SMS/%s' % index, text=line)
                    modem.AddSMS(sms)
        else:
            for index in xrange(int(options.sms_count)):
                sms = SMS(manager, name='/SMS/%s' % index,
                          text=options.sms_text)
                modem.AddSMS(sms)

        mainloop = gobject.MainLoop()

        def SignalHandler(signum, frame):
            logging.info('Signal handler called with signal: %s', signum)
            mainloop.quit()

        signal.signal(signal.SIGTERM, SignalHandler)

        mainloop.run()

if __name__ == '__main__':
    main()
