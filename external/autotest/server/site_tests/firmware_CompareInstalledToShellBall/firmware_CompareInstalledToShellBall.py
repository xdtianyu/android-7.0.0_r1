# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from autotest_lib.client.common_lib import error
from autotest_lib.server import autotest
from autotest_lib.server import test
from autotest_lib.server.cros.faft.rpc_proxy import RPCProxy

class firmware_CompareInstalledToShellBall(test.test):
    """Compare the installed BIOS and EC versions to those in the shellball."""
    version = 1

    def run_once(self, host):
        # Make sure the client library is on the device so that the proxy
        # code is there when we try to call it.
        client_at = autotest.Autotest(host)
        client_at.install()

        self.faft_client = RPCProxy(host)
        installed_ec = self.faft_client.ec.get_version()
        installed_bios = self.faft_client.system.get_crossystem_value('fwid')

        # Chromeboxes do not have an EC
        if 'mosys' in installed_ec:
            installed_ec = None

        available_ec = None
        available_bios = None
        shellball = host.run('/usr/sbin/chromeos-firmwareupdate -V').stdout
        for line in shellball.splitlines():
            if line.startswith('BIOS version:'):
                parts = line.split()
                available_bios = parts[2].strip()
            if line.startswith('EC version:'):
                parts = line.split()
                available_ec = parts[2].strip()

        error_message = None
        if installed_bios != available_bios:
            error_message = str('BIOS versions do not match! Installed: %s '
                                 'Available %s' % (installed_bios,
                                 available_bios))
        if installed_ec != available_ec:
            ec_message = str('EC versions do not match! Installed: %s '
                             'Available %s ' % (installed_ec, available_ec))
            if error_message:
                error_message += '\n' + ec_message
            else:
                error_message = ec_message

        if error_message:
            raise error.TestFail(error_message)
