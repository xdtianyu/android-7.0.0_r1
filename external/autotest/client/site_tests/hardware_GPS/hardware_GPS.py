# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import re
import socket
import time

from autotest_lib.client.bin import test
from autotest_lib.client.common_lib import error, utils


class hardware_GPS(test.test):
    version = 1

    def run_once(self):
        # Default gpsd port. Can be changed in /etc/init/gpsd.conf.
        gpsd_port = 2947
        match = False
        gpsd_started = False

        gpsd_status = utils.system_output('initctl status gpsd')
        if not 'start/running' in gpsd_status:
            utils.system('initctl start gpsd')
            gpsd_started = True
            for _ in range(10):
                try:
                    c = socket.create_connection(('localhost', gpsd_port))
                except socket.error:
                    time.sleep(1)
                    continue
                c.close()
                break

        gpspipe = utils.system_output(
            'gpspipe -r -n 10 localhost:%d' % gpsd_port, timeout=60)
        logging.debug(gpspipe)
        for line in gpspipe.split('\n'):
            line = line.strip()

            # For now - just look for any GPS sentence in the output.
            match = re.search(
                r'^\$GP(BOD|BWC|GGA|GLL|GSA|GSV|HDT|R00|RMA|RMB|' +
                r'RMC|RTE|STN|TRF|VBW|VTG|WPL|XTE|ZDA)',
                line)

            if match:
                break

        if gpsd_started:
            # If it was us who started it - shut it back down.
            utils.system('initctl stop gpsd')

        if not match:
            raise error.TestFail('Unable to find GPS device')
