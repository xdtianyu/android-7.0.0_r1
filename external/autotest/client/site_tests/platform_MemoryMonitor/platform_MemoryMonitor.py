# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

__author__ = 'namnguyen@chromium.org'

import collections
import itertools
import logging
import operator
import os
import re
import string

from autotest_lib.client.bin import utils, test


GeneralUsage = collections.namedtuple('GeneralUsage', 'total used free')
ProcessUsage = collections.namedtuple('ProcessUsage', 'pid user virtual '
    'resident shared command')  # command does NOT have arguments


def parse_mem(s):
    """Extracts a number out of a string such as 467m, 123k, 999g.

    @param s: a string to parse

    @return a float that s represents
    """

    multipliers = {'k': 1024, 'm': 1024**2, 'g': 1024**3}

    multiplier = multipliers.get(s[-1], 1)
    if multiplier != 1:
        s = s[:-1]

    return float(s) * multiplier


def parse_general_usage(line):
    """Extracts general memory usage from a line from top.

    @param line: string a general memory consumption line from top

    @return a GeneralUsage tuple
    """

    items = re.search(
        r'\s+(\d+) total,\s+(\d+) used,\s+(\d+) free', line).groups()
    return GeneralUsage(*[float(x) for x in items])


def parse_process_usage(line, headers):
    """Extracts memory usage numbers from a process line from top.

    @param line: string a process line from `top`
    @param headers: array of strings naming each field in the line

    @return a ProcessUsage tuple
    """

    interested_fields = {
        'pid': ('pid', int),
        'user': ('user', str),
        'virt': ('virtual', parse_mem),
        'res': ('resident', parse_mem),
        'shr': ('shared', parse_mem),
        'command': ('command', str),
    }

    fields = line.split()
    current_interest_idx = 0
    record = {}
    for i, field in enumerate(fields):
        if headers[i] not in interested_fields:
            continue
        key, extractor = interested_fields[headers[i]]
        record[key] = extractor(field)

    return ProcessUsage(**record)


def parse_processes(lines):
    """Extracts information about processes from `top`.

    @param lines: a list of lines from top, the header must be the first
        entry in this list
    @return a list of ProcessUsage
    """

    headers = [x.lower() for x in lines[0].split()]
    processes = []
    for line in lines[1:]:
        process_usage = parse_process_usage(line, headers)
        ignored = [process_usage.command.startswith(cmd) for cmd in
            ('autotest', 'top')]
        if any(ignored):
            continue
        processes.append(process_usage)
        logging.debug('Process usage: %r', process_usage)
    return processes


def report_top_processes(processes, n=10):
    """Returns a dictionary of top n processes.

    For example:
        {
            'top_1': 4000,
            'top_2': 3000,
            'top_3': 2500,
        }

    @param processes: a list of ProcessUsage
    @param n: maximum number of processes to return
    @return dictionary whose key correlate to the ranking, and values are
        amount of resident memory
    """

    get_resident = operator.attrgetter('resident')
    top_users = sorted(processes, key=get_resident, reverse=True)
    logging.info('Top 10 memory users:')
    perf_values = {}
    for i, process in enumerate(top_users[:n]):
        logging.info('%r', process)
        perf_values['top_%d' % (i + 1)] = process.resident
    return perf_values


def group_by_command(processes):
    """Returns resident memory of processes with the same command.

    For example:
        {
            'process_shill': 20971520,
            'process_sshd': 4792,
        }

    @param processes: a list of ProcessUsage
    @return dictionary whose keys correlate to the command line, and values
        the sum of resident memory used by all processes with the same
        command
    """

    get_command = operator.attrgetter('command')
    sorted_by_command = sorted(processes, key=get_command)
    grouped_by_command = itertools.groupby(sorted_by_command,
                                           key=get_command)
    top_by_command = []
    for command, grouped_processes in grouped_by_command:
        resident=sum(p.resident for p in grouped_processes)
        top_by_command.append((resident, command))
    top_by_command.sort(reverse=True)
    logging.info('Top processes by sum of memory consumption:')
    perf_values = {}
    for resident, command in top_by_command:
        command = command.replace(':', '_').replace('/', '_')
        logging.info('Command: %s, Resident: %f', command, resident)
        perf_values['process_%s' % command] = resident
    return perf_values


def group_by_service(processes):
    """Returns a collection of startup services and their memory usage.

    For example:
        {
            'service_chapsd': 6568,
            'service_cras': 3788,
            'service_ui': 329284024
        }

    @param processes: a list of ProcessUsage
    @returns dictionary whose keys correlate to the service name, and
        values are sum of resident memory used by that service
    """

    processes = dict((p.pid, p.resident) for p in processes)
    top_by_service = []
    initctl = utils.system_output('initctl list')
    logging.debug('Service list:\n%s', initctl)
    for line in initctl.split('\n'):
        if 'process' not in line:
            continue
        fields = line.split()
        service, main_process = fields[0], int(fields[3])
        resident = 0
        pstree = utils.system_output('pstree -p %d' % main_process)
        logging.debug('Service %s:\n%s', service, pstree)
        for pid in re.findall(r'\((\d+)\)', pstree, re.MULTILINE):
            pid = int(pid)
            logging.debug('Summing process %d', pid)
            resident += processes.get(pid, 0)
        top_by_service.append((resident, service))
    top_by_service.sort(reverse=True)
    logging.info('Top services:')
    perf_values = {}
    for resident, service in top_by_service:
        logging.info('Service: %s, Resident: %f', service, resident)
        perf_values['service_%s' % service] = resident
    return perf_values


