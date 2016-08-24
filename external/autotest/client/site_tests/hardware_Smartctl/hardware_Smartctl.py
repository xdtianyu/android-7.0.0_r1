# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging, re
from autotest_lib.client.bin import test
from autotest_lib.client.bin import site_utils, utils
from autotest_lib.client.common_lib import error


class hardware_Smartctl(test.test):
    """
    Run smartctl to retrieve S.M.A.R.T attribute and report in keyval format.
    """

    version = 1

    _SMARTCTL_DEVICE_MODEL_PATTERN = 'Device Model: *(?P<model>[^ ].*)$'
    _SMARTCTL_RESULT_PATTERN = '.*[P-][O-][S-][R-][C-][K-].*'

    # Temporary table: This value should be in smartctl in March 2014
    # http://sourceforge.net/apps/trac/smartmontools/ticket/272
    _SMARTCTL_LOOKUP_TABLE = {
            'SanDisk SSD i100': {
                    171 : 'Program_Fail_Count',
                    172 : 'Erase_Fail_Count',
                    173 : 'Average_Write_Erase_Count',
                    174 : 'Unexpected_Power_Loss_Count',
                    230 : 'Percent_Write_Erase_Count',
                    234 : 'Percent_Write_Erase_Count_BC'
            }
    }

    def run_once(self, iteration=1, dev=''):
        """
        Read S.M.A.R.T attribute from target device

        @param dev:    target device
        """
        if dev == '':
            logging.info('Run rootdev to determine boot device')
            dev = site_utils.get_root_device()

        logging.info(str('dev: %s' % dev))

        # Skip this test if dev is an eMMC device without raising an error
        if re.match('.*mmc.*', dev):
            logging.info('Target device is an eMMC device. Skip testing')
            self.write_perf_keyval({'device_model' : 'eMMC'})
            return

        last_result = ''


        # run multiple time to test the firmware part that retrieve SMART value
        for loop in range(1, iteration + 1):
            cmd = 'smartctl -a -f brief %s' % dev
            result = utils.run(cmd, ignore_status=True)
            exit_status = result.exit_status
            result_text = result.stdout
            result_lines = result_text.split('\n')

            # log all line if line count is different
            # otherwise log only changed line
            if result_text != last_result:
                logging.info(str('Iteration #%d' % loop))
                last_result_lines = last_result.split('\n')
                if len(last_result_lines) != len(result_lines):
                    for line in result_lines:
                        logging.info(line)
                else:
                    for i, line in enumerate(result_lines):
                        if line != last_result_lines[i]:
                            logging.info(line)
                last_result = result_text

            # Ignore error other than first two bits
            if exit_status & 0x3:
                # Error message should be in 4th line of the output
                msg = 'Test failed with error: %s' % result_lines[3]
                raise error.TestFail(msg)

        logging.info(str('smartctl exit status: 0x%x' % exit_status))

        # find drive model
        lookup_table = {}
        pattern = re.compile(self._SMARTCTL_DEVICE_MODEL_PATTERN)
        for line in result_lines:
            if pattern.match(line):
                model = pattern.match(line).group('model')
                for known_model in self._SMARTCTL_LOOKUP_TABLE:
                    if model.startswith(known_model):
                        lookup_table = self._SMARTCTL_LOOKUP_TABLE[known_model]
                        break
                break
        else:
            raise error.TestFail('Can not find drive model')

        # Example of smart ctl result
        # ID# ATTRIBUTE_NAME          FLAGS    VALUE WORST THRESH FAIL RAW_VALUE
        #  12 Power_Cycle_Count       -O----   100   100   000    -    204
        # use flag field to find a valid line
        pattern = re.compile(self._SMARTCTL_RESULT_PATTERN)
        keyval = {}
        fail = []
        for line in result_lines:
            if not pattern.match(line):
                continue
            field = line.split()

            id = int(field[0])
            if id in lookup_table:
                # look up table overwrite smartctl name
                key = lookup_table[id]
            else:
                key = field[1] # ATTRIBUTE_NAME
                if key == 'Unknown_Attribute':
                    key = "Smart_Attribute_ID_%d" % id

            keyval[key] = field[7] # RAW_VALUE

            # check for failing attribute
            if field[6] != '-':
                fail += [key]

        if len(keyval) == 0:
            raise error.TestFail(
                    'Test failed with error: Can not parse smartctl keyval')

        if len(fail) > 0:
            keyval['fail'] = fail

        keyval['exit_status'] = exit_status
        keyval['device_model'] = model
        self.write_perf_keyval(keyval)

