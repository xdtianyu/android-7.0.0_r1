# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging, os, re
from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.cros import crash_test


class logging_KernelCrash(crash_test.CrashTest):
    """
    Validates the contents of a kernel crash report.
    """
    version = 1

    def _test_reporter_startup(self):
        """Test that the crash_reporter is handling kernel crashes."""
        if not self._log_reader.can_find('Enabling kernel crash handling'):
            if not self._log_reader.can_find(
                'Kernel does not support crash dumping'):
                raise error.TestFail(
                    'Could not find kernel crash enabling message')


    def _get_kcrash_name(self):
        filename_match = re.search(r'Stored kcrash to (\S+)',
            self._log_reader.get_logs())
        if not filename_match:
            return None
        return filename_match.group(1)


    def _is_signature_match(self, signature):
        # Update these as kernels evolve:
        matches = r'write_breakme'    # for 2.6.38 kernels and 3.0.13 x86
        matches += r'|breakme_do_bug' # for 3.2 kernels
        matches += r'|__bug'          # for 3.0.13 ARM
        matches += r'|lkdtm_do_action'# for 3.8.11 with lkdtm
        regex = r'kernel-(' + matches + r')-[0-9A-F]{8}$'
        return (re.match(regex, signature) is not None)

    def _is_handled_reason(self, reason):
        return (re.match(r'(handling|developer build - always dumping)$',
                         reason) is not None)

    def _test_reporter_kcrash_storage(self):
        """Test that crash_reporter has properly stored the kcrash report."""
        announce_match = re.search(
            r'Received .* from kernel \(signature ([^\)]+)\) \(([^\)]+)\)',
            self._log_reader.get_logs())

        if not announce_match:
            raise error.TestFail('Could not find kernel crash announcement')

        logging.info('Signature: [%s]', announce_match.group(1))
        logging.info('Reason: [%s]', announce_match.group(2))

        if not self._is_signature_match(announce_match.group(1)):
            raise error.TestFail(
                'Kernel crash signature (%s) did not match expected pattern' %
                announce_match.group(1))

        kcrash_report = self._get_kcrash_name()

        if self._consent:
            if kcrash_report is None:
                raise error.TestFail(
                    'Could not find message with kcrash filename')
            if not self._is_handled_reason(announce_match.group(2)):
                raise error.TestFail('Did not announce handling of kcrash ' \
                                     '(%s)' % (announce_match.group(2)))
        else:
            if kcrash_report is not None:
                raise error.TestFail('Should not have found kcrash filename')
            if announce_match.group(2) != 'ignoring - no consent':
                raise error.TestFail('Did not announce ignoring of kcrash ' \
                                     '(%s)' % (announce_match.group(2)))
            return

        if not os.path.exists(kcrash_report):
            raise error.TestFail('Crash report %s gone' % kcrash_report)
        report_contents = utils.read_file(kcrash_report)
        src_re = r'kernel BUG at .*(fs/proc/breakme.c|drivers/misc/lkdtm.c)'
        if re.search(src_re, report_contents) == None:
            raise error.TestFail('Crash report has unexpected contents')


    def _test_sender_send_kcrash(self):
        """Test that crash_sender properly sends the crash report."""
        if not self._consent:
            return
        kcrash_report = self._get_kcrash_name()
        if not os.path.exists(kcrash_report):
            raise error.TestFail('Crash report %s gone' % kcrash_report)
        result = self._call_sender_one_crash(
            report=os.path.basename(kcrash_report))
        if (not result['send_attempt'] or not result['send_success'] or
            result['report_exists']):
            raise error.TestFail('kcrash not sent properly')
        if result['exec_name'] != 'kernel' or result['report_kind'] != 'kcrash':
            raise error.TestFail('kcrash exec name or report kind wrong ' \
                                 '(exec_name: [%s] report_kind: [%s]' %
                                 (result['exec_name'], result['report_kind']))
        if result['report_payload'] != kcrash_report:
            raise error.TestFail('Sent the wrong kcrash report')
        if not self._is_signature_match(result['sig']):
            raise error.TestFail('Sent the wrong kcrash signature')


    def run_once(self, is_before, consent):
        self._log_reader.set_start_by_reboot(-1)
        # We manage consent saving across tests.
        self._automatic_consent_saving = False
        self._consent = consent
        if is_before:
            self.run_crash_tests(['reporter_startup'], must_run_all=False)
            # Leave crash sending paused for the kernel crash.
            self._leave_crash_sending = False
        else:
            self.run_crash_tests(['reporter_startup',
                                  'reporter_kcrash_storage',
                                  'sender_send_kcrash'],
                                 clear_spool_first=False)
