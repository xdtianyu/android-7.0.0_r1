#pylint: disable=C0111

# Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import glob
import json
import logging
import os
import platform
import re
import signal
import tempfile
import time
import uuid

from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib import utils
from autotest_lib.client.bin import base_utils

_UI_USE_FLAGS_FILE_PATH = '/etc/ui_use_flags.txt'
_INTEL_PCI_IDS_FILE_PATH = '/usr/local/autotest/bin/intel_pci_ids.json'

pciid_to_intel_architecture = {}


class TimeoutError(error.TestError):
    """Error raised when we time out when waiting on a condition."""
    pass


class Crossystem(object):
    """A wrapper for the crossystem utility."""

    def __init__(self, client):
        self.cros_system_data = {}
        self._client = client

    def init(self):
        self.cros_system_data = {}
        (_, fname) = tempfile.mkstemp()
        f = open(fname, 'w')
        self._client.run('crossystem', stdout_tee=f)
        f.close()
        text = utils.read_file(fname)
        for line in text.splitlines():
            assignment_string = line.split('#')[0]
            if not assignment_string.count('='):
                continue
            (name, value) = assignment_string.split('=', 1)
            self.cros_system_data[name.strip()] = value.strip()
        os.remove(fname)

    def __getattr__(self, name):
        """
        Retrieve a crosssystem attribute.

        The call crossystemobject.name() will return the crossystem reported
        string.
        """
        return lambda: self.cros_system_data[name]


def get_oldest_pid_by_name(name):
    """
    Return the oldest pid of a process whose name perfectly matches |name|.

    name is an egrep expression, which will be matched against the entire name
    of processes on the system.  For example:

      get_oldest_pid_by_name('chrome')

    on a system running
      8600 ?        00:00:04 chrome
      8601 ?        00:00:00 chrome
      8602 ?        00:00:00 chrome-sandbox

    would return 8600, as that's the oldest process that matches.
    chrome-sandbox would not be matched.

    Arguments:
      name: egrep expression to match.  Will be anchored at the beginning and
            end of the match string.

    Returns:
      pid as an integer, or None if one cannot be found.

    Raises:
      ValueError if pgrep returns something odd.
    """
    str_pid = utils.system_output(
        'pgrep -o ^%s$' % name,
        ignore_status=True).rstrip()
    if str_pid:
        return int(str_pid)


def get_oldest_by_name(name):
    """Return pid and command line of oldest process whose name matches |name|.

    @param name: egrep expression to match desired process name.
    @return: A tuple of (pid, command_line) of the oldest process whose name
             matches |name|.

    """
    pid = get_oldest_pid_by_name(name)
    if pid:
        command_line = utils.system_output('ps -p %i -o command=' % pid,
                                           ignore_status=True).rstrip()
        return (pid, command_line)


def get_chrome_remote_debugging_port():
    """Returns remote debugging port for Chrome.

    Parse chrome process's command line argument to get the remote debugging
    port.
    """
    _, command = get_oldest_by_name('chrome')
    matches = re.search('--remote-debugging-port=([0-9]+)', command)
    if matches:
        return int(matches.group(1))


def get_process_list(name, command_line=None):
    """
    Return the list of pid for matching process |name command_line|.

    on a system running
      31475 ?    0:06 /opt/google/chrome/chrome --allow-webui-compositing -
      31478 ?    0:00 /opt/google/chrome/chrome-sandbox /opt/google/chrome/
      31485 ?    0:00 /opt/google/chrome/chrome --type=zygote --log-level=1
      31532 ?    1:05 /opt/google/chrome/chrome --type=renderer

    get_process_list('chrome')
    would return ['31475', '31485', '31532']

    get_process_list('chrome', '--type=renderer')
    would return ['31532']

    Arguments:
      name: process name to search for. If command_line is provided, name is
            matched against full command line. If command_line is not provided,
            name is only matched against the process name.
      command line: when command line is passed, the full process command line
                    is used for matching.

    Returns:
      list of PIDs of the matching processes.

    """
    # TODO(rohitbm) crbug.com/268861
    flag = '-x' if not command_line else '-f'
    name = '\'%s.*%s\'' % (name, command_line) if command_line else name
    str_pid = utils.system_output(
        'pgrep %s %s' % (flag, name),
        ignore_status=True).rstrip()
    return str_pid.split()


