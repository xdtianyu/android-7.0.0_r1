# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from autotest_lib.client.bin import test, utils
from autotest_lib.client.common_lib import error

import dbus, dbus.mainloop.glib, gobject

class network_SwitchCarrier(test.test):
    version = 1

    def fail(self, msg):
        print 'Failed: %s' % msg
        self.failed = error.TestFail(msg)
        self.loop.quit()

    def device_added(self, dev, *args, **kwargs):
        print 'Device added: %s' % dev
        self.modem = self.bus.get_object(self.CMM, dev)
        carrier = self.get_carrier()
        if not carrier:
            print 'No carrier.'
            return
        if carrier != self.to_carrier:
            self.fail('Wrong carrier: %s != %s' % (carrier, self.to_carrier))
        if not self.carriers:
            self.loop.quit() # success!
            return
        while len(self.carriers):
            try:
                self.to_carrier = self.carriers[0]
                self.carriers = self.carriers[1:]
                self.set_carrier(self.to_carrier)
                break
            except dbus.exceptions.DBusException, e:
                if e.get_dbus_message() == "Unknown carrier name":
                    print 'Ignoring invalid carrier %s' % self.to_carrier
                    continue
                raise

    def device_removed(self, *args, **kwargs):
        print 'Device removed.'

    def waitfor(self, signame, fn):
        print 'Waiting for %s' % signame
        self.bus.add_signal_receiver(fn, signal_name=signame,
                                     dbus_interface=self.IMM)

    def timeout(self):
        self.fail('Timeout')

    def get_carrier(self):
        status = self.modem.GetStatus(dbus_interface=self.IMODEM_SIMPLE)
        if not status or not 'carrier' in status:
            self.fail('Bogus GetStatus reply: %s' % status)
            return None
        return status['carrier']

    def set_carrier(self, c):
        print 'Switch: ? -> %s' % c
        self.modem.SetCarrier(c, dbus_interface=self.IMODEM_GOBI)

    def find_modem(self):
        modems = self.mm.EnumerateDevices(dbus_interface=self.IMM)
        if modems:
            self.modem = self.bus.get_object(self.CMM, modems[0])
        else:
            self.modem = None

    def run_once(self, start_carrier='Verizon Wireless',
                 carriers=None,
                 timeout_secs=90):
        carriers = carriers or ['Vodafone', 'Sprint', 'Verizon Wireless']
        self.CMM = 'org.chromium.ModemManager'
        self.IMM = 'org.freedesktop.ModemManager'
        self.IMODEM_SIMPLE = self.IMM + '.Modem.Simple'
        self.IMODEM_GOBI = 'org.chromium.ModemManager.Modem.Gobi'
        self.failed = None
        self.carriers = carriers
        self.to_carrier = None
        dbus.mainloop.glib.DBusGMainLoop(set_as_default=True)
        self.loop = gobject.MainLoop()
        self.bus = dbus.SystemBus()
        gobject.timeout_add(timeout_secs * 1000, self.timeout)
        self.mm = self.bus.get_object(self.CMM, '/org/chromium/ModemManager')
        self.find_modem()
        self.waitfor('DeviceRemoved', self.device_removed)
        self.waitfor('DeviceAdded', self.device_added)
        carrier = self.get_carrier()
        if not carrier:
            raise self.failed
        self.to_carrier = carrier
        self.device_added(self.modem.__dbus_object_path__) # start test
        self.loop.run()
        self.find_modem()
        if self.modem and self.to_carrier != carrier:
            self.set_carrier(carrier)
        if self.failed:
            raise self.failed
