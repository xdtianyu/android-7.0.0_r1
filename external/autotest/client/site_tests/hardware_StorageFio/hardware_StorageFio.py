# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging, os, re, time
from autotest_lib.client.bin import fio_util, site_utils, test, utils
from autotest_lib.client.common_lib import error


class hardware_StorageFio(test.test):
    """
    Runs several fio jobs and reports results.

    fio (flexible I/O tester) is an I/O tool for benchmark and stress/hardware
    verification.

    """

    version = 7
    DEFAULT_FILE_SIZE = 1024 * 1024 * 1024
    VERIFY_OPTION = 'v'

    # Initialize fail counter used to determine test pass/fail.
    _fail_count = 0

    def __get_disk_size(self):
        """Return the size in bytes of the device pointed to by __filename"""
        self.__filesize = utils.get_disk_size(self.__filename)

        if not self.__filesize:
            raise error.TestNAError(
                'Unable to find the partition %s, please plug in a USB '
                'flash drive and a SD card for testing external storage' %
                self.__filename)


    def __get_device_description(self):
        """Get the device vendor and model name as its description"""

        # Find the block device in sysfs. For example, a card read device may
        # be in /sys/devices/pci0000:00/0000:00:1d.7/usb1/1-5/1-5:1.0/host4/
        # target4:0:0/4:0:0:0/block/sdb.
        # Then read the vendor and model name in its grand-parent directory.

        # Obtain the device name by stripping the partition number.
        # For example, on x86: sda3 => sda; on ARM: mmcblk1p3 => mmcblk1.
        device = os.path.basename(
            re.sub('(sd[a-z]|mmcblk[0-9]+)p?[0-9]+', '\\1', self.__filename))
        findsys = utils.run('find /sys/devices -name %s' % device)
        device_path = findsys.stdout.rstrip()

        vendor_file = device_path.replace('block/%s' % device, 'vendor')
        model_file = device_path.replace('block/%s' % device, 'model')
        if os.path.exists(vendor_file) and os.path.exists(model_file):
            vendor = utils.read_one_line(vendor_file).strip()
            model = utils.read_one_line(model_file).strip()
            self.__description = vendor + ' ' + model
        else:
            self.__description = ''


    def initialize(self, dev='', filesize=DEFAULT_FILE_SIZE):
        """
        Set up local variables.

        @param dev: block device / file to test.
                Spare partition on root device by default
        @param filesize: size of the file. 0 means whole partition.
                by default, 1GB.
        """
        if dev != '' and (os.path.isfile(dev) or not os.path.exists(dev)):
            if filesize == 0:
                raise error.TestError(
                    'Nonzero file size is required to test file systems')
            self.__filename = dev
            self.__filesize = filesize
            self.__description = ''
            return

        if not dev:
            dev = site_utils.get_fixed_dst_drive()

        if dev == site_utils.get_root_device():
            if filesize == 0:
                raise error.TestError(
                    'Using the root device as a whole is not allowed')
            else:
                self.__filename = site_utils.get_free_root_partition()
        elif filesize != 0:
            # Use the first partition of the external drive
            if dev[5:7] == 'sd':
                self.__filename = dev + '1'
            else:
                self.__filename = dev + 'p1'
        else:
            self.__filename = dev
        self.__get_disk_size()
        self.__get_device_description()

        # Restrict test to use a given file size, default 1GiB
        if filesize != 0:
            self.__filesize = min(self.__filesize, filesize)

        self.__verify_only = False

        logging.info('filename: %s', self.__filename)
        logging.info('filesize: %d', self.__filesize)

    def run_once(self, dev='', quicktest=False, requirements=None,
                 integrity=False, wait=60 * 60 * 72):
        """
        Runs several fio jobs and reports results.

        @param dev: block device to test
        @param quicktest: short test
        @param requirements: list of jobs for fio to run
        @param integrity: test to check data integrity
        @param wait: seconds to wait between a write and subsequent verify

        """

        if requirements is not None:
            pass
        elif quicktest:
            requirements = [
                ('1m_write', []),
                ('16k_read', [])
            ]
        elif integrity:
            requirements = [
                ('8k_async_randwrite', []),
                ('8k_async_randwrite', [self.VERIFY_OPTION])
            ]
        elif dev in ['', site_utils.get_root_device()]:
            requirements = [
                ('surfing', []),
                ('boot', []),
                ('login', []),
                ('seq_read', []),
                ('seq_write', []),
                ('16k_read', []),
                ('16k_write', []),
                ('1m_stress', []),
            ]
        else:
            # TODO(waihong@): Add more test cases for external storage
            requirements = [
                ('seq_read', []),
                ('seq_write', []),
                ('16k_read', []),
                ('16k_write', []),
                ('1m_stress', []),
            ]

        results = {}
        for job, options in requirements:
            # Keys are labeled according to the test case name, which is
            # unique per run, so they cannot clash
            if self.VERIFY_OPTION in options:
                time.sleep(wait)
                self.__verify_only = True
            else:
                self.__verify_only = False
            env_vars = ' '.join(
                ['FILENAME=' + self.__filename,
                 'FILESIZE=' + str(self.__filesize),
                 'VERIFY_ONLY=' + str(int(self.__verify_only))
                ])
            job_file = os.path.join(self.bindir, job)
            results.update(fio_util.fio_runner(self, job_file, env_vars))

        # Output keys relevant to the performance, larger filesize will run
        # slower, and sda5 should be slightly slower than sda3 on a rotational
        # disk
        self.write_test_keyval({'filesize': self.__filesize,
                                'filename': self.__filename,
                                'device': self.__description})
        logging.info('Device Description: %s', self.__description)
        self.write_perf_keyval(results)
        for k, v in results.iteritems():
            if k.endswith('_error'):
                self._fail_count += int(v)
        if self._fail_count > 0:
            raise error.TestFail('%s failed verifications' %
                                 str(self._fail_count))
