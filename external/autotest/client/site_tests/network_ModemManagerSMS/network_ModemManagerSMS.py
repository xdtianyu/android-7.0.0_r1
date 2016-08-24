# Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging, os, subprocess
import dbus

from autotest_lib.client.bin import test
from autotest_lib.client.common_lib import error
from autotest_lib.client.cros.cellular import sms, mmtest

class network_ModemManagerSMS(test.test):
    version = 1

    def setup(self):
        self.job.setup_dep(['fakegudev', 'fakemodem'])

    def run_sms_test(self, testfunc, *args, **kwargs):
        paths = [os.path.join(self.srcdir, 'fake-gsm'),
                 os.path.join(self.srcdir, 'fake-icera')]

        with mmtest.ModemManagerTest(self.autodir, paths) as mmt:
            smsstore = sms.SmsStore(mmt.fakemodem)
            gsmsms = mmt.mm.GsmSms(mmt.modem_object_path)
            smstest = sms.SmsTest(gsmsms)

            testfunc(smsstore, smstest, *args, **kwargs)

    def test_sms_zero(self, smsstore, smstest):
        # leave smsstore empty
        smstest.test_has_none()

    def test_sms_one(self, smsstore, smstest):
        testsms = sms.sample
        smsstore.sms_insert(1, testsms['pdu'])
        smstest.test_has_one(testsms['parsed'])
        smsstore.sms_remove(1)
        smstest.test_has_none()

    def test_sms_arrive(self, smsstore, smstest):
        smstest.test_has_none()
        testsms = sms.sample
        smsstore.sms_receive(1, testsms['pdu'])
        # Note: this test doesn't check for the DBus signals that
        # are supposed to be sent when a new message arrives.
        # See network_ModemManagerSMSSignal for that.
        smstest.test_has_one(testsms['parsed'])
        smsstore.sms_remove(1)
        smstest.test_has_none()

    def test_sms_multipart_existing(self, smsstore, smstest):
        testsms = sms.sample_multipart
        smsstore.sms_insert(1, testsms['pdu'][0])
        smsstore.sms_insert(2, testsms['pdu'][1])
        smstest.test_has_one(testsms['parsed'])
        smsstore.sms_remove(1)
        smsstore.sms_remove(2)
        smstest.test_has_none()

    def test_sms_multipart_receive(self, smsstore, smstest):
        smstest.test_has_none()
        testsms = sms.sample_multipart
        smsstore.sms_receive(1, testsms['pdu'][0])
        # Can't use test_has_none() here because it will delete the
        # partial message
        smstest.test_list([])
        smstest.test_get(1, None)
        smsstore.sms_receive(2, testsms['pdu'][1])
        smstest.test_has_one(testsms['parsed'])
        smsstore.sms_remove(1)
        smsstore.sms_remove(2)
        smstest.test_has_none()

    def test_sms_multipart_reverse(self, smsstore, smstest):
        smstest.test_has_none()
        testsms = sms.sample_multipart
        smsstore.sms_receive(1, testsms['pdu'][1])
        # Can't use test_sms_has_none() here because it will delete the
        # partial message
        smstest.test_list([])
        smstest.test_get(1, None)
        smsstore.sms_receive(2, testsms['pdu'][0])
        smstest.test_has_one(testsms['parsed'])
        smsstore.sms_remove(1)
        smsstore.sms_remove(2)
        smstest.test_has_none()

    def run_once(self):
        self.job.install_pkg('fakegudev', 'dep',
                             os.path.join(self.autodir, 'deps', 'fakegudev'))
        self.job.install_pkg('fakemodem', 'dep',
                             os.path.join(self.autodir, 'deps', 'fakemodem'))

        subprocess.check_call(["modprobe", "tun"])
        subprocess.check_call(["initctl", "stop", "modemmanager"])

        try:
            self.run_sms_test(self.test_sms_zero)
            self.run_sms_test(self.test_sms_one)
            self.run_sms_test(self.test_sms_arrive)
            self.run_sms_test(self.test_sms_multipart_existing)
            self.run_sms_test(self.test_sms_multipart_receive)
            self.run_sms_test(self.test_sms_multipart_reverse)

        finally:
            subprocess.check_call(["initctl", "start", "modemmanager"])
            subprocess.check_call(["rmmod", "tun"])
