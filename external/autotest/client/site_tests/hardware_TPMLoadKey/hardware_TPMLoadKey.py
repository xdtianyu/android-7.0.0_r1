# Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
from autotest_lib.client.bin import test, utils
from autotest_lib.client.common_lib import error

class hardware_TPMLoadKey(test.test):

    # This test determines the TPM can execute a TPM LoadKey function
    #
    # The test sequence follows the steps below:
    #   1. Create a local file containing a text file to be "sealed"
    #   2. Execute the TPM Tools command for tpm_sealdata
    #   3. Parse response of tpm_sealdata for LoadKey function execution
    #   4. Test passes if LoadKey reports success.
    #

    version = 1

    # Runs a command, logs the output, and returns the exit status.
    def __run_cmd(self, cmd):
        result = utils.system_output(cmd, retain_output=True,
                                     ignore_status=False)
        logging.info(result)
        return result

    def run_once(self):
        # Create the test input file
        create_input_file_cmd = "echo 'This is a test' > in.dat"
        output = self.__run_cmd(create_input_file_cmd)

        # Execute the TPM Tools command for sealdata, causing a LoadKey
        #    event, and filter for a debug message indicating success
        seal_data_cmd = \
            "tpm_sealdata -i in.dat -o out.dat -z -l debug 2>&1 | grep LoadKey"
        output = self.__run_cmd(seal_data_cmd)

        # Confirm the LoadKey message reports success
        if (output.find("success") < 0):
            raise error.TestError("LoadKey execution did not succeed")
