# Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import datetime
from autotest_lib.client.bin import test
from autotest_lib.client.common_lib import error, smogcheck_tpm, smogcheck_util


class hardware_TPMtspi(test.test):
    version = 1

    def setup(self):
        smogcheck_util.enableI2C()

    def _prepareTpmController(self):
        """Prepare a TpmController instance for use.

        Returns:
          an operational TpmControler instance, ready to use.
        """
        try:
            return smogcheck_tpm.TpmController()
        except smogcheck_tpm.SmogcheckError, e:
            raise error.TestFail('Error creating a TpmController: %s', e)

    def run_once(self):
        self.tpm_obj = self._prepareTpmController()

        start_time = datetime.datetime.now()
        try:
            self.tpm_obj.setupContext()
            self.tpm_obj.getTpmVersion()
            self.tpm_obj.runTpmSelfTest()

            # TODO(tgao): uncomment to enable.
            #self.tpm_obj.takeTpmOwnership()

            # TODO(tgao): uncomment to enable.
            #self.tpm_obj.clearTpm()

            # TODO(tgao): uncomment to enable.
            #self.tpm_obj.setTpmActive('status')

            # TODO(tgao): uncomment to enable.
            #self.tpm_obj.setTpmActive('deactivate')

            # TODO(tgao): uncomment to enable.
            #self.tpm_obj.setTpmActive('activate')

            # TODO(tgao): uncomment to enable.
            #self.tpm_obj.setTpmActive('temp')

            # TODO(tgao): uncomment to enable.
            #self.tpm_obj.setTpmClearable('status')

            # TODO(tgao): uncomment to enable.
            #self.tpm_obj.setTpmClearable('owner')

            # TODO(tgao): uncomment to enable.
            #self.tpm_obj.setTpmClearable('force')

        except smogcheck_tpm.SmogcheckError, e:
            raise error.TestFail('Error: %r' % e)
        finally:
            # Close TPM context
            if self.tpm_obj.closeContext():
                raise error.TestFail('Error closing tspi context')

        end_time = datetime.datetime.now()
        smogcheck_util.computeTimeElapsed(end_time, start_time)
