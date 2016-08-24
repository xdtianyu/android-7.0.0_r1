#!/usr/bin/python
# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import mox
import pexpect
import unittest

import dli

import rpm_controller

import common
from autotest_lib.site_utils.rpm_control_system import utils


class TestRPMControllerQueue(mox.MoxTestBase):
    """Test request can be queued and processed in controller.
    """

    def setUp(self):
        super(TestRPMControllerQueue, self).setUp()
        self.rpm = rpm_controller.SentryRPMController('chromeos-rack1-host8')
        self.powerunit_info = utils.PowerUnitInfo(
                device_hostname='chromos-rack1-host8',
                powerunit_hostname='chromeos-rack1-rpm1',
                powerunit_type=utils.PowerUnitInfo.POWERUNIT_TYPES.RPM,
                outlet='.A100',
                hydra_hostname=None)


    def testQueueRequest(self):
        """Should create a new process to handle request."""
        new_state = 'ON'
        process = self.mox.CreateMockAnything()
        rpm_controller.multiprocessing.Process = self.mox.CreateMockAnything()
        rpm_controller.multiprocessing.Process(target=mox.IgnoreArg(),
                args=mox.IgnoreArg()).AndReturn(process)
        process.start()
        process.join()
        self.mox.ReplayAll()
        self.assertFalse(self.rpm.queue_request(self.powerunit_info, new_state))
        self.mox.VerifyAll()


class TestSentryRPMController(mox.MoxTestBase):
    """Test SentryRPMController."""


    def setUp(self):
        super(TestSentryRPMController, self).setUp()
        self.ssh = self.mox.CreateMockAnything()
        rpm_controller.pexpect.spawn = self.mox.CreateMockAnything()
        rpm_controller.pexpect.spawn(mox.IgnoreArg()).AndReturn(self.ssh)
        self.rpm = rpm_controller.SentryRPMController('chromeos-rack1-host8')
        self.powerunit_info = utils.PowerUnitInfo(
                device_hostname='chromos-rack1-host8',
                powerunit_hostname='chromeos-rack1-rpm1',
                powerunit_type=utils.PowerUnitInfo.POWERUNIT_TYPES.RPM,
                outlet='.A100',
                hydra_hostname=None)


    def testSuccessfullyChangeOutlet(self):
        """Should return True if change was successful."""
        prompt = 'Switched CDU:'
        password = 'admn'
        new_state = 'ON'
        self.ssh.expect('Password:', timeout=60)
        self.ssh.sendline(password)
        self.ssh.expect(prompt, timeout=60)
        self.ssh.sendline('%s %s' % (new_state, self.powerunit_info.outlet))
        self.ssh.expect('Command successful', timeout=60)
        self.ssh.sendline('logout')
        self.ssh.close(force=True)
        self.mox.ReplayAll()
        self.assertTrue(self.rpm.set_power_state(
                self.powerunit_info, new_state))
        self.mox.VerifyAll()


    def testUnsuccessfullyChangeOutlet(self):
        """Should return False if change was unsuccessful."""
        prompt = 'Switched CDU:'
        password = 'admn'
        new_state = 'ON'
        self.ssh.expect('Password:', timeout=60)
        self.ssh.sendline(password)
        self.ssh.expect(prompt, timeout=60)
        self.ssh.sendline('%s %s' % (new_state, self.powerunit_info.outlet))
        self.ssh.expect('Command successful',
                        timeout=60).AndRaise(pexpect.TIMEOUT('Timed Out'))
        self.ssh.sendline('logout')
        self.ssh.close(force=True)
        self.mox.ReplayAll()
        self.assertFalse(self.rpm.set_power_state(self.powerunit_info, new_state))
        self.mox.VerifyAll()


