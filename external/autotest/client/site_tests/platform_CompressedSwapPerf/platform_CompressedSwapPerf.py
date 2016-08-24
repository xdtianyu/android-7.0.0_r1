#!/usr/bin/python
#
# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging, numpy, os, shutil, socket
import struct, subprocess, tempfile, time

from autotest_lib.client.bin import utils, test
from autotest_lib.client.common_lib import error

def selection_sequential(cur_index, length):
    """
    Iterates over processes sequentially. This should cause worst-case
    behavior for an LRU swap policy.

    @param cur_index: Index of current hog (if sequential)
    @param length: Number of hog processes
    """
    return cur_index

def selection_exp(cur_index, length):
    """
    Iterates over processes randomly according to an exponential distribution.
    Simulates preference for a few long-lived tabs over others.

    @param cur_index: Index of current hog (if sequential)
    @param length: Number of hog processes
    """

    # Discard any values greater than the length of the array.
    # Inelegant, but necessary. Otherwise, the distribution will be skewed.
    exp_value = length
    while exp_value >= length:
        # Mean is index 4 (weights the first 4 indexes the most).
        exp_value = numpy.random.geometric(0.25) - 1
    return int(exp_value)

def selection_uniform(cur_index, length):
    """
    Iterates over processes randomly according to a uniform distribution.

    @param cur_index: Index of current hog (if sequential)
    @param length: Number of hog processes
    """
    return numpy.random.randint(0, length)

# The available selection functions to use.
selection_funcs = {'sequential': selection_sequential,
                   'exponential': selection_exp,
                   'uniform': selection_uniform}

def get_selection_funcs(selections):
    """
    Returns the selection functions listed by their names in 'selections'.

    @param selections: List of strings, where each string is a key for a
                       selection function
    """
    return {
            k: selection_funcs[k]
            for k in selection_funcs
            if k in selections
           }

def reset_zram():
    """
    Resets zram, clearing all swap space.
    """
    swapoff_timeout = 60
    zram_device = 'zram0'
    zram_device_path = os.path.join('/dev', zram_device)
    reset_path = os.path.join('/sys/block', zram_device, 'reset')
    disksize_path = os.path.join('/sys/block', zram_device, 'disksize')

    disksize = utils.read_one_line(disksize_path)

    # swapoff is prone to hanging, especially after heavy swap usage, so
    # time out swapoff if it takes too long.
    ret = utils.system('swapoff ' + zram_device_path,
                       timeout=swapoff_timeout, ignore_status=True)

    if ret != 0:
        raise error.TestFail('Could not reset zram - swapoff failed.')

    # Sleep to avoid "device busy" errors.
    time.sleep(1)
    utils.write_one_line(reset_path, '1')
    time.sleep(1)
    utils.write_one_line(disksize_path, disksize)
    utils.system('mkswap ' + zram_device_path)
    utils.system('swapon ' + zram_device_path)

swap_reset_funcs = {'zram': reset_zram}

