# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import collections
import logging
import random

from time import sleep

import common
from autotest_lib.client.common_lib import error
from autotest_lib.server import hosts
from autotest_lib.server import frontend
from autotest_lib.server import site_utils
from autotest_lib.server.cros.dynamic_suite import constants
from autotest_lib.server.cros.network import wifi_client

# Max number of retry attempts to lock a DUT.
MAX_RETRIES = 3

# Tuple containing the DUT objects
DUTObject = collections.namedtuple('DUTObject', ['host', 'wifi_client'])

class DUTSpec():
    """Object to specify the DUT spec.

    @attribute board_name: String representing the board name corresponding to
                           the board.
    @attribute host_name: String representing the host name corresponding to
                          the machine.
    """

    def __init__(self, board_name=None, host_name=None):
        """Initialize.

        @param board_name: String representing the board name corresponding to
                           the board.
        @param host_name: String representing the host name corresponding to
                          the machine.
        """
        self.board_name = board_name
        self.host_name = host_name

    def __repr__(self):
        """@return class name, dut host name, lock status and retries."""
        return 'class: %s, Board name: %s, Num DUTs = %d' % (
                self.__class__.__name__,
                self.board_name,
                self.host_name)


class DUTSetSpec(list):
    """Object to specify the DUT set spec. It's a list of DUTSpec objects."""

    def __init__(self):
        """Initialize."""
        super(DUTSetSpec, self)


class DUTPoolSpec(list):
    """Object to specify the DUT pool spec.It's a list of DUTSetSpec objects."""

    def __init__(self):
        """Initialize."""
        super(DUTPoolSpec, self)


class DUTLocker(object):
    """Object to keep track of DUT lock state.

    @attribute dut_spec: an DUTSpec object corresponding to the DUT we need.
    @attribute retries: an integer, max number of retry attempts to lock DUT.
    @attribute to_be_locked: a boolean, True iff DUT has not been locked.
    """


    def __init__(self, dut_spec, retries):
        """Initialize.

        @param dut_spec: a DUTSpec object corresponding to the spec of the DUT
                         to be locked.
        @param retries: an integer, max number of retry attempts to lock DUT.
        """
        self.dut_spec = dut_spec
        self.retries = retries
        self.to_be_locked = True

    def __repr__(self):
        """@return class name, dut host name, lock status and retries."""
        return 'class: %s, host name: %s, to_be_locked = %s, retries = %d' % (
                self.__class__.__name__,
                self.dut.host.hostname,
                self.to_be_locked,
                self.retries)


