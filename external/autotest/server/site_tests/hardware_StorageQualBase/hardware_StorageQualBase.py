# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from autotest_lib.server import autotest
from autotest_lib.server import hosts
from autotest_lib.server import test


class hardware_StorageQualBase(test.test):
    """Tests run at the beginning over the disk qual test.

    This code runs simple tests to ensure the device meet basic critera.
    """

    version = 1
    CLIENT_FUNCTIONAL_TESTS = [
        ('hardware_DiskSize', {'constraints': ['gb_main_disk_size >= 8']}),
        ('hardware_SsdDetection', {
            'constraints': ['mb_ssd_device_size >= 8000']}),
        ('hardware_StorageFio', {'constraints': [
            '_seq_read_read_bw_mean >= 50 * 1024',
            '_seq_write_write_bw_mean >= 15 * 1024',
            '_16k_write_write_iops >= 10'],
            'requirements': [
                ('seq_read', []),
                ('seq_write', []),
                ('16k_write', [])],
                })
    ]

    CRYPTO_RUNTIME = 5 * 60  # seconds.

    CRYPTO_TESTS = [
        'surfing',
        'boot',
        'login',
        'seq_read',
        'seq_write',
        '16k_read',
        '16k_write',
        '8k_read',
        '8k_write',
        '4k_read',
        '4k_write',
    ]


    def run_once(self, client_ip, client_tag='', crypto_runtime=CRYPTO_RUNTIME):
        client = hosts.create_host(client_ip)
        client_at = autotest.Autotest(client)
        for test_name, argv in self.CLIENT_FUNCTIONAL_TESTS:
            client_at.run_test(test_name, disable_sysinfo=True, tag=client_tag,
                               **argv)

        # Test real life performance
        for script in self.CRYPTO_TESTS:
            client_at.run_test('platform_CryptohomeFio',
                disable_sysinfo=True,
                from_internal_disk_only=True,
                script=script,
                tag='_'.join([client_tag, script]),
                runtime=crypto_runtime,
                disk_configs=['crypto', 'plain'])
