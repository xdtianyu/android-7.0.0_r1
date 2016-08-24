# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
import ConfigParser
import threading
import xmlrpclib

CONFIG_FILE = 'rpm_config.ini'
CONFIG = ConfigParser.ConfigParser()
CONFIG.read(CONFIG_FILE)
remote_uri = CONFIG.get('RPM_INFRASTRUCTURE', 'frontend_uri')


def queue_request(dut_hostname, state):
    client = xmlrpclib.ServerProxy(remote_uri, verbose=False)
    result = client.queue_request(dut_hostname, state)
    print dut_hostname, result


def test():
    """
    Simple Integration Testing of RPM Infrastructure.
    """
    threading.Thread(target=queue_request,
                     args=('chromeos1-rack8e-hostbs1', 'ON')).start()
    threading.Thread(target=queue_request,
                     args=('chromeos1-rack8e-hostbs2.cros', 'OFF')).start()
    threading.Thread(target=queue_request,
                     args=('chromeos1-rack8e-hostbs3', 'OFF')).start()
    threading.Thread(target=queue_request,
                     args=('chromeos-rack1-hostbs1', 'ON')).start()
    threading.Thread(target=queue_request,
                     args=('chromeos-rack1-hostbs2', 'OFF')).start()


if __name__ == "__main__":
    test()
