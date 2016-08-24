#!/usr/bin/python
#
# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging, os, random
from autotest_lib.client.bin import utils, test
from autotest_lib.client.common_lib import error

class kernel_VbootContextEC(test.test):
    '''Run the vboot context ec test.'''
    version = 1

    dts_node_path = ('/proc/device-tree/firmware'
                     '/chromeos/nonvolatile-context-storage')
    sys_vbootcontext_path = '/sys/bus/platform/drivers/cros-ec-vbc/'

    def run_once(self):
        arch = utils.get_arch()
        if not arch.startswith('arm'):
            logging.info('Skip test for non-ARM arch %s', arch)
            return

        media = utils.read_file(self.dts_node_path).strip('\n\x00')
        if media != 'cros-ec':
            logging.info('Skip test: Vboot Context storage media is "%s"',
                    media)
            return

        sysfs_entry = None
        for name in os.listdir(self.sys_vbootcontext_path):
            if name.startswith('cros-ec-vbc'):
                sysfs_entry = os.path.join(self.sys_vbootcontext_path, name,
                        'vboot_context')
                break
        else:
            raise error.TestFail('Could not find sysfs entry under %s',
                    self.sys_vbootcontext_path)

        # Retrieve Vboot Context
        vboot_context = utils.system_output('mosys nvram vboot read').strip()
        try:
            # Test read
            vc_expect = vboot_context
            vc_got = utils.read_file(sysfs_entry).strip('\n\x00')
            if vc_got != vc_expect:
                raise error.TestFail('Could not read Vboot Context: '
                                     'Expect "%s" but got "%s"' %
                                     (vc_expect, vc_got))
            # Test write of a random hex string
            vc_expect = ''.join(random.choice('0123456789abcdef')
                                for _ in xrange(32))
            utils.open_write_close(sysfs_entry, vc_expect)
            vc_got = utils.system_output('mosys nvram vboot read').strip()
            if vc_got != vc_expect:
                raise error.TestFail('Could not write Vboot Context: '
                                     'Expect "%s" but got "%s"' %
                                     (vc_expect, vc_got))
        finally:
            # Restore Vboot Context
            utils.run('mosys nvram vboot write "%s"' % vboot_context)
