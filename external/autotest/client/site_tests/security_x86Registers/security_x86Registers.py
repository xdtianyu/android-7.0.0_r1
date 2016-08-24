# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging, time
from autotest_lib.client.bin import test, utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.cros import power_utils
from autotest_lib.client.cros import sys_power

MSR_POSITIVE = {
    'Atom': {
        # VMX does not exist on Atom (so it reports as disabled).
        '0x3a':  [('2:0', 1)],
        },
    'Non-Atom': {
        # IA32_FEATURE_CONTROL[2:0]
        #   0 - Lock bit (1 = locked)
        #   1 - Enable VMX in SMX operation
        #   2 - Enable VMX outside SMX operation
        # Want value "1": VMX locked and disabled in all modes.
        '0x3a':  [('2:0', 1)],
        },
    }

MSR_NEGATIVE = {
    'Atom': {
        # Inverted from positive case: none of these bits should be set.
        '0x3a':  [('2:0', 6)],
        },
    'Non-Atom': {
        # Inverted from positive case: none of these bits should be set.
        '0x3a':  [('2:0', 6)],
        },
    }

RCBA_POSITIVE = {
    'Atom': {
        # GCS.BILD is not set on H2C UEFI Firmware. :(
        # https://code.google.com/p/chromium/issues/detail?id=269633
        '0x3410': [('0', 0)],
        },
    'Non-Atom': {
        # GCS (General Control and Status) register, BILD (BIOS Interface
        # Lock-Down) bit should be set.
        '0x3410': [('0', 1)],
        },
    }

RCBA_NEGATIVE = {
    'Atom': {
        # GCS register, BILD bit inverted from positive test.
        '0x3410': [('0', 1)],
        },
    'Non-Atom': {
        # GCS register, BILD bit inverted from positive test.
        '0x3410': [('0', 0)],
        },
    }

class security_x86Registers(test.test):
    """
    Checks various CPU and firmware registers for security-sensitive safe
    settings.
    """
    version = 1

    def _check_negative_positive(self, name, func, match_neg, match_pos):
        errors = 0

        # Catch missing test conditions.
        if len(match_neg) == 0:
            logging.error('BAD: no inverted %s tests defined!', name)
        if len(match_pos) == 0:
            logging.error('BAD: no positive %s tests defined!', name)

        # Negative tests; make sure infrastructure is working.
        logging.debug("=== BEGIN [expecting %s FAILs] ===", name)
        if func(match_neg) == 0:
            logging.error('BAD: inverted %s tests did not fail!', name)
            errors += 1
        logging.debug("=== END [expecting %s FAILs] ===", name)

        # Positive tests; make sure values are for real.
        logging.debug("=== BEGIN [expecting %s oks] ===", name)
        errors += func(match_pos)
        logging.debug("=== END [expecting %s oks] ===", name)

        logging.debug("%s errors found: %d", name, errors)
        return errors

    def _check_msr(self):
        return self._check_negative_positive('MSR',
                                             self._registers.verify_msr,
                                             MSR_NEGATIVE[self._cpu_type],
                                             MSR_POSITIVE[self._cpu_type])

    def _check_bios(self):
        return self._check_negative_positive('BIOS',
                                             self._registers.verify_rcba,
                                             RCBA_NEGATIVE[self._cpu_type],
                                             RCBA_POSITIVE[self._cpu_type])

    def _check_all(self):
        errors = 0
        errors += self._check_msr()
        errors += self._check_bios()
        return errors

    def run_once(self):
        errors = 0

        cpu_arch = power_utils.get_x86_cpu_arch()
        if not cpu_arch:
            cpu_arch = utils.get_cpu_arch()
            if cpu_arch == "arm":
                logging.debug('ok: skipping x86-only test on %s.', cpu_arch)
                return
            raise error.TestNAError('Unsupported CPU: %s' % (cpu_arch))

        self._cpu_type = 'Atom'
        if cpu_arch is not 'Atom':
            self._cpu_type = 'Non-Atom'

        self._registers = power_utils.Registers()

        # Check running machine.
        errors += self._check_all()

        # Pause briefly to make sure the RTC is ready for suspend/resume.
        time.sleep(3)
        # Suspend the system to RAM and return after 10 seconds.
        sys_power.do_suspend(10)

        # Check resumed machine.
        errors += self._check_all()

        if errors > 0:
            raise error.TestFail("x86 register mismatch detected")