def nuke_process_by_name(name, with_prejudice=False):
    """Tell the oldest process specified by name to exit.

    Arguments:
      name: process name specifier, as understood by pgrep.
      with_prejudice: if True, don't allow for graceful exit.

    Raises:
      error.AutoservPidAlreadyDeadError: no existing process matches name.
    """
    try:
        pid = get_oldest_pid_by_name(name)
    except Exception as e:
        logging.error(e)
        return
    if pid is None:
        raise error.AutoservPidAlreadyDeadError(
            'No process matching %s.' % name)
    if with_prejudice:
        utils.nuke_pid(pid, [signal.SIGKILL])
    else:
        utils.nuke_pid(pid)


def ensure_processes_are_dead_by_name(name, timeout_sec=10):
    """Terminate all processes specified by name and ensure they're gone.

    Arguments:
      name: process name specifier, as understood by pgrep.
      timeout_sec: maximum number of seconds to wait for processes to die.

    Raises:
      error.AutoservPidAlreadyDeadError: no existing process matches name.
      site_utils.TimeoutError: if processes still exist after timeout_sec.
    """
    def list_and_kill_processes(name):
        process_list = get_process_list(name)
        try:
            for pid in [int(str_pid) for str_pid in process_list]:
                utils.nuke_pid(pid)
        except error.AutoservPidAlreadyDeadError:
            pass
        return process_list

    poll_for_condition(lambda: list_and_kill_processes(name) == [],
                       timeout=timeout_sec)


def poll_for_condition(condition, exception=None, timeout=10,
                       sleep_interval=0.1, desc=None):
    """Poll until a condition becomes true.

    Arguments:
      condition: function taking no args and returning bool
      exception: exception to throw if condition doesn't become true
      timeout: maximum number of seconds to wait
      sleep_interval: time to sleep between polls
      desc: description of default TimeoutError used if 'exception' is None

    Returns:
      The true value that caused the poll loop to terminate.

    Raises:
      'exception' arg if supplied; site_utils.TimeoutError otherwise
    """
    start_time = time.time()
    while True:
        value = condition()
        if value:
            return value
        if time.time() + sleep_interval - start_time > timeout:
            if exception:
                logging.error(exception)
                raise exception

            if desc:
                desc = 'Timed out waiting for condition: %s' % desc
            else:
                desc = 'Timed out waiting for unnamed condition'
            logging.error(desc)
            raise TimeoutError, desc

        time.sleep(sleep_interval)


def save_vm_state(checkpoint):
    """Saves the current state of the virtual machine.

    This function is a NOOP if the test is not running under a virtual machine
    with the USB serial port redirected.

    Arguments:
      checkpoint - Name used to identify this state

    Returns:
      None
    """
    # The QEMU monitor has been redirected to the guest serial port located at
    # /dev/ttyUSB0. To save the state of the VM, we just send the 'savevm'
    # command to the serial port.
    proc = platform.processor()
    if 'QEMU' in proc and os.path.exists('/dev/ttyUSB0'):
        logging.info('Saving VM state "%s"', checkpoint)
        serial = open('/dev/ttyUSB0', 'w')
        serial.write("savevm %s\r\n" % checkpoint)
        logging.info('Done saving VM state "%s"', checkpoint)


def check_raw_dmesg(dmesg, message_level, whitelist):
    """Checks dmesg for unexpected warnings.

    This function parses dmesg for message with message_level <= message_level
    which do not appear in the whitelist.

    Arguments:
      dmesg - string containing raw dmesg buffer
      message_level - minimum message priority to check
      whitelist - messages to ignore

    Returns:
      List of unexpected warnings
    """
    whitelist_re = re.compile(r'(%s)' % '|'.join(whitelist))
    unexpected = []
    for line in dmesg.splitlines():
        if int(line[1]) <= message_level:
            stripped_line = line.split('] ', 1)[1]
            if whitelist_re.search(stripped_line):
                continue
            unexpected.append(stripped_line)
    return unexpected

