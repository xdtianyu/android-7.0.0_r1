# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import glob, logging, os, time
from autotest_lib.client.bin import test
from autotest_lib.client.common_lib import error

# choosing a (very) conservative threshold for now to help catch
# major breakages
percent_idle_time_threshold = 20

class power_CPUIdle(test.test):
    version = 1

    def run_once(self, sleep_time=5):
        all_cpus = cpus()

        idle_time_at_start, active_time_at_start = all_cpus.idle_time()
        logging.info('idle_time_at_start: %d' % idle_time_at_start)
        logging.info('active_time_at_start: %d' % active_time_at_start)

        # sleep for some time to allow the CPUs to drop into idle states
        time.sleep(sleep_time)

        idle_time_at_end, active_time_at_end = all_cpus.idle_time()
        logging.info('idle_time_at_end: %d' % idle_time_at_end)
        logging.info('active_time_at_end: %d' % idle_time_at_end)

        idle_time_delta_ms = (idle_time_at_end - idle_time_at_start) / 1000
        logging.info('idle_time_delta_ms: %d' % idle_time_delta_ms)

        active_time_delta_ms = (active_time_at_end - active_time_at_start) \
                               / 1000
        logging.info('active_time_delta_ms: %d' % active_time_delta_ms)

        total_time_delta_ms = active_time_delta_ms + idle_time_delta_ms
        logging.info('total_time_delta_ms: %d' % total_time_delta_ms)

        percent_active_time = active_time_delta_ms * 100.0 / total_time_delta_ms
        logging.info('percent active time : %.2f' % percent_active_time)

        percent_idle_time = idle_time_delta_ms * 100.0 / total_time_delta_ms
        logging.info('percent idle time : %.2f' % percent_idle_time)

        keyvals = {}
        keyvals['ms_active_time_delta'] = active_time_delta_ms
        keyvals['ms_idle_time_delta'] = idle_time_delta_ms
        keyvals['percent_active_time'] = percent_active_time
        keyvals['percent_idle_time'] = percent_idle_time
        self.write_perf_keyval(keyvals)

        if percent_idle_time < percent_idle_time_threshold:
            raise error.TestFail('Idle percent below threshold')



class cpus(object):
    def __init__(self):
        self.__base_path = '/sys/devices/system/cpu/cpu*/cpuidle'
        self.__cpus = []

        dirs = glob.glob(self.__base_path)
        if not dirs:
            raise error.TestError('cpuidle not supported')

        for dir in dirs:
            cpu = cpuidle(dir)
            self.__cpus.append(cpu)


    def idle_time(self):
        total_idle_time = 0
        total_active_time = 0
        for cpu in self.__cpus:
            idle_time, active_time = cpu.idle_time()
            total_idle_time += idle_time
            total_active_time += active_time
        return total_idle_time, total_active_time



class cpuidle(object):
    def __init__(self, path):
        self.__base_path = path
        self.__states = []

        dirs = glob.glob(os.path.join(self.__base_path, 'state*'))
        if not dirs:
            raise error.TestError('cpuidle states missing')

        for dir in dirs:
            state = cpuidle_state(dir)
            self.__states.append(state)


    def idle_time(self):
        total_idle_time = 0
        total_active_time = 0
        for state in self.__states:
            total_idle_time += state.idle_time()
            total_active_time += state.active_time()

        return total_idle_time, total_active_time



class cpuidle_state(object):
    def __init__(self, path):
        self.__base_path = path
        self.__name = self.__read_file('name').split()[0]
        self.__latency = int(self.__read_file('latency').split()[0])


    def __read_file(self, file_name):
        path = os.path.join(self.__base_path, file_name)
        f = open(path, 'r')
        data = f.read()
        f.close()
        return data


    def __is_idle_state(self):
        if self.__latency:
            # non-zero latency indicates non-C0 state
            return True
        return False


    def idle_time(self):
        time = 0
        if self.__is_idle_state():
            time = int(self.__read_file('time'))
        logging.info('idle_time(%s): %d' % (self.__name, time))
        return time


    def active_time(self):
        time = 0
        if not self.__is_idle_state():
            time = int(self.__read_file('time'))
        logging.info('active_time(%s): %d' % (self.__name, time))
        return time
