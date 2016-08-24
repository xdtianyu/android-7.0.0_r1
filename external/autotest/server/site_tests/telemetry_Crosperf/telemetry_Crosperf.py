# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
import logging
import os
import StringIO

import common
from autotest_lib.client.common_lib import error
from autotest_lib.server import test
from autotest_lib.server import utils
from autotest_lib.server.cros import telemetry_runner


TELEMETRY_TIMEOUT_MINS = 60
CHROME_SRC_ROOT = '/var/cache/chromeos-cache/distfiles/target/'
CLIENT_CHROME_ROOT = '/usr/local/telemetry/src'
RUN_BENCHMARK  = 'tools/perf/run_benchmark'


def _find_chrome_root_dir():
    # Look for chrome source root, either externally mounted, or inside
    # the chroot.  Prefer chrome-src-internal source tree to chrome-src.
    sources_list = ('chrome-src-internal', 'chrome-src')

    dir_list = [os.path.join(CHROME_SRC_ROOT, x) for x in sources_list]
    if 'CHROME_ROOT' in os.environ:
        dir_list.insert(0, os.environ['CHROME_ROOT'])

    for dir in dir_list:
        if os.path.exists(dir):
            chrome_root_dir = dir
            break
    else:
        raise error.TestError('Chrome source directory not found.')

    logging.info('Using Chrome source tree at %s', chrome_root_dir)
    return os.path.join(chrome_root_dir, 'src')


def _ensure_deps(dut, test_name):
    """
    Ensure the dependencies are locally available on DUT.

    @param dut: The autotest host object representing DUT.
    @param test_name: Name of the telemetry test.
    """
    # Get DEPs using host's telemetry.
    chrome_root_dir = _find_chrome_root_dir()
    format_string = ('python %s/tools/perf/fetch_benchmark_deps.py %s')
    command = format_string % (chrome_root_dir, test_name)
    logging.info('Getting DEPs: %s', command)
    stdout = StringIO.StringIO()
    stderr = StringIO.StringIO()
    try:
        result = utils.run(command, stdout_tee=stdout,
                           stderr_tee=stderr)
    except error.CmdError as e:
        logging.debug('Error occurred getting DEPs: %s\n %s\n',
                      stdout.getvalue(), stderr.getvalue())
        raise error.TestFail('Error occurred while getting DEPs.')

    # Download DEPs to DUT.
    # send_file() relies on rsync over ssh. Couldn't be better.
    stdout_str = stdout.getvalue()
    stdout.close()
    stderr.close()
    for dep in stdout_str.split():
        src = os.path.join(chrome_root_dir, dep)
        dst = os.path.join(CLIENT_CHROME_ROOT, dep)
        if not os.path.isfile(src):
            raise error.TestFail('Error occurred while saving DEPs.')
        logging.info('Copying: %s -> %s', src, dst)
        try:
            dut.send_file(src, dst)
        except:
            raise error.TestFail('Error occurred while sending DEPs to dut.\n')


class telemetry_Crosperf(test.test):
    """Run one or more telemetry benchmarks under the crosperf script."""
    version = 1

    def run_once(self, args, client_ip='', dut=None):
        """
        Run a single telemetry test.

        @param args: A dictionary of the arguments that were passed
                to this test.
        @param client_ip: The ip address of the DUT
        @param dut: The autotest host object representing DUT.

        @returns A TelemetryResult instance with the results of this execution.
        """
        test_name = args['test']
        test_args = ''
        if 'test_args' in args:
            test_args = args['test_args']

        # Decide whether the test will run locally or by a remote server.
        if 'run_local' in args and args['run_local'].lower() == 'true':
            # The telemetry scripts will run on DUT.
            _ensure_deps(dut, test_name)
            format_string = ('python %s --browser=system %s %s')
            command = format_string % (os.path.join(CLIENT_CHROME_ROOT,
                                                    RUN_BENCHMARK),
                                       test_args, test_name)
            runner = dut
        else:
            # The telemetry scripts will run on server.
            format_string = ('python %s --browser=cros-chrome --remote=%s '
                             '%s %s')
            command = format_string % (os.path.join(_find_chrome_root_dir(),
                                                    RUN_BENCHMARK),
                                       client_ip, test_args, test_name)
            runner = utils

        # Run the test.
        stdout = StringIO.StringIO()
        stderr = StringIO.StringIO()
        try:
            logging.info('CMD: %s', command)
            result = runner.run(command, stdout_tee=stdout, stderr_tee=stderr,
                                timeout=TELEMETRY_TIMEOUT_MINS*60)
            exit_code = result.exit_status
        except error.CmdError as e:
            logging.debug('Error occurred executing telemetry.')
            exit_code = e.result_obj.exit_status
            raise error.TestFail('An error occurred while executing '
                                 'telemetry test.')
        finally:
            stdout_str = stdout.getvalue()
            stderr_str = stderr.getvalue()
            stdout.close()
            stderr.close()


        # Parse the result.
        logging.debug('Telemetry completed with exit code: %d.'
                      '\nstdout:%s\nstderr:%s', exit_code,
                      stdout_str, stderr_str)
        logging.info('Telemetry completed with exit code: %d.'
                     '\nstdout:%s\nstderr:%s', exit_code,
                     stdout_str, stderr_str)

        result = telemetry_runner.TelemetryResult(exit_code=exit_code,
                                                  stdout=stdout_str,
                                                  stderr=stderr_str)

        result.parse_benchmark_results()
        for data in result.perf_data:
            self.output_perf_value(description=data['trace'],
                                   value=data['value'],
                                   units=data['units'],
                                   graph=data['graph'])

        return result