def verify_mesg_set(mesg, regex, whitelist):
    """Verifies that the exact set of messages are present in a text.

    This function finds all strings in the text matching a certain regex, and
    then verifies that all expected strings are present in the set, and no
    unexpected strings are there.

    Arguments:
      mesg - the mutiline text to be scanned
      regex - regular expression to match
      whitelist - messages to find in the output, a list of strings
          (potentially regexes) to look for in the filtered output. All these
          strings must be there, and no other strings should be present in the
          filtered output.

    Returns:
      string of inconsistent findings (i.e. an empty string on success).
    """

    rv = []

    missing_strings = []
    present_strings = []
    for line in mesg.splitlines():
        if not re.search(r'%s' % regex, line):
            continue
        present_strings.append(line.split('] ', 1)[1])

    for string in whitelist:
        for present_string in list(present_strings):
            if re.search(r'^%s$' % string, present_string):
                present_strings.remove(present_string)
                break
        else:
            missing_strings.append(string)

    if present_strings:
        rv.append('unexpected strings:')
        rv.extend(present_strings)
    if missing_strings:
        rv.append('missing strings:')
        rv.extend(missing_strings)

    return '\n'.join(rv)


def target_is_pie():
    """Returns whether the toolchain produces a PIE (position independent
    executable) by default.

    Arguments:
      None

    Returns:
      True if the target toolchain produces a PIE by default.
      False otherwise.
    """

    command = 'echo | ${CC} -E -dD -P - | grep -i pie'
    result = utils.system_output(command,
                                 retain_output=True,
                                 ignore_status=True)
    if re.search('#define __PIE__', result):
        return True
    else:
        return False


def target_is_x86():
    """Returns whether the toolchain produces an x86 object

    Arguments:
      None

    Returns:
      True if the target toolchain produces an x86 object
      False otherwise.
    """

    command = 'echo | ${CC} -E -dD -P - | grep -i 86'
    result = utils.system_output(command,
                                 retain_output=True,
                                 ignore_status=True)
    if re.search('__i386__', result) or re.search('__x86_64__', result):
        return True
    else:
        return False


def mounts():
    ret = []
    for line in file('/proc/mounts'):
        m = re.match(
            r'(?P<src>\S+) (?P<dest>\S+) (?P<type>\S+) (?P<opts>\S+).*', line)
        if m:
            ret.append(m.groupdict())
    return ret


def is_mountpoint(path):
    return path in [m['dest'] for m in mounts()]


def require_mountpoint(path):
    """
    Raises an exception if path is not a mountpoint.
    """
    if not is_mountpoint(path):
        raise error.TestFail('Path not mounted: "%s"' % path)


def random_username():
    return str(uuid.uuid4()) + '@example.com'


def get_signin_credentials(filepath):
    """Returns user_id, password tuple from credentials file at filepath.

    File must have one line of the format user_id:password

    @param filepath: path of credentials file.
    @return user_id, password tuple.
    """
    user_id, password = None, None
    if os.path.isfile(filepath):
        with open(filepath) as f:
            user_id, password = f.read().rstrip().split(':')
    return user_id, password


def parse_cmd_output(command, run_method=utils.run):
    """Runs a command on a host object to retrieve host attributes.

    The command should output to stdout in the format of:
    <key> = <value> # <optional_comment>


    @param command: Command to execute on the host.
    @param run_method: Function to use to execute the command. Defaults to
                       utils.run so that the command will be executed locally.
                       Can be replace with a host.run call so that it will
                       execute on a DUT or external machine. Method must accept
                       a command argument, stdout_tee and stderr_tee args and
                       return a result object with a string attribute stdout
                       which will be parsed.

    @returns a dictionary mapping host attributes to their values.
    """
    result = {}
    # Suppresses stdout so that the files are not printed to the logs.
    cmd_result = run_method(command, stdout_tee=None, stderr_tee=None)
    for line in cmd_result.stdout.splitlines():
        # Lines are of the format "<key>     = <value>      # <comment>"
        key_value = re.match(r'^\s*(?P<key>[^ ]+)\s*=\s*(?P<value>[^ '
                             r']+)(?:\s*#.*)?$', line)
        if key_value:
            result[key_value.group('key')] = key_value.group('value')
    return result


