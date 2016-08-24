# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
import subprocess
from autotest_lib.client.bin import test
from autotest_lib.client.common_lib import error


class platform_Quipper(test.test):
    """
    Collects perf data and convert it to a protobuf. Verifies that quipper
    completes successfully and that the output is nonzero.
    """
    version = 1

    def _get_quipper_command(self, duration, perf_options):
        return ('quipper', str(duration), 'perf', 'record', '-a') + \
               perf_options


    def _get_perf_command(self, duration, perf_options):
        return ('perf', 'record', '-a') + perf_options + \
               ('--', 'sleep', str(duration))


    def run_once(self):
        """
        See test description.
        """

        duration = 2

        # These are the various perf command options to add to
        # |quipper_command_base|, for a wide range of commands to test.
        quipper_command_options = (
            # Basic cycle-based profile.
            ('-e', 'cycles'),
            # Set a custom sampling frequency.
            ('-e', 'cycles', '-F', '3011'),
            # Set a custom sampling period.
            ('-e', 'cycles', '-c', '2000003'),
            # Test various events.
            ('-e', 'cycles,instructions,branch-misses,cache-misses'),
            # Test callgraph.
            ('-e', 'cycles', '-g'),
            # Test callgraph and raw data.
            ('-e', 'cycles', '-g', '-R'),
            # Test LBR.
            ('-e', 'cycles', '-b'),
            # Test LBR, callgraph, and raw data.
            ('-e', 'cycles', '-b', '-g', '-R'),
        )

        keyvals = {}
        # Run quipper with each of the options.
        for options in quipper_command_options:
            result = ""

            # Try running the associated perf command first.
            perf_command = self._get_perf_command(duration, options)

            # Generate a full quipper command by joining the base command
            # and various perf options.
            quipper_command = self._get_quipper_command(duration, options)
            quipper_command_string = ' '.join(quipper_command)

            try:
                result = subprocess.check_output(perf_command)
            except subprocess.CalledProcessError:
                # If the perf command fails, don't test quipper. But record that
                # it was skipped.
                keyvals['command'] = '(' + quipper_command_string + ')'
                keyvals['result_length'] = '(skipped)'
                self.write_perf_keyval(keyvals)
                continue

            try:
                result = subprocess.check_output(quipper_command,
                                                 stderr=subprocess.STDOUT)
            except subprocess.CalledProcessError:
                raise error.TestFail('Error running command: ' +
                                     quipper_command_string)

            # Write keyvals.
            keyvals['command'] = quipper_command_string;
            keyvals['result_length'] = len(result)
            self.write_perf_keyval(keyvals)

            # Verify the output size.
            if len(result) == 0:
                raise error.TestFail('Got no result data from quipper.')

