# Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging, os, stat

from autotest_lib.client.bin import test
from autotest_lib.client.common_lib import error


class network_3GGobiPorts(test.test):
    version = 1

    def run_once(self):
        failed_ports = []

        ports = ['/dev/ttyUSB0', '/dev/ttyUSB1', '/dev/ttyUSB2']

        for port in ports:
            if not os.path.exists(port):
                failed_ports.append(port)
                logging.error('Port %s does not exist.' % port)
                continue
            mode = os.stat(port).st_mode
            if not stat.S_ISCHR(mode):
                logging.error('Port %s is not a character device. mode = %s' % (
                    port, mode))
                failed_ports.append(port)
                continue
            logging.info('Port %s is a character device.' % port)

        if failed_ports:
            raise error.TestFail('Ports [%s] are missing or not char devices' %
                                 ', '.join(failed_ports))
