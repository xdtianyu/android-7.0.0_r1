#!/usr/bin/python
#
# Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

__author__ = 'kdlucas@chromium.org (Kelly Lucas)'

import logging
import os

from autotest_lib.client.bin import utils, test
from autotest_lib.client.common_lib import error


class platform_OSLimits(test.test):
    """
    Verify os limitations are set to correct levels.
    """
    version = 1

    def get_limit(self, key, path):
        """
        Find and return values held in path.

        Args:
            key: dictionary key of os limit.
            path: pathname of file with current value.
        Returns:
            value found in path. If it's a number we'll convert to integer.
        """

        value = None
        # Most files have only one value, but if there are multiple values we
        # will handle it differently. Determine this from the key.

        multivals = ['max_open', 'max_procs']
        limits = {'max_open': 'Max open files',
                  'max_procs': 'Max processes',
                 }

        if key in multivals:
            output = utils.read_file(path)
            lines = output.splitlines()
            for line in lines:
                if limits[key] in line:
                    fields = line.split(limits[key])
                    vals = fields[1].split()
                    value = (vals[0])
        else:
            value = (utils.read_one_line(path))

        if value == 'unlimited':
            return value
        else:
            return int(value)

    def run_once(self):
        errors = set()

        # Max procs, max threads, and file max are dependent upon total memory.
        # The kernel uses a formula similar to:
        #   MemTotal-kb / 128 = max procs
        #   MemTotal-kb / 64 = max threads
        #   MemTotal-kb / 10 = file_max
        # But note that MemTotal changes at the end of initialization.
        # The values used below for these settings should give sufficient head
        # room for usage and kernel allocation.

        ref_min = {'file_max': 50000,
                   'kptr_restrict': 1,
                   'max_open': 1024,
                   'max_procs': 3000,
                   'max_threads': 7000,
                   'ngroups_max': 65536,
                   'nr_open': 1048576,
                   'pid_max': 32768,
                   'mmap_min_addr': 65536,
                  }

        ref_equal = {'leases': 1,
                     'panic': -1,
                     'protected_hardlinks': 1,
                     'protected_symlinks': 1,
                     'ptrace_scope': 1,
                     'randomize_va_space': 2,
                     'sched_rt_period_us': 1000000,
                     'sched_rt_runtime_us': 800000,
                     'sysrq': 1,
                     'suid-dump': 2,
                     'tcp_syncookies': 1,
                    }

        refpath = {'file_max': '/proc/sys/fs/file-max',
                   'leases': '/proc/sys/fs/leases-enable',
                   'max_open': '/proc/self/limits',
                   'max_procs': '/proc/self/limits',
                   'max_threads': '/proc/sys/kernel/threads-max',
                   'mmap_min_addr': '/proc/sys/vm/mmap_min_addr',
                   'kptr_restrict': '/proc/sys/kernel/kptr_restrict',
                   'ngroups_max': '/proc/sys/kernel/ngroups_max',
                   'nr_open': '/proc/sys/fs/nr_open',
                   'panic': '/proc/sys/kernel/panic',
                   'pid_max': '/proc/sys/kernel/pid_max',
                   'protected_hardlinks': '/proc/sys/fs/protected_hardlinks',
                   'protected_symlinks': '/proc/sys/fs/protected_symlinks',
                   'ptrace_scope': '/proc/sys/kernel/yama/ptrace_scope',
                   'randomize_va_space': '/proc/sys/kernel/randomize_va_space',
                   'sched_rt_period_us': '/proc/sys/kernel/sched_rt_period_us',
                   'sched_rt_runtime_us': '/proc/sys/kernel/sched_rt_runtime_us',
                   'suid-dump': '/proc/sys/fs/suid_dumpable',
                   'sysrq': '/proc/sys/kernel/sysrq',
                   'tcp_syncookies': '/proc/sys/net/ipv4/tcp_syncookies',
                  }

        # Adjust arch-specific values.
        if utils.get_arch().startswith('arm'):
            ref_min['mmap_min_addr'] = 32768;

        if utils.get_arch().startswith('aarch64'):
            ref_min['mmap_min_addr'] = 32768;

        # Adjust version-specific details.
        kernel_ver = os.uname()[2]
        if utils.compare_versions(kernel_ver, "3.6") < 0:
            # Prior to kernel version 3.6, Yama handled link restrictions.
            refpath['protected_hardlinks'] = \
                '/proc/sys/kernel/yama/protected_nonaccess_hardlinks'
            refpath['protected_symlinks'] = \
                '/proc/sys/kernel/yama/protected_sticky_symlinks'

        # Create osvalue dictionary with the same keys as refpath.
        osvalue = {}
        for key in refpath:
            osvalue[key] = None

        for key in ref_min:
            osvalue[key] = self.get_limit(key, refpath[key])
            if osvalue[key] < ref_min[key]:
                logging.warning('%s is %d', refpath[key], osvalue[key])
                logging.warning('%s should be at least %d', refpath[key],
                             ref_min[key])
                errors.add(key)
            else:
                logging.info('%s is %d >= %d', refpath[key], osvalue[key],
                                               ref_min[key])

        for key in ref_equal:
            osvalue[key] = self.get_limit(key, refpath[key])
            if osvalue[key] != ref_equal[key]:
                logging.warning('%s is set to %d', refpath[key], osvalue[key])
                logging.warning('Expected %d', ref_equal[key])
                errors.add(key)
            else:
                logging.info('%s is %d', refpath[key], osvalue[key])

        # Look for anything from refpath that wasn't checked yet:
        for key in osvalue:
            if osvalue[key] == None:
                logging.warning('%s was never checked', key)
                errors.add(key)

        # If self.error is not zero, there were errors.
        if len(errors) > 0:
            raise error.TestFail('Found incorrect values: %s' %
                                 ', '.join(errors))