def set_from_keyval_output(out, delimiter=' '):
    """Parse delimiter-separated key-val output into a set of tuples.

    Output is expected to be multiline text output from a command.
    Stuffs the key-vals into tuples in a set to be later compared.

    e.g.  deactivated 0
          disableForceClear 0
          ==>  set(('deactivated', '0'), ('disableForceClear', '0'))

    @param out: multiple lines of space-separated key-val pairs.
    @param delimiter: character that separates key from val. Usually a
                      space but may be '=' or something else.
    @return set of key-val tuples.
    """
    results = set()
    kv_match_re = re.compile('([^ ]+)%s(.*)' % delimiter)
    for linecr in out.splitlines():
        match = kv_match_re.match(linecr.strip())
        if match:
            results.add((match.group(1), match.group(2)))
    return results


def get_cpu_usage():
    """Returns machine's CPU usage.

    This function uses /proc/stat to identify CPU usage.
    Returns:
        A dictionary with 'user', 'nice', 'system' and 'idle' values.
        Sample dictionary:
        {
            'user': 254544,
            'nice': 9,
            'system': 254768,
            'idle': 2859878,
        }
    """
    proc_stat = open('/proc/stat')
    cpu_usage_str = proc_stat.readline().split()
    proc_stat.close()
    return {
        'user': int(cpu_usage_str[1]),
        'nice': int(cpu_usage_str[2]),
        'system': int(cpu_usage_str[3]),
        'idle': int(cpu_usage_str[4])
    }


def compute_active_cpu_time(cpu_usage_start, cpu_usage_end):
    """Computes the fraction of CPU time spent non-idling.

    This function should be invoked using before/after values from calls to
    get_cpu_usage().
    """
    time_active_end = (
        cpu_usage_end['user'] + cpu_usage_end['nice'] + cpu_usage_end['system'])
    time_active_start = (cpu_usage_start['user'] + cpu_usage_start['nice'] +
                         cpu_usage_start['system'])
    total_time_end = (cpu_usage_end['user'] + cpu_usage_end['nice'] +
                      cpu_usage_end['system'] + cpu_usage_end['idle'])
    total_time_start = (cpu_usage_start['user'] + cpu_usage_start['nice'] +
                        cpu_usage_start['system'] + cpu_usage_start['idle'])
    return ((float(time_active_end) - time_active_start) /
            (total_time_end - total_time_start))


def is_pgo_mode():
    return 'USE_PGO' in os.environ


def wait_for_idle_cpu(timeout, utilization):
    """Waits for the CPU to become idle (< utilization).

    Args:
        timeout: The longest time in seconds to wait before throwing an error.
        utilization: The CPU usage below which the system should be considered
                idle (between 0 and 1.0 independent of cores/hyperthreads).
    """
    time_passed = 0.0
    fraction_active_time = 1.0
    sleep_time = 1
    logging.info('Starting to wait up to %.1fs for idle CPU...', timeout)
    while fraction_active_time >= utilization:
        cpu_usage_start = get_cpu_usage()
        # Split timeout interval into not too many chunks to limit log spew.
        # Start at 1 second, increase exponentially
        time.sleep(sleep_time)
        time_passed += sleep_time
        sleep_time = min(16.0, 2.0 * sleep_time)
        cpu_usage_end = get_cpu_usage()
        fraction_active_time = \
                compute_active_cpu_time(cpu_usage_start, cpu_usage_end)
        logging.info('After waiting %.1fs CPU utilization is %.3f.',
                     time_passed, fraction_active_time)
        if time_passed > timeout:
            logging.warning('CPU did not become idle.')
            log_process_activity()
            # crosbug.com/37389
            if is_pgo_mode():
                logging.info('Still continuing because we are in PGO mode.')
                return True

            return False
    logging.info('Wait for idle CPU took %.1fs (utilization = %.3f).',
                 time_passed, fraction_active_time)
    return True


def log_process_activity():
    """Logs the output of top.

    Useful to debug performance tests and to find runaway processes.
    """
    logging.info('Logging current process activity using top.')
    cmd = 'top -b -n1 -c'
    output = utils.run(cmd)
    logging.info(output)


