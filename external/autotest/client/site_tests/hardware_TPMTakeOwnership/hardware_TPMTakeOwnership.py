# Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import time

from autotest_lib.client.bin import test
from autotest_lib.client.common_lib import error, smogcheck_tpm, \
    smogcheck_ttci, smogcheck_util
from autotest_lib.client.cros import service_stopper


class hardware_TPMTakeOwnership(test.test):
    version = 1


    def initialize(self):
        smogcheck_util.enableI2C()
        self.ttci_obj = None
        self.tpm_obj = None
        self.attr_dict = dict()  # Attributes to output
        self.perf_dict = dict()  # Performance measures to output
        self._services = service_stopper.ServiceStopper(['cryptohomed',
                                                         'chapsd', 'tcsd'])
        self._services.stop_services()


    def _prepareTpmController(self):
        """Prepare a TpmController instance for use.

        Returns:
          an operational TpmControler instance, ready to use.

        Raises:
          TestFail: if error creating a new TpmController instance.
        """
        try:
            self.tpm_obj = smogcheck_tpm.TpmController()
        except smogcheck_tpm.SmogcheckError as e:
            raise error.TestFail('Error creating a TpmController: %s', e)


    def _prepareTtciController(self):
        """Prepare TtciController instances for use.

        Returns:
          an operational TtciController instance, ready to use.

        Raises:
          TestFail: if error creating a new TtciController instance.
        """
        try:
            self.ttci_obj = smogcheck_ttci.TtciController()
        except smogcheck_ttci.TtciError as e:
            raise error.TestFail('Error creating a TtciController: %s' % e)


    def _sleep(self, amount):
        """Sleeps for 'amount' of time and logs a message.

        Args:
          amount: an integer or float in seconds.
        """
        time.sleep(amount)
        if amount >= 1:
            logging.debug('Slept for %0.2f second', amount)
        elif amount >= 0.001:
            logging.debug('Slept for %0.2f millisecond', (amount * 1000))
        else:
            logging.debug('Slept for %0.2f microsecond', (amount * 1000000))


    def run_once(self, loop=-1, max_acceptable_delay=-1):
        self._prepareTtciController()
        self._prepareTpmController()

        timestamps = dict()
        time_list = []
        try:
            # Verify TPM is operational before triggering hardware Reset
            self.tpm_obj.runTpmSelfTest()

            # Activate hardware Reset signal
            if self.ttci_obj.TTCI_Set_Reset_Control(turn_on=True):
                raise error.TestFail('TTCI_Set_Reset_Control() error: %s' %
                                     self.ttci_obj.err)
            logging.info('TPM hardware Reset signal activated')

            # Wait for 100 milisec
            self._sleep(0.1)

            # Deactivate hardware Reset signal
            if self.ttci_obj.TTCI_Set_Reset_Control(turn_on=False):
                raise error.TestFail('TTCI_Set_Reset_Control() error: %s' %
                                     self.ttci_obj.err)
            logging.info('TPM hardware Reset signal DEactivated')

            # Run TPM_Starup
            smogcheck_util.runInSubprocess(['tpmc', 'startup'])

            # Run TPM_SelfTestFull
            smogcheck_util.runInSubprocess(['tpmc', 'test'])

            # Run TPM_AssertPhysicalPresence
            smogcheck_util.runInSubprocess(['tpmc', 'ppon'])

            # Run TPM_OwnerClear
            smogcheck_util.runInSubprocess(['tpmc', 'clear'])

            for i in range(loop):
                smogcheck_util.runInSubprocess(['start', 'tcsd'])
                # Wait 3 sec for tcsd to start
                self._sleep(3)

                # Run TPM_TakeOwnership and record elapsed time
                timestamps[i] = self.tpm_obj.takeTpmOwnership()

                smogcheck_util.runInSubprocess(['stop', 'tcsd'])
                # Wait for 1 sec for tcsd to stop
                self._sleep(1)

                # Run TPM_OwnerClear
                smogcheck_util.runInSubprocess(['tpmc', 'clear'])

            # Output timing measurements
            for k, v in timestamps.iteritems():
                sec, ms = divmod(v/1000, 1000)
                key = 'iteration_%d_delay_in_sec' % k
                delay_float = float(v)/1000000
                self.perf_dict[key] = delay_float
                time_list.append(delay_float)
            self.perf_dict['num_total_iterations'] = len(timestamps)
            # TODO(tgao): modify generate_test_report to support attr_dict
            #self.attr_dict['timing_measurement_for'] = 'TPM_TakeOwnership'
            time_list.sort()
            time_list.reverse()
            count = 0
            for i in time_list:
                if i <= max_acceptable_delay:
                    break
                logging.debug('Actual value (%0.2f) exceeds max (%0.2f)',
                              i, max_acceptable_delay)
                count += 1
            self.perf_dict['num_iterations_exceeding_max_delay'] = count
            self.perf_dict['max_acceptable_delay_in_sec'] = max_acceptable_delay
            self.perf_dict['min_delay_in_sec_actual'] = time_list[-1]
            # Set this attribute last. If it exceeds user-specified limit in
            # test suite control file, output report would still be complete
            self.perf_dict['max_delay_in_sec_actual'] = time_list[0]

        except smogcheck_tpm.SmogcheckError as e:
            raise error.TestFail('Error: %r' % e)
        finally:
            # Output attibutes and performance keyval pairs
            self.write_iteration_keyval(self.attr_dict, self.perf_dict)

            # Close TPM context
            if self.tpm_obj.closeContext():
                raise error.TestFail('Error closing tspi context')


    def cleanup(self):
        self._services.restore_services()