class TestWebPoweredRPMController(mox.MoxTestBase):
    """Test WebPoweredRPMController."""


    def setUp(self):
        super(TestWebPoweredRPMController, self).setUp()
        self.dli_ps = self.mox.CreateMock(dli.powerswitch)
        hostname = 'chromeos-rack8a-rpm1'
        self.web_rpm = rpm_controller.WebPoweredRPMController(hostname,
                                                              self.dli_ps)
        outlet = 8
        dut = 'chromeos-rack8a-host8'
        # Outlet statuses are in the format "u'ON'"
        initial_state = 'u\'ON\''
        self.test_status_list_initial = [[outlet, dut, initial_state]]
        self.powerunit_info = utils.PowerUnitInfo(
                device_hostname=dut,
                powerunit_hostname=hostname,
                powerunit_type=utils.PowerUnitInfo.POWERUNIT_TYPES.RPM,
                outlet=outlet,
                hydra_hostname=None)


    def testSuccessfullyChangeOutlet(self):
        """Should return True if change was successful."""
        test_status_list_final = [[8, 'chromeos-rack8a-host8','u\'OFF\'']]
        self.dli_ps.statuslist().AndReturn(self.test_status_list_initial)
        self.dli_ps.off(8)
        self.dli_ps.statuslist().AndReturn(test_status_list_final)
        self.mox.ReplayAll()
        self.assertTrue(self.web_rpm.set_power_state(
                self.powerunit_info, 'OFF'))
        self.mox.VerifyAll()


    def testUnsuccessfullyChangeOutlet(self):
        """Should return False if Outlet State does not change."""
        test_status_list_final = [[8, 'chromeos-rack8a-host8','u\'ON\'']]
        self.dli_ps.statuslist().AndReturn(self.test_status_list_initial)
        self.dli_ps.off(8)
        self.dli_ps.statuslist().AndReturn(test_status_list_final)
        self.mox.ReplayAll()
        self.assertFalse(self.web_rpm.set_power_state(
                self.powerunit_info, 'OFF'))
        self.mox.VerifyAll()


    def testNoOutlet(self):
        """Should return False if DUT hostname is not on the RPM device."""
        self.powerunit_info.outlet=None
        self.assertFalse(self.web_rpm.set_power_state(
                self.powerunit_info, 'OFF'))


