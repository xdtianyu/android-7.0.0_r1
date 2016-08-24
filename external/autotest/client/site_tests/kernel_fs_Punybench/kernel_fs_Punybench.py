#!/usr/bin/python
#
# Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import optparse
import os, re
from autotest_lib.client.bin import utils, test
from autotest_lib.client.common_lib import error

re_float = r"[+-]? *(?:\d+(?:\.\d*)?|\.\d+)(?:[eE][+-]?\d+)?"

class kernel_fs_Punybench(test.test):
    """Run a selected subset of the puny benchmarks
    """
    version = 1
    Bin = '/usr/local/opt/punybench/bin/'


    def initialize(self):
        self.results = []
        self.job.drop_caches_between_iterations = True


    def _run(self, cmd, args):
        """Run a puny benchmark

        Prepends the path to the puny benchmark bin.

        @param cmd: command to be run
        @param args: arguments for the command
        """
        result = utils.system_output(
            os.path.join(self.Bin, cmd) + ' ' + args)
        logging.debug(result)
        return result


    @staticmethod
    def _ecrypt_mount(dir, mnt):
        """Mount the eCrypt File System

        @param dir: directory where encrypted file system is stored
        @param mnt: mount point for encrypted file system
        """
        options = ('-o'
                   ' key=passphrase:passphrase_passwd=secret'
                   ',ecryptfs_cipher=aes'
                   ',ecryptfs_key_bytes=32'
                   ',no_sig_cache'
                   ',ecryptfs_passthrough=no'
                   ',ecryptfs_enable_filename_crypto=no')
        utils.system_output('mkdir -p %s %s' % (dir, mnt))
        utils.system_output('mount -t ecryptfs %s %s %s' %
                           (options, dir, mnt))


    @staticmethod
    def _ecrypt_unmount(dir, mnt):
        """Unmount the eCrypt File System and remove it and its mount point

        @param dir: directory where encrypted file system was stored
        @param mnt: mount point for encrypted file system
        """
        utils.system_output('umount ' + mnt)
        utils.system_output('rm -R ' + dir)
        utils.system_output('rm -R ' + mnt)


    @staticmethod
    def _find_max(tag, text):
        """Find the max in a memcpy result.

        @param tag: name of sub-test to select from text.
        @param text: output from memcpy test.
        @return Best result from that sub-test.

        Example input text:
          memcpy (Meg = 2**20)
          0. 4746.96 MiB/sec
          1. 4748.99 MiB/sec
          2. 4748.14 MiB/sec
          3. 4748.59 MiB/sec
          simple (Meg = 2**20)
          0. 727.996 MiB/sec
          1. 728.031 MiB/sec
          2. 728.22 MiB/sec
          3. 728.049 MiB/sec
          32bit (Meg = 2**20)
          0. 2713.16 MiB/sec
          1. 2719.93 MiB/sec
          2. 2724.33 MiB/sec
          3. 2711.5 MiB/sec
        """
        r1 = re.search(tag + ".*\n(\d.*sec\n)+", text)
        r2 = re.findall(r"\d+\. (" + re_float + r") M.*\n", r1.group(0))
        return max(float(result) for result in r2)


    def _memcpy(self):
        """Measure memory to memory copy.

        The size has to be large enough that it doesn't fit
        in the cache. We then take the best of serveral runs
        so we have a guarenteed not to exceed number.

        Several different ways are used to move memory.
        """
        size = 64 * 1024 * 1024
        loops = 4
        iterations = 10
        args  = '-z %d -i %d -l %d' % (size, iterations, loops)
        result = self._run('memcpy', args)

        for tag in ['memcpy', '32bit', '64bit']:
            value = self._find_max(tag, result)
            self.write_perf_keyval({tag + '_MiB_s': value})


    @staticmethod
    def _get_mib_s(tag, text):
        """Extract the MiB/s for tag from text

        @param tag: name of sub-test to select from text
        @param text: extact MiB/s from this text

        Example input text:
          SDRAM:
          memcpy_trivial:  (2097152 bytes copy) =  727.6 MiB/s /  729.9 MiB/s
          memcpy        :  (2097152 bytes copy) = 4514.2 MiB/s / 4746.9 MiB/s
          memcpy_trivial:  (3145728 bytes copy) =  727.7 MiB/s /  729.5 MiB/s
          memcpy        :  (3145728 bytes copy) = 4489.5 MiB/s / 4701.5 MiB/s
        """
        r1 = re.search(tag + ".*\n.*\n.*", text)
        r2 = re.search(r"[^\s]+ MiB/s$", r1.group(0))
        r3 = re.search(re_float, r2.group(0))
        return r3.group(0)


    def _memcpy_test(self):
        """Test the various caches and alignments

        WARNING: test will have to be changed if cache sizes change.
        """
        result = self._run('memcpy_test', "")
        self.write_perf_keyval({'L1cache_MiB_s':
                               self._get_mib_s('L1 cache', result)})
        self.write_perf_keyval({'L2cache_MiB_s':
                               self._get_mib_s('L2 cache', result)})
        self.write_perf_keyval({'SDRAM_MiB_s':
                               self._get_mib_s('SDRAM', result)})


    def _threadtree(self, prefix, dir):
        """Create and manipulate directory trees.

        Threadtree creates a directory tree with files for each task.
        It then copies that tree then deletes it.

        @param prefix: prefix to use on name/value pair for identifying results
        @param dir: directory path to use for test

        Example results:
          opens   =       3641
          created =       2914
          dirs    =       1456
          files   =       1458
          deleted =       4372
          read    = 1046306816
          written = 2095407104
           51.7   2. timer avg= 57.9 stdv= 8.76
        """
        iterations = 4
        tasks = 2
        width = 3
        depth = 5
        args = ('-d %s -i %d -t %d -w %d -k %d' %
               (dir, iterations, tasks, width, depth))
        result = self._run('threadtree', args)
        r1 = re.search(r"timer avg= *([^\s]*).*$", result)
        timer_avg = float(r1.group(1))
        p = tasks * pow(width, depth + 1) / timer_avg
        self.write_perf_keyval({prefix + 'threadtree_ops': p})


    def _uread(self, prefix, file):
        """Read a large file.

        @param prefix: prefix to use on name/value pair for identifying results
        @param file: file path to use for test

        The size should be picked so the file will
        not fit in memory.

        Example results:
          size=8589934592 n=1 55.5 3. timer avg= 55.5 stdv=0.0693 147.6 MiB/s
          size=8589934592 n=1 55.6 4. timer avg= 55.5 stdv=0.0817 147.5 MiB/s
        """
        args = '-f %s' % file
        result = self._run('uread', args)
        r1 = re.search(r"[^\s]+ MiB/s.*$", result)
        r2 = re.search(re_float, r1.group(0))
        mib_s = r2.group(0)
        self.write_perf_keyval({prefix + 'uread_MiB_s': mib_s})


    def _ureadrand(self, prefix, file):
        """Read randomly a large file

        @param prefix: prefix to use on name/value pair for identifying results
        @param file: file path to use for test

        Example results (modified to fit in 80 columes):
size=8589934592 n=10000 4.7 3. timer avg= 4 stdv= 4.6 9.1 MiB/s 2326 IOPs/sec
size=8589934592 n=10000 4.9 4. timer avg= 4.2 stdv= 4.5 8.8 MiB/s 2262 IOPs/sec
        """
        args = '-f %s' % file
        result = self._run('ureadrand', args)
        r1 = re.search(r"([^\s]+ IOPs/sec).*$", result)
        r2 = re.search(re_float, r1.group(0))
        iops = r2.group(0)
        self.write_perf_keyval({prefix + 'ureadrand_iops': iops})


    def _uwrite(self, prefix, file):
        """Write a large file.

        @param prefix: prefix to use on name/value pair for identifying results
        @param file: file path to use for test

        The size should be picked so the file will not fit in memory.

        Example results:
          size=8589934592 n=1 55.5 3. timer avg= 55.5 stdv=0.0693 147.6 MiB/s
          size=8589934592 n=1 55.6 4. timer avg= 55.5 stdv=0.0817 147.5 MiB/s
        """
        args = '-f %s' % file
        result = self._run('uwrite', args)
        r1 = re.search(r"[^\s]+ MiB/s.*$", result)
        r2 = re.search(re_float, r1.group(0))
        mib_s = r2.group(0)
        self.write_perf_keyval({prefix + 'uwrite_MiB_s': mib_s})


    def _uwriterand(self, prefix, file, size):
        """Write randomly a file

        @param prefix: prefix to use on name/value pair for identifying results
        @param file: file path to use for test
        @param size: size of file - large files are much slower than small files

        Example results (modified to fit in 80 columes):
size=16777216 n=1000 13.4 1. timer avg= 13.4 stdv= 0 0.29 MiB/s 74.8 IOPs/sec
size=16777216 n=1000 13.3 2. timer avg= 13.3 stdv=0.032 0.3 MiB/s 75.0 IOPs/sec

        """
        loops = 4
        iterations = 1000
        args = ('-f %s -z %d -i %d -l %d -b12' %
                (file, size, iterations, loops))
        result = self._run('uwriterand', args)
        r1 = re.search(r"([^\s]+ IOPs/sec).*$", result)
        r2 = re.search(re_float, r1.group(0))
        iops = r2.group(0)
        self.write_perf_keyval({prefix + 'uwriterand_iops': iops})


    def _uwritesync(self, prefix, file):
        """Synchronously writes a file

        @param prefix: prefix to use on name/value pair for identifying results
        @param file: file path to use for test

        Example results (modified to fit in 80 columes):
size=409600 n=100 4.58 3. timer avg= 4.41 stdv=0.195 0.0887 MiB/s 22.7 IOPs/sec
size=409600 n=100 4.84 4. timer avg= 4.52 stdv= 0.27 0.0885 MiB/s 22.15 IOPs/sec
        """
        loops = 4  # minimum loops to average or see trends
        num_blocks_to_write = 100  # Because sync writes are slow,
                                   # don't do too many
        args = ('-f %s -i %d -l %d -b12' %
                (file, num_blocks_to_write, loops))
        result = self._run('uwritesync', args)
        r1 = re.search(r"([^\s]+ IOPs/sec).*$", result)
        r2 = re.search(re_float, r1.group(0))
        iops = r2.group(0)
        self.write_perf_keyval({prefix + 'uwritesync_iops': iops})


    def _disk_tests(self, prefix,  dir, file):
        """Run this collection of disk tests

        @param prefix: prefix to use on name/value pair for identifying results
        @param dir: directory path to use for tests
        @param file: file path to use for tests
        """
        self._uread(prefix, file)
        self._ureadrand(prefix, file)
        self._uwrite(prefix, file)
        self._uwriterand(prefix + '_large_', file, 8 * 1024 * 1024 * 1024)
        # This tests sometimes gives invalid results
        # self._uwriterand(prefix + '_small_', file, 8 * 1024)
        self._uwritesync(prefix, file)
        self._threadtree(prefix, dir)


    def _ecryptfs(self):
        """Setup up to run disk tests on encrypted volume
        """
        dir = '/usr/local/ecrypt_tst'
        mnt = '/usr/local/ecrypt_mnt'
        self._ecrypt_mount(dir, mnt)
        self._disk_tests('ecryptfs_', mnt + '/_Dir', mnt + '/xyzzy')
        self._ecrypt_unmount(dir, mnt)


    def _parse_args(self, args):
        """Parse input arguments to this autotest.

        Args:
        @param args: List of arguments to parse.
        @return
          opts: Options, as per optparse.
          args: Non-option arguments, as per optparse.
        """
        parser = optparse.OptionParser()
        parser.add_option('--disk', dest='want_disk_tests',
                          action='store_true', default=False,
                          help='Run disk tests.')
        parser.add_option('--ecryptfs', dest='want_ecryptfs_tests',
                          action='store_true', default=False,
                          help='Run ecryptfs tests.')
        parser.add_option('--mem', dest='want_mem_tests',
                          action='store_true', default=False,
                          help='Run memory tests.')
        parser.add_option('--nop', dest='want_nop_tests',
                          action='store_true', default=False,
                          help='Do nothing.')
        return parser.parse_args(args)


    def run_once(self, args=[]):
        """Run the PyAuto performance tests.

        @param args: Either space-separated arguments or a list of string
              arguments.  If this is a space separated string, we'll just
              call split() on it to get a list.  The list will be sent to
              optparse for parsing.
        """
        if isinstance(args, str):
            args = args.split()
        options, test_args = self._parse_args(args)

        if test_args:
            raise error.TestFail("Unknown args: %s" % repr(test_args))

        if not os.path.exists(self.Bin):
            raise error.TestFail("%s does not exist" % self.Bin)

        try:
            restart_swap = True
            utils.system_output('swapoff /dev/zram0')
        except:
            restart_swap = False
        utils.system_output('stop ui')
        if options.want_nop_tests:
            pass
        if options.want_mem_tests:
            self._memcpy_test()
            self._memcpy()
        if options.want_disk_tests:
            self._disk_tests('ext4_', '/usr/local/_Dir', '/usr/local/xyzzy')
        if options.want_ecryptfs_tests:
            self._ecryptfs()

        if restart_swap:
            utils.system_output('swapon /dev/zram0')
        utils.system_output('start ui')
