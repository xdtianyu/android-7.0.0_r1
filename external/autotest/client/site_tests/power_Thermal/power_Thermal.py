# Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import glob, logging, os, tempfile, threading, time
from autotest_lib.client.bin import test
from autotest_lib.client.common_lib import error, utils

class PlatformDescriptor(object):
    '''
    An object to keep platform specific information.

    @num_cores - number of CPU cores in this platform
    @max_cpu_freq - maximum frequency the CPU can be running at
    @min_cpu_freq - minimal frequency the CPU can be running at
    '''

    def __init__(self, num_cores, max_cpu_freq, min_cpu_freq):
        self.num_cores = num_cores
        self.max_cpu_freq = max_cpu_freq
        self.min_cpu_freq = min_cpu_freq


# Base name of the sysfs file where CPU temperature is reported. The file is
# exported by the temperature monitor driver and is located in the appropriate
# device's subtree. We use the file name to locate the subtree, only one file
# with this name is expected to exist in /sys. The ext_ prefix indicates that
# this is a reading off a sensor located next to the CPU. This facility could
# be not available on some platforms, the test would need to be updated to
# accommodate those.
#
# The `standard' temperature reading available through
# /sys/class/hwmon/hwmon0/device/temperature does not represent the actual CPU
# temperature and when the CPU load changes, the 'standard' temperature
# reading changes much slower and not to such a large extent than the value in
# */ext_temperature.
EXT_TEMP_SENSOR_FILE = 'ext_temperature'

# Base name of the file where the throttling temperature is set (if CPU temp
# exceeds this value, clock throttling starts).
THROTTLE_EXT_LIMIT_FILE = 'throttle_ext_limit'

# Root directory for all sysfs information about the CPU(s).
CPU_INFO_ROOT = '/sys/devices/system/cpu'

# Template to get access to the directory/file containing current per core
# information.
PER_CORE_FREQ_TEMPLATE = CPU_INFO_ROOT + '/cpu%d/cpufreq/%s'

# Base name for the temporary files used by this test.
TMP_FILE_TEMPLATE = '/tmp/thermal_'

# Temperature difference expected to be caused by increased CPU activity.
DELTA = 3.0

# Name of the file controlling core's clocking discipline.
GOVERNOR = 'scaling_governor'

# Name of the file providing space separated list of available clocking
# disciplines.
AVAILABLE_GOVERNORS = 'scaling_available_governors'

def clean_up(obj):
    '''
    A function to register with the autotest engine to ensure proper cleanup.

    It will be called after the test has run, either completing successfully
    or throwing an exception.
    '''

    obj.cleanup()


