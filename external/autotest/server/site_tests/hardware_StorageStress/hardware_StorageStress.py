# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging, sys, time
from autotest_lib.client.common_lib import error
from autotest_lib.server import autotest
from autotest_lib.server import hosts
from autotest_lib.server import test

class hardware_StorageStress(test.test):
    """
    Integrity stress test for storage device
    """
    version = 1

    _HOURS_IN_SEC = 3600
    # Define default value for the test case
    _TEST_GAP = 60 # 1 min
    _TEST_DURATION = 12 * _HOURS_IN_SEC
    _SUSPEND_DURATION = _HOURS_IN_SEC
    _FIO_REQUIREMENT_FILE = '8k_async_randwrite'
    _FIO_WRITE_FLAGS = []
    _FIO_VERIFY_FLAGS = ['--verifyonly']

    def run_once(self, client_ip, gap=_TEST_GAP, duration=_TEST_DURATION,
                 power_command='reboot', storage_test_command='integrity',
                 suspend_duration=_SUSPEND_DURATION, storage_test_argument=''):
        """
        Run the Storage stress test
        Use hardwareStorageFio to run some test_command repeatedly for a long
        time. Between each iteration of test command, run power command such as
        reboot or suspend.

        @param client_ip:     string of client's ip address (required)
        @param gap:           gap between each test (second) default = 1 min
        @param duration:      duration to run test (second) default = 12 hours
        @param power_command: command to do between each test Command
                              possible command: reboot / suspend / nothing
        @param storage_test_command:  FIO command to run
                              - integrity:  Check data integrity
                              - full_write: Check performance consistency
                                            for full disk write. Use argument
                                            to determine which disk to write
        @param suspend_duration: if power_command is suspend, how long the DUT
                              is suspended.
        """

        # init test
        if not client_ip:
            error.TestError("Must provide client's IP address to test")

        self._client = hosts.create_host(client_ip)
        self._client_at = autotest.Autotest(self._client)
        self._results = {}
        self._suspend_duration = suspend_duration

        # parse power command
        if power_command == 'nothing':
            power_func = self._do_nothing
        elif power_command == 'reboot':
            power_func = self._do_reboot
        elif power_command == 'suspend':
            power_func = self._do_suspend
        else:
            raise error.TestFail(
                'Test failed with error: Invalid power command')

        # Test is doing a lot of disk activity, monitor disk data at each iteration.
        self.job.add_sysinfo_logfile('/var/log/storage_info.txt', on_every_test=True)

        # parse test command
        if storage_test_command == 'integrity':
            setup_func = self._write_data
            loop_func = self._verify_data
        elif storage_test_command == 'full_write':
            setup_func = self._do_nothing
            loop_func = self._full_disk_write
            # Do at least 2 soak runs. Given the absolute minimum of a loop is
            # around 1h, duration should be at least 1h.
            self._soak_time = min(self._TEST_DURATION, duration / 4)
        else:
            raise error.TestFail('Test failed with error: Invalid test command')

        # init statistic variable
        min_time_per_loop = sys.maxsize
        max_time_per_loop = 0
        all_loop_time = 0
        avr_time_per_loop = 0
        self._loop_count = 0
        setup_func()

        start_time = time.time()

        while time.time() - start_time < duration:
            # sleep
            time.sleep(gap)

            self._loop_count += 1

            # do power command & verify data & calculate time
            loop_start_time = time.time()
            power_func()
            loop_func()
            loop_time = time.time() - loop_start_time

            # update statistic
            all_loop_time += loop_time
            min_time_per_loop = min(loop_time, min_time_per_loop)
            max_time_per_loop = max(loop_time, max_time_per_loop)

        if self._loop_count > 0:
            avr_time_per_loop = all_loop_time / self._loop_count

        logging.info(str('check data count: %d' % self._loop_count))

        # report result
        self.write_perf_keyval({'loop_count':self._loop_count})
        self.write_perf_keyval({'min_time_per_loop':min_time_per_loop})
        self.write_perf_keyval({'max_time_per_loop':max_time_per_loop})
        self.write_perf_keyval({'avr_time_per_loop':avr_time_per_loop})

    def _do_nothing(self):
        pass

    def _do_reboot(self):
        """
        Reboot host machine
        """
        self._client.reboot()

    def _do_suspend(self):
        """
        Suspend host machine
        """
        self._client.suspend(suspend_time=self._suspend_duration)

    @classmethod
    def _check_client_test_result(cls, client):
        """
        Check result of the client test.
        Auto test will store results in the file named status.
        We check that the second to last line in that file begin with 'END GOOD'

        @ raise an error if test fails.
        """
        client_result_dir = '%s/results/default' % client.autodir
        command = 'tail -2 %s/status | head -1' % client_result_dir
        status = client.run(command).stdout.strip()
        logging.info(status)
        if status[:8] != 'END GOOD':
            raise error.TestFail('client in StorageStress failed.')


    def _write_data(self):
        """
        Write test data to host using hardware_StorageFio
        """
        logging.info('_write_data')
        self._client_at.run_test('hardware_StorageFio', disable_sysinfo=True,
            wait=0, tag='%s_%d' % ('write_data', self._loop_count),
            requirements=[(self._FIO_REQUIREMENT_FILE, self._FIO_WRITE_FLAGS)])
        self._check_client_test_result(self._client)

    def _verify_data(self):
        """
        Verify test data using hardware_StorageFio
        """
        logging.info(str('_verify_data #%d' % self._loop_count))
        self._client_at.run_test('hardware_StorageFio', disable_sysinfo=True,
            wait=0, tag='%s_%d' % ('verify_data', self._loop_count),
            requirements=[(self._FIO_REQUIREMENT_FILE, self._FIO_VERIFY_FLAGS)])
        self._check_client_test_result(self._client)

    def _full_disk_write(self):
        """
        Do the root device full area write and report performance
        Write random pattern for few hours, then do a write and a verify,
        noting the latency.
        """
        logging.info(str('_full_disk_write #%d' % self._loop_count))

        # use the default requirement that write different pattern arround.
        self._client_at.run_test('hardware_StorageFio',
                                 disable_sysinfo=True,
                                 tag='%s_%d' % ('soak', self._loop_count),
                                 requirements=[('64k_stress', [])],
                                 time_length=self._soak_time)
        self._check_client_test_result(self._client)

        self._client_at.run_test('hardware_StorageFio',
                                 disable_sysinfo=True,
                                 tag='%s_%d' % ('surf', self._loop_count),
                                 requirements=[('surfing', [])],
                                 time_length=self._soak_time)
        self._check_client_test_result(self._client)

        self._client_at.run_test('hardware_StorageFio',
                                 disable_sysinfo=True,
                                 tag='%s_%d' % ('integrity', self._loop_count),
                                 wait=0, integrity=True)
        self._check_client_test_result(self._client)

        self._client_at.run_test('hardware_StorageWearoutDetect',
                                 tag='%s_%d' % ('wearout', self._loop_count),
                                 wait=0, use_cached_result=False)
        # No checkout for wearout, to test device pass their limits.