def parse_smap(filename):
    """Parses /proc/*/smaps file to extract detailed memory usage of a process.

    The return value is a dictionary of component paths (such as "/bin/bash",
    "[vdso]") and 2-tuple (shared, private) sums of memory in KB.

    For example:

        {
            '/bin/bash': (460, 0),
            '/lib64/ld-2.15.so': (4, 50),
            '[vdso]': (4, 0),
            '[stack]': (20, 0),
        }

    @param filename: The full path to an smaps file
    @returns dictionary of component paths (such as "/bin/bash", "[vdso]") and
        2-tuple (shared, private) sums of memory in KB.
    """

    region_regexp = re.compile(
        r"""(?P<Address>[0-9A-Fa-f]+-[0-9A-Fa-f]+)\s+
            (?P<Permissions>[rwxsp-]{4})\s+
            (?P<Offset>[0-9A-Fa-f]+)\s+
            (?P<Device>[0-9A-Fa-f]+:[0-9A-Fa-f]+)\s+
            (?P<Inode>\d+)\s+
            (?P<Path>.+)?""", re.X)
    stat_regexp = re.compile(
        r"""(?P<Name>\w+):\s+
            (?P<Total>\d+)\s(?P<Unit>\w\w)""", re.X)

    regions = {}

    with open(filename, 'r') as smaps:
        current_address = None
        for line in smaps:
            parsed_region = region_regexp.match(line)
            if parsed_region:
                current_address = parsed_region.group('Address')
                if current_address in regions:
                    raise Exception('reused address %s' % current_address)
                regions[current_address] = {
                    'Path': parsed_region.group('Path'),
                    'Permissions': parsed_region.group('Permissions'),
                    'Offset': parsed_region.group('Offset')
                }
                continue

            parsed_stat = stat_regexp.match(line)
            if parsed_stat:
                name = parsed_stat.group('Name')
                total = int(parsed_stat.group('Total'))
                regions[current_address][name] = total
                continue

    paths = collections.defaultdict(lambda: (0, 0))
    for stats in regions.values():
        path = stats['Path']
        shared = stats['Shared_Clean'] + stats['Shared_Dirty']
        private = stats['Private_Clean'] + stats['Private_Dirty']
        paths[path] = (paths[path][0] + shared, paths[path][1] + private)

    return paths


def report_shared_libraries(processes):
    """Report memory used by shared libraries.

    This is the sum of (maximum shared memory across all processes, and sum of
    all private memory across all processes) in bytes.

    For example:

        {
            'lib_usr_lib64_xorg_modules_input_evdev_drv.so': 69632,
            'lib_opt_google_chrome_pepper_libpepflashplayer.so': 23928832,
        }

    @param processes: List of ProcessUsage objects
    @returns dictionary whose keys correlate to the library name, and
        values are sum of resident memory used by that library
    """

    allowed_chars = string.ascii_letters + string.digits + '.-_'
    libs_max_shared_sum_private = collections.defaultdict(lambda: (0, 0))
    for process in processes:
        proc_exe = None
        try:
            exe_link = '/proc/%d/exe' % process.pid
            proc_exe = os.readlink(exe_link)
        except OSError:
            # os.readlink() raises OSError if file is not found.
            continue

        components = None
        try:
            smap_file = '/proc/%d/smaps' % process.pid
            components = parse_smap(smap_file)
        except IOError:
            # IOError can be raised if smaps is not found.
            continue

        for lib, (shared, private) in components.items():
            # smaps file contains info about stack, and heap too but we are
            # only interested in the shared library.
            if not lib or lib.startswith('[') or lib == proc_exe:
                continue
            # Filter key to comply with OutputPerfValues().
            key = [(c if c in allowed_chars else '_') for c in lib]
            key = 'lib' + ''.join(key)
            max_shared, sum_private = libs_max_shared_sum_private[key]
            max_shared = max(max_shared, shared)
            sum_private += private
            libs_max_shared_sum_private[key] = (max_shared, sum_private)

    smaps = dict((key, sum(values) * 1024) for key, values in
                 libs_max_shared_sum_private.items())
    return smaps


class platform_MemoryMonitor(test.test):
    """Monitor memory usage trend."""

    version = 1

    def run_once(self):
        """Execute the test logic."""

        cmd = 'top -b -n 1'
        output = utils.system_output(cmd)
        logging.debug('Output from top:\n%s', output)
        lines = output.split('\n')
        # Ignore the first 3 lines, they're not relevant in this test.
        lines = lines[3:]
        mem_general = parse_general_usage(lines[0])
        logging.info('Total, used, and free memory (in KiB): %r, %r, %r',
                     *mem_general)
        swap_general = parse_general_usage(lines[1])
        logging.info('Total, used, and free swap (in KiB): %r, %r, %r',
                     *swap_general)

        perf_values = {
            'mem_total': mem_general.total * 1024,
            'mem_used': mem_general.used * 1024,
            'mem_free': mem_general.free * 1024,
            'swap_total': swap_general.total * 1024,
            'swap_used': swap_general.used * 1024,
            'swap_free': swap_general.free * 1024,
        }

        # Ignore general mem, swap and a blank line.
        lines = lines[3:]

        processes = parse_processes(lines)
        perf_values.update(report_top_processes(processes))
        perf_values.update(group_by_command(processes))
        perf_values.update(group_by_service(processes))
        perf_values.update(report_shared_libraries(processes))

        for key, val in perf_values.items():
            graph_name = key.split('_')[0]
            self.output_perf_value(key, val, units="bytes",
                higher_is_better=False, graph=graph_name)
        self.write_perf_keyval(perf_values)
