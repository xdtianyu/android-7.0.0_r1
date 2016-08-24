# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from autotest_lib.client.bin import test, utils
from autotest_lib.client.common_lib import error

import dbus, dbus.mainloop.glib, gobject
import glib

from autotest_lib.client.cros import flimflam_test_path
from autotest_lib.client.cros.cellular import mm
from autotest_lib.client.cros.mainloop import ExceptionForward
from autotest_lib.client.cros.mainloop import ExceptionForwardingMainLoop

import flimflam

class State:
    ENABLING = 0
    REGISTERING = 1
    CONNECTING = 2
    WAITING = 3
    DISCONNECTING = 4
    DISABLING = 5

class DormancyTester(ExceptionForwardingMainLoop):
    def __init__(self, loops, flim, device, *args, **kwargs):
        self.loopsleft = loops
        self.flim = flim
        self.device = device
        super(DormancyTester, self).__init__(
                *args, timeout_s=20 * loops + 20, **kwargs)

    def countdown(self):
        self.loopsleft -= 1
        print 'Countdown: %d' % (self.loopsleft,)
        if self.loopsleft == 0:
            self.quit()

    @ExceptionForward
    def enable(self):
        print 'Enabling...'
        self.state = State.ENABLING
        self.flim.EnableTechnology('cellular')

    @ExceptionForward
    def disable(self):
        print 'Disabling...'
        self.state = State.DISABLING
        self.flim.DisableTechnology('cellular')

    @ExceptionForward
    def connect(self):
        print 'Connecting...'
        self.state = State.CONNECTING
        self.flim.ConnectService(service=self.service, config_timeout=120)

    @ExceptionForward
    def disconnect(self):
        print 'Disconnecting...'
        self.state = State.DISCONNECTING
        self.flim.DisconnectService(service=self.service, wait_timeout=60)

    @ExceptionForward
    def PropertyChanged(self, *args, **kwargs):
        if args[0] == 'Powered':
            if not args[1]:
                self.HandleDisabled()
            else:
                self.HandleEnabled()
        elif args[0] == 'Connected':
            if not args[1]:
                self.HandleDisconnected()
            else:
                self.HandleConnected()
        elif args[0] == 'Services':
            self.CheckService()

    @ExceptionForward
    def DormancyStatus(self, *args, **kwargs):
        if args[0]:
            self.HandleDormant()
        else:
            self.HandleAwake()

    def FindService(self):
        self.service = self.flim.FindElementByPropertySubstring('Service',
                                                                'Type',
                                                                'cellular')

    def CheckService(self):
        self.FindService()
        if self.state == State.REGISTERING and self.service:
            self.HandleRegistered()

    def HandleDisabled(self):
        if self.state != State.DISABLING:
            raise error.TestFail('Disabled while not in state Disabling')
        print 'Disabled'
        self.countdown()
        self.enable()

    def HandleEnabled(self):
        if self.state != State.ENABLING:
            raise error.TestFail('Enabled while not in state Enabling')
        print 'Enabled'
        self.state = State.REGISTERING
        print 'Waiting for registration...'
        self.CheckService()

    def HandleRegistered(self):
        if self.state != State.REGISTERING:
            raise error.TestFail('Registered while not in state Registering')
        print 'Registered'
        self.connect()

    def HandleConnected(self):
        if self.state != State.CONNECTING:
            raise error.TestFail('Connected while not in state Connecting')
        print 'Connected'
        self.state = State.WAITING
        print 'Waiting for dormancy...'

    def HandleDormant(self):
        if self.state != State.WAITING:
            print 'Dormant while not in state Waiting; ignoring.'
            return
        print 'Dormant'
        self.disconnect()

    def HandleAwake(self):
        print 'Awake'

    def HandleDisconnected(self):
        if self.state != State.DISCONNECTING:
            raise error.TestFail(
                'Disconnected while not in state Disconnecting')
        print 'Disconnected'
        self.disable()

    def idle(self):
        connected = False
        powered = False

        device_props = self.device.GetProperties(utf8_strings = True)

        self.FindService()
        if self.service:
            service_props = self.service.GetProperties(utf8_strings = True)
            if service_props['State'] in ['online', 'portal', 'ready']:
                connected = True
            print 'Service exists, and state is %s.' % (service_props['State'],)
        else:
            print 'Service does not exist.'

        if device_props['Powered']:
            print 'Device is powered.'
            powered = True
        else:
            print 'Device is unpowered.'

        if powered and connected:
            print 'Starting with Disconnect.'
            self.disconnect()
        elif powered and (not connected):
            print 'Starting with Disable.'
            self.disable()
        elif (not powered) and (not connected):
            print 'Starting with Enable.'
            self.enable()
        else:
            raise error.TestFail('Service online but device unpowered!')



class network_3GDormancyDance(test.test):
    version = 1

    def FindModemPath(self):
        for modem in mm.EnumerateDevices():
            (obj, path) = modem
            try:
                if path.index('/org/chromium/ModemManager/Gobi') == 0:
                    return path
            except ValueError:
                pass
        return None

    def RequestDormancyEvents(self, modem_path):
        modem = dbus.Interface(
            self.bus.get_object('org.chromium.ModemManager', modem_path),
            dbus_interface='org.chromium.ModemManager.Modem.Gobi')
        modem.RequestEvents('+dormancy')

    def PropertyChanged(self, *args, **kwargs):
        self.tester.PropertyChanged(*args, **kwargs)

    def DormancyStatus(self, *args, **kwargs):
        self.tester.DormancyStatus(*args, **kwargs)

    def run_once(self, name='wwan', loops=20, seed=None):
        dbus.mainloop.glib.DBusGMainLoop(set_as_default=True)
        self.bus = dbus.SystemBus()

        main_loop = gobject.MainLoop()

        modem_path = self.FindModemPath()
        if not modem_path:
            raise error.TestFail('No Gobi modem found.')
        print 'Modem: %s' % (modem_path,)
        self.RequestDormancyEvents(modem_path)

        flim = flimflam.FlimFlam()
        device = flim.FindElementByNameSubstring('Device', name)

        if not device:
            device = flim.FindElementByPropertySubstring('Device',
                                                         'Interface',
                                                          name)
        self.bus.add_signal_receiver(self.PropertyChanged,
                                     signal_name='PropertyChanged')
        self.bus.add_signal_receiver(self.DormancyStatus,
                                     signal_name='DormancyStatus')
        self.tester = DormancyTester(main_loop=main_loop,
                                     loops=loops, flim=flim, device=device)
        self.tester.run()
