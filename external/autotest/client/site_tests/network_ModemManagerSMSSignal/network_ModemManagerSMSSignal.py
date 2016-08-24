# Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging, os, subprocess
import dbus, dbus.mainloop.glib, gobject

from autotest_lib.client.bin import test
from autotest_lib.client.common_lib import error
from autotest_lib.client.cros.cellular import sms, mmtest
from autotest_lib.client.cros.mainloop import GenericTesterMainLoop
from autotest_lib.client.cros.mainloop import ExceptionForward

class SmsTester(GenericTesterMainLoop):
    def __init__(self, autodir, srcdir, mmt, *args, **kwargs):
        self.autodir = autodir
        self.srcdir = srcdir
        self.mmt = mmt
        self.remaining_requirements = ['Received', 'Completed']
        super(SmsTester, self).__init__(*args, timeout_s = 10, **kwargs)

    # The GenericTesterMainLoop will run this routine from the idle
    # loop.  In a successful test, the two SMS signals will be
    # recieved by the routines registered in the main test class and
    # will call in to the methods below. Order of reception is not
    # important.
    @ExceptionForward
    def perform_one_test(self):
        self.gsmsms = self.mmt.mm.GsmSms(self.mmt.modem_object_path)
        self.smstest = sms.SmsTest(self.gsmsms)
        self.smsstore = sms.SmsStore(self.mmt.fakemodem)
        # Actual test
        self.smstest.test_has_none()
        self.testsms = sms.sample
        self.smsstore.sms_receive(1, self.testsms['pdu'])

    @ExceptionForward
    def SmsReceived(self, index, complete):
        if index != 1:
            raise error.TestFail("Wrong index %d != 1" % index)
        if complete == False:
            raise error.TestFail("Message not complete")
        self.requirement_completed('Received')

    @ExceptionForward
    def SmsCompleted(self, index, complete):
        if index != 1:
            raise error.TestFail("Wrong index %d != 1" % index)
        if complete == False:
            raise error.TestFail("Message not complete")
        self.smstest.test_has_one(self.testsms['parsed'])
        self.smsstore.sms_remove(1)
        self.smstest.test_has_none()
        self.requirement_completed('Completed')


class SmsMultipartTester(GenericTesterMainLoop):
    def __init__(self, autodir, srcdir, mmt, *args, **kwargs):
        self.autodir = autodir
        self.srcdir = srcdir
        self.mmt = mmt
        self.remaining_requirements = ['Received', 'Received', 'Completed']
        super(SmsMultipartTester, self).__init__(*args, timeout_s = 10,
                                                  **kwargs)

    # The GenericTesterMainLoop will run this routine from the idle
    # loop.  In a successful test, the first SMSReceived signal will
    # be recieved by the routine registered in the main test class and
    # will call SmsReceived below; that routine will send the second
    # part of the message, and the two resulting signals will call
    # SmsReceived and SmsCompleted.
    @ExceptionForward
    def perform_one_test(self):
        self.gsmsms = self.mmt.mm.GsmSms(self.mmt.modem_object_path)
        self.smstest = sms.SmsTest(self.gsmsms)
        self.smsstore = sms.SmsStore(self.mmt.fakemodem)
        # Actual test
        self.smstest.test_has_none()
        self.testsms = sms.sample_multipart
        self.smsstore.sms_receive(1, self.testsms['pdu'][0])
        self.second = False

    @ExceptionForward
    def SmsReceived(self, index, complete):
        logging.info("Received, index %d"%index)
        if index != 1:
            raise error.TestFail("Wrong index %d != 1" % index)
        if complete != self.second:
            raise error.TestFail("Complete is wrong, should be %s" %
                                 self.second)
        self.requirement_completed('Received')
        if self.second == False:
            self.smsstore.sms_receive(2, self.testsms['pdu'][1])
            self.second = True

    @ExceptionForward
    def SmsCompleted(self, index, complete):
        logging.info("Completed, index %d"%index)
        if index != 1:
            raise error.TestFail("Wrong index %d != 1" % index)
        if complete == False:
            raise error.TestFail("Message not complete")
        self.smstest.test_has_one(self.testsms['parsed'])
        self.smsstore.sms_remove(1)
        self.smsstore.sms_remove(2)
        self.smstest.test_has_none()
        self.requirement_completed('Completed')


class network_ModemManagerSMSSignal(test.test):
    version = 1

    def setup(self):
        self.job.setup_dep(['fakegudev', 'fakemodem'])

    @ExceptionForward
    def SmsReceived(self, *args, **kwargs):
        self.smstester.SmsReceived(*args, **kwargs)

    @ExceptionForward
    def SmsCompleted(self, *args, **kwargs):
        self.smstester.SmsCompleted(*args, **kwargs)

    def run_once(self, **kwargs):
        self.job.install_pkg('fakegudev', 'dep',
                             os.path.join(self.autodir, 'deps', 'fakegudev'))
        self.job.install_pkg('fakemodem', 'dep',
                             os.path.join(self.autodir, 'deps', 'fakemodem'))
        subprocess.check_call(["modprobe", "tun"])
        subprocess.check_call(["initctl", "stop", "modemmanager"])

        dbus.mainloop.glib.DBusGMainLoop(set_as_default=True)
        self.main_loop = gobject.MainLoop()
        self.bus = dbus.SystemBus()
        self.bus.add_signal_receiver(self.SmsReceived,
                                     signal_name='SmsReceived')
        self.bus.add_signal_receiver(self.SmsCompleted,
                                     signal_name='Completed')

        try:
            paths = [os.path.join(self.srcdir, 'fake-gsm'),
                     os.path.join(self.srcdir, 'fake-icera')]
            with mmtest.ModemManagerTest(self.autodir, paths) as mmt:
                self.smstester = SmsTester(self.autodir, self.srcdir,
                                           mmt, self, self.main_loop)
                self.smstester.run(**kwargs)

            with mmtest.ModemManagerTest(self.autodir, paths) as mmt:
                self.smstester = SmsMultipartTester(self.autodir, self.srcdir,
                                                    mmt, self, self.main_loop)
                self.smstester.run(**kwargs)

        finally:
            subprocess.check_call(["initctl", "start", "modemmanager"])
            subprocess.check_call(["rmmod", "tun"])
