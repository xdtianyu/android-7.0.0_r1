# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging, os, re

from autotest_lib.client.bin import test
from autotest_lib.client.bin import utils as bin_utils
from autotest_lib.client.common_lib import error, utils

class platform_AesThroughput(test.test):
    version = 1


    def setup(self):
        self.results = {'bytes_per_sec_ideal_min' : 20 * 1024 * 1024}


    def run_once(self):
        num_cpus = bin_utils.count_cpus()
        logging.debug('Running using all cpus: %d' % num_cpus)
        results = self.openssl_speed('aes-256-cbc', '-multi %d' % num_cpus)
        parsed = self.parse_results(results)
        self.update_stats(parsed)
        self.export_stats()


    def openssl_speed(self, cipher, options=''):
        cmd = 'openssl speed %s -mr %s' % (cipher, options)
        return utils.system_output(cmd, retain_output=True)


    def parse_results(self, results, name=''):
        # Split the results into lines.
        # We really only want the final line for our purposes.
        type, times = results.split("\n")[-1].split(' ')
        # +F:num:aes-256 cbc -> aes_256_cbc
        type = re.sub('[- ]', '_', type.split(':')[-1])
        # cbc:time:time:time:... -> time, time, ...
        times = times.split(':')[1:]

        # Build the key names
        if len(name) > 0:
          name = name + '_'
        key_prefix = 'bytes_per_sec_' + name + type + '_blocksz_'
        keys = ['16_bytes', '64_bytes', '256_bytes', '1024_bytes', '8192_bytes']
        keys = [key_prefix+k for k in keys]

        if len(times) > len(keys):
            logging.debug(results)
            raise error.TestFail('openssl output format parsing failed')
        return dict(zip(keys, times))


    def update_stats(self, keyvals):
        self.results.update(keyvals)


    def export_stats(self):
        self.write_perf_keyval(self.results)
