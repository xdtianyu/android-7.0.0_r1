#!/usr/bin/python
# Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import cellular_logging
import dbus, os, subprocess, time

from autotest_lib.client.common_lib import error
from autotest_lib.client.cros import flimflam_test_path
from autotest_lib.client.cros.cellular import modem

log = cellular_logging.SetupCellularLogging('mm_test')


class ModemManagerTest(object):
    """Wrapper for starting up ModemManager in an artificial testing
    environment, connected to a fake modem program and talking to a
    fake (tun) network device.

    The test using this must ensure the setup of the fakegudev and
    fakemodem deps.
    """

    def __init__(self, autodir, modem_pattern_files):
        self.autodir=autodir # not great. Examine deps directly?
        self.modem_pattern_files = modem_pattern_files
        self.modemmanager = None
        self.fakemodem_process = None
        self.fakenet_process = None

    def _start_fake_network(self):
        """Start the fakenetwork program and return the fake interface name

        Start up the fakenet program, which uses the tun driver to create
        a network device.

        Returns the name of the fake network interface.
        Sets self.fakenet_process as a handle to the process.
        """
        self.fakenet_process = subprocess.Popen(
            os.path.join(self.autodir,'deps/fakemodem/bin','fakenet'),
            stdout=subprocess.PIPE)
        return self.fakenet_process.stdout.readline().rstrip()


    def _start_fake_modem(self, patternfiles):
        """Start the fakemodem program and return the pty path to access it

        Start up the fakemodem program
        Argument:
        patternfiles -- List of files to read for command/response patterns

        Returns the device path of the pty that serves the fake modem, e.g.
        /dev/pts/4.
        Sets self.fakemodem_process as a handle to the process, and
        self.fakemodem as a DBus interface to it.
        """
        scriptargs = ["--patternfile=" + x for x in patternfiles]
        name = os.path.join(self.autodir, 'deps/fakemodem/bin', 'fakemodem')
        self.fakemodem_process = subprocess.Popen(
            [os.path.join(self.autodir, 'deps/fakemodem/bin', 'fakemodem')]
            + scriptargs,
            stdout=subprocess.PIPE)
        ptyname = self.fakemodem_process.stdout.readline().rstrip()
        time.sleep(2) # XXX
        self.fakemodem = dbus.Interface(
            dbus.SystemBus().get_object('org.chromium.FakeModem', '/'),
            'org.chromium.FakeModem')
        return ptyname


    def _start_modemmanager(self, netname, modemname):
        """Start modemmanager under the control of fake devices.

        Arguments:
        netname -- fake network interface name (e.g. tun0)
        modemname -- path to pty slave device of fake modem (e.g. /dev/pts/4)

        Returns...

        """
        id_props = ['property_ID_MM_CANDIDATE=1',
                    'property_ID_VENDOR_ID=04e8', # Samsung USB VID
                    'property_ID_MODEL_ID=6872' # Y3300 modem PID
                    ]
        tty_device = (['device_file=%s' % (modemname),
                       'name=%s' % (modemname[5:]), # remove leading /dev/
                       'subsystem=tty',
                       'driver=fake',
                       'sysfs_path=/sys/devices/fake/tty',
                       'parent=/dev/fake-parent'] +
                      id_props)
        net_device = (['device_file=/dev/fakenet',
                       'name=%s' % (netname),
                       'subsystem=net',
                       'driver=fake',
                       'sysfs_path=/sys/devices/fake/net',
                       'parent=/dev/fake-parent'] +
                      id_props)
        parent_device=['device_file=/dev/fake-parent',
                       'sysfs_path=/sys/devices/fake/parent',
                       'devtype=usb_device',
                       'subsystem=usb']
        environment = { 'FAKEGUDEV_DEVICES' : ':'.join(tty_device +
                                                       net_device +
                                                       parent_device),
                        'FAKEGUDEV_BLOCK_REAL' : 'true',
                        'G_DEBUG' : 'fatal_criticals',
                        'LD_PRELOAD' : os.path.join(self.autodir,
                                                    "deps/fakegudev/lib",
                                                    "libfakegudev.so") }
        self.modemmanager = subprocess.Popen(['/usr/sbin/modem-manager',
                                              '--debug',
                                              '--log-level=DEBUG',
                                              '--log-file=/tmp/mm-log'],
                                             env=environment)
        time.sleep(3) # wait for DeviceAdded signal?
        self.modemmanager.poll()
        if self.modemmanager.returncode is not None:
            self.modemmanager = None
            raise error.TestFail("ModemManager quit early")

        # wait for MM to stabilize?
        return modem.ModemManager(provider='org.freedesktop')

    def _stop_fake_network(self):
        if self.fakenet_process:
            self.fakenet_process.poll()
            if self.fakenet_process.returncode is None:
                self.fakenet_process.terminate()
                self.fakenet_process.wait()

    def _stop_fake_modem(self):
        if self.fakemodem_process:
            self.fakemodem_process.poll()
            if self.fakemodem_process.returncode is None:
                self.fakemodem_process.terminate()
                self.fakemodem_process.wait()

    def _stop_modemmanager(self):
        if self.modemmanager:
            self.modemmanager.poll()
            if self.modemmanager.returncode is None:
                self.modemmanager.terminate()
                self.modemmanager.wait()


    def __enter__(self):
        fakenetname = self._start_fake_network()
        fakemodemname = self._start_fake_modem(self.modem_pattern_files)
        self.mm = self._start_modemmanager(fakenetname, fakemodemname)
        # This would be better handled by listening for DeviceAdded, but
        # since we've blocked everything else and only supplied data for
        # one modem, it's going to be right
        self.modem_object_path = self.mm.path + '/Modems/0'
        return self

    def __exit__(self, exception, value, traceback):
        self._stop_modemmanager()
        self._stop_fake_modem()
        self._stop_fake_network()
        return False
