# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from autotest_lib.client.bin import test
from autotest_lib.client.common_lib import error

import logging
import os
import errno

class security_RuntimeExecStack(test.test):
    """Tests that processes have non-executable stacks

    Examines the /proc/$pid/maps file of all running processes for the
    stack segments' markings. If "x" is found, it fails.
    """
    version = 1

    def check_no_exec_stack(self, maps):
        """Reads process memory map and checks there are no executable stacks.

        Args:
            @param maps: opened /proc/<pid>/maps file

        Returns:
          A tuple containing the error code and a string (usually a single line)
          with debug information. Error code could be:
            0: ok: stack not executable (second element will be None)
            1: error: stack is executable
            2: error: stack is not writable
            3: error: stack not found
        """
        contents = ''
        stack_count = 0
        for line in maps:
            line = line.strip()
            contents += line + '\n'

            if '[stack' not in line:
                continue
            stack_count += 1

            perms = line.split(' ', 2)[1]

            # Stack segment is executable.
            if 'x' in perms:
                return 1, line

            # Sanity check we have stack segment perms.
            if not 'w' in perms:
                return 2, line

        if stack_count > 0:
            # Stack segments are non-executable.
            return 0, None
        else:
            # Should be impossible: no stack segment seen.
            return 3, contents

    def run_once(self):
        failed = set([])

        for pid in os.listdir('/proc'):
            maps_path = '/proc/%s/maps' % (pid)
            # Is this a pid directory?
            if not os.path.exists(maps_path):
                continue
            # Is this a kernel thread?
            try:
                os.readlink('/proc/%s/exe' % (pid))
            except OSError, e:
                if e.errno == errno.ENOENT:
                    continue
            try:
                maps = open(maps_path)
                cmd = open('/proc/%s/cmdline' % (pid)).read()
            except IOError:
                # Allow the path to vanish out from under us. If
                # we've failed for any other reason, raise the failure.
                if os.path.exists(maps_path):
                    raise
                logging.debug('ignored: pid %s vanished', pid)
                continue

            # Clean up cmdline for reporting.
            cmd = cmd.replace('\x00', ' ')
            exe = cmd
            if ' ' in exe:
                exe = exe[:exe.index(' ')]

            # Check the stack segment.
            stack, report = self.check_no_exec_stack(maps)

            # Report outcome.
            if stack == 0:
                logging.debug('ok: %s %s', pid, exe)
            else:
                logging.info('FAIL: %s %s %s', pid, cmd, report)
                failed.add(exe)

        if len(failed) != 0:
            msg = 'Bad stacks segments: %s' % (', '.join(failed))
            raise error.TestFail(msg)
