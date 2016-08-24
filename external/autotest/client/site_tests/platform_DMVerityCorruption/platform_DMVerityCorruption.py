# Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import os
from autotest_lib.client.common_lib import error, utils
from autotest_lib.client.cros import verity_utils

class platform_DMVerityCorruption(verity_utils.VerityImageTest):
    version = 1

    def mod_zerofill_block(self, run_count, backing_path, block_size,
                           block_count):
        logging.info('mod_zerofill_block(%d, %s, %d, %d)' % (
                     run_count, backing_path, block_size, block_count))
        dd_cmd = 'dd if=/dev/zero of=%s bs=%d seek=%d count=1'
        run_count = run_count % block_count
        verity_utils.system(dd_cmd % (backing_path, block_size, run_count))

    def mod_Afill_hash_block(self, run_count, backing_path, block_size,
                             block_count):
        logging.info('mod_Afill_hash_block(%d, %s, %d, %d)' % (
                     run_count, backing_path, block_size, block_count))
        with open(backing_path, 'wb') as dev:
          dev.seek(block_count * block_size, os.SEEK_SET)
          dev.seek(run_count * block_size, os.SEEK_CUR)
          dev.write('A' * block_size)

    def run_once(self):
        # Ensure that basic verification is working.
        # This should NOT fail.
        self.mod_and_test(self.mod_nothing, 1, True)

        # Corrupt the image once per block (on a per-block basis).
        self.mod_and_test(self.mod_zerofill_block, self.image_blocks, False)

        # Repeat except each block in the hash tree data
        hash_blocks = (os.path.getsize(self.verity.hash_file) /
                       verity_utils.BLOCK_SIZE)
        self.mod_and_test(self.mod_Afill_hash_block, hash_blocks, False)