class TestCiscoPOEController(mox.MoxTestBase):
    """Test CiscoPOEController."""


    STREAM_WELCOME = 'This is a POE switch.\n\nUser Name:'
    STREAM_PWD = 'Password:'
    STREAM_DEVICE = '\nchromeos2-poe-sw8#'
    STREAM_CONFIG = 'chromeos2-poe-sw8(config)#'
    STREAM_CONFIG_IF = 'chromeos2-poe-sw8(config-if)#'
    STREAM_STATUS = ('\n                                             '
                     'Flow Link          Back   Mdix\n'
                     'Port     Type         Duplex  Speed Neg      '
                     'ctrl State       Pressure Mode\n'
                     '-------- ------------ ------  ----- -------- '
                     '---- ----------- -------- -------\n'
                     'fa32     100M-Copper  Full    100   Enabled  '
                     'Off  Up          Disabled Off\n')
    SERVO = 'chromeos1-rack3-host12-servo'
    SWITCH = 'chromeos2-poe-switch8'
    PORT = 'fa32'
    POWERUNIT_INFO = utils.PowerUnitInfo(
            device_hostname=PORT,
            powerunit_hostname=SERVO,
            powerunit_type=utils.PowerUnitInfo.POWERUNIT_TYPES.POE,
            outlet=PORT,
            hydra_hostname=None)


    def setUp(self):
        super(TestCiscoPOEController, self).setUp()
        self.mox.StubOutWithMock(pexpect.spawn, '_spawn')
        self.mox.StubOutWithMock(pexpect.spawn, 'read_nonblocking')
        self.mox.StubOutWithMock(pexpect.spawn, 'sendline')
        self.poe = rpm_controller.CiscoPOEController(self.SWITCH)
        pexpect.spawn._spawn(mox.IgnoreArg(), mox.IgnoreArg())
        pexpect.spawn.read_nonblocking(
                mox.IgnoreArg(), mox.IgnoreArg()).AndReturn(self.STREAM_WELCOME)
        pexpect.spawn.sendline(self.poe._username)
        pexpect.spawn.read_nonblocking(
                mox.IgnoreArg(), mox.IgnoreArg()).AndReturn(self.STREAM_PWD)
        pexpect.spawn.sendline(self.poe._password)
        pexpect.spawn.read_nonblocking(
                mox.IgnoreArg(), mox.IgnoreArg()).AndReturn(self.STREAM_DEVICE)


    def testLogin(self):
        """Test we can log into the switch."""
        self.mox.ReplayAll()
        self.assertNotEqual(self.poe._login(), None)
        self.mox.VerifyAll()


    def _EnterConfigurationHelper(self, success=True):
        """A helper function for testing entering configuration terminal.

        @param success: True if we want the process to pass, False if we
                        want it to fail.
        """
        pexpect.spawn.sendline('configure terminal')
        pexpect.spawn.read_nonblocking(
                mox.IgnoreArg(), mox.IgnoreArg()).AndReturn(self.STREAM_CONFIG)
        pexpect.spawn.sendline('interface %s' % self.PORT)
        if success:
            pexpect.spawn.read_nonblocking(
                    mox.IgnoreArg(),
                    mox.IgnoreArg()).AndReturn(self.STREAM_CONFIG_IF)
        else:
            self.mox.StubOutWithMock(pexpect.spawn, '__str__')
            exception = pexpect.TIMEOUT(
                    'Could not enter configuration terminal.')
            pexpect.spawn.read_nonblocking(
                    mox.IgnoreArg(),
                    mox.IgnoreArg()).MultipleTimes().AndRaise(exception)
            pexpect.spawn.__str__().AndReturn('A pexpect.spawn object.')
            pexpect.spawn.sendline('end')


    def testSuccessfullyChangeOutlet(self):
        """Should return True if change was successful."""
        self._EnterConfigurationHelper()
        pexpect.spawn.sendline('power inline auto')
        pexpect.spawn.sendline('end')
        pexpect.spawn.read_nonblocking(
                mox.IgnoreArg(), mox.IgnoreArg()).AndReturn(self.STREAM_DEVICE)
        pexpect.spawn.sendline('show interface status %s' % self.PORT)
        pexpect.spawn.read_nonblocking(
                mox.IgnoreArg(), mox.IgnoreArg()).AndReturn(self.STREAM_STATUS)
        pexpect.spawn.read_nonblocking(
                mox.IgnoreArg(), mox.IgnoreArg()).AndReturn(self.STREAM_DEVICE)
        pexpect.spawn.sendline('exit')
        self.mox.ReplayAll()
        self.assertTrue(self.poe.set_power_state(self.POWERUNIT_INFO, 'ON'))
        self.mox.VerifyAll()


    def testUnableToEnterConfigurationTerminal(self):
        """Should return False if unable to enter configuration terminal."""
        self._EnterConfigurationHelper(success=False)
        pexpect.spawn.sendline('exit')
        self.mox.ReplayAll()
        self.assertFalse(self.poe.set_power_state(self.POWERUNIT_INFO, 'ON'))
        self.mox.VerifyAll()


    def testUnableToExitConfigurationTerminal(self):
        """Should return False if unable to exit configuration terminal."""
        self.mox.StubOutWithMock(pexpect.spawn, '__str__')
        self.mox.StubOutWithMock(rpm_controller.CiscoPOEController,
                                 '_enter_configuration_terminal')
        self.poe._enter_configuration_terminal(
                mox.IgnoreArg(), mox.IgnoreArg()).AndReturn(True)
        pexpect.spawn.sendline('power inline auto')
        pexpect.spawn.sendline('end')
        exception = pexpect.TIMEOUT('Could not exit configuration terminal.')
        pexpect.spawn.read_nonblocking(
                mox.IgnoreArg(),
                mox.IgnoreArg()).MultipleTimes().AndRaise(exception)
        pexpect.spawn.__str__().AndReturn('A pexpect.spawn object.')
        pexpect.spawn.sendline('exit')
        self.mox.ReplayAll()
        self.assertFalse(self.poe.set_power_state(self.POWERUNIT_INFO, 'ON'))
        self.mox.VerifyAll()


    def testUnableToVerifyState(self):
        """Should return False if unable to verify current state."""
        self.mox.StubOutWithMock(pexpect.spawn, '__str__')
        self.mox.StubOutWithMock(rpm_controller.CiscoPOEController,
                                 '_enter_configuration_terminal')
        self.mox.StubOutWithMock(rpm_controller.CiscoPOEController,
                                 '_exit_configuration_terminal')
        self.poe._enter_configuration_terminal(
                mox.IgnoreArg(), mox.IgnoreArg()).AndReturn(True)
        pexpect.spawn.sendline('power inline auto')
        self.poe._exit_configuration_terminal(mox.IgnoreArg()).AndReturn(True)
        pexpect.spawn.sendline('show interface status %s' % self.PORT)
        exception = pexpect.TIMEOUT('Could not verify state.')
        pexpect.spawn.read_nonblocking(
                mox.IgnoreArg(),
                mox.IgnoreArg()).MultipleTimes().AndRaise(exception)
        pexpect.spawn.__str__().AndReturn('A pexpect.spawn object.')
        pexpect.spawn.sendline('exit')
        self.mox.ReplayAll()
        self.assertFalse(self.poe.set_power_state(self.POWERUNIT_INFO, 'ON'))
        self.mox.VerifyAll()


if __name__ == "__main__":
    unittest.main()