class power_Thermal(test.test):
    version = 1


    def _cpu_heater(self):
        '''
        A function to execute some code to heat up the target.

        This function is run on a separate thread, all it does - opens a file
        for writing, writes it with 100K characters, closes and removes the
        file, it is running in a tight loop until the stop_all_workers flag
        turns True.

        Multiple threads are spawn to cause maximum CPU activity.
        '''

        (handle, fname) = tempfile.mkstemp(
            prefix=os.path.basename(TMP_FILE_TEMPLATE),
            dir=os.path.dirname(TMP_FILE_TEMPLATE))
        os.close(handle)
        os.remove(fname)
        while not self.stop_all_workers:
            f = open(fname, 'w')
            f.write('x' * 100000)
            f.close()
            os.remove(fname)


    def _add_heater_thread(self):
        '''Add a thread to run another instance of _cpu_heater().'''

        thread_count = len(self.worker_threads)
        logging.info('adding thread number %d' % thread_count)
        new_thread = threading.Thread(target=self._cpu_heater)
        self.worker_threads.append(new_thread)
        new_thread.daemon = True
        new_thread.start()


    def _throttle_count(self):
        '''
        Return current throttling status of all cores.

        The return integer value is the sum of all cores' throttling status.
        When the sum is equal the core number - all cores are throttling.
        '''

        count = 0
        for cpu in range(self.pl_desc.num_cores):
            count += int(utils.read_file(
                    PER_CORE_FREQ_TEMPLATE % (cpu, 'throttle')))
        return count


    def _cpu_freq(self, cpu):
        '''Return current clock frequency of a CPU, integer in Kilohertz.'''

        return int(utils.read_file(
                PER_CORE_FREQ_TEMPLATE % (cpu, 'cpuinfo_cur_freq')))


    def _cpu_temp(self):
        '''Return current CPU temperature, a float value.'''

        return float(utils.read_file(
                os.path.join(self.temperature_data_path, EXT_TEMP_SENSOR_FILE)))


    def _throttle_limit(self):
        '''
        Return current CPU throttling temperature threshold.

        If CPU temperature exceeds this value, clock throttling is activated,
        causing CPU slowdown.

        Returns the limit as a float value.
        '''

        return float(utils.read_file(
                os.path.join(self.temperature_data_path,
                             THROTTLE_EXT_LIMIT_FILE)))


    def _set_throttle_limit(self, new_limit):
        '''
        Set current CPU throttling temperature threshold.

        The passed in float value is rounded to the nearest integer.
        '''

        utils.open_write_close(
            os.path.join(
                self.temperature_data_path, THROTTLE_EXT_LIMIT_FILE),
            '%d' % int(round(new_limit)))


    def _check_freq(self):
        '''Verify that all CPU clocks are in range for this target.'''

        for cpu in range(self.pl_desc.num_cores):
            freq = self._cpu_freq(cpu)
            if self.pl_desc.min_cpu_freq <= freq <= self.pl_desc.max_cpu_freq:
                return
            raise error.TestError('Wrong cpu %d frequency reading %d' % (
                    cpu, freq))


    def _get_cpu_freq_raised(self):
        '''
        Bring all cores clock to max frequency.

        This function uses the scaling_governor mechanism to force the cores
        to run at maximum frequency, writing the string 'performance' into
        each core's governor file.

        The current value (if not 'performance') is preserved to be restored
        in the end of the test.

        Returns a dictionary where keys are the core numbers and values are
        the preserved governor setting.

        raises TestError in case 'performance' setting is not allowed on any
               of the cores, or the clock frequency does not reach max on any
               of the cores in 1 second.
        '''

        rv = {}
        for cpu in range(self.pl_desc.num_cores):
            target = 'performance'
            gov_file = PER_CORE_FREQ_TEMPLATE % (cpu, GOVERNOR)
            current_gov = utils.read_file(gov_file).strip()
            available_govs = utils.read_file(PER_CORE_FREQ_TEMPLATE % (
                    cpu, AVAILABLE_GOVERNORS)).split()

            if current_gov != target:
                if not target in available_govs:
                    raise error.TestError('core %d does not allow setting %s'
                                          % (cpu, target))
                logging.info('changing core %d governor from %s to %s' % (
                        cpu, current_gov, target))
                utils.open_write_close(gov_file, target)
                rv[cpu] = current_gov

        for _ in range(2):  # Wait for no more than 1 second
            for cpu in range(self.pl_desc.num_cores):
                if self._cpu_freq(cpu) != self.pl_desc.max_cpu_freq:
                    break
            else:
                return rv

        freqs = []
        for cpu in range(self.pl_desc.num_cores):
            freqs.append('%d' % self._cpu_freq(cpu))
        raise error.TestError('failed to speed up some CPU clocks: %s' %
                              ', '.join(freqs))


    def _get_cpu_temp_raised(self):
        '''
        Start more threads to increase CPU temperature.

        This function starts 10 threads and waits till either of the two
        events happen:

        - the throttling is activated (the threshold is expected to be set at
          DELTA/2 above the temperature when the test started). This is
          considered a success, the function returns.

        - the temperature raises DELTA degrees above the original temperature
          but throttling does not start. This is considered an overheating
          failure, a test error is raised.

        If the temperature does not reach the DELTA and throttling does not
        start in 30 seconds - a test error is also raised in this case.
        '''

        base_temp = self._cpu_temp()
        # Start 10 more cpu heater threads
        for _ in range(10):
            self._add_heater_thread()

        # Wait 30 seconds for the temp to raise DELTA degrees or throttling to
        # start
        for count in range(30):
            new_temp = self._cpu_temp()
            if new_temp - base_temp >= DELTA:
                raise error.TestError(
                    'Reached temperature of %2.1fC in %d'
                    ' seconds, no throttling.'
                    % count)
            if self._throttle_count() == self.pl_desc.num_cores:
                logging.info('full throttle after %d seconds' % count)
                return
            time.sleep(1)
        raise error.TestError(
            'failed to raise CPU temperature from %s (reached %s), '
            '%d cores throttled' % (
                str(base_temp), str(new_temp), self._throttle_count()))

    def _get_platform_descriptor(self):
        '''Fill out the platform descriptor to be used by the test.'''

        present = utils.read_file(os.path.join(CPU_INFO_ROOT, 'present'))
        if present.count('-') != 1:
            raise error.TestError(
                "can't determine number of cores from %s" % present)
        (min_core, max_core) = tuple(int(x) for x in present.split('-'))
        min_freq = int(utils.read_file(
            PER_CORE_FREQ_TEMPLATE % (0, 'cpuinfo_min_freq')))
        max_freq = int(utils.read_file(
            PER_CORE_FREQ_TEMPLATE % (0, 'cpuinfo_max_freq')))

        return PlatformDescriptor(max_core - min_core + 1, max_freq, min_freq)


    def _prepare_test(self):
        '''Prepare test: check initial conditions and set variables.'''

        ext_temp_path = utils.system_output(
            'find /sys -name %s' % EXT_TEMP_SENSOR_FILE).splitlines()
        if len(ext_temp_path) != 1:
            raise error.TestError('found %d sensor files' % len(ext_temp_path))

        self.temperature_data_path = os.path.dirname(ext_temp_path[0])

        self.stop_all_workers = False

        self.pl_desc = self._get_platform_descriptor()

        # Verify CPU frequency is in range.
        self._check_freq()

        # Make sure we are not yet throttling.
        if self._throttle_count():
            raise error.TestError('Throttling active before test started')

        # Remember throttling level setting before test started.
        self.preserved_throttle_limit = self._throttle_limit()

        if self.preserved_throttle_limit - self._cpu_temp() < 4 * DELTA:
            raise error.TestError('Target is too hot: %s C' % str(
                    self._cpu_temp()))

        # list to keep track of threads started to heat up CPU.
        self.worker_threads = []

        # Dictionary of saved cores' scaling governor settings.
        self.saved_governors = {}

        self.register_after_iteration_hook(clean_up)


    def run_once(self):
        self._prepare_test()
        logging.info('starting temperature is %s' % str(self._cpu_temp()))
        logging.info('starting frequency is %s' % str(self._cpu_freq(0)))

        self.saved_governors = self._get_cpu_freq_raised()
        self._set_throttle_limit(self._cpu_temp() + DELTA/2)
        self._get_cpu_temp_raised()
        self._set_throttle_limit(self.preserved_throttle_limit)

        # Half a second after restoring the throttling limit is plenty for
        # throttling to stop.
        time.sleep(.5)
        if self._throttle_count():
            raise error.TestError('Throttling did not stop')

        logging.info('ending temperature is %s' % str(self._cpu_temp()))
        logging.info('ending frequency is %s' % str(self._cpu_freq(0)))


    def cleanup(self):
        self.stop_all_workers = True
        self._set_throttle_limit(self.preserved_throttle_limit)
        logging.info('stopping %d thread(s)' % len(self.worker_threads))
        runaway_threads = 0
        while self.worker_threads:
            t = self.worker_threads.pop()
            t.join(.5)
            if t.isAlive():
                runaway_threads += 1
        if runaway_threads:
            for f in glob.glob('%s*' % TMP_FILE_TEMPLATE):
                logging.info('removing %s' % f)
                os.remove(f)
            raise error.TestError(
                'Failed to join %d worker thread(s)' % runaway_threads)

        if not self.saved_governors:
            return

        for (cpu, gov) in self.saved_governors.iteritems():
            gov_file = PER_CORE_FREQ_TEMPLATE % (cpu, GOVERNOR)
            logging.info('restoring core %d governor to %s' % (cpu, gov))
            utils.open_write_close(gov_file, gov)
        self.saved_governors = {}