def wait_for_cool_machine():
    """
    A simple heuristic to wait for a machine to cool.
    The code looks a bit 'magic', but we don't know ambient temperature
    nor machine characteristics and still would like to return the caller
    a machine that cooled down as much as reasonably possible.
    """
    temperature = get_current_temperature_max()
    # We got here with a cold machine, return immediately. This should be the
    # most common case.
    if temperature < 50:
        return True
    logging.info('Got a hot machine of %dC. Sleeping 1 minute.', temperature)
    # A modest wait should cool the machine.
    time.sleep(60.0)
    temperature = get_current_temperature_max()
    # Atoms idle below 60 and everyone else should be even lower.
    if temperature < 62:
        return True
    # This should be rare.
    logging.info('Did not cool down (%dC). Sleeping 2 minutes.', temperature)
    time.sleep(120.0)
    temperature = get_current_temperature_max()
    # A temperature over 65'C doesn't give us much headroom to the critical
    # temperatures that start at 85'C (and PerfControl as of today will fail at
    # critical - 10'C).
    if temperature < 65:
        return True
    logging.warning('Did not cool down (%dC), giving up.', temperature)
    log_process_activity()
    return False


# System paths for machine performance state.
_CPUINFO = '/proc/cpuinfo'
_DIRTY_WRITEBACK_CENTISECS = '/proc/sys/vm/dirty_writeback_centisecs'
_KERNEL_MAX = '/sys/devices/system/cpu/kernel_max'
_MEMINFO = '/proc/meminfo'
_TEMP_SENSOR_RE = 'Reading temperature...([0-9]*)'


def _get_line_from_file(path, line):
    """
    line can be an integer or
    line can be a string that matches the beginning of the line
    """
    with open(path) as f:
        if isinstance(line, int):
            l = f.readline()
            for _ in range(0, line):
                l = f.readline()
            return l
        else:
            for l in f:
                if l.startswith(line):
                    return l
    return None


def _get_match_from_file(path, line, prefix, postfix):
    """
    Matches line in path and returns string between first prefix and postfix.
    """
    match = _get_line_from_file(path, line)
    # Strip everything from front of line including prefix.
    if prefix:
        match = re.split(prefix, match)[1]
    # Strip everything from back of string including first occurence of postfix.
    if postfix:
        match = re.split(postfix, match)[0]
    return match


def _get_float_from_file(path, line, prefix, postfix):
    match = _get_match_from_file(path, line, prefix, postfix)
    return float(match)


def _get_int_from_file(path, line, prefix, postfix):
    match = _get_match_from_file(path, line, prefix, postfix)
    return int(match)


def _get_hex_from_file(path, line, prefix, postfix):
    match = _get_match_from_file(path, line, prefix, postfix)
    return int(match, 16)


# The paths don't change. Avoid running find all the time.
_hwmon_paths = None

def _get_hwmon_paths(file_pattern):
    """
    Returns a list of paths to the temperature sensors.
    """
    # Some systems like daisy_spring only have the virtual hwmon.
    # And other systems like rambi only have coretemp.0. See crbug.com/360249.
    #    /sys/class/hwmon/hwmon*/
    #    /sys/devices/virtual/hwmon/hwmon*/
    #    /sys/devices/platform/coretemp.0/
    if not _hwmon_paths:
        cmd = 'find /sys/ -name "' + file_pattern + '"'
        _hwon_paths = utils.run(cmd, verbose=False).stdout.splitlines()
    return _hwon_paths


def get_temperature_critical():
    """
    Returns temperature at which we will see some throttling in the system.
    """
    min_temperature = 1000.0
    paths = _get_hwmon_paths('temp*_crit')
    for path in paths:
        temperature = _get_float_from_file(path, 0, None, None) * 0.001
        # Today typical for Intel is 98'C to 105'C while ARM is 85'C. Clamp to
        # the lowest known value.
        if (min_temperature < 60.0) or min_temperature > 150.0:
            logging.warning('Critical temperature of %.1fC was reset to 85.0C.',
                            min_temperature)
            min_temperature = 85.0

        min_temperature = min(temperature, min_temperature)
    return min_temperature


def get_temperature_input_max():
    """
    Returns the maximum currently observed temperature.
    """
    max_temperature = -1000.0
    paths = _get_hwmon_paths('temp*_input')
    for path in paths:
        temperature = _get_float_from_file(path, 0, None, None) * 0.001
        max_temperature = max(temperature, max_temperature)
    return max_temperature