class CliqueDUTBatchLocker(object):
    """Object to lock/unlock an DUT.

    @attribute SECONDS_TO_SLEEP: an integer, number of seconds to sleep between
                                 retries.
    @attribute duts_to_lock: a list of DUTLocker objects.
    @attribute locked_duts: a list of DUTObject's corresponding to DUT's which
                            have already been allocated.
    @attribute manager: a HostLockManager object, used to lock/unlock DUTs.
    """

    MIN_SECONDS_TO_SLEEP = 30
    MAX_SECONDS_TO_SLEEP = 120

    def __init__(self, lock_manager, dut_pool_spec, retries=MAX_RETRIES):
        """Initialize.

        @param lock_manager: a HostLockManager object, used to lock/unlock DUTs.
        @param dut_pool_spec: A DUTPoolSpec object corresponding to the DUT's in
                              the pool.
        @param retries: Number of times to retry the locking of DUT's.

        """
        self.lock_manager = lock_manager
        self.duts_to_lock = self._construct_dut_lockers(dut_pool_spec, retries)
        self.locked_duts = []

    @staticmethod
    def _construct_dut_lockers(dut_pool_spec, retries):
        """Convert DUTObject objects to DUTLocker objects for locking.

        @param dut_pool_spec: A DUTPoolSpec object corresponding to the DUT's in
                              the pool.
        @param retries: an integer, max number of retry attempts to lock DUT.

        @return a list of DUTLocker objects.
        """
        dut_lockers_list = []
        for dut_set_spec in dut_pool_spec:
            dut_set_lockers_list = []
            for dut_spec in dut_set_spec:
                dut_locker = DUTLocker(dut_spec, retries)
                dut_set_lockers_list.append(dut_locker)
            dut_lockers_list.append(dut_set_lockers_list)
        return dut_lockers_list

    def _allocate_dut(self, host_name=None, board_name=None):
        """Allocates a machine to the DUT pool for running the test.

        Locks the allocated machine if the machine was discovered via AFE
        to prevent tests stomping on each other.

        @param host_name: Host name for the DUT.
        @param board_name: Board name Label to use for finding the DUT.

        @return: hostname of the device locked in AFE.
        """
        hostname = None
        if host_name:
            if self.lock_manager.lock([host_name]):
                logging.info('Locked device %s.', host_name)
                hostname = host_name
            else:
                logging.error('Unable to lock device %s.', host_name)
        else:
            afe = frontend.AFE(debug=True,
                               server=site_utils.get_global_afe_hostname())
            labels = []
            labels.append(constants.BOARD_PREFIX + board_name)
            labels.append('clique_dut')
            try:
                hostname = site_utils.lock_host_with_labels(
                        afe, self.lock_manager, labels=labels) + '.cros'
            except error.NoEligibleHostException as e:
                raise error.TestError("Unable to find a suitable device.")
            except error.TestError as e:
                logging.error(e)
        return hostname

    @staticmethod
    def _create_dut_object(host_name):
        """Create the DUTObject tuple for the DUT.

        @param host_name: Host name for the DUT.

        @return: Tuple of Host and Wifi client objects representing DUTObject
                 for invoking RPC calls.
        """
        dut_host = hosts.create_host(host_name)
        dut_wifi_client = wifi_client.WiFiClient(dut_host, './debug', False)
        return DUTObject(dut_host, dut_wifi_client)

    def _lock_dut_in_afe(self, dut_locker):
        """Locks an DUT host in AFE.

        @param dut_locker: an DUTLocker object, DUT to be locked.
        @return a hostname iff dut_locker is locked, else returns None.
        """
        logging.debug('Trying to find a device with spec (%s, %s)',
                      dut_locker.dut_spec.host_name,
                      dut_locker.dut_spec.board_name)
        host_name = self._allocate_dut(
            dut_locker.dut_spec.host_name, dut_locker.dut_spec.board_name)
        if host_name:
            logging.info('Locked %s', host_name)
            dut_locker.to_be_locked = False
        else:
            dut_locker.retries -= 1
            logging.info('%d retries left for (%s, %s)',
                         dut_locker.retries,
                         dut_locker.dut_spec.host_name,
                         dut_locker.dut_spec.board_name)
            if dut_locker.retries == 0:
                raise error.TestError("No more retries left to lock a "
                                      "suitable device.")
        return host_name

    def get_dut_pool(self):
        """Allocates a batch of locked DUTs for the test.

        @return a list of DUTObject, locked on AFE.
        """
        # We need this while loop to continuously loop over the for loop.
        # To exit the while loop, we either:
        #  - locked batch_size number of duts and return them
        #  - exhausted all retries on a dut in duts_to_lock

        # It is important to preserve the order of DUT sets, but the order of
        # DUT's within the set is not important as all the DUT's within a set
        # have to perform the same role.
        dut_pool = []
        for dut_set in self.duts_to_lock:
            dut_pool.append([])

        num_duts_to_lock = sum(map(len, self.duts_to_lock))
        while num_duts_to_lock:
            set_num = 0
            for dut_locker_set in self.duts_to_lock:
                for dut_locker in dut_locker_set:
                    if dut_locker.to_be_locked:
                        host_name = self._lock_dut_in_afe(dut_locker)
                        if host_name:
                            dut_object = self._create_dut_object(host_name)
                            self.locked_duts.append(dut_object)
                            dut_pool[set_num].append(dut_object)
                            num_duts_to_lock -= 1
                set_num += 1

            logging.info('Remaining DUTs to lock: %d', num_duts_to_lock)

            if num_duts_to_lock:
                seconds_to_sleep = random.randint(self.MIN_SECONDS_TO_SLEEP,
                                                  self.MAX_SECONDS_TO_SLEEP)
                logging.debug('Sleep %d sec before retry', seconds_to_sleep)
                sleep(seconds_to_sleep)
        return dut_pool

    def _unlock_one_dut(self, dut):
        """Unlock one DUT after we're done.

        @param dut: a DUTObject corresponding to the DUT.
        """
        host_name = dut.host.host_name
        if self.manager.unlock(hosts=[host_name]):
            self._locked_duts.remove(dut)
        else:
            logging.error('Tried to unlock a host we have not locked (%s)?',
                           host_name)

    def unlock_duts(self):
        """Unlock DUTs after we're done."""
        for dut in self.locked_duts:
            self._unlock_one_dut(dut)

    def unlock_and_close_duts(self):
        """Unlock DUTs after we're done and close the associated WifiClient."""
        for dut in self.locked_duts:
            dut.wifi_client.close()
            self._unlock_one_dut(dut)
