# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging, os, shutil, tempfile
from autotest_lib.client.bin import fio_util, test, utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.cros import cryptohome

TEST_USER = 'test@chromium.org'
TEST_PASSWORD = 'test'

class platform_CryptohomeFio(test.test):
    """Run FIO in the crypto partition."""

    version = 2

    USE_CRYPTO = 'crypto'
    USE_PLAIN = 'plain'
    USE_TMPFS = 'tmpfs'
    DISK_CONFIG_KEYS = [ USE_CRYPTO, USE_PLAIN, USE_TMPFS ]

    def initialize(self, from_internal_disk_only=True):
        """ Check that we are running on the fixed device"""
        if from_internal_disk_only and not utils.is_booted_from_internal_disk():
            raise error.TestNAError('Test only on internal disk')

    def run_once(self, runtime, disk_configs,
                 script=None, sysctls_list=None):
        """
        Create a 300MB file in tmpfs/encrypted/unencrypted location
        and run fio tesst.

        @param disk_configs: list of keys from DISK_CONFIG_KEYS.
        @param script: fio script to run
        @param sysctls_list: list of dictionary of sysctls to alter.
        """
        if not set(disk_configs).issubset(set(self.DISK_CONFIG_KEYS)):
            raise error.TestFail('Unknown keys in disk config')
        for config in disk_configs:
            for sysctls in sysctls_list or [ {} ]:

                graph_descr = ''
                for key, val in sysctls.iteritems():
                    utils.sysctl(key, val)
                    graph_descr += '-'.join([os.path.basename(key), str(val)])
                # Mount a test cryptohome vault.
                if config == self.USE_CRYPTO:
                    cryptohome.mount_vault(TEST_USER, TEST_PASSWORD,
                                           create=True)
                    tmpdir = cryptohome.user_path(TEST_USER)
                elif config == self.USE_TMPFS:
                    tmpdir = None
                else:
                    tmpdir = self.tmpdir
                self.__work_dir = tempfile.mkdtemp(dir=tmpdir)

                results = {}
                # TODO make these parameters to run_once & check disk for space.
                self.__filesize = '300m'
                self.__runtime = str(runtime)
                env_vars = ' '.join(
                    ['FILENAME=' + os.path.join(self.__work_dir, script),
                     'FILESIZE=' + self.__filesize,
                     'RUN_TIME=' + self.__runtime
                     ])
                job_file = os.path.join(self.bindir, script)
                results.update(fio_util.fio_runner(self, job_file, env_vars,
                    name_prefix=graph_descr + config))
                self.write_perf_keyval(results)


                logging.info('Finished with FS stress, cleaning up.')
                if config == self.USE_CRYPTO:
                    cryptohome.unmount_vault(TEST_USER)
                    cryptohome.remove_vault(TEST_USER)
                else:
                    shutil.rmtree(self.__work_dir)