def get_thermal_zone_temperatures():
    """
    Returns the maximum currently observered temperature in thermal_zones.
    """
    temperatures = []
    for path in glob.glob('/sys/class/thermal/thermal_zone*/temp'):
        try:
            temperatures.append(
                _get_float_from_file(path, 0, None, None) * 0.001)
        except IOError:
            # Some devices (e.g. Veyron) may have reserved thermal zones that
            # are not active. Trying to read the temperature value would cause a
            # EINVAL IO error.
            continue
    return temperatures


def get_ec_temperatures():
    """
    Uses ectool to return a list of all sensor temperatures in Celsius.
    """
    temperatures = []
    # TODO(ihf): On all ARM boards I tested 'ectool temps all' returns 200K
    # for all sensors. Remove this check once crbug.com/358342 is fixed.
    if 'arm' in utils.get_arch():
        return temperatures
    try:
        full_cmd = 'ectool temps all'
        lines = utils.run(full_cmd, verbose=False).stdout.splitlines()
        for line in lines:
            temperature = int(line.split(': ')[1]) - 273
            temperatures.append(temperature)
    except Exception:
        logging.warning('Unable to read temperature sensors using ectool.')
    for temperature in temperatures:
        # Sanity check for real world values.
        assert ((temperature > 10.0) and
                (temperature < 150.0)), ('Unreasonable temperature %.1fC.' %
                                         temperature)

    return temperatures


def get_current_temperature_max():
    """
    Returns the highest reported board temperature (all sensors) in Celsius.
    """
    temperature = max([get_temperature_input_max()] +
                      get_thermal_zone_temperatures() +
                      get_ec_temperatures())
    # Sanity check for real world values.
    assert ((temperature > 10.0) and
            (temperature < 150.0)), ('Unreasonable temperature %.1fC.' %
                                     temperature)
    return temperature


def get_cpu_cache_size():
    """
    Returns the last level CPU cache size in kBytes.
    """
    cache_size = _get_int_from_file(_CPUINFO, 'cache size', ': ', ' KB')
    # Sanity check.
    assert cache_size >= 64, 'Unreasonably small cache.'
    return cache_size


def get_cpu_model_frequency():
    """
    Returns the model frequency from the CPU model name on Intel only. This
    might be redundant with get_cpu_max_frequency. Unit is Hz.
    """
    frequency = _get_float_from_file(_CPUINFO, 'model name', ' @ ', 'GHz')
    return 1.e9 * frequency


def get_cpu_max_frequency():
    """
    Returns the largest of the max CPU core frequencies. The unit is Hz.
    """
    max_frequency = -1
    paths = _get_cpufreq_paths('cpuinfo_max_freq')
    for path in paths:
        # Convert from kHz to Hz.
        frequency = 1000 * _get_float_from_file(path, 0, None, None)
        max_frequency = max(frequency, max_frequency)
    # Sanity check.
    assert max_frequency > 1e8, 'Unreasonably low CPU frequency.'
    return max_frequency


def get_cpu_min_frequency():
    """
    Returns the smallest of the minimum CPU core frequencies.
    """
    min_frequency = 1e20
    paths = _get_cpufreq_paths('cpuinfo_min_freq')
    for path in paths:
        frequency = _get_float_from_file(path, 0, None, None)
        min_frequency = min(frequency, min_frequency)
    # Sanity check.
    assert min_frequency > 1e8, 'Unreasonably low CPU frequency.'
    return min_frequency


def get_cpu_model():
    """
    Returns the CPU model.
    Only works on Intel.
    """
    cpu_model = _get_int_from_file(_CPUINFO, 'model\t', ': ', None)
    return cpu_model


def get_cpu_family():
    """
    Returns the CPU family.
    Only works on Intel.
    """
    cpu_family = _get_int_from_file(_CPUINFO, 'cpu family\t', ': ', None)
    return cpu_family


def get_board():
    """
    Get the ChromeOS release board name from /etc/lsb-release.
    """
    f = open('/etc/lsb-release')
    try:
        return re.search('BOARD=(.*)', f.read()).group(1)
    finally:
        f.close()


def get_board_type():
    """
    Get the ChromeOS board type from /etc/lsb-release.

    @return device type.
    """
    with open('/etc/lsb-release') as f:
        pat = re.search('DEVICETYPE=(.*)', f.read())
        if pat:
            return pat.group(1)
    return ''


