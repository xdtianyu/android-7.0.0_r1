# Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging, os, time

from autotest_lib.client.common_lib import error
from autotest_lib.client.cros import constants
from autotest_lib.client.cros.crash_test import CrashTest as CrashTestDefs
from autotest_lib.server import test

class platform_KernelErrorPaths(test.test):
    """Performs various kernel crash tests and makes sure that the expected
       results are found in the crash report."""
    version = 1

    def _run_client_command(self, command):
        try:
            # Simply sending the trigger into lkdtm resets the target
            # immediately, leaving files unsaved to disk and the master ssh
            # connection wedged for a long time. The sequence below borrowed
            # from logging_KernelCrashServer.py makes sure that the test
            # proceeds smoothly.
            self.client.run(
                'sh -c "sync; sleep 1; %s" >/dev/null 2>&1 &' % command)
        except error.AutoservRunError, e:
            # It is expected that this will cause a non-zero exit status.
            pass

    def _provoke_crash(self, interface, trigger, cpu):
        """
        This test is ensuring that the machine will reboot on any
        type of kernel panic.  If the sysctls below are not set
        correctly, the machine will not reboot.  After verifying
        that the machine has the proper sysctl state, we make it
        reboot by writing to lkdtm.

        @param interface: which filesystem interface to write into
        @param trigger: the text string to write for triggering a crash
        @param cpu: None or a specific cpu number to pin before crashing
        """
        self.client.run('sysctl kernel.panic|grep "kernel.panic = -1"');
        self.client.run('sysctl kernel.panic_on_oops|'
                        'grep "kernel.panic_on_oops = 1"');

        if cpu != None:
            # Run on a specific CPU using taskset
            command = "echo %s | taskset -c %d tee %s" % (trigger, cpu,
                                                          interface)
        else:
            # Run normally
            command = "echo %s > %s" % (trigger, interface)

        logging.info("KernelErrorPaths: executing '%s' on %s",
                     command, self.client.hostname)
        self._run_client_command(command)

    def _exists_on_client(self, f):
        return self.client.run('ls "%s"' % f,
                               ignore_status=True).exit_status == 0

    def _enable_consent(self):
        """ Enable consent so that crashes get stored in /var/spool/crash. """
        self._consent_files = [
            (CrashTestDefs._PAUSE_FILE, None, 'chronos'),
            (CrashTestDefs._CONSENT_FILE, None, 'chronos'),
            (constants.SIGNED_POLICY_FILE, 'mock_metrics_on.policy', 'root'),
            (constants.OWNER_KEY_FILE, 'mock_metrics_owner.key', 'root'),
            ]
        for dst, src, owner in self._consent_files:
            if self._exists_on_client(dst):
                self.client.run('mv "%s" "%s.autotest_backup"' % (dst, dst))
            if src:
                full_src = os.path.join(self.autodir, 'client/cros', src)
                self.client.send_file(full_src, dst)
            else:
                self.client.run('touch "%s"' % dst)
            self.client.run('chown "%s" "%s"' % (owner, dst))

    def _restore_consent_files(self):
        """ Restore consent files to their previous values. """
        for f, _, _ in self._consent_files:
            self.client.run('rm -f "%s"' % f)
            if self._exists_on_client('%s.autotest_backup' % f):
                self.client.run('mv "%s.autotest_backup" "%s"' % (f, f))

    def _wait_for_restart_and_check(self, boot_id, trigger, text, cpu=0,
                                    timeout=10):
        """
        Wait for panic reboot to complete and check @text in kcrash file.

        @param bootid: Boot ID of the current boot.
        @param trigger: Text string that specifies what caused the panic/reboot.
        @param text: Text string to match in the kcrash file.
        @param cpu: CPU on which the trigger happened.
        @param timeout: Time to wait for the remote host to go down.

        @raises error.TestFail if the @text string is not found in kcrash file.
        """
        try:
            self.client.wait_for_restart(
                down_timeout=timeout,
                down_warning=timeout,
                old_boot_id=boot_id,
                # Extend the default reboot timeout as some targets take
                # longer than normal before ssh is available again.
                timeout=self.client.DEFAULT_REBOOT_TIMEOUT * 4)
        except error.AutoservShutdownError:
            self.client.run('ps alx')
            raise

        # give the crash_reporter some time to log the crash
        time.sleep(5)

        # check if dir /var/spool/crash exists on client or not
        if not self._exists_on_client(self._crash_log_dir):
            raise error.TestFail(
                '%s does not exists on client' % self._crash_log_dir)

        # check if kernel.*.kcrash files are on the client or not
        kcrash_file_path = '%s/kernel.*.kcrash' % self._crash_log_dir
        if not self.client.list_files_glob(kcrash_file_path):
            raise error.TestFail('No kcrash files found on client')

        result = self.client.run('cat %s/kernel.*.kcrash' %
                                 self._crash_log_dir)
        if text not in result.stdout:
            raise error.TestFail(
                "No '%s' in the log after sending '%s' on cpu %d" %
                (text, trigger, cpu))

    def _client_run_output(self, cmd):
        return self.client.run(cmd).stdout.strip()

    def _get_pid(self, comm, parent):
        """
        Fetch PID of process named comm.

        This function tries to lookup the PID for process named @comm. If
        @parent is not None, the parent process is first looked up and then the
        PID of child process matching @comm is returned. Since this method is
        typically called when processes are getting killed/re-spawned, lets
        try looking up the PID up to 10 times if there were errors.

        @param comm: Name of the process whose PID needs to be fetched.
        @param parent: Name of @comm's parent process. This parameter can be
                       None.

        @returns PID of matching process.

        @raises error.TestFail exception if PID for @comm is not found.
        """
        for _ in range(10):
            try:
                if parent:
                    ppid = self._client_run_output('ps -C %s -o pid=' % parent)
                    pid = self._client_run_output('ps --ppid %s -o pid=' % ppid)
                    new_comm = self._client_run_output('ps -p %s -o comm=' %
                                                       pid)
                    if comm != new_comm:
                        logging.info("comm mismatch: %s != %s", comm, new_comm)
                        time.sleep(1)
                        continue
                else:
                    pid = self._client_run_output('ps -C %s -o pid=' % comm)
                return pid
            except error.AutoservRunError as e:
                logging.debug("AutotestRunError is: %s", e)
                time.sleep(1)
        raise error.TestFail("Unable to get pid. comm = %s, parent = %s"
                             % (comm, parent))

    def _trigger_sysrq_x(self):
        self._run_client_command('echo x > /proc/sysrq-trigger')

    def _test_sysrq_x(self):
        """
        Test sysrq-x.

        To help debug system hangs, we ask users to invoke alt-volume_up-x
        key combination. The kernel sysrq-x handler is what handles the
        alt-volume_up-x key combination. The sysrq-x handler in the kernel
        does the following for successive sysrq-x invocations within a 20
        second interval:
        1. Abort the chrome process whose parent is the session_manager process.
        2. Abort the X process. On Freon enabled systems, X is no longer present
           so this step is a no-op.
        3. Panic the kernel.
        This function tests the above steps.
        """
        for process, parent in [('chrome', 'session_manager'),
                                ('X', None)]:
            if process is 'X':
                # With Freon there is no longer an X process. Lets send the
                # sysrq_x and then continue on.
                self._trigger_sysrq_x()
                continue
            orig_pid = self._get_pid(process, parent)
            self._trigger_sysrq_x()
            for _ in range(10):
                new_pid = self._get_pid(process, parent)
                logging.info("%s's original pid was %s and new pid is %s",
                              process, orig_pid, new_pid)
                if new_pid != orig_pid:
                    break
                time.sleep(1)
            else:
                raise error.TestFail('%s did not restart on sysrq-x' % process)

        boot_id = self.client.get_boot_id()
        trigger = 'sysrq-x'
        text = 'sysrq_handle_cros_xkey'
        self._trigger_sysrq_x()
        self._wait_for_restart_and_check(boot_id, trigger, text)

    def _test_panic_path(self, lkdtm, kcrash_tuple):
        """
        Test the kernel panic paths.
        """

        # Figure out which kernel crash interface is available.
        interface = "/sys/kernel/debug/provoke-crash/DIRECT"
        trigger = lkdtm
        breakme, timeout, all_cpu, text = kcrash_tuple
        if not self._exists_on_client(interface):
            interface = "/proc/breakme"
            trigger = breakme
            logging.info("Falling back to %s", interface)

        # Find out how many cpus we have
        client_no_cpus = int(
            self.client.run('cat /proc/cpuinfo | grep processor | wc -l')
                            .stdout.strip())
        no_cpus = 1

        # Skip any triggers that are undefined for the given interface.
        if trigger == None:
            logging.info("Skipping unavailable trigger %s", lkdtm)
            return
        if lkdtm == "HARDLOCKUP":
            # ARM systems do not (presently) have NMI, so skip them for now.
            arch = self.client.get_arch()
            if arch.startswith('arm'):
                logging.info("Skipping %s on architecture %s.",
                             trigger, arch)
                return
            # Make sure a soft lockup detection doesn't get in the way.
            self.client.run("sysctl -w kernel.softlockup_panic=0")

        if trigger == "SPINLOCKUP":
            # This needs to be pre-triggered so the second one locks.
            self._provoke_crash(interface, trigger, None)

        if not all_cpu:
            no_cpus = 1
        else:
            no_cpus = client_no_cpus
        for cpu in range(no_cpus):
            # Always run on at least one cpu
            # Delete crash results, if any
            self.client.run('rm -f %s/*' % self._crash_log_dir)
            boot_id = self.client.get_boot_id()
            # This should cause target reset.
            # Run on a specific cpu if we're running on all of them,
            # otherwise run normally
            if all_cpu :
                self._provoke_crash(interface, trigger, cpu)
            else:
                self._provoke_crash(interface, trigger, None)
            self._wait_for_restart_and_check(boot_id, trigger, text,
                                             cpu=cpu, timeout=timeout)

    def run_once(self, kcrashes, host=None):
        self.client = host
        self._enable_consent()
        self._crash_log_dir = CrashTestDefs._SYSTEM_CRASH_DIR

        # kcrash data is given by a dictionary with key lkdtm string to write
        # to /sys/kernel/debug/provoke-crash/DIRECT on the target. The dict
        # value is a tupple conraining 1) the string to write to /proc/breakme.
        # if lkdtm is not available, 2) the timeout, and 3)whether we run
        # the tests on all CPUs or not. Some tests take less to run than other
        # (null pointer and panic) so it would be best if we would run them on
        # all the CPUS as it wouldn't add that much time to the total.
        # The final component is the crash report string to look for in the
        # crash dump after target restarts.
        kcrash_types = {
            'LOOP' : ('softlockup', 25, False, 'BUG: soft lockup'),
            'BUG' : ('bug', 10, False, 'kernel BUG at'),
            'HUNG_TASK' : ('hungtask', 300, False, 'hung_task: blocked tasks'),
            'SOFTLOCKUP' : (None, 25, False, 'BUG: soft lockup'),
            'HARDLOCKUP' : ('nmiwatchdog', 50, False,
                            'Watchdog detected hard LOCKUP'),
            'SPINLOCKUP' : (None, 25, False, 'softlockup: hung tasks'),
            'EXCEPTION' : ('nullptr',     10, True,
             # x86 gives "BUG: unable to" while ARM gives "Unableto".
                           'nable to handle kernel NULL pointer '
                           'dereference at'),
            'PANIC' : ('panic', 10, True, 'Kernel panic - not syncing:'),
            'CORRUPT_STACK' : (None, 10, True,
                               'stack-protector: Kernel stack is '
                               'corrupted in:')
            }

        bad_kcrashes = []

        #Expected input is comma-delimited kcrashes string
        kcrash_list = kcrashes.split(',')
        if 'SYSRQ_X' in kcrash_list or 'ALL' in kcrash_list:
            self._test_sysrq_x()
            if 'SYSRQ_X' in kcrash_list:
                kcrash_list.remove('SYSRQ_X')
            if 'ALL' in kcrash_list:
                kcrash_list = kcrash_types.keys()
        for kcrash in kcrash_list:
            if kcrash_types.get(kcrash) == None:
                bad_kcrashes.append(kcrash)
                continue
            self._test_panic_path(kcrash,kcrash_types[kcrash])

        if len(bad_kcrashes) > 0:
            raise error.TestFail("Wrong kcrash type "
                                 "requested (%s)" % str(bad_kcrashes))

    def cleanup(self):
        self._restore_consent_files()
        test.test.cleanup(self)
