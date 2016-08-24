# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import os
import re
import stat
import subprocess

from autotest_lib.client.bin import test
from autotest_lib.client.common_lib import error
from autotest_lib.client.cros import asan

class security_OpenFDs(test.test):
    """Checks a number of sensitive processes on Chrome OS for unexpected open
    file descriptors.
    """

    version = 1

    @staticmethod
    def _S_ISANONFD(mode):
        """
        Returns whether |mode| represents an "anonymous inode" file descriptor.
        Python does not expose a type-checking macro for anonymous inode fds.
        Implements the same interface as stat.S_ISREG(x).

        @param mode: mode bits, usually from 'stat(path).st_mode'
        """
        return 0 == (mode & 0770000)


    def get_fds(self, pid, typechecker):
        """
        Returns the set of open file descriptors for |pid|.
        Each open fd is represented as 'mode path', e.g.: '0500 /dev/random'.

        @param pid: pid of process
        @param typechecker: callback restricting allowed fd types
        """
        proc_fd_dir = os.path.join('/proc/', pid, 'fd')
        fd_perms = set([])
        for link in os.listdir(proc_fd_dir):
            link_path = os.path.join(proc_fd_dir, link)
            target = os.readlink(link_path)
            # The "mode" on the link tells us if the file is
            # opened for read/write. We are more interested
            # in that than the permissions of the file on the fs.
            link_st_mode = os.lstat(link_path).st_mode
            # On the other hand, we need the type information
            # off the real file, otherwise we're going to get
            # S_ISLNK for everything.
            real_st_mode = os.stat(link_path).st_mode
            if not typechecker(real_st_mode):
                raise error.TestFail('Pid %s has fd for %s, disallowed type' %
                                     (pid, target))
            mode = stat.S_IMODE(link_st_mode)
            fd_perms.add('%s %s' % (oct(mode), target))
        return fd_perms


    def snapshot_system(self):
        """
        Dumps a systemwide snapshot of open-fd and process table
        information into the results directory, to assist with any
        triage/debug later.
        """
        subprocess.call('ps -ef > "%s/ps-ef.txt"' % self.resultsdir,
                        shell=True)
        subprocess.call('ls -l /proc/*[0-9]*/fd > "%s/proc-fd.txt"' %
                        self.resultsdir, shell=True)


    def apply_filters(self, fds, filters):
        """
        Removes every item in |fds| matching any of the regexes in |filters|.
        This modifies the set in-place, and returns a list containing
        any regexes which did not match anything.

        @param fds: set of 'mode path' strings representing open fds
        @param filters: list of regexes to filter open fds with
        """
        failed_filters = set()
        for filter_re in filters:
            expected_fds = set([fd_perm for fd_perm in fds
                                if re.match(filter_re, fd_perm)])
            if not expected_fds:
                failed_filters.add(filter_re)
            fds -= expected_fds
        return failed_filters


    def find_pids(self, process, arg_regex):
        """
        Finds all pids for |process| whose command line arguments
        match |arg_regex|. Returns a list of pids.

        @param process: process name
        @param arg_regex: regex to match command line arguments
        """
        p1 = subprocess.Popen(['ps', '-C', process, '-o', 'pid,command'],
                              stdout=subprocess.PIPE)
        # We're adding '--ignored= --type=renderer' to the GPU process cmdline
        # to fix crbug.com/129884.
        # This process has different characteristics, so we need to avoid
        # finding it when we find --type=renderer tests processes.
        p2 = subprocess.Popen(['grep', '-v', '--',
                               '--ignored=.*%s' % arg_regex],
                              stdin=p1.stdout, stdout=subprocess.PIPE)
        p3 = subprocess.Popen(['grep', arg_regex], stdin=p2.stdout,
                              stdout=subprocess.PIPE)
        p4 = subprocess.Popen(['awk', '{print $1}'], stdin=p3.stdout,
                              stdout=subprocess.PIPE)
        output = p4.communicate()[0]
        return output.splitlines()


    def check_process(self, process, args, filters, typechecker):
        """
        Checks a process for unexpected open file descriptors:
        * Identifies all instances (pids) of |process|.
        * Identifies all file descriptors open by those pids.
        * Reports any fds not accounted for by regexes in |filters|.
        * Reports any filters which fail to match any open fds.

        If there were any fds unaccounted for, or failed filters,
        mark the test failed.

        @param process: process name
        @param args: regex to match command line arguments
        @param filters: list of regexes to filter open fds with
        @param typechecker: callback restricting allowed fd types
        """
        logging.debug('Checking %s %s', process, args)
        test_pass = True
        for pid in self.find_pids(process, args):
            logging.debug('Found pid %s for %s', pid, process)
            fds = self.get_fds(pid, typechecker)
            failed_filters = self.apply_filters(fds, filters)
            # Log failed filters to allow pruning the list.
            if failed_filters:
                logging.error('Some filter(s) failed to match any fds: %s',
                              repr(failed_filters))
            if fds:
                logging.error('Found unexpected fds in %s %s: %s',
                              process, args, repr(fds))
                test_pass = False
        return test_pass


    def run_once(self):
        """
        Checks a number of sensitive processes on Chrome OS for
        unexpected open file descriptors.
        """
        self.snapshot_system()

        passes = []
        filters = [r'0700 anon_inode:\[event.*\]',
                   r'0[35]00 pipe:.*',
                   r'0[57]00 socket:.*',
                   r'0500 /dev/null',
                   r'0[57]00 /dev/urandom',
                   r'0300 /var/log/chrome/chrome_.*',
                   r'0[37]00 /var/log/ui/ui.*',
                  ]

        # Whitelist fd-type check, suitable for Chrome processes.
        # Notably, this omits S_ISDIR.
        allowed_fd_type_check = lambda x: (stat.S_ISREG(x) or
                                           stat.S_ISCHR(x) or
                                           stat.S_ISSOCK(x) or
                                           stat.S_ISFIFO(x) or
                                           security_OpenFDs._S_ISANONFD(x))

        # TODO(jorgelo): revisit this and potentially remove.
        if asan.running_on_asan():
            # On ASan, allow all fd types and opening /proc
            logging.info("Running on ASan, allowing /proc")
            allowed_fd_type_check = lambda x: True
            filters.append(r'0500 /proc')

        passes.append(self.check_process('chrome', 'type=plugin', filters,
                                         allowed_fd_type_check))

        filters.extend([r'0[57]00 /dev/shm/..*',
                        r'0500 /opt/google/chrome/.*.pak',
                        r'0500 /opt/google/chrome/icudtl.dat',
                        # These used to be bundled with the Chrome binary.
                        # See crbug.com/475170.
                        r'0500 /opt/google/chrome/natives_blob.bin',
                        r'0500 /opt/google/chrome/snapshot_blob.bin',
                        # Font files can be kept open in renderers
                        # for performance reasons.
                        # See crbug.com/452227.
                        r'0500 /usr/share/fonts/.*',
                       ])
        try:
            # Renderers have access to DRM vgem device for graphics tile upload.
            # See crbug.com/537474.
            filters.append(r'0700 /dev/dri/%s' % os.readlink('/dev/dri/vgem'))
        except OSError:
            # /dev/dri/vgem doesn't exist.
            pass

        passes.append(self.check_process('chrome', 'type=renderer', filters,
                                         allowed_fd_type_check))

        if False in passes:
            raise error.TestFail("Unexpected open file descriptors.")