def get_board_with_frequency_and_memory():
    """
    Returns a board name modified with CPU frequency and memory size to
    differentiate between different board variants. For instance
    link -> link_1.8GHz_4GB.
    """
    board_name = get_board()
    # Rounded to nearest GB and GHz.
    memory = int(round(get_mem_total() / 1024.0))
    # Convert frequency to GHz with 1 digit accuracy after the decimal point.
    frequency = int(round(get_cpu_max_frequency() * 1e-8)) * 0.1
    board = "%s_%1.1fGHz_%dGB" % (board_name, frequency, memory)
    return board


def get_mem_total():
    """
    Returns the total memory available in the system in MBytes.
    """
    mem_total = _get_float_from_file(_MEMINFO, 'MemTotal:', 'MemTotal:', ' kB')
    # Sanity check, all Chromebooks have at least 1GB of memory.
    assert mem_total > 1024 * 1024, 'Unreasonable amount of memory.'
    return mem_total / 1024


def get_mem_free():
    """
    Returns the currently free memory in the system in MBytes.
    """
    mem_free = _get_float_from_file(_MEMINFO, 'MemFree:', 'MemFree:', ' kB')
    return mem_free / 1024


def get_kernel_max():
    """
    Returns content of kernel_max.
    """
    kernel_max = _get_int_from_file(_KERNEL_MAX, 0, None, None)
    # Sanity check.
    assert ((kernel_max > 0) and (kernel_max < 257)), 'Unreasonable kernel_max.'
    return kernel_max


def set_high_performance_mode():
    """
    Sets the kernel governor mode to the highest setting.
    Returns previous governor state.
    """
    original_governors = get_scaling_governor_states()
    set_scaling_governors('performance')
    return original_governors


def set_scaling_governors(value):
    """
    Sets all scaling governor to string value.
    Sample values: 'performance', 'interactive', 'ondemand', 'powersave'.
    """
    paths = _get_cpufreq_paths('scaling_governor')
    for path in paths:
        cmd = 'echo %s > %s' % (value, path)
        logging.info('Writing scaling governor mode \'%s\' -> %s', value, path)
        # On Tegra CPUs can be dynamically enabled/disabled. Ignore failures.
        utils.system(cmd, ignore_status=True)


def _get_cpufreq_paths(filename):
    """
    Returns a list of paths to the governors.
    """
    cmd = 'ls /sys/devices/system/cpu/cpu*/cpufreq/' + filename
    paths = utils.run(cmd, verbose=False).stdout.splitlines()
    return paths


def get_scaling_governor_states():
    """
    Returns a list of (performance governor path, current state) tuples.
    """
    paths = _get_cpufreq_paths('scaling_governor')
    path_value_list = []
    for path in paths:
        value = _get_line_from_file(path, 0)
        path_value_list.append((path, value))
    return path_value_list


def restore_scaling_governor_states(path_value_list):
    """
    Restores governor states. Inverse operation to get_scaling_governor_states.
    """
    for (path, value) in path_value_list:
        cmd = 'echo %s > %s' % (value.rstrip('\n'), path)
        # On Tegra CPUs can be dynamically enabled/disabled. Ignore failures.
        utils.system(cmd, ignore_status=True)


def get_dirty_writeback_centisecs():
    """
    Reads /proc/sys/vm/dirty_writeback_centisecs.
    """
    time = _get_int_from_file(_DIRTY_WRITEBACK_CENTISECS, 0, None, None)
    return time


def set_dirty_writeback_centisecs(time=60000):
    """
    In hundredths of a second, this is how often pdflush wakes up to write data
    to disk. The default wakes up the two (or more) active threads every five
    seconds. The ChromeOS default is 10 minutes.

    We use this to set as low as 1 second to flush error messages in system
    logs earlier to disk.
    """
    # Flush buffers first to make this function synchronous.
    utils.system('sync')
    if time >= 0:
        cmd = 'echo %d > %s' % (time, _DIRTY_WRITEBACK_CENTISECS)
        utils.system(cmd)


