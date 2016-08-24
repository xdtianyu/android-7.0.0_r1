# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import dbus
import logging
import random

from autotest_lib.client.bin import test
from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.cros.cellular import test_environment
from autotest_lib.client.cros.cellular.pseudomodem import sim

# This is a software only test. Most time delayes are only dbus update delays.
DEFAULT_OPERATION_TIMEOUT=3

class network_SIMLocking(test.test):
    """
    Test the SIM locking functionality of shill.

    This test has the following test_cases:
      - Attempt to enable SIM lock with incorrect sim-pin. Verify that the
        attempt fails.
      - Successfully pin-lock the SIM.
      - Unlock a pin-locked SIM.
      - Attempt to unlock a pin-locked SIM with incorrect sim-pin, until it gets
        puk-locked.
      - Unblock a puk-locked SIM.
      - Attempt to unblock a puk-locked SIM with incorrect sim-puk, until the
        SIM gets blocked. At this point, a sim-pin2 might be expected by some
        SIMs. This test does not attempt to unlock the SIM using sim-pin2.
      - Test the functionality to change sim-pin.

    """

    version = 1

    def _bad_pin(self):
        """ Obtain a pin that does not match the valid sim-pin. """
        # Restricting the values to be  >= 1000 ensures four digit string.
        bad_pin = random.randint(1000, 9999)
        if str(bad_pin) == self.current_pin:
            bad_pin += 1
        return str(bad_pin)


    def _bad_puk(self):
        """ Obtain a puk that does not match the valid sim-puk. """
        # Restricting the values to be  >= 10000000 ensures 8 digit string.
        bad_puk = random.randint(10000000, 99999999)
        if str(bad_puk) == self.current_puk:
            bad_puk += 1
        return str(bad_puk)


    def _enter_incorrect_pin(self):
        try:
            self.device.EnterPin(self._bad_pin())
            raise error.TestFail('Cellular device did not complain although '
                                 'an incorrect pin was given')
        except dbus.DBusException as e:
            if e.get_dbus_name() == self.test_env.shill.ERROR_INCORRECT_PIN:
                logging.info('Obtained expected result: EnterPin failed with '
                             'incorrect PIN.')
            else:
                raise


    def _enter_incorrect_puk(self):
        try:
            self.device.UnblockPin(self._bad_puk(), self.current_pin)
            raise error.TestFail('Cellular device did not complain although '
                                 'an incorrect puk was given')
        except dbus.DBusException as e:
            if e.get_dbus_name() == self.test_env.shill.ERROR_INCORRECT_PIN:
                logging.info('Obtained expected result: UnblockPin failed with '
                             'incorrect PUK.')
            else:
                raise


    def _get_sim_lock_status(self):
        """ Helper method to safely obtain SIM lock status. """
        properties = self.device.GetProperties(utf8_strings=True)
        sim_lock_status = properties.get(
                self.test_env.shill.DEVICE_PROPERTY_SIM_LOCK_STATUS,
                None)
        if sim_lock_status is None:
            raise error.TestFail( 'Failed to read SIM_LOCK_STATUS.')
        return self.test_env.shill.dbus2primitive(sim_lock_status)


    def _is_sim_lock_enabled(self):
        """ Helper method to check if the SIM lock is enabled. """
        lock_status = self._get_sim_lock_status()
        lock_enabled = lock_status.get(
                self.test_env.shill.PROPERTY_KEY_SIM_LOCK_ENABLED,
                None)
        if lock_enabled is None:
            raise error.TestFail('Failed to find LockEnabled key in '
                                 'the lock status value.')
        return lock_enabled


    def _is_sim_pin_locked(self):
        """ Helper method to check if the SIM has been pin-locked. """
        lock_status = self._get_sim_lock_status()
        lock_type = lock_status.get(
                self.test_env.shill.PROPERTY_KEY_SIM_LOCK_TYPE,
                None)
        if lock_type is None:
            raise error.TestFail('Failed to find LockType key in the '
                                 'lock status value.')
        return lock_type == self.test_env.shill.VALUE_SIM_LOCK_TYPE_PIN


    def _is_sim_puk_locked(self):
        """ Helper method to check if the SIM has been puk-locked. """
        lock_status = self._get_sim_lock_status()
        lock_type = lock_status.get(
                self.test_env.shill.PROPERTY_KEY_SIM_LOCK_TYPE,
                None)
        if lock_type is None:
            raise error.TestFail('Failed to find LockType key in the '
                                 'lock status value.')
        return lock_type == self.test_env.shill.VALUE_SIM_LOCK_TYPE_PUK


    def _get_retries_left(self):
        """ Helper method to get the number of unlock retries left. """
        lock_status = self._get_sim_lock_status()
        retries_left = lock_status.get(
                self.test_env.shill.PROPERTY_KEY_SIM_LOCK_RETRIES_LEFT,
                None)
        if retries_left is None:
            raise error.TestFail('Failed to find LockRetriesLeft key '
                                 'in the lock status value.')
        if retries_left < 0:
            raise error.TestFail('Malformed RetriesLeft: %s' %
                                 str(retries_left))
        return retries_left

    def _reset_modem_with_sim_lock(self):
        """ Helper method to reset the modem with the SIM locked. """
        # When the SIM is locked, the enable operation fails and
        # hence set expect_powered flag to False.
        # The enable operation is deferred by Shill until the modem goes into
        # the disabled state after the SIM is unlocked.
        self.device, self.service = self.test_env.shill.reset_modem(
                self.device,
                expect_powered=False,
                expect_service=False)

    def _pin_lock_sim(self):
        """ Helper method to pin-lock a SIM, assuming nothing bad happens. """
        self.device.RequirePin(self.current_pin, True)
        self._reset_modem_with_sim_lock()
        if not self._is_sim_pin_locked():
            raise error.TestFail('Expected SIM to be locked after reset.')


    def _puk_lock_sim(self):
        """ Helper method to puk-lock a SIM, assuming nothing bad happens. """
        self._pin_lock_sim()
        while not self._is_sim_puk_locked():
            try:
                self._enter_incorrect_pin()
            except dbus.DBusException as e:
                if e.get_dbus_name() != self.test_env.shill.ERROR_PIN_BLOCKED:
                    raise
        if not self._is_sim_puk_locked():
            raise error.TestFail('Expected SIM to be puk-locked.')




    def test_unsuccessful_enable_lock(self):
        """ Test SIM lock enable failes with incorrect sim-pin. """
        logging.debug('Attempting to enable SIM lock with incorrect PIN.')
        try:
            self.device.RequirePin(self._bad_pin(), True)
            raise error.TestFail('Cellular device did not complain although '
                                 'an incorrect pin was given')
        except dbus.DBusException as e:
            if e.get_dbus_name() == self.test_env.shill.ERROR_INCORRECT_PIN:
                logging.info('Obtained expected result: pin-lock enable failed '
                             'with incorrect PIN.')
            else:
                raise

        if self._is_sim_lock_enabled():
            raise error.TestFail('SIM lock got enabled by incorrect PIN.')

        # SIM lock should not be enabled, and lock not set after reset.
        self.device, self.service = self.test_env.shill.reset_modem(self.device)
        self.test_env.shill.wait_for_property_in(self.service,
                                                 'state',
                                                 ['online'],
                                                 DEFAULT_OPERATION_TIMEOUT)
        if (self._is_sim_lock_enabled() or self._is_sim_pin_locked() or
            self._is_sim_puk_locked()):
            raise error.TestFail('Cellular device locked by an incorrect pin.')


    def test_cause_sim_pin_lock(self):
        """
        Test successfully enabling SIM lock and locking the SIM with
        pin-lock.

        """
        logging.debug('Attempting to enable SIM lock with correct pin.')
        self.device.RequirePin(self.current_pin, True)

        if not self._is_sim_lock_enabled():
            raise error.TestFail('SIM lock was not enabled by correct PIN.')

        self._reset_modem_with_sim_lock()
        # SIM lock should be enabled, and lock set after reset.
        if not self._is_sim_lock_enabled() or not self._is_sim_pin_locked():
            raise error.TestFail('Cellular device not locked after reset.')


    def test_unlock_sim_pin_lock(self):
        """
        Test successfully unlocking the SIM after it has been pin-locked.

        """
        # First, pin-lock the SIM.
        self._pin_lock_sim()

        retries_left = self._get_retries_left()
        self.device.EnterPin(self.current_pin)

        if self._is_sim_pin_locked():
            raise error.TestFail('Failed to unlock a pin-locked SIM with '
                                 'correct pin.')
        if not self._is_sim_lock_enabled():
            raise error.TestFail('SIM lock got disabled when attemping to'
                                 'unlock a pin-locked SIM.')
        if self._get_retries_left() != retries_left:
            raise error.TestFail('Unexpected change in number of retries left '
                                 'after a successful unlock of pin-locked SIM. '
                                 'retries before:%d, after:%d' %
                                 (retries_left, self._get_retries_left()))
        # The shill service reappears after the SIM is unlocked.
        # We need a fresh handle on the service.
        utils.poll_for_condition(
                lambda: self.test_env.shill.get_service_for_device(self.device))
        self.service = self.test_env.shill.get_service_for_device(self.device)
        self.test_env.shill.wait_for_property_in(self.service,
                                                 'state',
                                                 ['online'],
                                                 DEFAULT_OPERATION_TIMEOUT)


    def test_cause_sim_puk_lock(self):
        """ Test the flow that causes a SIM to be puk-locked. """
        # First, pin-lock the SIM.
        self._pin_lock_sim()

        # Expire all unlock pin-lock retries.
        retries_left = self._get_retries_left()
        if retries_left <= 0:
            raise error.TestFail('Expected a positive number of sim-puk '
                                 'retries.')

        while self._get_retries_left() > 1:
            # Don't execute the loop down to 0, as retries_left may be reset to
            # a higher value corresponding to the puk-lock retries.
            self._enter_incorrect_pin()
            if retries_left - self._get_retries_left() != 1:
                raise error.TestFail('RetriesLeft not decremented correctly by '
                                     'an attempt to unlock pin-lock with bad '
                                     'PIN.')
            retries_left = self._get_retries_left()

        # retries_left == 1
        try:
            self._enter_incorrect_pin()
            raise error.TestFail('Shill failed to throw PinBlocked error.')
        except dbus.DBusException as e:
            if e.get_dbus_name() != self.test_env.shill.ERROR_PIN_BLOCKED:
                raise

        # At this point, the SIM should be puk-locked.
        if not self._is_sim_lock_enabled() or not self._is_sim_puk_locked():
            raise error.TestFail('Could not puk-lock the SIM after sufficient '
                                 'incorrect attempts to unlock.')
        if not self._get_retries_left():
            raise error.TestFail('RetriesLeft not updated to puk-lock retries '
                                 'after the SIM got puk-locked.')


    def test_unlock_sim_puk_lock(self):
        """ Unlock a puk-locked SIM. """
        # First, puk-lock the SIM
        self._puk_lock_sim()

        retries_left = self._get_retries_left()
        self.device.UnblockPin(self.current_puk, self.current_pin)

        if self._is_sim_puk_locked():
            raise error.TestFail('Failed to unlock a puk-locked SIM with '
                                 'correct puk.')
        if self._is_sim_pin_locked():
            raise error.TestFail('pin-lock got unlocked while unlocking the '
                                 'puk-lock.')
        if not self._is_sim_lock_enabled():
            raise error.TestFail('SIM lock got disabled when attemping to'
                                 'unlock a pin-locked SIM.')

    def test_brick_sim(self):
        """ Test the flow that expires all pin-lock and puk-lock retries. """
        # First, puk-lock the SIM.
        self._puk_lock_sim()

        # Expire all unlock puk-lock retries.
        retries_left = self._get_retries_left()
        if retries_left <= 0:
            raise error.TestFail('Expected a positive number of sim-puk '
                                 'retries.')

        while self._get_retries_left() > 1:
            # Don't execute the loop down to 0, as the exception raised on the
            # last attempt is different.
            self._enter_incorrect_puk()
            if retries_left - self._get_retries_left() != 1:
                raise error.TestFail('RetriesLeft not decremented correctly by '
                                     'an attempt to unlock puk-lock with bad '
                                     'PUK.')
            retries_left = self._get_retries_left()

        # retries_left == 1
        try:
            self._enter_incorrect_puk()
            raise error.TestFail('Shill failed to throw SimFailure error.')
        except dbus.DBusException as e:
            if e.get_dbus_name() != self.test_env.shill.ERROR_FAILURE:
                raise


    def test_change_pin(self):
        """ Test changing pin successfully and unsuccessfully. """
        # The currently accepted behaviour of ChangePin is -- it succeeds if
        #  (1) SIM locking is enabled.
        #  (2) SIM is currently not locked.
        #  (3) The correct sim-pin is used as the old_pin argument in ChangePin.
        # ChangePin will fail in all other conditions. It sometimes fails
        # obviously, with an error. In other cases, it silently fails to change
        # the sim-pin.
        new_pin = self._bad_pin()
        # Attempt to change the sim-pin when SIM locking is not enabled.
        try:
            self.device.ChangePin(self.current_pin, new_pin)
            raise error.TestFail('Expected ChangePin to fail when SIM lock is '
                                 'not enabled.')
        except dbus.DBusException as e:
            if e.get_dbus_name() != self.test_env.shill.ERROR_FAILURE:
                raise

        self.device.RequirePin(self.current_pin, True)
        # Attempt to change the sim-pin with incorrect current sim_pin.
        try:
            self.device.ChangePin(self._bad_pin(), new_pin)
            raise error.TestFail('Expected ChangePin to fail with incorrect '
                                 'sim-pin.')
        except dbus.DBusException as e:
            if e.get_dbus_name() != self.test_env.shill.ERROR_INCORRECT_PIN:
                raise

        # Change sim-pin successfully.
        self.device.ChangePin(self.current_pin, new_pin)
        self.current_pin = new_pin
        self.device.RequirePin(self.current_pin, False)
        if self._is_sim_lock_enabled():
            raise error.TestFail('Expected to be able to disable SIM lock with '
                                 'the new sim-pin')


    def _run_internal(self, test_to_run):
        """
        Entry point to run all tests.

        @param test_to_run is a function that runs the required test.

        """
        self.current_pin = sim.SIM.DEFAULT_PIN
        self.current_puk = sim.SIM.DEFAULT_PUK

        # Resetting modemmanager invalidates the shill dbus object for the
        # modem.
        self.device = self.test_env.shill.find_cellular_device_object()
        if not self.device:
            raise error.TestFail('Failed to find a cellular device.')

        # Be a little cynical and make sure that SIM locks are as expected
        # before we begin.
        if (self._is_sim_lock_enabled() or self._is_sim_pin_locked() or
            self._is_sim_puk_locked()):
            raise error.TestFail(
                    'Cellular device in bad initial sim-lock state. '
                    'LockEnabled: %b, PinLocked:%b, PukLocked:%b.' %
                    (self._is_sim_lock_enabled(), self._is_sim_pin_locked(),
                     self._is_sim_puk_locked()))

        test_to_run()


    def run_once(self):
        """Entry function into the test."""
        random.seed()
        test_list = [self.test_unsuccessful_enable_lock,
                     self.test_cause_sim_pin_lock,
                     self.test_unlock_sim_pin_lock,
                     self.test_cause_sim_puk_lock,
                     self.test_unlock_sim_puk_lock,
                     self.test_brick_sim,
                     self.test_change_pin]

        # Some of these tests render the modem unusable, so run each test
        # with a fresh pseudomodem.
        for test in test_list:
            self.test_env = test_environment.CellularPseudoMMTestEnvironment(
                    pseudomm_args=({'family': '3GPP'},))
            with self.test_env:
                self._run_internal(test)
