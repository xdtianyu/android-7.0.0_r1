# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging, re
from autotest_lib.client.bin import test, utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.cros import service_stopper


# Expected results of 'tpmc getX' commands.
TPMC_EXPECTED = {
    'getvf': # volatile (ST_CLEAR) flags
     set([('deactivated', '0'), ('physicalPresence', '0'),
          ('physicalPresenceLock', '1'), ('bGlobalLock', '1')]),
    'getpf': # permanent flags
     set([('disable', '0'), ('ownership', '1'), ('deactivated', '0'),
          ('physicalPresenceHWEnable', '0'), ('physicalPresenceCMDEnable', '1'),
          ('physicalPresenceLifetimeLock', '1'), ('nvLocked', '1')])}


def missing_firmware_version():
    """Check for empty fwid.

    @return True if no fwid else False.
    """
    cmd = 'crossystem fwid'
    return not utils.system_output(cmd, ignore_status=True).strip()


def __run_tpmc_cmd(subcommand):
    """Make this test more readable by simplifying commonly used tpmc command.

    @param subcommand: String of the tpmc subcommand (getvf, getpf, getp, ...)
    @return String output (which may be empty).
    """
    cmd = 'tpmc %s' % subcommand
    return utils.system_output(cmd, ignore_status=True).strip()


def check_tpmc(subcommand, expected):
    """Runs tpmc command and checks the output against an expected result.

    The expected results take 2 different forms:
    1. A regular expression that is matched.
    2. A set of tuples that are matched.

    @param subcommand: String of the tpmc subcommand (getvf, getpf, getp, ...)
    @param expected: Either a String re or the set of expected tuples.
    @raises error.TestError() for invalidly matching expected.
    """
    error_msg = 'invalid response to tpmc %s' % subcommand
    if isinstance(expected, str):
        out = __run_tpmc_cmd(subcommand)
        if (not re.match(expected, out)):
            raise error.TestError('%s: %s' % (error_msg, out))
    else:
        result_set = utils.set_from_keyval_output(__run_tpmc_cmd(subcommand))
        if set(expected) <= result_set:
            return
        raise error.TestError('%s: expected=%s.' %
                              (error_msg, sorted(set(expected) - result_set)))


class hardware_TPMCheck(test.test):
    """Check that the state of the TPM is as expected."""
    version = 1


    def initialize(self):
        # Must stop the TCSD process to be able to collect TPM status,
        # then restart TCSD process to leave system in a known good state.
        # Must also stop services which depend on tcsd.
        self._services = service_stopper.ServiceStopper(['cryptohomed',
                                                         'chapsd', 'tcsd'])
        self._services.stop_services()


    def run_once(self):
        """Run a few TPM state checks."""
        if missing_firmware_version():
            logging.warning('no firmware version, skipping test')
            return

        # Check volatile and permanent flags
        for subcommand in ['getvf', 'getpf']:
            check_tpmc(subcommand, TPMC_EXPECTED[subcommand])

        # Check space permissions
        check_tpmc('getp 0x1007', '.*0x8001')
        check_tpmc('getp 0x1008', '.*0x1')

        # Check kernel space UID
        check_tpmc('read 0x1008 0x5', '.* 4c 57 52 47$')


    def cleanup(self):
        self._services.restore_services()
