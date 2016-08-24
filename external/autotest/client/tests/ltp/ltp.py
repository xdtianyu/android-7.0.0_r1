# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import os
from autotest_lib.client.bin import utils, test
from autotest_lib.client.common_lib import error

import parse_ltp_out


class ltp(test.test):
    version = 6

    def _import_site_config(self):
        site_config_path = os.path.join(os.path.dirname(__file__),
                                        'site_config.py')
        if os.path.exists(site_config_path):
            # for some reason __import__ with full path does not work within
            # autotest, although it works just fine on the same client machine
            # in the python interactive shell or separate testcases
            execfile(site_config_path)
            self.site_ignore_tests = locals().get('ignore_tests', [])
        else:
            self.site_ignore_tests = []


    def initialize(self):
        self._import_site_config()
        self.job.require_gcc()


    # http://sourceforge.net/projects/ltp/files/LTP%20Source/ltp-20120104/
    #        ltp-full-20120104.bz2
    def setup(self, tarball = 'ltp-full-20120104.bz2'):
        tarball = utils.unmap_url(self.bindir, tarball, self.tmpdir)
        utils.extract_tarball_to_dir(tarball, self.srcdir)
        os.chdir(self.srcdir)
        ltpbin_dir = os.path.join(self.srcdir, 'bin')
        os.mkdir(ltpbin_dir)

        utils.system('patch -p1 < ../patches/getdents.patch')
        utils.system('patch -p1 < ../patches/cpuid.patch')
        utils.system('patch -p1 < ../patches/kill-ipc.patch')
        utils.system('patch -p1 < ../patches/genpow.patch')
        utils.system('patch -p1 < ../patches/sysctl.patch')
        utils.make('autotools')
        utils.configure('--prefix=%s' % ltpbin_dir)
        utils.make('-j %d all' % utils.count_cpus())
        utils.system('yes n | make SKIP_IDCHECK=1 install')


    # Note: to run specific test(s), runltp supports an option (-f)
    #       to specify a custom 'scenario group' which is a comma-separated
    #       list of cmdfiles and/or an option (-s) to specify a grep match
    #       pattern for individual test names.
    # e.g. -for all tests in math cmdfile:
    #       job.run_test('ltp', '-f math')
    #      -for just the float_bessel test in the math cmdfile:
    #       job.run_test('ltp', '-f math -s float_bessel')
    #      -for the math and memory management cmdfiles:
    #       job.run_test('ltp', '-f math,mm')
    # Note: the site_excluded file lists individual test tags for tests
    #       to exclude (see the comment at the top of site_excluded).
    def run_once(self, args = '', script = 'runltp', ignore_tests=[]):

        ignore_tests = ignore_tests + self.site_ignore_tests

        # In case the user wants to run another test script
        if script == 'runltp':
            logfile = os.path.join(self.resultsdir, 'ltp.log')
            outfile = os.path.join(self.resultsdir, 'ltp.out')
            failcmdfile = os.path.join(self.debugdir, 'failcmdfile')
            excludecmdfile = os.path.join(self.bindir, 'site_excluded')
            args2 = '-p -l %s -C %s -d %s -o %s -S %s' % (logfile, failcmdfile,
                                                          self.tmpdir, outfile,
                                                          excludecmdfile)
            args = args + ' ' + args2

        ltpbin_dir = os.path.join(self.srcdir, 'bin')
        cmd = os.path.join(ltpbin_dir, script) + ' ' + args
        result = utils.run(cmd, ignore_status=True)

        if script == 'runltp':
            parse_ltp_out.summarize(outfile)

        # look for any failed test command.
        try:
            f = open(failcmdfile)
        except IOError:
            logging.warning('Expected to find failcmdfile but did not.')
            return
        failed_cmd = f.read().strip()
        f.close()
        if failed_cmd:
            raise error.TestFail(failed_cmd)
