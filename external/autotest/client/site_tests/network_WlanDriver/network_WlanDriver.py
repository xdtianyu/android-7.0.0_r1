# Copyright (c) 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging

from autotest_lib.client.bin import test
from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros.network import interface


class network_WlanDriver(test.test):
    """
    Ensure wireless devices have the expected associated kernel driver.
    """
    version = 1
    DEVICES = [ 'wlan0', 'mlan0' ]
    EXPECTED_DRIVER = {
            'Atheros AR9280': {
                    '3.4': 'wireless/ath/ath9k/ath9k.ko',
                    '3.8': 'wireless-3.4/ath/ath9k/ath9k.ko'
            },
            'Atheros AR9382': {
                    '3.4': 'wireless/ath/ath9k/ath9k.ko',
                    '3.8': 'wireless-3.4/ath/ath9k/ath9k.ko'
            },
            'Intel 7260': {
                    '3.8': 'wireless/iwl7000/iwlwifi/iwlwifi.ko',
                    '3.10': 'wireless-3.8/iwl7000/iwlwifi/iwlwifi.ko',
                    '3.14': 'wireless-3.8/iwl7000/iwlwifi/iwlwifi.ko'
            },
            'Intel 7265': {
                    '3.8': 'wireless/iwl7000/iwlwifi/iwlwifi.ko',
                    '3.10': 'wireless-3.8/iwl7000/iwlwifi/iwlwifi.ko',
                    '3.14': 'wireless-3.8/iwl7000/iwlwifi/iwlwifi.ko',
                    '3.18': 'wireless/iwl7000/iwlwifi/iwlwifi.ko'
            },
            'Atheros AR9462': {
                    '3.4': 'wireless/ath/ath9k_btcoex/ath9k_btcoex.ko',
                    '3.8': 'wireless-3.4/ath/ath9k_btcoex/ath9k_btcoex.ko'
            },
            'Marvell 88W8797 SDIO': {
                    '3.4': 'wireless/mwifiex/mwifiex_sdio.ko',
                    '3.8': 'wireless-3.4/mwifiex/mwifiex_sdio.ko'
            },
            'Marvell 88W8887 SDIO': {
                     '3.14': 'wireless-3.8/mwifiex/mwifiex_sdio.ko'
            },
            'Marvell 88W8897 PCIE': {
                     '3.8': 'wireless/mwifiex/mwifiex_pcie.ko',
                     '3.10': 'wireless-3.8/mwifiex/mwifiex_pcie.ko'
            },
            'Marvell 88W8897 SDIO': {
                     '3.8': 'wireless/mwifiex/mwifiex_sdio.ko',
                     '3.10': 'wireless-3.8/mwifiex/mwifiex_sdio.ko',
                     '3.14': 'wireless-3.8/mwifiex/mwifiex_sdio.ko'
            },
            'Broadcom BCM4354 SDIO': {
                     '3.8': 'wireless/brcm80211/brcmfmac/brcmfmac.ko',
                     '3.14': 'wireless-3.8/brcm80211/brcmfmac/brcmfmac.ko'
            },
            'Broadcom BCM4356 PCIE': {
                     '3.10': 'wireless-3.8/brcm80211/brcmfmac/brcmfmac.ko'
            },
    }


    def run_once(self):
        """Test main loop"""
        # full_revision looks like "3.4.0".
        full_revision = utils.system_output('uname -r')
        # base_revision looks like "3.4".
        base_revision = '.'.join(full_revision.split('.')[:2])
        logging.info('Kernel base is %s', base_revision)

        found_devices = 0
        for device in self.DEVICES:
            net_if = interface.Interface(device)
            device_description = net_if.device_description

            if not device_description:
                continue

            device_name, module_path = device_description
            logging.info('Device name %s, module path %s',
                         device_name, module_path)
            if not device_name in self.EXPECTED_DRIVER:
                raise error.TestFail('Unexpected device name %s' %
                                     device_name)

            if not base_revision in self.EXPECTED_DRIVER[device_name]:
                raise error.TestNAError('Unexpected base kernel revision %s '
                                        'with device name %s' %
                                        (base_revision, device_name))

            expected_driver = self.EXPECTED_DRIVER[device_name][base_revision]
            if module_path != expected_driver:
                raise error.TestFail('Unexpected driver for %s/%s; '
                                     'got %s but expected %s' %
                                     (base_revision, device_name,
                                      module_path, expected_driver))
            found_devices += 1
        if not found_devices:
            raise error.TestNAError('Found no recognized wireless devices?')
