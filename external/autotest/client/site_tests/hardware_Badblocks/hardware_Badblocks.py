# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging, re, subprocess, threading
import common
from autotest_lib.client.bin import site_utils, test, utils
from autotest_lib.client.common_lib import error


class hardware_Badblocks(test.test):
    """
    Runs badblocks on the root partition that is not being used.

    """

    version = 1

    # Define output that is expected from a successful badblocks run.
    _EXPECTED_BADBLOCKS_OUTPUT = (
            'Pass completed, 0 bad blocks found. (0/0/0 errors)')

    # Define Linux badblocks utility name.
    _BADBLOCKS = 'badblocks'

    # Define variables to store some statistics of the runs.
    _pass_count = 0
    _fail_count = 0
    _longest_runtime = 0


    def _get_sector_size(self, dev):
        """
        Finds block device's sector size in bytes.

        @return the sector size.

        """

        argv = ('parted ' + dev + ' print | grep "Sector size" | awk -F ' +
                '"/" \'{print $3}\' | sed \'$s/.$//\'')

        return utils.system_output(argv)


    def _timeout(self, badblocks_proc):
        """
        Timeout callback for the badblocks process.

        Kills badblocks process if still running and fails test.

        """

        # Kill badblocks, report if not killed, log any exceptions.
        if badblocks_proc.poll() == None:
            try:
                logging.info('badblocks taking too long---sending SIGKILL')
                badblocks_proc.kill()
            except Exception as e:
                logging.info('%s', e)
            finally:
                # name of the kernel function in which the process is sleeping.
                argv = ('ps eopid,fname,wchan | grep ' + self._BADBLOCKS +
                        ' | awk \'{print $3}\'')
                waiton = utils.system_output(argv)
                if waiton:
                    logging.info('badblocks is waiting on %s', waiton)
                raise error.TestError('Error: badblocks timed out.')


    def _run_badblocks(self, dev, sector_size, tmout):
        """
        Runs badblocks.

        """

        # Run badblocks on the selected partition, with parameters:
        # -s = show progress
        # -v = verbose (print error count)
        # -w = destructive write+read test
        # -b = block size (set equal to sector size)
        argv = [self._BADBLOCKS, '-svw', '-d', str(sector_size), dev]
        msg = 'Running: ' + ' '.join(map(str, argv))
        logging.info(msg)
        badblocks_proc = subprocess.Popen(
                argv,
                shell=False,
                stderr=subprocess.STDOUT, # Combine stderr with stdout.
                stdout=subprocess.PIPE)

        # Start timeout timer thread.
        t = threading.Timer(tmout, self._timeout, [badblocks_proc])
        t.start()

        # Get badblocks output.
        stdout, _ = badblocks_proc.communicate()

        # Stop timer if badblocks has finished.
        t.cancel()

        # Check badblocks exit status.
        if badblocks_proc.returncode != 0:
            raise error.TestError('badblocks returned with code: %s',
                                  badblocks_proc.returncode)

        # Parse and log badblocks output.
        logging.info('badblocks output:')
        lines = stdout.split('\n')
        del lines[-1] # Remove blank line at end.
        logging.info(lines[0])
        logging.info(lines[1])
        # Log the progress of badblocks (line 2 onwards, minus last line).
        for line in lines[2:-1]:
            # replace backspace characters with a newline character.
            line = re.sub(r'[\b]+', '\n', line)
            # Log test pattern info.
            pattern_info = line[:line.find(':') + 1]
            logging.info('%s', pattern_info)
            sublines = line[line.find(':') + 2:].split('\n')
            for subline in sublines:
                logging.info('%s', subline)
        # Log result (last line).
        logging.info(lines[-1])

        # Get run time in seconds.
        min_sec = re.match(r'(\w+):(\w+)', lines[-2].split()[-4])
        runtime = int(min_sec.group(1)) * 60 + int(min_sec.group(2))

        # Update longest run time.
        if self._longest_runtime < runtime:
            self._longest_runtime = runtime

        # Check badblocks result.
        result = lines[-1].strip()
        if result != self._EXPECTED_BADBLOCKS_OUTPUT:
            self._fail_count += 1
            return
        self._pass_count += 1


    def run_once(self, iters=1, tmout=60 * 60):
        """
        Executes test.

        @param iters: Number of times to run badblocks.
        @param tmout: Time allowed badblocks to run before killing it.
                      (Default time is 60 minutes.)

        """

        # Log starting message.
        logging.info('Statring hardware_Badblocks Test.')
        logging.info('Iterations: %d', iters)
        logging.info('badblocks Timeout (sec): %d', tmout)

        # Determine which device and partition to use.
        logging.info('Determine unused root partition to test on:')
        dev = site_utils.get_free_root_partition()
        logging.info('Testing on ' + dev)

        # Get block device's sector size.
        logging.info('Determine block device sector size:')
        sector_size = self._get_sector_size(site_utils.get_root_device())
        logging.info('Sector size (bytes): ' + sector_size)

        # Get partition size.
        logging.info('Determine partition size:')
        part_size = utils.get_disk_size(dev)
        logging.info('Partition size (bytes): %s', part_size)

        # Run badblocks.
        for i in range(iters):
            logging.info('Starting iteration %d', i)
            self._run_badblocks(dev, sector_size, tmout)

        # Report statistics.
        logging.info('Total pass: %d', self._pass_count)
        logging.info('Total fail: %d', self._fail_count)
        stats = {}
        stats['ea_badblocks_runs'] = iters
        stats['ea_passed_count'] = self._pass_count
        stats['ea_failed_count'] = self._fail_count
        stats['sec_longest_run'] = self._longest_runtime
        # TODO: change write_perf_keyval() to output_perf_value() as soon as
        # autotest is ready for it.
        self.write_perf_keyval(stats)

        # Report test pass/fail.
        if self._pass_count != iters:
            raise error.TestFail('One or more runs found bad blocks on'
                                 ' storage device.')
