# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
# Author: Cosimo Alfarano <cosimo.alfarano@collabora.co.uk>

import datetime
import logging
import os

from autotest_lib.client.cros import storage as storage_mod
from autotest_lib.client.common_lib import autotemp, error
from autotest_lib.client.bin  import base_utils

USECS_IN_SEC = 1000000.0

class hardware_Usb30Throughput(storage_mod.StorageTester):
    version = 1
    preserve_srcdir = True
    _autosrc = None
    _autodst = None
    results = {}


    def cleanup(self):
        if self._autosrc:
            self._autosrc.clean()
        if self._autodst:
            self._autodst.clean()

        self.scanner.unmount_all()

        super(hardware_Usb30Throughput, self).cleanup()


    def run_once(self, measurements=5, size=1, min_speed=300.0):
        """
        @param measurements: (int) the number of measurements to do.
                For the test to fail at least one measurement needs to be
                below |min_speed|
        @param size: (int) size of the file to be copied for testing the
                transfer rate, it represent the size in megabytes.
                Generally speaking, the bigger is the file used for
                |measurements| the slower the test will run and the more
                accurate it will be.
                e.g.: 10 is 10MB, 101 is 101MB
        @param min_speed: (float) in Mbit/sec. It's the min throughput a USB 3.0
                device should perform to be accepted. Conceptually it's the max
                USB 3.0 throughput minus a tollerance.
                Defaults to 300Mbit/sec (ie 350Mbits/sec minus ~15% tollerance)
        """
        volume_filter = {'bus': 'usb'}
        storage = self.wait_for_device(volume_filter, cycles=1,
                                       mount_volume=True)[0]

        # in Megabytes (power of 10, to be consistent with the throughput unit)
        size *= 1000*1000

        self._autosrc = autotemp.tempfile(unique_id='autotest.src',
                                          dir=storage['mountpoint'])
        self._autodst = autotemp.tempfile(unique_id='autotest.dst',
                                          dir=self.tmpdir)

        # Create random file
        storage_mod.create_file(self._autosrc.name, size)

        num_failures = 0
        for measurement in range(measurements):
            xfer_rate = get_xfer_rate(self._autosrc.name, self._autodst.name)
            key = 'Mbit_per_sec_measurement_%d' % measurement
            self.results[key] = xfer_rate
            logging.debug('xfer rate (measurement %d) %.2f (min=%.2f)',
                          measurement, xfer_rate, min_speed)

            if xfer_rate < min_speed:
                num_failures += 1

        # Apparently self.postprocess_iteration is not called on TestFail
        # so we need to process data here in order to have some performance log
        # even on TestFail
        self.results['Mbit_per_sec_average'] = (sum(self.results.values()) /
            len(self.results))
        self.write_perf_keyval(self.results)

        if num_failures > 0:
            msg = ('%d/%d measured transfer rates under performed '
                   '(min_speed=%.2fMbit/sec)' % (num_failures, measurements,
                   min_speed))
            raise error.TestFail(msg)


def get_xfer_rate(src, dst):
    """Compute transfer rate from src to dst as Mbit/sec

    Execute a copy from |src| to |dst| and returns the file copy transfer rate
    in Mbit/sec

    @param src, dst: paths for source and destination

    @return trasfer rate (float) in Mbit/sec
    """
    assert os.path.isfile(src)
    assert os.path.isfile(dst)

    base_utils.drop_caches()
    start = datetime.datetime.now()
    base_utils.force_copy(src, dst)
    end = datetime.datetime.now()
    delta = end - start

    # compute seconds (as float) from microsecs
    delta_secs = delta.seconds + (delta.microseconds/USECS_IN_SEC)
    # compute Mbit from bytes
    size_Mbit = (os.path.getsize(src)*8.0)/(1000*1000)

    logging.info('file trasferred: size (Mbits): %f, start: %f, end: %d,'
                 ' delta (secs): %f',
                 size_Mbit,
                 start.second+start.microsecond/USECS_IN_SEC,
                 end.second+end.microsecond/USECS_IN_SEC,
                 delta_secs)

    # return the xfer rate in Mbits/secs having bytes/microsec
    return size_Mbit / delta_secs
