# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os, os.path, logging
from autotest_lib.client.bin import test, utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros import chrome
from autotest_lib.client.cros.crash_test import CrashTest

class logging_CrashServices(test.test):
    version = 3

    process_list = {
        '/sbin/agetty' : ['.core', '.dmp', '.meta'],
        '/usr/sbin/cryptohomed' : ['.core', '.dmp', '.meta'],
        '/usr/bin/metrics_daemon' : ['.core', '.dmp', '.meta'],
        '/usr/bin/powerd' : ['.core', '.dmp', '.meta', '.log'],
        '/usr/sbin/rsyslogd': ['.core', '.dmp', '.meta'],
        # Removing tcsd crash with reference to crbug.com/380359
        # '/usr/sbin/tcsd' : ['.core', '.dmp', '.meta'],
        '/usr/bin/tlsdated' : ['.core', '.dmp', '.meta'],
        '/usr/bin/shill' : ['.core', '.dmp', '.meta'],
        '/usr/sbin/update_engine' : ['.core', '.dmp', '.meta', '.log'],
        '/usr/sbin/wpa_supplicant' : ['.core', '.dmp', '.meta'],
        '/sbin/session_manager' : ['.core', '.dmp', '.meta']
    }

    def _kill_processes(self, name):
        """Kills the process passed as the parameter

        @param name: Name of the process to be killed.

        @returns: exit status of the kill command.

        """
        return utils.system("killall -w -s SEGV %s" % name, ignore_status=True)


    def _find_crash_files(self, process_name, extension):
        """Find if the crash dumps with appropriate extensions are created.

        @param process_name: Name of the process killed.
        @param extension: Extension of the dump files to be created.

        @returns: Returns the name of the dump file.

        """
        return self._find_file_in_path(CrashTest._SYSTEM_CRASH_DIR,
                                       process_name, extension)


    def _find_file_in_path(self, path, process_name, filetype):
        """Checks the creation of the the dump files with appropriate extensions.
           Also check for the file size of the dumps created.

        @param path: Dirctory path where the dump files are expected.
        @param process_name: Name of the process.
        @param filetype: Extension of the dump file.

        @returns: Name of the dump file.

        """
        try:
            entries = os.listdir(path)
        except OSError:
            return None

        for entry in entries:
            (filename, ext) = os.path.splitext(entry)
            if ext == filetype and filename.startswith(process_name):
                logging.info('the path is %s' % os.path)
                if os.path.getsize(path + '/' + entry) > 0 :
                    return entry
        return None


    def _test_process(self, process_path, crash_extensions):
        """Calls a function to kill the process and then wait
           for the creation of the dump files.

        @param process_path: Path of the process to be killed.
        @param crash_extensions: Extension of the dump file expected.

        """
        if self._kill_processes(process_path):
            raise error.TestFail("Failed to kill process %s" % process_path)

        process_name = os.path.basename(process_path)

        for crash_ext in crash_extensions:
            # wait for appropriate dump files in a crash directory.
            utils.poll_for_condition(
                condition=lambda: self._find_crash_files(process_name,
                                                         crash_ext),
                desc="Waiting for %s for %s" % (crash_ext, process_path))


    def run_once(self, process_path=None, crash_extensions=None):
        if process_path:
            self._test_process(process_path,crash_extensions)
            return

        with chrome.Chrome():
            if not utils.is_freon():
                process_path = '/usr/bin/X'
                crash_extensions = ['.core', '.dmp', '.meta']
                self.job.run_test("logging_CrashServices",
                                  process_path=process_path,
                                  crash_extensions=crash_extensions,
                                  tag=os.path.basename(process_path))
            for process_path in self.process_list.keys():
                self.job.run_test("logging_CrashServices",
                                  process_path=process_path,
                                  crash_extensions=self.process_list.get(process_path),
                                  tag=os.path.basename(process_path))