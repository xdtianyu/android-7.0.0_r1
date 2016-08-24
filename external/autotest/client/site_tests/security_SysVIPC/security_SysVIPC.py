# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import os
import re

from collections import namedtuple

from autotest_lib.client.bin import test, utils
from autotest_lib.client.common_lib import error

ShmRecord = namedtuple('ShmRecord', ['owner', 'perms', 'attached'])
SemaphoreRecord = namedtuple('SemaphoreRecord', ['owner', 'perms'])

class security_SysVIPC(test.test):
    """Detect emergence of new attack surfaces in SysV IPC."""
    version = 1
    expected_shm = set([ShmRecord(owner='cras', perms='640',
                                  attached=('/usr/bin/cras',))])
    expected_sem = set([SemaphoreRecord(owner='root', perms='600')])

    def dump_ipcs_to_results(self):
        """Writes a copy of the 'ipcs' output to the autotest results dir."""
        utils.system_output('ipcs > "%s/ipcs-output.txt"' % self.resultsdir)


    def find_attached(self, shmid):
        """Find programs attached to a given shared memory segment.

        Returns full paths to each program identified.

        Args:
          @param shmid: the id as shown in ipcs and related utilities.
        """
        # This finds /proc/*/exe entries where maps shows they have
        # attached to the specified shm segment.
        cmd = 'grep "%s */SYSV" /proc/*/maps | sed "s/maps.*/exe/g"' % shmid
        # Then we just need to readlink each of the links. Even though
        # we ultimately convert to a sorted tuple, we use a set to avoid
        # accumulating duplicates as we go along.
        exes = set()
        for link in utils.system_output(cmd).splitlines():
            exes.add(os.readlink(link))
        return tuple(sorted(exes))


    def observe_shm(self):
        """Return a set of ShmRecords representing current system shm usage."""
        seen = set()
        cmd = 'ipcs -m | grep ^0'
        for line in utils.system_output(cmd, ignore_status=True).splitlines():
            fields = re.split('\s+', line)
            shmid = fields[1]
            owner = fields[2]
            perms = fields[3]
            attached = self.find_attached(shmid)
            seen.add(ShmRecord(owner=owner, perms=perms, attached=attached))
        return seen


    def observe_sems(self):
        """Return a set of SemaphoreRecords representing current usage."""
        seen = set()
        cmd = 'ipcs -s | grep ^0'
        for line in utils.system_output(cmd, ignore_status=True).splitlines():
            fields = re.split('\s+', line)
            seen.add(SemaphoreRecord(owner=fields[2], perms=fields[3]))
        return seen


    def run_once(self):
        """Main entry point to run the security_SysVIPC autotest."""
        test_fail = False
        self.dump_ipcs_to_results()
        # Check Shared Memory.
        observed_shm = self.observe_shm()
        missing = self.expected_shm.difference(observed_shm)
        extra = observed_shm.difference(self.expected_shm)
        if missing:
            logging.error('Expected shm(s) not found:')
            logging.error(missing)
        if extra:
            test_fail = True
            logging.error('Unexpected shm(s) found:')
            logging.error(extra)

        # Check Semaphores.
        observed_sem = self.observe_sems()
        missing = self.expected_sem.difference(observed_sem)
        extra = observed_sem.difference(self.expected_sem)
        if missing:
            logging.error('Expected semaphore(s) not found:')
            logging.error(missing)
        if extra:
            test_fail = True
            logging.error('Unexpected semaphore(s) found:')
            logging.error(extra)

        # Also check Message Queues. Since we currently expect
        # none, we can avoid over-engineering this check.
        queues = utils.system_output('ipcs -q | grep ^0', ignore_status=True)
        if queues:
            test_fail = True
            logging.error('Unexpected message queues found:')
            logging.error(queues)

        if test_fail:
            raise error.TestFail('SysV IPCs did not match expectations')
