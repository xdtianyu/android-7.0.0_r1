# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import grp, logging, os, pwd, re, stat, subprocess
from signal import SIGSEGV
from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.cros import crash_test, cros_ui, upstart


_COLLECTION_ERROR_SIGNATURE = 'crash_reporter-user-collection'
_CORE2MD_PATH = '/usr/bin/core2md'
_LEAVE_CORE_PATH = '/root/.leave_core'
_MAX_CRASH_DIRECTORY_SIZE = 32


class logging_UserCrash(crash_test.CrashTest):
    version = 1


    def setup(self):
        os.chdir(self.srcdir)
        utils.make('clean')
        utils.make('all')


    def _test_reporter_startup(self):
        """Test that the core_pattern is set up by crash reporter."""
        # Turn off crash filtering so we see the original setting.
        self.disable_crash_filtering()
        output = utils.read_file(self._CORE_PATTERN).rstrip()
        expected_core_pattern = ('|%s --user=%%P:%%s:%%u:%%e' %
                                 self._CRASH_REPORTER_PATH)
        if output != expected_core_pattern:
            raise error.TestFail('core pattern should have been %s, not %s' %
                                 (expected_core_pattern, output))

        self._log_reader.set_start_by_reboot(-1)

        if not self._log_reader.can_find('Enabling user crash handling'):
            raise error.TestFail(
                'user space crash handling was not started during last boot')


    def _test_reporter_shutdown(self):
        """Test the crash_reporter shutdown code works."""
        self._log_reader.set_start_by_current()
        utils.system('%s --clean_shutdown' % self._CRASH_REPORTER_PATH)
        output = utils.read_file(self._CORE_PATTERN).rstrip()
        if output != 'core':
            raise error.TestFail('core pattern should have been core, not %s' %
                                 output)


    def _prepare_crasher(self):
        """Extract the crasher and set its permissions.

        crasher is only gzipped to subvert Portage stripping.
        """
        self._crasher_path = os.path.join(self.srcdir, 'crasher_nobreakpad')
        utils.system('cd %s; tar xzf crasher.tgz-unmasked' %
                     self.srcdir)
        # Make sure all users (specifically chronos) have access to
        # this directory and its decendents in order to run crasher
        # executable as different users.
        utils.system('chmod -R a+rx ' + self.bindir)


    def _populate_symbols(self):
        """Set up Breakpad's symbol structure.

        Breakpad's minidump processor expects symbols to be in a directory
        hierarchy:
          <symbol-root>/<module_name>/<file_id>/<module_name>.sym
        """
        # Dump the symbols from the crasher
        self._symbol_dir = os.path.join(self.srcdir, 'symbols')
        utils.system('rm -rf %s' % self._symbol_dir)
        os.mkdir(self._symbol_dir)

        basename = os.path.basename(self._crasher_path)
        utils.system('/usr/bin/dump_syms %s > %s.sym' %
                     (self._crasher_path,
                      basename))
        sym_name = '%s.sym' % basename
        symbols = utils.read_file(sym_name)
        # First line should be like:
        # MODULE Linux x86 7BC3323FBDBA2002601FA5BA3186D6540 crasher_XXX
        #  or
        # MODULE Linux arm C2FE4895B203D87DD4D9227D5209F7890 crasher_XXX
        first_line = symbols.split('\n')[0]
        tokens = first_line.split()
        if tokens[0] != 'MODULE' or tokens[1] != 'Linux':
          raise error.TestError('Unexpected symbols format: %s',
                                first_line)
        file_id = tokens[3]
        target_dir = os.path.join(self._symbol_dir, basename, file_id)
        os.makedirs(target_dir)
        os.rename(sym_name, os.path.join(target_dir, sym_name))


    def _is_frame_in_stack(self, frame_index, module_name,
                           function_name, file_name,
                           line_number, stack):
        """Search for frame entries in the given stack dump text.

        A frame entry looks like (alone on a line):
          16  crasher_nobreakpad!main [crasher.cc : 21 + 0xb]

        Args:
          frame_index: number of the stack frame (0 is innermost frame)
          module_name: name of the module (executable or dso)
          function_name: name of the function in the stack
          file_name: name of the file containing the function
          line_number: line number
          stack: text string of stack frame entries on separate lines.

        Returns:
          Boolean indicating if an exact match is present.

        Note:
          We do not care about the full function signature - ie, is it
          foo or foo(ClassA *).  These are present in function names
          pulled by dump_syms for Stabs but not for DWARF.
        """
        regexp = (r'\n\s*%d\s+%s!%s.*\[\s*%s\s*:\s*%d\s.*\]' %
                  (frame_index, module_name,
                   function_name, file_name,
                   line_number))
        logging.info('Searching for regexp ' + regexp)
        return re.search(regexp, stack) is not None


    def _verify_stack(self, stack, basename, from_crash_reporter):
        logging.debug('Crash stackwalk was: %s' % stack)

        # Should identify cause as SIGSEGV at address 0x16
        match = re.search(r'Crash reason:\s+(.*)', stack)
        expected_address = '0x16'
        if from_crash_reporter:
            # We cannot yet determine the crash address when coming
            # through core files via crash_reporter.
            expected_address = '0x0'
        if not match or match.group(1) != 'SIGSEGV':
            raise error.TestFail('Did not identify SIGSEGV cause')
        match = re.search(r'Crash address:\s+(.*)', stack)
        if not match or match.group(1) != expected_address:
            raise error.TestFail('Did not identify crash address %s' %
                                 expected_address)

        # Should identify crash at *(char*)0x16 assignment line
        if not self._is_frame_in_stack(0, basename,
                                       'recbomb', 'bomb.cc', 9, stack):
            raise error.TestFail('Did not show crash line on stack')

        # Should identify recursion line which is on the stack
        # for 15 levels
        if not self._is_frame_in_stack(15, basename, 'recbomb',
                                       'bomb.cc', 12, stack):
            raise error.TestFail('Did not show recursion line on stack')

        # Should identify main line
        if not self._is_frame_in_stack(16, basename, 'main',
                                       'crasher.cc', 20, stack):
            raise error.TestFail('Did not show main on stack')


    def _run_crasher_process(self, username, cause_crash=True, consent=True,
                             crasher_path=None):
        """Runs the crasher process.

        Will wait up to 5 seconds for crash_reporter to report the crash.
        crash_reporter_caught will be marked as true when the "Received crash
        notification message..." appears. While associated logs are likely to be
        available at this point, the function does not guarantee this.

        Args:
          username: runs as given user
          extra_args: additional parameters to pass to crasher process

        Returns:
          A dictionary with keys:
            returncode: return code of the crasher
            crashed: did the crasher return segv error code
            crash_reporter_caught: did crash_reporter catch a segv
            output: stderr/stdout output of the crasher process
        """
        if crasher_path is None: crasher_path = self._crasher_path
        self.enable_crash_filtering(os.path.basename(crasher_path))

        if username != 'root':
            crasher_command = ['su', username, '-c']
            expected_result = 128 + SIGSEGV
        else:
            crasher_command = []
            expected_result = -SIGSEGV

        crasher_command.append(crasher_path)
        basename = os.path.basename(crasher_path)
        if not cause_crash:
            crasher_command.append('--nocrash')
        self._set_consent(consent)
        crasher = subprocess.Popen(crasher_command,
                                   stdout=subprocess.PIPE,
                                   stderr=subprocess.PIPE)
        output = crasher.communicate()[1]
        logging.debug('Output from %s: %s' %
                      (crasher_command, output))

        # Grab the pid from the process output.  We can't just use
        # crasher.pid unfortunately because that may be the PID of su.
        match = re.search(r'pid=(\d+)', output)
        if not match:
            raise error.TestFail('Could not find pid output from crasher: %s' %
                                 output)
        pid = int(match.group(1))

        expected_uid = pwd.getpwnam(username)[2]
        if consent:
            handled_string = 'handling'
        else:
            handled_string = 'ignoring - no consent'
        expected_message = (
            'Received crash notification for %s[%d] sig 11, user %d (%s)' %
            (basename, pid, expected_uid, handled_string))

        # Wait until no crash_reporter is running.
        utils.poll_for_condition(
            lambda: utils.system('pgrep -f crash_reporter.*:%s' % basename,
                                 ignore_status=True) != 0,
            timeout=10,
            exception=error.TestError(
                'Timeout waiting for crash_reporter to finish: ' +
                self._log_reader.get_logs()))

        logging.debug('crash_reporter_caught message: ' + expected_message)
        is_caught = False
        try:
            utils.poll_for_condition(
                lambda: self._log_reader.can_find(expected_message),
                timeout=5)
            is_caught = True
        except utils.TimeoutError:
            pass

        result = {'crashed': crasher.returncode == expected_result,
                  'crash_reporter_caught': is_caught,
                  'output': output,
                  'returncode': crasher.returncode}
        logging.debug('Crasher process result: %s' % result)
        return result


    def _check_crash_directory_permissions(self, crash_dir):
        stat_info = os.stat(crash_dir)
        user = pwd.getpwuid(stat_info.st_uid)[0]
        group = grp.getgrgid(stat_info.st_gid)[0]
        mode = stat.S_IMODE(stat_info.st_mode)

        if crash_dir == '/var/spool/crash':
            expected_user = 'root'
            expected_group = 'root'
            expected_mode = 01755
        else:
            expected_user = 'chronos'
            expected_group = 'chronos'
            expected_mode = 0755

        if user != expected_user or group != expected_group:
            raise error.TestFail(
                'Expected %s.%s ownership of %s (actual %s.%s)' %
                (expected_user, expected_group, crash_dir, user, group))
        if mode != expected_mode:
            raise error.TestFail(
                'Expected %s to have mode %o (actual %o)' %
                (crash_dir, expected_mode, mode))


    def _check_minidump_stackwalk(self, minidump_path, basename,
                                  from_crash_reporter):
        # Now stackwalk the minidump
        stack = utils.system_output('/usr/bin/minidump_stackwalk %s %s' %
                                    (minidump_path, self._symbol_dir))
        self._verify_stack(stack, basename, from_crash_reporter)


    def _check_generated_report_sending(self, meta_path, payload_path,
                                        username, exec_name, report_kind,
                                        expected_sig=None):
        # Now check that the sending works
        result = self._call_sender_one_crash(
            username=username,
            report=os.path.basename(payload_path))
        if (not result['send_attempt'] or not result['send_success'] or
            result['report_exists']):
            raise error.TestFail('Report not sent properly')
        if result['exec_name'] != exec_name:
            raise error.TestFail('Executable name incorrect')
        if result['report_kind'] != report_kind:
            raise error.TestFail('Expected a minidump report')
        if result['report_payload'] != payload_path:
            raise error.TestFail('Sent the wrong minidump payload')
        if result['meta_path'] != meta_path:
            raise error.TestFail('Used the wrong meta file')
        if expected_sig is None:
            if result['sig'] is not None:
                raise error.TestFail('Report should not have signature')
        else:
            if not 'sig' in result or result['sig'] != expected_sig:
                raise error.TestFail('Report signature mismatch: %s vs %s' %
                                     (result['sig'], expected_sig))

        # Check version matches.
        lsb_release = utils.read_file('/etc/lsb-release')
        version_match = re.search(r'CHROMEOS_RELEASE_VERSION=(.*)', lsb_release)
        if not ('Version: %s' % version_match.group(1)) in result['output']:
            raise error.TestFail('Did not find version %s in log output' %
                                 version_match.group(1))


    def _run_crasher_process_and_analyze(self, username,
                                         cause_crash=True, consent=True,
                                         crasher_path=None):
        self._log_reader.set_start_by_current()

        if crasher_path is None: crasher_path = self._crasher_path
        result = self._run_crasher_process(username, cause_crash=cause_crash,
                                           consent=consent,
                                           crasher_path=crasher_path)

        if not result['crashed'] or not result['crash_reporter_caught']:
            return result;

        crash_dir = self._get_crash_dir(username)

        if not consent:
            if os.path.exists(crash_dir):
                raise error.TestFail('Crash directory should not exist')
            return result

        crash_contents = os.listdir(crash_dir)
        basename = os.path.basename(crasher_path)

        breakpad_minidump = None
        crash_reporter_minidump = None
        crash_reporter_meta = None
        crash_reporter_log = None

        self._check_crash_directory_permissions(crash_dir)

        logging.debug('Contents in %s: %s' % (crash_dir, crash_contents))

        for filename in crash_contents:
            if filename.endswith('.core'):
                # Ignore core files.  We'll test them later.
                pass
            elif (filename.startswith(basename) and
                  filename.endswith('.dmp')):
                # This appears to be a minidump created by the crash reporter.
                if not crash_reporter_minidump is None:
                    raise error.TestFail('Crash reporter wrote multiple '
                                         'minidumps')
                crash_reporter_minidump = os.path.join(crash_dir, filename)
            elif (filename.startswith(basename) and
                  filename.endswith('.meta')):
                if not crash_reporter_meta is None:
                    raise error.TestFail('Crash reporter wrote multiple '
                                         'meta files')
                crash_reporter_meta = os.path.join(crash_dir, filename)
            elif (filename.startswith(basename) and
                  filename.endswith('.log')):
                if not crash_reporter_log is None:
                    raise error.TestFail('Crash reporter wrote multiple '
                                         'log files')
                crash_reporter_log = os.path.join(crash_dir, filename)
            else:
                # This appears to be a breakpad created minidump.
                if not breakpad_minidump is None:
                    raise error.TestFail('Breakpad wrote multimpe minidumps')
                breakpad_minidump = os.path.join(crash_dir, filename)

        if breakpad_minidump:
            raise error.TestFail('%s did generate breakpad minidump' % basename)

        if not crash_reporter_meta:
            raise error.TestFail('crash reporter did not generate meta')

        result['minidump'] = crash_reporter_minidump
        result['basename'] = basename
        result['meta'] = crash_reporter_meta
        result['log'] = crash_reporter_log
        return result


    def _check_crashed_and_caught(self, result):
        if not result['crashed']:
            raise error.TestFail('crasher did not do its job of crashing: %d' %
                                 result['returncode'])

        if not result['crash_reporter_caught']:
            logging.debug('Messages that should have included segv: %s' %
                          self._log_reader.get_logs())
            raise error.TestFail('Did not find segv message')


    def _check_crashing_process(self, username, consent=True):
        result = self._run_crasher_process_and_analyze(username,
                                                       consent=consent)

        self._check_crashed_and_caught(result)

        if not consent:
            return

        if not result['minidump']:
            raise error.TestFail('crash reporter did not generate minidump')

        if not self._log_reader.can_find('Stored minidump to ' +
                                         result['minidump']):
            raise error.TestFail('crash reporter did not announce minidump')

        self._check_minidump_stackwalk(result['minidump'],
                                       result['basename'],
                                       from_crash_reporter=True)
        self._check_generated_report_sending(result['meta'],
                                             result['minidump'],
                                             username,
                                             result['basename'],
                                             'minidump')

    def _test_no_crash(self):
        """Test a program linked against libcrash_dumper can exit normally."""
        self._log_reader.set_start_by_current()
        result = self._run_crasher_process_and_analyze(username='root',
                                                       cause_crash=False)
        if (result['crashed'] or
            result['crash_reporter_caught'] or
            result['returncode'] != 0):
            raise error.TestFail('Normal exit of program with dumper failed')


    def _test_chronos_crasher(self):
        """Test a user space crash when running as chronos is handled."""
        self._check_crashing_process('chronos')


    def _test_chronos_crasher_no_consent(self):
        """Test that without consent no files are stored."""
        results = self._check_crashing_process('chronos', consent=False)


    def _test_root_crasher(self):
        """Test a user space crash when running as root is handled."""
        self._check_crashing_process('root')


    def _test_root_crasher_no_consent(self):
        """Test that without consent no files are stored."""
        results = self._check_crashing_process('root', consent=False)


    def _check_filter_crasher(self, should_receive):
        self._log_reader.set_start_by_current()
        crasher_basename = os.path.basename(self._crasher_path)
        utils.system(self._crasher_path, ignore_status=True);
        if should_receive:
            to_find = 'Received crash notification for ' + crasher_basename
        else:
            to_find = 'Ignoring crash from ' + crasher_basename
        utils.poll_for_condition(
            lambda: self._log_reader.can_find(to_find),
            timeout=10,
            exception=error.TestError(
              'Timeout waiting for: ' + to_find + ' in ' +
              self._log_reader.get_logs()))


    def _test_crash_filtering(self):
        """Test that crash filtering (a feature needed for testing) works."""
        crasher_basename = os.path.basename(self._crasher_path)
        self._log_reader.set_start_by_current()

        self.enable_crash_filtering('none')
        self._check_filter_crasher(False)

        self.enable_crash_filtering('sleep')
        self._check_filter_crasher(False)

        self.disable_crash_filtering()
        self._check_filter_crasher(True)


    def _test_max_enqueued_crashes(self):
        """Test that _MAX_CRASH_DIRECTORY_SIZE is enforced."""
        self._log_reader.set_start_by_current()
        username = 'root'

        crash_dir = self._get_crash_dir(username)
        full_message = ('Crash directory %s already full with %d pending '
                        'reports' % (crash_dir, _MAX_CRASH_DIRECTORY_SIZE))

        # Fill up the queue.
        for i in range(0, _MAX_CRASH_DIRECTORY_SIZE):
          result = self._run_crasher_process(username)
          if not result['crashed']:
            raise error.TestFail('failure while setting up queue: %d' %
                                 result['returncode'])
          if self._log_reader.can_find(full_message):
            raise error.TestFail('unexpected full message: ' + full_message)

        crash_dir_size = len(os.listdir(crash_dir))
        # For debugging
        utils.system('ls -l %s' % crash_dir)
        logging.info('Crash directory had %d entries' % crash_dir_size)

        # Crash a bunch more times, but make sure no new reports
        # are enqueued.
        for i in range(0, 10):
          self._log_reader.set_start_by_current()
          result = self._run_crasher_process(username)
          logging.info('New log messages: %s' % self._log_reader.get_logs())
          if not result['crashed']:
            raise error.TestFail('failure after setting up queue: %d' %
                                 result['returncode'])
          utils.poll_for_condition(
              lambda: self._log_reader.can_find(full_message),
              timeout=20,
              exception=error.TestFail('expected full message: ' +
                                       full_message))
          if crash_dir_size != len(os.listdir(crash_dir)):
            utils.system('ls -l %s' % crash_dir)
            raise error.TestFail('expected no new files (now %d were %d)',
                                 len(os.listdir(crash_dir)),
                                 crash_dir_size)


    def _check_collection_failure(self, test_option, failure_string):
        # Add parameter to core_pattern.
        old_core_pattern = utils.read_file(self._CORE_PATTERN)[:-1]
        try:
            utils.system('echo "%s %s" > %s' % (old_core_pattern, test_option,
                                                self._CORE_PATTERN))
            result = self._run_crasher_process_and_analyze('root',
                                                           consent=True)
            self._check_crashed_and_caught(result)
            if not self._log_reader.can_find(failure_string):
                raise error.TestFail('Did not find fail string in log %s' %
                                     failure_string)
            if result['minidump']:
                raise error.TestFail('failed collection resulted in minidump')
            if not result['log']:
                raise error.TestFail('failed collection had no log')
            log_contents = utils.read_file(result['log'])
            logging.debug('Log contents were: ' + log_contents)
            if not failure_string in log_contents:
                raise error.TestFail('Expected logged error '
                                     '\"%s\" was \"%s\"' %
                                     (failure_string, log_contents))
            # Verify we are generating appropriate diagnostic output.
            if ((not '===ps output===' in log_contents) or
                (not '===meminfo===' in log_contents)):
                raise error.TestFail('Expected full logs, got: ' + log_contents)
            self._check_generated_report_sending(result['meta'],
                                                 result['log'],
                                                 'root',
                                                 result['basename'],
                                                 'log',
                                                 _COLLECTION_ERROR_SIGNATURE)
        finally:
            utils.system('echo "%s" > %s' % (old_core_pattern,
                                             self._CORE_PATTERN))


    def _test_core2md_failure(self):
        self._check_collection_failure('--core2md_failure',
                                       'Problem during %s [result=1]: Usage:' %
                                       _CORE2MD_PATH)


    def _test_internal_directory_failure(self):
        self._check_collection_failure('--directory_failure',
                                       'Purposefully failing to create')


    def _test_crash_logs_creation(self):
        logs_triggering_crasher = os.path.join(os.path.dirname(self.bindir),
                                               'crash_log_test')
        # Copy crasher_path to a test location with correct mode and a
        # special name to trigger crash log creation.
        utils.system('cp -a "%s" "%s"' % (self._crasher_path,
                                          logs_triggering_crasher))
        result = self._run_crasher_process_and_analyze(
            'root', crasher_path=logs_triggering_crasher)
        self._check_crashed_and_caught(result)
        contents = utils.read_file(result['log'])
        if contents != 'hello world\n':
            raise error.TestFail('Crash log contents unexpected: %s' % contents)
        if not ('log=' + result['log']) in utils.read_file(result['meta']):
            raise error.TestFail('Meta file does not reference log')


    def _test_crash_log_infinite_recursion(self):
        recursion_triggering_crasher = os.path.join(
            os.path.dirname(self.bindir), 'crash_log_recursion_test')
        # The configuration file hardcodes this path, so make sure it's still
        # the same.
        if (recursion_triggering_crasher !=
            '/usr/local/autotest/tests/crash_log_recursion_test'):
          raise error.TestError('Path to recursion test changed')
        # Copy crasher_path to a test location with correct mode and a
        # special name to trigger crash log creation.
        utils.system('cp -a "%s" "%s"' % (self._crasher_path,
                                          recursion_triggering_crasher))
        # Simply completing this command means that we avoided
        # infinite recursion.
        result = self._run_crasher_process(
            'root', crasher_path=recursion_triggering_crasher)


    def _check_core_file_persisting(self, expect_persist):
        self._log_reader.set_start_by_current()

        result = self._run_crasher_process('root')

        if not result['crashed']:
            raise error.TestFail('crasher did not crash')

        crash_contents = os.listdir(self._get_crash_dir('root'))

        logging.debug('Contents of crash directory: %s', crash_contents)
        logging.debug('Log messages: %s' % self._log_reader.get_logs())

        if expect_persist:
            if not self._log_reader.can_find('Leaving core file at'):
                raise error.TestFail('Missing log message')
            expected_core_files = 1
        else:
            if self._log_reader.can_find('Leaving core file at'):
                raise error.TestFail('Unexpected log message')
            expected_core_files = 0

        dmp_files = 0
        core_files = 0
        for filename in crash_contents:
            if filename.endswith('.dmp'):
                dmp_files += 1
            if filename.endswith('.core'):
                core_files += 1

        if dmp_files != 1:
            raise error.TestFail('Should have been exactly 1 dmp file')
        if core_files != expected_core_files:
            raise error.TestFail('Should have been exactly %d core files' %
                                 expected_core_files)


    def _test_core_file_removed_in_production(self):
        """Test that core files do not stick around for production builds."""
        # Avoid remounting / rw by instead creating a tmpfs in /root and
        # populating it with everything but the
        utils.system('tar -cvz -C /root -f /tmp/root.tgz .')
        utils.system('mount -t tmpfs tmpfs /root')
        try:
            utils.system('tar -xvz -C /root -f /tmp/root.tgz .')
            os.remove(_LEAVE_CORE_PATH)
            if os.path.exists(_LEAVE_CORE_PATH):
                raise error.TestFail('.leave_core file did not disappear')
            self._check_core_file_persisting(False)
        finally:
            os.system('umount /root')


    def initialize(self):
        super(logging_UserCrash, self).initialize()

        # If the device has a GUI, return the device to the sign-in screen, as
        # some tests will fail inside a user session.
        if upstart.has_service('ui'):
            cros_ui.restart()


    # TODO(kmixter): Test crashing a process as ntp or some other
    # non-root, non-chronos user.

    def run_once(self):
        self._prepare_crasher()
        self._populate_symbols()

        # Run the test once without re-initializing
        # to catch problems with the default crash reporting setup
        self.run_crash_tests(['reporter_startup'],
                              initialize_crash_reporter=False,
                              must_run_all=False)

        self.run_crash_tests(['reporter_startup',
                              'reporter_shutdown',
                              'no_crash',
                              'chronos_crasher',
                              'chronos_crasher_no_consent',
                              'root_crasher',
                              'root_crasher_no_consent',
                              'crash_filtering',
                              'max_enqueued_crashes',
                              'core2md_failure',
                              'internal_directory_failure',
                              'crash_logs_creation',
                              'crash_log_infinite_recursion',
                              'core_file_removed_in_production'],
                              initialize_crash_reporter=True)
