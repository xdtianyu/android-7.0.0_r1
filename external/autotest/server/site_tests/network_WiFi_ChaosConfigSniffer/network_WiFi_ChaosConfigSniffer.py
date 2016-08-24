# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import os
import pprint

from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros.network import iw_runner
from autotest_lib.server import test

class network_WiFi_ChaosConfigSniffer(test.test):
    """ Test to grab debugging info about chaos configuration falures. """

    version = 1


    def run_once(self, wifi_client=None, ssids=[]):
        missing_ssids = []
        for ssid in ssids:
            logging.info('Scanning for SSID: %s', ssid)
            networks = wifi_client.iw_runner.wait_for_scan_result(
                wifi_client._wifi_if, ssids=[ssid], timeout_seconds=60)
            if networks == None:
                missing_ssids.append(ssid)
            else:
                path = os.path.join(self.outputdir, str('%s.txt' % ssid))
                network = networks[0]
                f = open(path, 'wb')
                f.write('Scan information:\n')
                f.write(pprint.pformat(network))
                f.write('\n\n\nInfo to be added to the config file:\n')
                f.write('[%s]\n' % network.bss)
                f.write('brand = <Enter AP brand>\n')
                f.write('wan_hostname = <Enter hostname, do not '
                        'include .cros>\n')
                f.write('ssid = %s\n' % network.ssid)
                f.write('frequency = %s\n' % network.frequency)
                f.write('rpm_managed = True\n')
                if network.frequency > 2484:
                    f.write('bss5 = %s\n' % network.bss)
                else:
                    f.write('bss = %s\n' % network.bss)
                f.write('wan mac = <Enter WAN MAC address>\n')
                f.write('model = <Enter model>\n')
                f.write('security = %s\n' % network.security)
                if (network.security == iw_runner.SECURITY_WPA or
                    network.security == iw_runner.SECURITY_WPA2 or
                    network.security == iw_runner.SECURITY_MIXED):
                    f.write('psk = chromeos\n')
                f.write('class_name = StaticAPConfigurator\n')
                f.close()
        if len(missing_ssids) > 0:
            logging.error('The following SSIDs could not be found:')
            for ssid in missing_ssids:
                logging.error('\t%s', ssid)
            raise error.TestError('Some SSIDs could not be found, please check '
                                  'the configuration on the APs.')
