# Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import os
import struct
from autotest_lib.client.common_lib import error, utils
from autotest_lib.client.cros import verity_utils

class platform_DMVerityBitCorruption(verity_utils.VerityImageTest):
    version = 1

    _adjustment = 0
    _mask = 0x80

    def mod_tweak_block(self, run_count, backing_path, block_size,
                            block_count):
        logging.info('mod_tweak_block(%d, %s, %d, %d)' % (
                     run_count, backing_path, block_size, block_count))
        run_count = run_count % block_count
        with open(backing_path, 'r+b') as dev:
            dev.seek(run_count * block_size + self._adjustment)
            (byte,) = struct.unpack('B', dev.read(1))  # Get raw byte value.
            dev.seek(run_count * block_size + self._adjustment)
            dev.write(struct.pack('B', byte ^ self._mask))


    def mod_tweak_hash_block(self, run_count, backing_path, block_size,
                             block_count):
        logging.info('mod_tweak_hash_block(%d, %s, %d, %d)' % (
                     run_count, backing_path, block_size, block_count))
        with open(backing_path, 'r+b') as dev:
            # move to the start of the appropriate hash block
            dev.seek(block_count * block_size, os.SEEK_SET)
            dev.seek(run_count * block_size + self._adjustment, os.SEEK_CUR)
            (byte,) = struct.unpack('B', dev.read(1))
            # return to the start of the appropriate hash block
            dev.seek(block_count * block_size, os.SEEK_SET)
            dev.seek(run_count * block_size + self._adjustment, os.SEEK_CUR)
            dev.write(struct.pack('B', byte ^ self._mask))


    def run_once(self, bit_loc='first'):
        if bit_loc == 'first':
            pass
        elif bit_loc == 'last':
            self._adjustment = verity_utils.BLOCK_SIZE - 1
            self._mask = 0x01
        elif bit_loc == 'middle':
            self._adjustment = verity_utils.BLOCK_SIZE/2
        else:
            raise error.TestError('bit_loc must be first, last, or middle')

        # Corrupt the |bit_loc| bit of each block (on a per-block basis).
        self.mod_and_test(self.mod_tweak_block, self.image_blocks, False)

        # Repeat except on each block in the hash tree data.
        hash_blocks = (os.path.getsize(self.verity.hash_file) /
                       verity_utils.BLOCK_SIZE)
        self.mod_and_test(self.mod_tweak_hash_block, hash_blocks, False)
