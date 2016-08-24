# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os, pwd, subprocess, tempfile

from autotest_lib.client.bin import test
from autotest_lib.client.common_lib import error

class TLSDate:
    """
    A single tlsdate invocation. Takes care of setting up a temporary cachedir
    for it, along with collecting output from both it and its helper processes.
    """
    def __init__(self, test_obj):
        self._proc = None
        self._testdir = tempfile.mkdtemp(suffix='tlsdate')
        self._cachedir = self._testdir + '/cache'
        self._outfile = self._testdir + '/out'
        self._subprog = '?'
        self._test_obj = test_obj
        self._output = None
        self._tlsdate_uid = pwd.getpwnam('tlsdate').pw_uid
        os.mkdir(self._cachedir)
        # Let the tlsdate user (tlsdate) write.
        os.chown(self._testdir, self._tlsdate_uid, -1)
        # Allow support shell library to be sourced.
        os.chown(self._test_obj.srcdir + '/time.sh', self._tlsdate_uid, -1)


    def start(self, subprog):
        print 'running with %s' % self._test_obj.srcdir + '/' + subprog
        self._subprog = subprog
        # Make sure the tlsdate user can access the files
        fake_tlsdate = self._test_obj.srcdir + '/' + subprog
        os.chown(fake_tlsdate, self._tlsdate_uid, -1)
        args = ['/usr/bin/tlsdated', '-p',
                '-f', self._test_obj.srcdir + '/test.conf',
                '-c', self._cachedir,
                '-v',
                fake_tlsdate,
                self._outfile]
        self._proc = subprocess.Popen(args, stdin=subprocess.PIPE,
                                      stderr=subprocess.PIPE)


    def route_up(self):
        self._proc.stdin.write('n')
        self._proc.stdin.flush()


    def kill(self):
        self._proc.terminate()


    def output(self):
        if not self._output:
            self._output = self._proc.communicate()[1].split('\n')
        return self._output


    def in_output(self, string):
        for x in self.output():
            if string in x:
                return True
        return False


    def subproc_output(self):
        with open(self._outfile) as f:
            return [x.rstrip() for x in f.readlines()]


    def ok(self):
        return 'ok' in self.subproc_output()


class platform_TLSDate(test.test):
    version = 1

    def require_ok(self, t):
        if not t.ok():
            raise error.TestFail('Expected success, got:' +
                                 ';'.join(t.subproc_output()))


    def require_output(self, t, string):
        if not t.in_output(string):
            raise error.TestFail('Needed "%s" but got "%s"' % (string,
                                 ';'.join(t.output())))


    def require_not_output(self, t, string):
        if t.in_output(string):
            raise error.TestFail('Needed no "%s" but got "%s"' % (string,
                                 ';'.join(t.output())))


    def test_delay_subproc(self):
        """
        Tests that a subprocess that delays for one second is waited on
        successfully the second time.
        """
        t = TLSDate(self)
        t.start('delay_subproc')
        self.require_output(t, 'attempt 1 backoff')
        self.require_output(t, 'time set from the network')
        self.require_ok(t)


    def test_hang_subproc(self):
        """
        Tests that a subprocess that delays for too long is considered hung and
        killed.
        """
        t = TLSDate(self)
        t.start('hang_subproc')
        self.require_output(t, 'attempt 1 backoff')
        self.require_output(t, 'tlsdate timed out')
        self.require_ok(t)


    def test_fail_routes(self):
        """
        Tests that if the initial tlsdate call fails, we wait for a route to
        appear, then rerun tlsdate.
        """
        t = TLSDate(self)
        t.start('fail_routes')
        t.route_up()
        self.require_output(t, 'status:2')
        self.require_output(t, 'stdin')
        self.require_output(t, 'time set from the network')
        self.require_ok(t)


    def run_once(self):
        self.test_delay_subproc()
        self.test_hang_subproc()
        self.test_fail_routes()
