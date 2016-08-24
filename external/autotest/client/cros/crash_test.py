# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import contextlib, fcntl, logging, os, re, shutil

import common, constants, cros_logging
from autotest_lib.client.bin import test, utils
from autotest_lib.client.common_lib import error


class CrashTest(test.test):
    """
    This class deals with running crash tests, which are tests which crash a
    user-space program (or the whole machine) and generate a core dump. We
    want to check that the correct crash dump is available and can be
    retrieved.

    Chromium OS has a crash sender which checks for new crash data and sends
    it to a server. This crash data is used to track software quality and find
    bugs. The system crash sender normally is always running, but can be paused
    by creating _PAUSE_FILE. When crash sender sees this, it pauses operation.

    The pid of the system crash sender is stored in _CRASH_SENDER_RUN_PATH so
    we can use this to kill the system crash sender for when we want to run
    our own.

    For testing purposes we sometimes want to run the crash sender manually.
    In this case we can set 'OVERRIDE_PAUSE_SENDING=1' in the environment and
    run the crash sender manually (as a child process).

    Also for testing we sometimes want to mock out the crash sender, and just
    have it pretend to succeed or fail. The _MOCK_CRASH_SENDING file is used
    for this. If it doesn't exist, then the crash sender runs normally. If
    it exists but is empty, the crash sender will succeed (but actually do
    nothing). If the file contains something, then the crash sender will fail.

    If the user consents to sending crash tests, then the _CONSENT_FILE will
    exist in the home directory. This test needs to create this file for the
    crash sending to work.

    Crash reports are rate limited to a certain number of reports each 24
    hours. If the maximum number has already been sent then reports are held
    until later. This is administered by a directory _CRASH_SENDER_RATE_DIR
    which contains one temporary file for each time a report is sent.

    The class provides the ability to push a consent file. This disables
    consent for this test but allows it to be popped back at later. This
    makes nested tests easier. If _automatic_consent_saving is True (the
    default) then consent will be pushed at the start and popped at the end.

    Interesting variables:
        _log_reader: the log reader used for reading log files
        _leave_crash_sending: True to enable crash sending on exit from the
            test, False to disable it. (Default True).
        _automatic_consent_saving: True to push the consent at the start of
            the test and pop it afterwards. (Default True).

    Useful places to look for more information are:

    chromeos/src/platform/crash-reporter/crash_sender
        - sender script which crash crash reporter to create reports, then

    chromeos/src/platform/crash-reporter/
        - crash reporter program
    """


    _CONSENT_FILE = '/home/chronos/Consent To Send Stats'
    _CORE_PATTERN = '/proc/sys/kernel/core_pattern'
    _CRASH_REPORTER_PATH = '/sbin/crash_reporter'
    _CRASH_SENDER_PATH = '/sbin/crash_sender'
    _CRASH_SENDER_RATE_DIR = '/var/lib/crash_sender'
    _CRASH_SENDER_RUN_PATH = '/var/run/crash_sender.pid'
    _CRASH_SENDER_LOCK_PATH = '/var/lock/crash_sender'
    _CRASH_TEST_IN_PROGRESS = '/tmp/crash-test-in-progress'
    _MOCK_CRASH_SENDING = '/tmp/mock-crash-sending'
    _PAUSE_FILE = '/var/lib/crash_sender_paused'
    _SYSTEM_CRASH_DIR = '/var/spool/crash'
    _FALLBACK_USER_CRASH_DIR = '/home/chronos/crash'
    _USER_CRASH_DIRS = '/home/chronos/u-*/crash'

    # Use the same file format as crash does normally:
    # <basename>.#.#.#.meta
    _FAKE_TEST_BASENAME = 'fake.1.2.3'

    def _set_system_sending(self, is_enabled):
        """Sets whether or not the system crash_sender is allowed to run.

        This is done by creating or removing _PAUSE_FILE.

        crash_sender may still be allowed to run if _set_child_sending is
        called with True and it is run as a child process.

        @param is_enabled: True to enable crash_sender, False to disable it.
        """
        if is_enabled:
            if os.path.exists(self._PAUSE_FILE):
                os.remove(self._PAUSE_FILE)
        else:
            utils.system('touch ' + self._PAUSE_FILE)


    def _set_child_sending(self, is_enabled):
        """Overrides crash sending enabling for child processes.

        When the system crash sender is disabled this test can manually run
        the crash sender as a child process. Normally this would do nothing,
        but this function sets up crash_sender to ignore its disabled status
        and do its job.

        @param is_enabled: True to enable crash sending for child processes.
        """
        if is_enabled:
            os.environ['OVERRIDE_PAUSE_SENDING'] = "1"
        else:
            del os.environ['OVERRIDE_PAUSE_SENDING']


    def _set_force_official(self, is_enabled):
        """Sets whether or not reports will upload for unofficial versions.

        Normally, crash reports are only uploaded for official build
        versions.  If the override is set, however, they will also be
        uploaded for unofficial versions.

        @param is_enabled: True to enable uploading for unofficial versions.
        """
        if is_enabled:
            os.environ['FORCE_OFFICIAL'] = "1"
        elif os.environ.get('FORCE_OFFICIAL'):
            del os.environ['FORCE_OFFICIAL']


    def _set_mock_developer_mode(self, is_enabled):
        """Sets whether or not we should pretend we booted in developer mode.

        @param is_enabled: True to pretend we are in developer mode.
        """
        if is_enabled:
            os.environ['MOCK_DEVELOPER_MODE'] = "1"
        elif os.environ.get('MOCK_DEVELOPER_MODE'):
            del os.environ['MOCK_DEVELOPER_MODE']


    def _reset_rate_limiting(self):
        """Reset the count of crash reports sent today.

        This clears the contents of the rate limiting directory which has
        the effect of reseting our count of crash reports sent.
        """
        utils.system('rm -rf ' + self._CRASH_SENDER_RATE_DIR)


    def _clear_spooled_crashes(self):
        """Clears system and user crash directories.

        This will remove all crash reports which are waiting to be sent.
        """
        utils.system('rm -rf ' + self._SYSTEM_CRASH_DIR)
        utils.system('rm -rf %s %s' % (self._USER_CRASH_DIRS,
                                       self._FALLBACK_USER_CRASH_DIR))


    def _kill_running_sender(self):
        """Kill the the crash_sender process if running.

        We use the PID file to find the process ID, then kill it with signal 9.
        """
        if not os.path.exists(self._CRASH_SENDER_RUN_PATH):
            return
        running_pid = int(utils.read_file(self._CRASH_SENDER_RUN_PATH))
        logging.warning('Detected running crash sender (%d), killing',
                        running_pid)
        utils.system('kill -9 %d' % running_pid)
        os.remove(self._CRASH_SENDER_RUN_PATH)


    def _set_sending_mock(self, mock_enabled, send_success=True):
        """Enables / disables mocking of the sending process.

        This uses the _MOCK_CRASH_SENDING file to achieve its aims. See notes
        at the top.

        @param mock_enabled: If True, mocking is enabled, else it is disabled.
        @param send_success: If mock_enabled this is True for the mocking to
                indicate success, False to indicate failure.
        """
        if mock_enabled:
            if send_success:
                data = ''
            else:
                data = '1'
            logging.info('Setting sending mock')
            utils.open_write_close(self._MOCK_CRASH_SENDING, data)
        else:
            utils.system('rm -f ' + self._MOCK_CRASH_SENDING)


    def _set_consent(self, has_consent):
        """Sets whether or not we have consent to send crash reports.

        This creates or deletes the _CONSENT_FILE to control whether
        crash_sender will consider that it has consent to send crash reports.
        It also copies a policy blob with the proper policy setting.

        @param has_consent: True to indicate consent, False otherwise
        """
        autotest_cros_dir = os.path.dirname(__file__)
        if has_consent:
            if os.path.isdir(constants.WHITELIST_DIR):
                # Create policy file that enables metrics/consent.
                shutil.copy('%s/mock_metrics_on.policy' % autotest_cros_dir,
                            constants.SIGNED_POLICY_FILE)
                shutil.copy('%s/mock_metrics_owner.key' % autotest_cros_dir,
                            constants.OWNER_KEY_FILE)
            # Create deprecated consent file.  This is created *after* the
            # policy file in order to avoid a race condition where chrome
            # might remove the consent file if the policy's not set yet.
            # We create it as a temp file first in order to make the creation
            # of the consent file, owned by chronos, atomic.
            # See crosbug.com/18413.
            temp_file = self._CONSENT_FILE + '.tmp';
            utils.open_write_close(temp_file, 'test-consent')
            utils.system('chown chronos:chronos "%s"' % (temp_file))
            shutil.move(temp_file, self._CONSENT_FILE)
            logging.info('Created ' + self._CONSENT_FILE)
        else:
            if os.path.isdir(constants.WHITELIST_DIR):
                # Create policy file that disables metrics/consent.
                shutil.copy('%s/mock_metrics_off.policy' % autotest_cros_dir,
                            constants.SIGNED_POLICY_FILE)
                shutil.copy('%s/mock_metrics_owner.key' % autotest_cros_dir,
                            constants.OWNER_KEY_FILE)
            # Remove deprecated consent file.
            utils.system('rm -f "%s"' % (self._CONSENT_FILE))


    def _set_crash_test_in_progress(self, in_progress):
        if in_progress:
            utils.open_write_close(self._CRASH_TEST_IN_PROGRESS, 'in-progress')
            logging.info('Created ' + self._CRASH_TEST_IN_PROGRESS)
        else:
            utils.system('rm -f "%s"' % (self._CRASH_TEST_IN_PROGRESS))


    def _get_pushed_consent_file_path(self):
        """Returns filename of the pushed consent file."""
        return os.path.join(self.bindir, 'pushed_consent')


    def _get_pushed_policy_file_path(self):
        """Returns filename of the pushed policy file."""
        return os.path.join(self.bindir, 'pushed_policy')


    def _get_pushed_owner_key_file_path(self):
        """Returns filename of the pushed owner.key file."""
        return os.path.join(self.bindir, 'pushed_owner_key')


    def _push_consent(self):
        """Push the consent file, thus disabling consent.

        The consent files can be created in the new test if required. Call
        _pop_consent() to restore the original state.
        """
        if os.path.exists(self._CONSENT_FILE):
            shutil.move(self._CONSENT_FILE,
                        self._get_pushed_consent_file_path())
        if os.path.exists(constants.SIGNED_POLICY_FILE):
            shutil.move(constants.SIGNED_POLICY_FILE,
                        self._get_pushed_policy_file_path())
        if os.path.exists(constants.OWNER_KEY_FILE):
            shutil.move(constants.OWNER_KEY_FILE,
                        self._get_pushed_owner_key_file_path())


    def _pop_consent(self):
        """Pop the consent files, enabling/disabling consent as it was before
        we pushed the consent."""
        if os.path.exists(self._get_pushed_consent_file_path()):
            shutil.move(self._get_pushed_consent_file_path(),
                        self._CONSENT_FILE)
        else:
            utils.system('rm -f "%s"' % self._CONSENT_FILE)
        if os.path.exists(self._get_pushed_policy_file_path()):
            shutil.move(self._get_pushed_policy_file_path(),
                        constants.SIGNED_POLICY_FILE)
        else:
            utils.system('rm -f "%s"' % constants.SIGNED_POLICY_FILE)
        if os.path.exists(self._get_pushed_owner_key_file_path()):
            shutil.move(self._get_pushed_owner_key_file_path(),
                        constants.OWNER_KEY_FILE)
        else:
            utils.system('rm -f "%s"' % constants.OWNER_KEY_FILE)


    def _get_crash_dir(self, username):
        """Returns full path to the crash directory for a given username

        This only really works (currently) when no one is logged in.  That
        is OK (currently) as the only test that uses this runs when no one
        is actually logged in.

        @param username: username to use:
                'chronos': Returns user crash directory.
                'root': Returns system crash directory.
        """
        if username == 'chronos':
            return self._FALLBACK_USER_CRASH_DIR
        else:
            return self._SYSTEM_CRASH_DIR


    def _initialize_crash_reporter(self):
        """Start up the crash reporter."""
        utils.system('%s --init --nounclean_check' % self._CRASH_REPORTER_PATH)
        # Completely disable crash_reporter from generating crash dumps
        # while any tests are running, otherwise a crashy system can make
        # these tests flaky.
        self.enable_crash_filtering('none')


    def get_crash_dir_name(self, name):
        """Return the full path for |name| inside the system crash directory."""
        return os.path.join(self._SYSTEM_CRASH_DIR, name)


    def write_crash_dir_entry(self, name, contents):
        """Writes an empty file to the system crash directory.

        This writes a file to _SYSTEM_CRASH_DIR with the given name. This is
        used to insert new crash dump files for testing purposes.

        @param name: Name of file to write.
        @param contents: String to write to the file.
        """
        entry = self.get_crash_dir_name(name)
        if not os.path.exists(self._SYSTEM_CRASH_DIR):
            os.makedirs(self._SYSTEM_CRASH_DIR)
        utils.open_write_close(entry, contents)
        return entry


    def write_fake_meta(self, name, exec_name, payload, log=None,
                        complete=True):
        """Writes a fake meta entry to the system crash directory.

        @param name: Name of file to write.
        @param exec_name: Value for exec_name item.
        @param payload: Value for payload item.
        @param log: Value for log item.
        @param complete: True to close off the record, otherwise leave it
                incomplete.
        """
        last_line = ''
        if complete:
            last_line = 'done=1\n'
        contents = ('exec_name=%s\n'
                    'ver=my_ver\n'
                    'payload=%s\n'
                    '%s' % (exec_name, payload,
                            last_line))
        if log:
            contents = ('log=%s\n' % log) + contents
        return self.write_crash_dir_entry(name, contents)


    def _prepare_sender_one_crash(self,
                                  send_success,
                                  reports_enabled,
                                  report):
        """Create metadata for a fake crash report.

        This enabled mocking of the crash sender, then creates a fake
        crash report for testing purposes.

        @param send_success: True to make the crash_sender success, False to
                make it fail.
        @param reports_enabled: True to enable consent to that reports will be
                sent.
        @param report: Report to use for crash, if None we create one.
        """
        self._set_sending_mock(mock_enabled=True, send_success=send_success)
        self._set_consent(reports_enabled)
        if report is None:
            # Use the same file format as crash does normally:
            # <basename>.#.#.#.meta
            payload = self.write_crash_dir_entry(
                '%s.dmp' % self._FAKE_TEST_BASENAME, '')
            report = self.write_fake_meta(
                '%s.meta' % self._FAKE_TEST_BASENAME, 'fake', payload)
        return report


    def _parse_sender_output(self, output):
        """Parse the log output from the crash_sender script.

        This script can run on the logs from either a mocked or true
        crash send.

        @param output: output from the script

        @returns A dictionary with these values:
            error_type: an error type, if given
            exec_name: name of executable which crashed
            image_type: type of image ("dev","force-official",...), if given
            boot_mode: current boot mode ("dev",...), if given
            meta_path: path to the report metadata file
            output: the output from the script, copied
            report_kind: kind of report sent (minidump vs kernel)
            send_attempt: did the script attempt to send a crash.
            send_success: if it attempted, was the crash send successful.
            sig: signature of the report, if given.
            sleep_time: if it attempted, how long did it sleep before
              sending (if mocked, how long would it have slept)
        """
        sleep_match = re.search('Scheduled to send in (\d+)s', output)
        send_attempt = sleep_match is not None
        if send_attempt:
            sleep_time = int(sleep_match.group(1))
        else:
            sleep_time = None

        meta_match = re.search('Metadata: (\S+) \((\S+)\)', output)
        if meta_match:
            meta_path = meta_match.group(1)
            report_kind = meta_match.group(2)
        else:
            meta_path = None
            report_kind = None

        payload_match = re.search('Payload: (\S+)', output)
        if payload_match:
            report_payload = payload_match.group(1)
        else:
            report_payload = None

        exec_name_match = re.search('Exec name: (\S+)', output)
        if exec_name_match:
            exec_name = exec_name_match.group(1)
        else:
            exec_name = None

        sig_match = re.search('sig: (\S+)', output)
        if sig_match:
            sig = sig_match.group(1)
        else:
            sig = None

        error_type_match = re.search('Error type: (\S+)', output)
        if error_type_match:
            error_type = error_type_match.group(1)
        else:
            error_type = None

        image_type_match = re.search('Image type: (\S+)', output)
        if image_type_match:
            image_type = image_type_match.group(1)
        else:
            image_type = None

        boot_mode_match = re.search('Boot mode: (\S+)', output)
        if boot_mode_match:
            boot_mode = boot_mode_match.group(1)
        else:
            boot_mode = None

        send_success = 'Mocking successful send' in output
        return {'exec_name': exec_name,
                'report_kind': report_kind,
                'meta_path': meta_path,
                'report_payload': report_payload,
                'send_attempt': send_attempt,
                'send_success': send_success,
                'sig': sig,
                'error_type': error_type,
                'image_type': image_type,
                'boot_mode': boot_mode,
                'sleep_time': sleep_time,
                'output': output}


    def wait_for_sender_completion(self):
        """Wait for crash_sender to complete.

        Wait for no crash_sender's last message to be placed in the
        system log before continuing and for the process to finish.
        Otherwise we might get only part of the output."""
        utils.poll_for_condition(
            lambda: self._log_reader.can_find('crash_sender done.'),
            timeout=60,
            exception=error.TestError(
              'Timeout waiting for crash_sender to emit done: ' +
              self._log_reader.get_logs()))
        utils.poll_for_condition(
            lambda: utils.system('pgrep crash_sender',
                                 ignore_status=True) != 0,
            timeout=60,
            exception=error.TestError(
                'Timeout waiting for crash_sender to finish: ' +
                self._log_reader.get_logs()))


    def _call_sender_one_crash(self,
                               send_success=True,
                               reports_enabled=True,
                               username='root',
                               report=None,
                               should_fail=False):
        """Call the crash sender script to mock upload one crash.

        @param send_success: Mock a successful send if true
        @param reports_enabled: Has the user consented to sending crash reports.
        @param username: user to emulate a crash from
        @param report: report to use for crash, if None we create one.

        @returns a dictionary describing the result with the keys
          from _parse_sender_output, as well as:
            report_exists: does the minidump still exist after calling
              send script
            rate_count: how many crashes have been uploaded in the past
              24 hours.
        """
        report = self._prepare_sender_one_crash(send_success,
                                                reports_enabled,
                                                report)
        self._log_reader.set_start_by_current()
        script_output = ""
        try:
            script_output = utils.system_output(
                '/bin/sh -c "%s" 2>&1' % self._CRASH_SENDER_PATH,
                ignore_status=should_fail)
        except error.CmdError as err:
            raise error.TestFail('"%s" returned an unexpected non-zero '
                                 'value (%s).'
                                 % (err.command, err.result_obj.exit_status))

        self.wait_for_sender_completion()
        output = self._log_reader.get_logs()
        logging.debug('Crash sender message output:\n' + output)

        if script_output != '':
            logging.debug('crash_sender stdout/stderr: ' + script_output)

        if os.path.exists(report):
            report_exists = True
            os.remove(report)
        else:
            report_exists = False
        if os.path.exists(self._CRASH_SENDER_RATE_DIR):
            rate_count = len(os.listdir(self._CRASH_SENDER_RATE_DIR))
        else:
            rate_count = 0

        result = self._parse_sender_output(output)
        result['report_exists'] = report_exists
        result['rate_count'] = rate_count

        # Show the result for debugging but remove 'output' key
        # since it's large and earlier in debug output.
        debug_result = dict(result)
        del debug_result['output']
        logging.debug('Result of send (besides output): %s', debug_result)

        return result


    def _replace_crash_reporter_filter_in(self, new_parameter):
        """Replaces the --filter_in= parameter of the crash reporter.

        The kernel is set up to call the crash reporter with the core dump
        as stdin when a process dies. This function adds a filter to the
        command line used to call the crash reporter. This is used to ignore
        crashes in which we have no interest.

        This removes any --filter_in= parameter and optionally replaces it
        with a new one.

        @param new_parameter: This is parameter to add to the command line
                instead of the --filter_in=... that was there.
        """
        core_pattern = utils.read_file(self._CORE_PATTERN)[:-1]
        core_pattern = re.sub('--filter_in=\S*\s*', '',
                              core_pattern).rstrip()
        if new_parameter:
            core_pattern += ' ' + new_parameter
        utils.system('echo "%s" > %s' % (core_pattern, self._CORE_PATTERN))


    def enable_crash_filtering(self, name):
        """Add a --filter_in argument to the kernel core dump cmdline.

        @param name: Filter text to use. This is passed as a --filter_in
                argument to the crash reporter.
        """
        self._replace_crash_reporter_filter_in('--filter_in=' + name)


    def disable_crash_filtering(self):
        """Remove the --filter_in argument from the kernel core dump cmdline.

        Next time the crash reporter is invoked (due to a crash) it will not
        receive a --filter_in paramter."""
        self._replace_crash_reporter_filter_in('')


    @contextlib.contextmanager
    def hold_crash_lock(self):
        """A context manager to hold the crash sender lock."""
        with open(self._CRASH_SENDER_LOCK_PATH, 'w+') as f:
            fcntl.flock(f.fileno(), fcntl.LOCK_EX)
            try:
                yield
            finally:
                fcntl.flock(f.fileno(), fcntl.LOCK_UN)


    def initialize(self):
        """Initalize the test."""
        test.test.initialize(self)
        self._log_reader = cros_logging.make_system_log_reader()
        self._leave_crash_sending = True
        self._automatic_consent_saving = True
        self.enable_crash_filtering('none')
        self._set_crash_test_in_progress(True)


    def cleanup(self):
        """Cleanup after the test.

        We reset things back to the way we think they should be. This is
        intended to allow the system to continue normal operation.

        Some variables silently change the behavior:
            _automatic_consent_saving: if True, we pop the consent file.
            _leave_crash_sending: True to enable crash sending, False to
                disable it
        """
        self._reset_rate_limiting()
        self._clear_spooled_crashes()
        self._set_system_sending(self._leave_crash_sending)
        self._set_sending_mock(mock_enabled=False)
        if self._automatic_consent_saving:
            self._pop_consent()
        self.disable_crash_filtering()
        self._set_crash_test_in_progress(False)
        test.test.cleanup(self)


    def run_crash_tests(self,
                        test_names,
                        initialize_crash_reporter=False,
                        clear_spool_first=True,
                        must_run_all=True):
        """Run crash tests defined in this class.

        @param test_names: Array of test names.
        @param initialize_crash_reporter: Should set up crash reporter for every
                run.
        @param clear_spool_first: Clear all spooled user/system crashes before
                starting the test.
        @param must_run_all: Should make sure every test in this class is
                mentioned in test_names.
        """
        if self._automatic_consent_saving:
            self._push_consent()

        if must_run_all:
            # Sanity check test_names is complete
            for attr in dir(self):
                if attr.find('_test_') == 0:
                    test_name = attr[6:]
                    if not test_name in test_names:
                        raise error.TestError('Test %s is missing' % test_name)

        for test_name in test_names:
            logging.info(('=' * 20) + ('Running %s' % test_name) + ('=' * 20))
            if initialize_crash_reporter:
                self._initialize_crash_reporter()
            # Disable crash_sender from running, kill off any running ones, but
            # set environment so crash_sender may run as a child process.
            self._set_system_sending(False)
            self._set_child_sending(True)
            self._kill_running_sender()
            self._reset_rate_limiting()
            # Default to not overriding for unofficial versions.
            self._set_force_official(False)
            # Default to not pretending we're in developer mode.
            self._set_mock_developer_mode(False)
            if clear_spool_first:
                self._clear_spooled_crashes()

            # Call the test function
            getattr(self, '_test_' + test_name)()