def get_gpu_family():
    """Return the GPU family name"""
    global pciid_to_intel_architecture

    cpuarch = base_utils.get_cpu_soc_family()
    if cpuarch == 'exynos5' or cpuarch == 'rockchip':
        return 'mali'
    if cpuarch == 'tegra':
        return 'tegra'
    if os.path.exists('/sys/bus/platform/drivers/pvrsrvkm'):
        return 'rogue'

    pci_path = '/sys/bus/pci/devices/0000:00:02.0/device'

    if not os.path.exists(pci_path):
        raise error.TestError('PCI device 0000:00:02.0 not found')

    device_id = utils.read_one_line(pci_path).lower()

    # Only load Intel PCI ID file once and only if necessary.
    if not pciid_to_intel_architecture:
        with open(_INTEL_PCI_IDS_FILE_PATH, 'r') as in_f:
            pciid_to_intel_architecture = json.load(in_f)

    return pciid_to_intel_architecture[device_id]


_BOARDS_WITHOUT_MONITOR = [
    'anglar', 'mccloud', 'monroe', 'ninja', 'rikku', 'guado', 'jecht', 'tidus',
    'veyron_brian', 'beltino', 'panther', 'stumpy', 'panther', 'tricky', 'zako'
]


def has_no_monitor():
    """Return whether a machine doesn't have a built-in monitor"""
    board_name = get_board()
    if (board_name in _BOARDS_WITHOUT_MONITOR):
        return True

    return False


def get_fixed_dst_drive():
    """
    Return device name for internal disk.
    Example: return /dev/sda for falco booted from usb
    """
    cmd = ' '.join(['. /usr/sbin/write_gpt.sh;',
                    '. /usr/share/misc/chromeos-common.sh;',
                    'load_base_vars;',
                    'get_fixed_dst_drive'])
    return utils.system_output(cmd)


def get_root_device():
    """
    Return root device.
    Will return correct disk device even system boot from /dev/dm-0
    Example: return /dev/sdb for falco booted from usb
    """
    return utils.system_output('rootdev -s -d')


def get_root_partition():
    """
    Return current root partition
    Example: return /dev/sdb3 for falco booted from usb
    """
    return utils.system_output('rootdev -s')


def get_free_root_partition(root_part=None):
    """
    Return currently unused root partion
    Example: return /dev/sdb5 for falco booted from usb

    @param root_part: cuurent root partition
    """
    spare_root_map = {'3': '5', '5': '3'}
    if not root_part:
        root_part = get_root_partition()
    return root_part[:-1] + spare_root_map[root_part[-1]]


def is_booted_from_internal_disk():
    """Return True if boot from internal disk. False, otherwise."""
    return get_root_device() == get_fixed_dst_drive()


def get_ui_use_flags():
    """Parses the USE flags as listed in /etc/ui_use_flags.txt.

    @return: A list of flag strings found in the ui use flags file.
    """
    flags = []
    for flag in utils.read_file(_UI_USE_FLAGS_FILE_PATH).splitlines():
        # Removes everything after the '#'.
        flag_before_comment = flag.split('#')[0].strip()
        if len(flag_before_comment) != 0:
            flags.append(flag_before_comment)

    return flags


def is_freon():
    """Returns False if the system uses X, True otherwise."""
    return 'X' not in get_ui_use_flags()


def graphics_platform():
    """
    Return a string identifying the graphics platform,
    e.g. 'glx' or 'x11_egl' or 'gbm'
    """
    use_flags = get_ui_use_flags()
    if 'X' not in use_flags:
        return 'null'
    elif 'opengles' in use_flags:
        return 'x11_egl'
    return 'glx'


def graphics_api():
    """Return a string identifying the graphics api, e.g. gl or gles2."""
    use_flags = get_ui_use_flags()
    if 'opengles' in use_flags:
        return 'gles2'
    return 'gl'


def assert_has_X_server():
    """Using X is soon to be deprecated. Print warning or raise error."""
    if is_freon():
        # TODO(ihf): Think about if we could support X for testing for a while.
        raise error.TestFail('freon: can\'t use X server.')
    logging.warning('freon: Using the X server will be deprecated soon.')


def is_vm():
    """Check if the process is running in a virtual machine.

    @return: True if the process is running in a virtual machine, otherwise
             return False.
    """
    try:
        virt = utils.run('sudo -n virt-what').stdout.strip()
        logging.debug('virt-what output: %s', virt)
        return bool(virt)
    except error.CmdError:
        logging.warn('Package virt-what is not installed, default to assume '
                     'it is not a virtual machine.')
        return False