class platform_CompressedSwapPerf(test.test):
    """Runs basic performance benchmarks on compressed swap.

    Launches a number of "hog" processes that can be told to "balloon"
    (allocating a specified amount of memory in 1 MiB chunks) and can
    also be "poked", which reads from and writes to random places in memory
    to force swapping in and out. Hog processes report back statistics on how
    long a "poke" took (real and CPU time) and number of page faults.
    """
    version = 1
    executable = 'hog'
    swap_enable_file = '/home/chronos/.swap_enabled'
    swap_disksize_file = '/sys/block/zram0/disksize'

    CMD_POKE = 1
    CMD_BALLOON = 2
    CMD_EXIT = 3

    CMD_FORMAT = "=L"
    CMD_FORMAT_SIZE = struct.calcsize(CMD_FORMAT)

    RESULT_FORMAT = "=QQQQ"
    RESULT_FORMAT_SIZE = struct.calcsize(RESULT_FORMAT)

    def setup(self):
        """
        Compiles the hog program.
        """
        os.chdir(self.srcdir)
        utils.make(self.executable)

    def report_stat(self, units, swap_target, selection, metric, stat, value):
        """
        Reports a single performance statistic. This function puts the supplied
        args into an autotest-approved format.

        @param units: String describing units of the statistic
        @param swap_target: Current swap target, 0.0 <= swap_target < 1.0
        @param selection: Name of selection function to report for
        @param metric: Name of the metric that is being reported
        @param stat: Name of the statistic (e.g. median, 99th percentile)
        @param value: Actual floating-point value
        """
        swap_target_str = '%.2f' % swap_target
        perfkey_name_list = [ units, 'swap', swap_target_str,
                              selection, metric, stat ]

        # Filter out any args that evaluate to false.
        perfkey_name_list = filter(None, perfkey_name_list)
        perf_key = '_'.join(perfkey_name_list)
        self.write_perf_keyval({perf_key: value})

    def report_stats(self, units, swap_target, selection, metric, values):
        """
        Reports interesting statistics from a list of recorded values.

        @param units: String describing units of the statistic
        @param swap_target: Current swap target
        @param selection: Name of current selection function
        @param metric: Name of the metric that is being reported
        @param values: List of floating point measurements for this metric
        """
        if not values:
            logging.info('Cannot report empty list!')
            return

        values = sorted(values)
        mean = float(sum(values)) / len(values)
        median = values[int(0.5*len(values))]
        percentile_95 = values[int(0.95*len(values))]
        percentile_99 = values[int(0.99*len(values))]

        self.report_stat(units, swap_target, selection, metric, 'mean', mean)
        self.report_stat(units, swap_target, selection,
                         metric, 'median', median)
        self.report_stat(units, swap_target, selection,
                         metric, '95th_percentile', percentile_95)
        self.report_stat(units, swap_target, selection,
                         metric, '99th_percentile', percentile_99)

    def sample_memory_state(self):
        """
        Samples memory info from /proc/meminfo and use that to calculate swap
        usage and total memory usage, adjusted for double-counting swap space.
        """
        self.mem_total = utils.read_from_meminfo('MemTotal')
        self.swap_total = utils.read_from_meminfo('SwapTotal')
        self.mem_free = utils.read_from_meminfo('MemFree')
        self.swap_free = utils.read_from_meminfo('SwapFree')
        self.swap_used = self.swap_total - self.swap_free

        used_phys_memory = self.mem_total - self.mem_free

        # Get zram's actual compressed size and convert to KiB.
        swap_phys_size = utils.read_one_line('/sys/block/zram0/compr_data_size')
        swap_phys_size = int(swap_phys_size) / 1024

        self.total_usage = used_phys_memory - swap_phys_size + self.swap_used
        self.usage_ratio = float(self.swap_used) / self.swap_total

    def send_poke(self, hog_sock):
        """Pokes a hog process.
        Poking a hog causes it to simulate activity and report back on
        the same socket.

        @param hog_sock: An open socket to the hog process
        """
        hog_sock.send(struct.pack(self.CMD_FORMAT, self.CMD_POKE))

    def send_balloon(self, hog_sock, alloc_mb):
        """Tells a hog process to allocate more memory.

        @param hog_sock: An open socket to the hog process
        @param alloc_mb: Amount of memory to allocate, in MiB
        """
        hog_sock.send(struct.pack(self.CMD_FORMAT, self.CMD_BALLOON))
        hog_sock.send(struct.pack(self.CMD_FORMAT, alloc_mb))

    def send_exit(self, hog_sock):
        """Tells a hog process to exit and closes the socket.

        @param hog_sock: An open socket to the hog process
        """
        hog_sock.send(struct.pack(self.CMD_FORMAT, self.CMD_EXIT))
        hog_sock.shutdown(socket.SHUT_RDWR)
        hog_sock.close()

    def recv_poke_results(self, hog_sock):
        """Returns the results from poking a hog as a tuple.

        @param hog_sock: An open socket to the hog process
        @return: A tuple (wall_time, user_time, sys_time, fault_count)
        """
        try:
            result = hog_sock.recv(self.RESULT_FORMAT_SIZE)
            if len(result) != self.RESULT_FORMAT_SIZE:
                logging.info("incorrect result, len %d",
                               len(result))
            else:
                result_unpacked = struct.unpack(self.RESULT_FORMAT, result)
                wall_time = result_unpacked[0]
                user_time = result_unpacked[1]
                sys_time = result_unpacked[2]
                fault_count = result_unpacked[3]

                return (wall_time, user_time, sys_time, fault_count)
        except socket.error:
            logging.info('Hog died while touching memory')



    def recv_balloon_results(self, hog_sock, alloc_mb):
        """Receives a balloon response from a hog.
        If a hog succeeds in allocating more memory, it will respond on its
        socket with the original allocation size.

        @param hog_sock: An open socket to the hog process
        @param alloc_mb: Amount of memory to allocate, in MiB
        @raise TestFail: Fails if hog could not allocate memory, or if
                         there is a communication problem.
        """
        balloon_result = hog_sock.recv(self.CMD_FORMAT_SIZE)
        if len(balloon_result) != self.CMD_FORMAT_SIZE:
            return False

        balloon_result_unpack = struct.unpack(self.CMD_FORMAT, balloon_result)

        return balloon_result_unpack == alloc_mb

    def run_single_test(self, compression_factor, num_procs, cycles,
                        swap_target, switch_delay, temp_dir, selections):
        """
        Runs the benchmark for a single swap target usage.

        @param compression_factor: Compression factor (int)
                                   example: compression_factor=3 is 1:3 ratio
        @param num_procs: Number of hog processes to use
        @param cycles: Number of iterations over hogs list for a given swap lvl
        @param swap_target: Floating point value of target swap usage
        @param switch_delay: Number of seconds to wait between poking hogs
        @param temp_dir: Path of the temporary directory to use
        @param selections: List of selection function names
        """
        # Get initial memory state.
        self.sample_memory_state()
        swap_target_usage = swap_target * self.swap_total

        # usage_target is our estimate on the amount of memory that needs to
        # be allocated to reach our target swap usage.
        swap_target_phys = swap_target_usage / compression_factor
        usage_target = self.mem_free - swap_target_phys + swap_target_usage

        hogs = []
        paths = []
        sockets = []
        cmd = [ os.path.join(self.srcdir, self.executable) ]

        # Launch hog processes.
        while len(hogs) < num_procs:
            socket_path = os.path.join(temp_dir, str(len(hogs)))
            paths.append(socket_path)
            launch_cmd = list(cmd)
            launch_cmd.append(socket_path)
            launch_cmd.append(str(compression_factor))
            p = subprocess.Popen(launch_cmd)
            utils.write_one_line('/proc/%d/oom_score_adj' % p.pid, '15')
            hogs.append(p)

        # Open sockets to hog processes, waiting for them to bind first.
        time.sleep(5)
        for socket_path in paths:
            hog_sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
            sockets.append(hog_sock)
            hog_sock.connect(socket_path)

        # Allocate conservatively until we reach our target.
        while self.usage_ratio <= swap_target:
            free_per_hog = (usage_target - self.total_usage) / len(hogs)
            alloc_per_hog_mb = int(0.80 * free_per_hog) / 1024
            if alloc_per_hog_mb <= 0:
                alloc_per_hog_mb = 1

            # Send balloon command.
            for hog_sock in sockets:
                self.send_balloon(hog_sock, alloc_per_hog_mb)

            # Wait until all hogs report back.
            for hog_sock in sockets:
                self.recv_balloon_results(hog_sock, alloc_per_hog_mb)

            # We need to sample memory and swap usage again.
            self.sample_memory_state()

        # Once memory is allocated, report how close we got to the swap target.
        self.report_stat('percent', swap_target, None,
                         'usage', 'value', self.usage_ratio)

        # Run tests by sending "touch memory" command to hogs.
        for f_name, f in get_selection_funcs(selections).iteritems():
            result_list = []

            for count in range(cycles):
                for i in range(len(hogs)):
                    selection = f(i, len(hogs))
                    hog_sock = sockets[selection]
                    retcode = hogs[selection].poll()

                    # Ensure that the hog is not dead.
                    if retcode is None:
                        # Delay between switching "tabs".
                        if switch_delay > 0.0:
                            time.sleep(switch_delay)

                        self.send_poke(hog_sock)

                        result = self.recv_poke_results(hog_sock)
                        if result:
                            result_list.append(result)
                    else:
                        logging.info("Hog died unexpectedly; continuing")

            # Convert from list of tuples (rtime, utime, stime, faults) to
            # a list of rtimes, a list of utimes, etc.
            results_unzipped = [list(x) for x in zip(*result_list)]
            wall_times = results_unzipped[0]
            user_times = results_unzipped[1]
            sys_times = results_unzipped[2]
            fault_counts = results_unzipped[3]

            # Calculate average time to service a fault for each sample.
            us_per_fault_list = []
            for i in range(len(sys_times)):
                if fault_counts[i] == 0.0:
                    us_per_fault_list.append(0.0)
                else:
                    us_per_fault_list.append(sys_times[i] * 1000.0 /
                                             fault_counts[i])

            self.report_stats('ms', swap_target, f_name, 'rtime', wall_times)
            self.report_stats('ms', swap_target, f_name, 'utime', user_times)
            self.report_stats('ms', swap_target, f_name, 'stime', sys_times)
            self.report_stats('faults', swap_target, f_name, 'faults',
                              fault_counts)
            self.report_stats('us_fault', swap_target, f_name, 'fault_time',
                              us_per_fault_list)

        # Send exit message to all hogs.
        for hog_sock in sockets:
            self.send_exit(hog_sock)

        time.sleep(1)

        # If hogs didn't exit normally, kill them.
        for hog in hogs:
            retcode = hog.poll()
            if retcode is None:
                logging.debug("killing all remaining hogs")
                utils.system("killall -TERM hog")
                # Wait to ensure hogs have died before continuing.
                time.sleep(5)
                break

    def run_once(self, compression_factor=3, num_procs=50, cycles=20,
                 selections=None, swap_targets=None, switch_delay=0.0):
        if selections is None:
            selections = ['sequential', 'uniform', 'exponential']
        if swap_targets is None:
            swap_targets = [0.00, 0.25, 0.50, 0.75, 0.95]

        swaptotal = utils.read_from_meminfo('SwapTotal')

        # Check for proper swap space configuration.
        # If the swap enable file says "0", swap.conf does not create swap.
        if os.path.exists(self.swap_enable_file):
            enable_size = utils.read_one_line(self.swap_enable_file)
        else:
            enable_size = "nonexistent" # implies nonzero
        if enable_size == "0":
            if swaptotal != 0:
                raise error.TestFail('The swap enable file said 0, but'
                                     ' swap was still enabled for %d.' %
                                     swaptotal)
            logging.info('Swap enable (0), swap disabled.')
        else:
            # Rather than parsing swap.conf logic to calculate a size,
            # use the value it writes to /sys/block/zram0/disksize.
            if not os.path.exists(self.swap_disksize_file):
                raise error.TestFail('The %s swap enable file should have'
                                     ' caused zram to load, but %s was'
                                     ' not found.' %
                                     (enable_size, self.swap_disksize_file))
            disksize = utils.read_one_line(self.swap_disksize_file)
            swaprequested = int(disksize) / 1000
            if (swaptotal < swaprequested * 0.9 or
                swaptotal > swaprequested * 1.1):
                raise error.TestFail('Our swap of %d K is not within 10%%'
                                     ' of the %d K we requested.' %
                                     (swaptotal, swaprequested))
            logging.info('Swap enable (%s), requested %d, total %d',
                         enable_size, swaprequested, swaptotal)

        # We should try to autodetect this if we add other swap methods.
        swap_method = 'zram'

        for swap_target in swap_targets:
            logging.info('swap_target is %f', swap_target)
            temp_dir = tempfile.mkdtemp()
            try:
                # Reset swap space to make sure nothing leaks between runs.
                swap_reset = swap_reset_funcs[swap_method]
                swap_reset()
                self.run_single_test(compression_factor, num_procs, cycles,
                                     swap_target, switch_delay, temp_dir,
                                     selections)
            except socket.error:
                logging.debug('swap target %f failed; oom killer?', swap_target)

            shutil.rmtree(temp_dir)

