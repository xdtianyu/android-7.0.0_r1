# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""A test verifying Address Space Layout Randomization

Uses system calls to get important pids and then gets information about
the pids in /proc/<pid>/maps. Restarts the tested processes and reads
information about them again. If ASLR is enabled, memory mappings should
change.
"""

from autotest_lib.client.bin import test
from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.cros import upstart

import logging
import time
import pprint
import re

def _pidof(exe_name):
    """Returns the PID of the first process with the given name."""
    pid = utils.system_output('pidof %s' % exe_name, ignore_status=True).strip()
    if len(pid.split()) > 1:
        pid = pid.split()[0]

    return pid


class Process(object):
    """Holds information about a process.

    Stores basic information about a process. This class is a base for
    UpstartProcess and SystemdProcess declared below.

    Attributes:
        _name: String name of process.
        _service_name: Name of the service corresponding to the process.
        _parent: String name of process's parent. Defaults to None.
    """

    _START_POLL_INTERVAL_SECONDS = 1
    _START_TIMEOUT = 30

    def __init__(self, name, service_name, parent=None):
        self._name = name
        self._service_name = service_name
        self._parent = parent

    def get_name(self):
        return self._name

    def get_pid(self):
        """Gets pid of process, waiting for it if not found.

        Raises:
            error.TestFail: corresponding process is not found.
        """
        retries = 0
        ps_results = ""
        while retries < self._START_TIMEOUT:
            if self._parent is None:
                ps_results = _pidof(self._name)
            else:
                ppid = _pidof(self._parent)
                get_pid_command = ('ps -C %s -o pid,ppid | grep " %s$"'
                    ' | awk \'{print $1}\'') % (self._name, ppid)
                ps_results = utils.system_output(get_pid_command).strip()

            if ps_results != "":
                return ps_results

            # The process could not be found. We then sleep, hoping the
            # process is just slow to initially start.
            time.sleep(self._START_POLL_INTERVAL_SECONDS)
            retries += 1

        # We never saw the process, so abort with details on who was missing.
        raise error.TestFail('Never saw a pid for "%s"' % (self._name))


class UpstartProcess(Process):
    """Represents an Upstart service."""

    def exists(self):
        """Checks if the service is present in Upstart configuration."""
        return upstart.has_service(self._service_name)

    def restart(self):
        """Restarts the process via initctl."""
        utils.system('initctl restart %s' % self._service_name)

class SystemdProcess(Process):
    """Represents an systemd service."""

    def exists(self):
        """Checks if the service is present in systemd configuration."""
        cmd = 'systemctl show -p LoadState %s.service' % self._service_name
        output = utils.system_output(cmd, ignore_status=True).strip()
        return output == 'LoadState=loaded'

    def restart(self):
        """Restarts the process via systemctl."""
        utils.system('systemctl restart %s' % self._service_name)


class Mapping(object):
    """Holds information about a process's address mapping.

    Stores information about one memory mapping for a process.

    Attributes:
        _name: String name of process/memory occupying the location.
        _start: String containing memory address range start.
    """
    def __init__(self, name, start):
        self._start = start
        self._name = name

    def set_start(self, new_value):
        self._start = new_value

    def get_start(self):
        return self._start

    def __repr__(self):
        return "<mapping %s %s>" % (self._name, self._start)


class security_ASLR(test.test):
    """Runs ASLR tests

    See top document comments for more information.

    Attributes:
        version: Current version of the test.
    """
    version = 1

    _TEST_ITERATION_COUNT = 5

    _ASAN_SYMBOL = "__asan_init"

    # 'update_engine' should at least be present on all boards.
    _PROCESS_LIST = [UpstartProcess('chrome', 'ui', parent='session_manager'),
                     UpstartProcess('debugd', 'debugd'),
                     UpstartProcess('update_engine', 'update-engine'),
                     SystemdProcess('update_engine', 'update-engine')]


    def get_processes_to_test(self):
        """Gets processes to test for main function.

        Called by run_once to get processes for this program to test.
        Filters binaries that actually exist on the system.
        This has to be a method because it constructs process objects.

        Returns:
            A list of process objects to be tested (see below for
            definition of process class).
        """
        return [p for p in self._PROCESS_LIST if p.exists()]


    def running_on_asan(self):
        """Returns whether we're running on ASan."""
        # -q, --quiet         * Only output 'bad' things
        # -F, --format <arg>  * Use specified format for output
        # -g, --gmatch        * Use regex rather than string compare (with -s)
        # -s, --symbol <arg>  * Find a specified symbol
        scanelf_command = "scanelf -qF'%s#F'"
        scanelf_command += " -gs %s `which debugd`" % self._ASAN_SYMBOL
        symbol = utils.system_output(scanelf_command)
        logging.debug("running_on_asan(): symbol: '%s', _ASAN_SYMBOL: '%s'",
                      symbol, self._ASAN_SYMBOL)
        return symbol != ""


    def test_randomization(self, process):
        """Tests ASLR of a single process.

        This is the main test function for the program. It creates data
        structures out of useful information from sampling /proc/<pid>/maps
        after restarting the process and then compares address starting
        locations of all executable, stack, and heap memory from each iteration.

        @param process: a process object representing the process to be tested.

        Returns:
            A dict containing a Boolean for whether or not the test passed
            and a list of string messages about passing/failing cases.
        """
        test_result = dict([('pass', True), ('results', []), ('cases', dict())])
        name = process.get_name()
        mappings = list()
        pid = -1
        for i in range(self._TEST_ITERATION_COUNT):
            new_pid = process.get_pid()
            if pid == new_pid:
                raise error.TestFail(
                    'Service "%s" retained PID %d after restart.' % (name, pid))
            pid = new_pid
            mappings.append(self.map(pid))
            process.restart()
        logging.debug('Complete mappings dump for process %s:\n%s',
                      name, pprint.pformat(mappings, 4))

        initial_map = mappings[0]
        for i, mapping in enumerate(mappings[1:]):
            logging.debug('Iteration %d', i)
            for key in mapping.iterkeys():
                # Set default case result to fail, pass when an address change
                # occurs.
                if not test_result['cases'].has_key(key):
                    test_result['cases'][key] = dict([('pass', False),
                            ('number', 0),
                            ('total', self._TEST_ITERATION_COUNT)])
                was_same = (initial_map.has_key(key) and
                        initial_map[key].get_start() ==
                        mapping[key].get_start())
                if was_same:
                    logging.debug("Bad: %s address didn't change", key)
                else:
                    logging.debug('Good: %s address changed', key)
                    test_result['cases'][key]['number'] += 1
                    test_result['cases'][key]['pass'] = True
        for case, result in test_result['cases'].iteritems():
            if result['pass']:
                test_result['results'].append( '[PASS] Address for %s '
                        'successfully changed' % case)
            else:
                test_result['results'].append('[FAIL] Address for %s had '
                        'deterministic value: %s' % (case,
                        mappings[0][case].get_start()))
            test_result['pass'] = test_result['pass'] and result['pass']
        return test_result


    def map(self, pid):
        """Creates data structure from table in /proc/<pid>/maps.

        Gets all data from /proc/<pid>/maps, parses each entry, and saves
        entries corresponding to executable, stack, or heap memory into
        a dictionary.

        @param pid: a string containing the pid to be tested.

        Returns:
            A dict mapping names to mapping objects (see above for mapping
            definition).
        """
        memory_map = dict()
        maps_file = open("/proc/%s/maps" % pid)
        for maps_line in maps_file:
            result = self.parse_result(maps_line)
            if result is None:
                continue
            name = result['name']
            start = result['start']
            perms = result['perms']
            is_memory = name == '[heap]' or name == '[stack]'
            is_useful = re.search('x', perms) is not None or is_memory
            if not is_useful:
                continue
            if not name in memory_map:
                memory_map[name] = Mapping(name, start)
            elif memory_map[name].get_start() < start:
                memory_map[name].set_start(start)
        return memory_map


    def parse_result(self, result):
        """Builds dictionary from columns of a line of /proc/<pid>/maps

        Uses regular expressions to determine column separations. Puts
        column data into a dict mapping column names to their string values.

        @param result: one line of /proc/<pid>/maps as a string, for any <pid>.

        Returns:
            None if the regular expression wasn't matched. Otherwise:
            A dict of string column names mapped to their string values.
            For example:

        {'start': '9e981700000', 'end': '9e981800000', 'perms': 'rwxp',
            'something': '00000000', 'major': '00', 'minor': '00', 'inode':
            '00'}
        """
        # Build regex to parse one line of proc maps table.
        memory = r'(?P<start>\w+)-(?P<end>\w+)'
        perms = r'(?P<perms>(r|-)(w|-)(x|-)(s|p))'
        something = r'(?P<something>\w+)'
        devices = r'(?P<major>\w+):(?P<minor>\w+)'
        inode = r'(?P<inode>[0-9]+)'
        name = r'(?P<name>([a-zA-Z0-9/]+|\[heap\]|\[stack\]))'
        regex = r'%s +%s +%s +%s +%s +%s' % (memory, perms, something,
            devices, inode, name)
        found_match = re.match(regex, result)
        if found_match is None:
            return None
        parsed_result = found_match.groupdict()
        return parsed_result


    def run_once(self):
        """Main function.

        Called when test is run. Gets processes to test and calls test on
        them.

        Raises:
            error.TestFail if any processes' memory mapping addresses are the
            same after restarting.
        """

        if self.running_on_asan():
            logging.warning("security_ASLR is not available on ASan.")
            return

        processes = self.get_processes_to_test()
        # If we don't find any of the processes we wanted to test, we fail.
        if len(processes) == 0:
            proc_names = ", ".join([p.get_name() for p in self._PROCESS_LIST])
            raise error.TestFail(
                'Could not find any of "%s" processes to test' % proc_names)

        aslr_enabled = True
        full_results = dict()
        for process in processes:
            test_results = self.test_randomization(process)
            full_results[process.get_name()] = test_results['results']
            if not test_results['pass']:
                aslr_enabled = False

        logging.debug('SUMMARY:')
        for process_name, results in full_results.iteritems():
            logging.debug('Results for %s:', process_name)
            for result in results:
                logging.debug(result)

        if not aslr_enabled:
            raise error.TestFail('One or more processes had deterministic '
                    'memory mappings')
