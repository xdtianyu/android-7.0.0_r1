# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import cStringIO, collections, dbus, gzip, logging, subprocess

from autotest_lib.client.bin import test
from autotest_lib.client.common_lib import error


class platform_DebugDaemonGetPerfData(test.test):
    """
    This autotest tests the collection of perf data.  It calls perf indirectly
    through debugd -> quipper -> perf.

    The perf data is collected both when the system is idle and when there is a
    process running in the background.

    The perf data is collected over various durations.
    """

    version = 1

    # A list of durations over which to gather perf data using quipper (given in
    # seconds), plus the number of times to run perf with each duration.
    # e.g. the entry "1: 50" means to run perf for 1 second 50 times.
    _profile_duration_and_repetitions = [
        (1, 3),
        (5, 1)
    ]

    # Commands to repeatedly run in the background when collecting perf data.
    _system_load_commands = {
        'idle'     : 'sleep 1',
        'busy'     : 'ls',
    }

    _dbus_debugd_object = '/org/chromium/debugd'
    _dbus_debugd_name = 'org.chromium.debugd'

    # For storing the size of returned results.
    SizeInfo = collections.namedtuple('SizeInfo', ['size', 'size_zipped'])

    def gzip_string(self, string):
        """
        Gzip a string.

        @param string: The input string to be gzipped.

        Returns:
          The gzipped string.
        """
        string_file = cStringIO.StringIO()
        gzip_file = gzip.GzipFile(fileobj=string_file, mode='wb')
        gzip_file.write(string)
        gzip_file.close()
        return string_file.getvalue()


    def validate_get_perf_method(self, duration, num_reps, load_type):
        """
        Validate a debugd method that returns perf data.

        @param duration: The duration to use for perf data collection.
        @param num_reps: Number of times to run.
        @param load_type: A label to use for storing into perf keyvals.
        """
        # Dictionary for storing results returned from debugd.
        # Key:   Name of data type (string)
        # Value: Sizes of results in bytes (list of SizeInfos)
        stored_results = collections.defaultdict(list)

        for _ in range(num_reps):
            perf_command = ['perf', 'record', '-a', '-e', 'cycles',
                            '-c', '1000003']
            status, perf_data, perf_stat = (
                self.dbus_iface.GetPerfOutput(duration, perf_command))
            if status != 0:
                raise error.TestFail('GetPerfOutput() returned status %d',
                                     status)
            if len(perf_data) == 0 and len(perf_stat) == 0:
                raise error.TestFail('GetPerfOutput() returned no data')
            if len(perf_data) > 0 and len(perf_stat) > 0:
                raise error.TestFail('GetPerfOutput() returned both '
                                     'perf_data and perf_stat')

            result_type = None
            if perf_data:
                result = perf_data
                result_type = "perf_data"
            else:   # if perf_stat
                result = perf_stat
                result_type = "perf_stat"

            logging.info('GetPerfOutput() for %s seconds returned %d '
                         'bytes of type %s',
                         duration, len(result), result_type)
            if len(result) < 10:
                raise error.TestFail('Perf output too small')

            # Convert |result| from an array of dbus.Bytes to a string.
            result = ''.join(chr(b) for b in result)

            # If there was an error in collecting a profile with quipper, debugd
            # will output an error message. Make sure to check for this message.
            # It is found in PerfTool::GetPerfDataHelper() in
            # debugd/src/perf_tool.cc.
            if result.startswith('<process exited with status: '):
                raise error.TestFail('Quipper failed: %s' % result)

            stored_results[result_type].append(
                self.SizeInfo(len(result), len(self.gzip_string(result))))

        for result_type, sizes in stored_results.iteritems():
            key = 'mean_%s_size_%s_%d' % (result_type, load_type, duration)
            total_size = sum(entry.size for entry in sizes)
            total_size_zipped = sum(entry.size_zipped for entry in sizes)

            keyvals = {}
            keyvals[key] = total_size / len(sizes)
            keyvals[key + '_zipped'] = total_size_zipped / len(sizes)
            self.write_perf_keyval(keyvals)


    def run_once(self, *args, **kwargs):
        """
        Primary autotest function.
        """

        bus = dbus.SystemBus()
        proxy = bus.get_object(self._dbus_debugd_name, self._dbus_debugd_object)
        self.dbus_iface = dbus.Interface(proxy,
                                         dbus_interface=self._dbus_debugd_name)

        # Open /dev/null to redirect unnecessary output.
        devnull = open('/dev/null', 'w')

        load_items = self._system_load_commands.iteritems()
        for load_type, load_command in load_items:
            # Repeatedly run the comand for the current load.
            cmd = 'while true; do %s; done' % load_command
            process = subprocess.Popen(cmd, stdout=devnull, shell=True)

            for duration, num_reps in self._profile_duration_and_repetitions:
                # Collect perf data from debugd.
                self.validate_get_perf_method(duration, num_reps, load_type)

            # Terminate the process and actually wait for it to terminate.
            process.terminate()
            while process.poll() == None:
                pass

        devnull.close()
