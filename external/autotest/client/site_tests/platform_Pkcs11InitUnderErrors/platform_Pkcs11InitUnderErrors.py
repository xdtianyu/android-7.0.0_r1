# Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging, os, shutil

from autotest_lib.client.bin import test, utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.cros import pkcs11

class Pkcs11InitFailure(error.TestError):
    pass


class platform_Pkcs11InitUnderErrors(test.test):
    version = 1

    def __chaps_init_iteration(self):
        # Try initializing and using the temporary chaps test token.
        pkcs11.load_p11_test_token()
        if not pkcs11.verify_p11_test_token():
            return False
        pkcs11.unload_p11_test_token()
        pkcs11.restore_p11_test_token()
        return True

    def __test_chaps_init(self):
        pkcs11.setup_p11_test_token(True)
        dbpath = pkcs11.get_p11_test_token_db_path()
        # Make sure the test token is functional.
        if not self.__chaps_init_iteration():
            raise error.TestFail('Token verification failed.')
        # Erase the chaps database directory.
        shutil.rmtree(dbpath, ignore_errors=True)
        if not self.__chaps_init_iteration():
            raise error.TestFail('Token verification failed after erasing the '
                                 'database directory.')
        # Corrupt each file in the chaps database directory.
        for f in os.listdir(dbpath):
            utils.system('dd if=/dev/zero of=%s bs=1 count=1000 >/dev/null 2>&1'
                % os.path.join(dbpath, f))
        if not self.__chaps_init_iteration():
            raise error.TestFail('Token verification failed after corrupting '
                                 'the database.')
        pkcs11.cleanup_p11_test_token()

    def run_once(self):
        self.__test_chaps_init()
        return

