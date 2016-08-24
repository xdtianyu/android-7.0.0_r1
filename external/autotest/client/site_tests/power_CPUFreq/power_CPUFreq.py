# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import glob, logging, os
from autotest_lib.client.bin import test
from autotest_lib.client.common_lib import error, utils

SYSFS_CPUQUIET_ENABLE = '/sys/devices/system/cpu/cpuquiet/tegra_cpuquiet/enable'
SYSFS_INTEL_PSTATE_PATH = '/sys/devices/system/cpu/intel_pstate'

class power_CPUFreq(test.test):
    version = 1

    def initialize(self):
        # Store the setting if the system has CPUQuiet feature
        if os.path.exists(SYSFS_CPUQUIET_ENABLE):
            self.is_cpuquiet_enabled = utils.read_file(SYSFS_CPUQUIET_ENABLE)
            utils.write_one_line(SYSFS_CPUQUIET_ENABLE, '0')

    def run_once(self):
        # TODO(crbug.com/485276) Revisit this exception once we've refactored
        # test to account for intel_pstate cpufreq driver
        if os.path.exists(SYSFS_INTEL_PSTATE_PATH):
            raise error.TestNAError('Test does NOT support intel_pstate driver')

        cpufreq_path = '/sys/devices/system/cpu/cpu*/cpufreq'

        dirs  = glob.glob(cpufreq_path)
        if not dirs:
            raise error.TestFail('cpufreq not supported')

        keyvals = {}
        try:
            # First attempt to set all frequencies on each core before going
            # on to the next core.
            self.test_cores_in_series(dirs)
            # Record that it was the first test that passed.
            keyvals['test_cores_in_series'] = 1
        except error.TestFail as exception:
            if str(exception) == 'Unable to set frequency':
                # If test_cores_in_series fails, try to set each frequency for
                # all cores before moving on to the next frequency.

                self.test_cores_in_parallel(dirs)
                # Record that it was the second test that passed.
                keyvals['test_cores_in_parallel'] = 1
            else:
                raise exception

        self.write_perf_keyval(keyvals);

    def test_cores_in_series(self, dirs):
        for dir in dirs:
            cpu = cpufreq(dir)

            if 'userspace' not in cpu.get_available_governors():
                raise error.TestError('userspace governor not supported')

            available_frequencies = cpu.get_available_frequencies()
            if len(available_frequencies) == 1:
                raise error.TestFail('Not enough frequencies supported!')

            # save cpufreq state so that it can be restored at the end
            # of the test
            cpu.save_state()

            # set cpufreq governor to userspace
            cpu.set_governor('userspace')

            # cycle through all available frequencies
            for freq in available_frequencies:
                cpu.set_frequency(freq)
                if freq != cpu.get_current_frequency():
                    cpu.restore_state()
                    raise error.TestFail('Unable to set frequency')

            # restore cpufreq state
            cpu.restore_state()

    def test_cores_in_parallel(self, dirs):
        cpus = [cpufreq(dir) for dir in dirs]
        cpu0 = cpus[0]

        # Use the first CPU's frequencies for all CPUs.  Assume that they are
        # the same.
        available_frequencies = cpu0.get_available_frequencies()
        if len(available_frequencies) == 1:
            raise error.TestFail('Not enough frequencies supported!')

        for cpu in cpus:
            if 'userspace' not in cpu.get_available_governors():
                raise error.TestError('userspace governor not supported')

            # save cpufreq state so that it can be restored at the end
            # of the test
            cpu.save_state()

            # set cpufreq governor to userspace
            cpu.set_governor('userspace')

        # cycle through all available frequencies
        for freq in available_frequencies:
            for cpu in cpus:
                cpu.set_frequency(freq)
            for cpu in cpus:
                if freq != cpu.get_current_frequency():
                    cpu.restore_state()
                    raise error.TestFail('Unable to set frequency')

        for cpu in cpus:
            # restore cpufreq state
            cpu.restore_state()

    def cleanup(self):
        # Restore the original setting if system has CPUQuiet feature
        if os.path.exists(SYSFS_CPUQUIET_ENABLE):
            utils.open_write_close(
                SYSFS_CPUQUIET_ENABLE, self.is_cpuquiet_enabled)

class cpufreq(object):
    def __init__(self, path):
        self.__base_path = path
        self.__save_files_list = ['scaling_max_freq', 'scaling_min_freq',
                                  'scaling_governor']


    def __write_file(self, file_name, data):
        path = os.path.join(self.__base_path, file_name)
        utils.open_write_close(path, data)


    def __read_file(self, file_name):
        path = os.path.join(self.__base_path, file_name)
        f = open(path, 'r')
        data = f.read()
        f.close()
        return data


    def save_state(self):
        logging.info('saving state:')
        for file in self.__save_files_list:
            data = self.__read_file(file)
            setattr(self, file, data)
            logging.info(file + ': '  + data)


    def restore_state(self):
        logging.info('restoring state:')
        for file in self.__save_files_list:
            # Sometimes a newline gets appended to a data string and it throws
            # an error when being written to a sysfs file.  Call strip() to
            # eliminateextra whitespace characters so it can be written cleanly
            # to the file.
            data = getattr(self, file).strip()
            logging.info(file + ': '  + data)
            self.__write_file(file, data)


    def get_available_governors(self):
        governors = self.__read_file('scaling_available_governors')
        logging.info('available governors: %s' % governors)
        return governors.split()


    def get_current_governor(self):
        governor = self.__read_file('scaling_governor')
        logging.info('current governor: %s' % governor)
        return governor.split()[0]


    def set_governor(self, governor):
        logging.info('setting governor to %s' % governor)
        self.__write_file('scaling_governor', governor)


    def get_available_frequencies(self):
        frequencies = self.__read_file('scaling_available_frequencies')
        logging.info('available frequencies: %s' % frequencies)
        return [int(i) for i in frequencies.split()]


    def get_current_frequency(self):
        freq = int(self.__read_file('scaling_cur_freq'))
        logging.info('current frequency: %s' % freq)
        return freq


    def set_frequency(self, frequency):
        logging.info('setting frequency to %d' % frequency)
        if frequency >= self.get_current_frequency():
            file_list = ['scaling_max_freq', 'scaling_min_freq',
                         'scaling_setspeed']
        else:
            file_list = ['scaling_min_freq', 'scaling_max_freq',
                         'scaling_setspeed']

        for file in file_list:
            self.__write_file(file, str(frequency))
