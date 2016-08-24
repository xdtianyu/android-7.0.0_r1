# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from autotest_lib.client.bin import test, utils
from autotest_lib.client.common_lib import error

import logging, os, re

class network_WiFiTxRx(test.test):
    version = 1

    def run_once(self, interface=None,
                 min_tx=-90, min_rx=-90,
                 min_qual=0.14,
                 max_tx=20,  max_rx=20 ):
        '''Check 802.11 WiFi signal strength

        @param interface: Network interface to test (None to pick the default)
        @param min_tx: Minimum acceptable WiFi Tx value in dBm (integer)
        @param min_rx: Minimum acceptable WiFi Rx value in dBm (integer)
        @param min_qual: Minimum signal quality (float, range 0 - 1)
        @param max_tx: Maximum allowable WiFi Tx value in dBm (integer)
        @param max_rx: Maximum allowable WiFi Rx value in dBm (integer)

        |min_tx|, |min_rx| and |min_qual| are the only values you are likely to
        want to change, and they should be set at sensible defaults anyway.
        '''
        flipflop = os.path.dirname(__file__)
        flipflop = os.path.join(flipflop, 'network-flipflop.sh')

        if interface:
            flipflop = '%s %s' % (flipflop, interface)

        data = utils.system_output(flipflop, retain_output=True)
        results = data.splitlines()

        pattern = (r'802\.11([a-z]+) '
                  r'freq [0-9]+(?:\.[0-9]+)? [A-Z]Hz '
                  r'quality (\d+)/(\d+) '
                  r'rx (-?\d+) dBm '
                  r'tx (-?\d+) dBm')
        regex = re.compile(pattern)

        success = 0

        range_fail = '802.11%s %s outside range (%d dBm: %d to %d)'

        for association in results:
            readings = regex.match(association)
            if readings:
                modulation = readings.group(1)
                quality = int(readings.group(2)) / int(readings.group(3))
                rx = int(readings.group(4))
                tx = int(readings.group(5))

                if min_qual > quality:
                    raise error.TestFail('802.11%s quality too low (%f < %f)'
                                         % (modulation, quality, min_qual))

                if tx < min_tx or tx > max_tx:
                    raise error.TestFail(range_fail % (modulation, 'Tx',
                                                       tx, min_tx, max_tx))

                if rx < min_rx or rx > max_rx:
                    raise error.TestFail(range_fail % (modulation, 'Rx',
                                                       rx, min_rx, max_rx))

                success += 1
                logging.info('SUCCESS: 802.11%s signal is acceptable '
                    '(Rx:%d dBm, Tx:%d dBm)', modulation, rx, tx)

        if not success:
            raise error.TestFail('No AP associations established')

        return 0
