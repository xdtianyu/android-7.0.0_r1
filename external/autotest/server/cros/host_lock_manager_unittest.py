#!/usr/bin/python
#
# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Unit tests for server/cros/host_lock_manager.py."""

import mox
import unittest
import common

from autotest_lib.server import frontend
from autotest_lib.server.cros import host_lock_manager

class HostLockManagerTest(mox.MoxTestBase):
    """Unit tests for host_lock_manager.HostLockManager.

    @attribute HOST1: a string, fake host.
    @attribute HOST2: a string, fake host.
    @attribute HOST3: a string, fake host.
    """

    HOST1 = 'host1'
    HOST2 = 'host2'
    HOST3 = 'host3'


    class FakeHost(object):
        """Fake version of Host object defiend in server/frontend.py.

        @attribute locked: a boolean, True == host is locked.
        @attribute locked_by: a string, fake user.
        @attribute lock_time: a string, fake timestamp.
        """

        def __init__(self, locked=False):
            """Initialize.

            @param locked: a boolean, True == host is locked.
            """
            self.locked = locked
            self.locked_by = 'fake_user'
            self.lock_time = 'fake time'


    class MockHostLockManager(host_lock_manager.HostLockManager):
        """Mock out _host_modifier() in HostLockManager class..

        @attribute locked: a boolean, True == host is locked.
        @attribute locked_by: a string, fake user.
        @attribute lock_time: a string, fake timestamp.
        """

        def _host_modifier(self, hosts, operation, lock_reason=''):
            """Overwrites original _host_modifier().

            Add hosts to self.locked_hosts for LOCK and remove hosts from
            self.locked_hosts for UNLOCK.

            @param a set of strings, host names.
            @param operation: a string, LOCK or UNLOCK.
            @param lock_reason: a string, a reason for locking the hosts
            """
            if operation == self.LOCK:
                assert lock_reason
                self.locked_hosts = self.locked_hosts.union(hosts)
            elif operation == self.UNLOCK:
                self.locked_hosts = self.locked_hosts.difference(hosts)


    def setUp(self):
        super(HostLockManagerTest, self).setUp()
        self.afe = self.mox.CreateMock(frontend.AFE)
        self.manager = host_lock_manager.HostLockManager(self.afe)


    def testCheckHost_SkipsUnknownHost(self):
        """Test that host unknown to AFE is skipped."""
        self.afe.get_hosts(hostname=self.HOST1).AndReturn(None)
        self.mox.ReplayAll()
        actual = self.manager._check_host(self.HOST1, None)
        self.assertEquals(None, actual)


    def testCheckHost_DetectsLockedHost(self):
        """Test that a host which is already locked is skipped."""
        host_info = [self.FakeHost(locked=True)]
        self.afe.get_hosts(hostname=self.HOST1).AndReturn(host_info)
        self.mox.ReplayAll()
        actual = self.manager._check_host(self.HOST1, self.manager.LOCK)
        self.assertEquals(None, actual)


    def testCheckHost_DetectsUnlockedHost(self):
        """Test that a host which is already unlocked is skipped."""
        host_info = [self.FakeHost()]
        self.afe.get_hosts(hostname=self.HOST1).AndReturn(host_info)
        self.mox.ReplayAll()
        actual = self.manager._check_host(self.HOST1, self.manager.UNLOCK)
        self.assertEquals(None, actual)


    def testCheckHost_ReturnsHostToLock(self):
        """Test that a host which can be locked is returned."""
        host_info = [self.FakeHost()]
        self.afe.get_hosts(hostname=self.HOST1).AndReturn(host_info)
        self.mox.ReplayAll()
        host_with_dot = '.'.join([self.HOST1, 'cros'])
        actual = self.manager._check_host(host_with_dot, self.manager.LOCK)
        self.assertEquals(self.HOST1, actual)


    def testCheckHost_ReturnsHostToUnlock(self):
        """Test that a host which can be unlocked is returned."""
        host_info = [self.FakeHost(locked=True)]
        self.afe.get_hosts(hostname=self.HOST1).AndReturn(host_info)
        self.mox.ReplayAll()
        host_with_dot = '.'.join([self.HOST1, 'cros'])
        actual = self.manager._check_host(host_with_dot, self.manager.UNLOCK)
        self.assertEquals(self.HOST1, actual)


    def testLock_WithNonOverlappingHosts(self):
        """Tests host locking, all hosts not in self.locked_hosts."""
        hosts = [self.HOST2]
        manager = self.MockHostLockManager(self.afe)
        manager.locked_hosts = set([self.HOST1])
        manager.lock(hosts, lock_reason='Locking for test')
        self.assertEquals(set([self.HOST1, self.HOST2]), manager.locked_hosts)


    def testLock_WithPartialOverlappingHosts(self):
        """Tests host locking, some hosts not in self.locked_hosts."""
        hosts = [self.HOST1, self.HOST2]
        manager = self.MockHostLockManager(self.afe)
        manager.locked_hosts = set([self.HOST1, self.HOST3])
        manager.lock(hosts, lock_reason='Locking for test')
        self.assertEquals(set([self.HOST1, self.HOST2, self.HOST3]),
                          manager.locked_hosts)


    def testLock_WithFullyOverlappingHosts(self):
        """Tests host locking, all hosts in self.locked_hosts."""
        hosts = [self.HOST1, self.HOST2]
        self.manager.locked_hosts = set(hosts)
        self.manager.lock(hosts)
        self.assertEquals(set(hosts), self.manager.locked_hosts)


    def testUnlock_WithNonOverlappingHosts(self):
        """Tests host unlocking, all hosts not in self.locked_hosts."""
        hosts = [self.HOST2]
        self.manager.locked_hosts = set([self.HOST1])
        self.manager.unlock(hosts)
        self.assertEquals(set([self.HOST1]), self.manager.locked_hosts)


    def testUnlock_WithPartialOverlappingHosts(self):
        """Tests host locking, some hosts not in self.locked_hosts."""
        hosts = [self.HOST1, self.HOST2]
        manager = self.MockHostLockManager(self.afe)
        manager.locked_hosts = set([self.HOST1, self.HOST3])
        manager.unlock(hosts)
        self.assertEquals(set([self.HOST3]), manager.locked_hosts)


    def testUnlock_WithFullyOverlappingHosts(self):
        """Tests host locking, all hosts in self.locked_hosts."""
        hosts = [self.HOST1, self.HOST2]
        manager = self.MockHostLockManager(self.afe)
        manager.locked_hosts = set([self.HOST1, self.HOST2, self.HOST3])
        manager.unlock(hosts)
        self.assertEquals(set([self.HOST3]), manager.locked_hosts)


    def testHostModifier_WithHostsToLock(self):
        """Test host locking."""
        hosts = set([self.HOST1])
        self.manager.locked_hosts = set([self.HOST2])
        self.mox.StubOutWithMock(self.manager, '_check_host')
        self.manager._check_host(self.HOST1,
                                 self.manager.LOCK).AndReturn(self.HOST1)
        self.afe.run('modify_hosts',
                     host_filter_data={'hostname__in': [self.HOST1]},
                     update_data={'locked': True, 'lock_reason': 'Test'})
        self.mox.ReplayAll()
        self.manager._host_modifier(hosts, self.manager.LOCK,
                                    lock_reason='Test')
        self.assertEquals(set([self.HOST1, self.HOST2]),
                          self.manager.locked_hosts)


    def testHostModifier_WithHostsToUnlock(self):
        """Test host unlocking."""
        hosts = set([self.HOST1])
        self.manager.locked_hosts = set([self.HOST1, self.HOST2])
        self.mox.StubOutWithMock(self.manager, '_check_host')
        self.manager._check_host(self.HOST1,
                                 self.manager.UNLOCK).AndReturn(self.HOST1)
        self.afe.run('modify_hosts',
                     host_filter_data={'hostname__in': [self.HOST1]},
                     update_data={'locked': False})
        self.mox.ReplayAll()
        self.manager._host_modifier(hosts, self.manager.UNLOCK)
        self.assertEquals(set([self.HOST2]), self.manager.locked_hosts)


    def testHostModifier_WithoutLockReason(self):
        """Test host locking without providing a lock reason."""
        hosts = set([self.HOST1])
        self.manager.locked_hosts = set([self.HOST2])
        self.mox.StubOutWithMock(self.manager, '_check_host')
        self.manager._check_host(self.HOST1,
                                 self.manager.LOCK).AndReturn(self.HOST1)
        self.afe.run('modify_hosts',
                     host_filter_data={'hostname__in': [self.HOST1]},
                     update_data={'locked': True,
                                  'lock_reason': None})
        self.mox.ReplayAll()
        self.manager._host_modifier(hosts, self.manager.LOCK)
        self.assertEquals(set([self.HOST2]), self.manager.locked_hosts)


if __name__ == '__main__':
    unittest.main()
