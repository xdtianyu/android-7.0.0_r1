# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import os

from autotest_lib.client.bin import site_utils, test, utils
from autotest_lib.client.common_lib import error

DEFAULT_MIN_GB = 16
# Allowable amount of bits eMMC vendor can use in firmware to support bad block
# replacement and metadata.
EMMC_VENDOR_ALLOWED_GB = 0.25
# Amount of data available for user data in device, the rest is left for
# over provisioning.
# Typically a SATA device will use 7% over provisioning [the difference
# between GB and GiB], but some eMMC device can use 9%.
# With Flash becoming more error prone as lithography shrinks, the trend
# is to increase over provisioning.
DEFAULT_USER_DENSITY = 0.9


class hardware_DiskSize(test.test):
    """
    Check that disk size is around 16GB at least.
    """

    version = 1

    def _is_emmc(self):
        path = os.path.join("/sys/class/block/", self._device,
                            "device", "type")
        if not os.path.exists(path):
            return False
        return utils.read_one_line(path) == 'MMC'


    @classmethod
    def _gib_to_gb(cls, gib):
        return float(gib) * (1 << 30) / (10 ** 9)

    def _compute_min_gb(self):
        """Computes minimum size allowed primary storage device.

        TODO(tbroch): Add computation of raw bytes in eMMC using 'Chip Specific
        Data' (CSD & EXT_CSD) defined by JEDEC JESD84-A44.pdf if possible.

        CSD :: /sys/class/block/<device>/device/csd
        EXT_CSD :: debugfs

        Algorithm should look something like this:
        CSD[C_SIZE] = 0xfff == eMMC > 2GB
        EXT_CSD[SEC_COUNT] = # of 512byte sectors

        Now for existing eMMC I've examined I do see the C_SIZE == 0xfff.
        Unfortunately the SEC_COUNT appears to have excluded the sectors
        reserved for metadata & repair.  Perhaps thats by design in which case
        there is no mechanism to determine the actual raw sectors.

        For now I use 0.25GB as an acceptable fudge.

        Returns:
            integer, in GB of minimum storage size.
        """

        min_gb = DEFAULT_MIN_GB
        if self._is_emmc():
            min_gb -= EMMC_VENDOR_ALLOWED_GB
        min_gb *= DEFAULT_USER_DENSITY
        return self._gib_to_gb(min_gb)


    def run_once(self):
        root_dev = site_utils.get_root_device()
        self._device = os.path.basename(root_dev)
        disk_size = utils.get_disk_size(root_dev)
        if not disk_size:
            raise error.TestError('Unable to determine main disk size')

        # Capacity of a hard disk is quoted with SI prefixes, incrementing by
        # powers of 1000, instead of powers of 1024.
        gb = float(disk_size) / (10 ** 9)

        self.write_perf_keyval({"gb_main_disk_size": gb})
        min_gb = self._compute_min_gb()
        logging.info("DiskSize: %.3f GB MinDiskSize: %.3f GB", gb, min_gb)
        if (gb < min_gb):
            raise error.TestError("DiskSize %.3f GB below minimum (%.3f GB)" \
                % (gb, min_gb))

