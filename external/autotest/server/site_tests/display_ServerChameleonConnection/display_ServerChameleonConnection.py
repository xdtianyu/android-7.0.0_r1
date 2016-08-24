# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""This is a server-side test to check the Chameleon connection."""

import logging

from autotest_lib.client.common_lib import error
from autotest_lib.client.cros.chameleon import chameleon_port_finder
from autotest_lib.server import test
from autotest_lib.server.cros.multimedia import remote_facade_factory


class display_ServerChameleonConnection(test.test):
    """Chameleon connection server test.

    This test talks to a Chameleon board from DUT. Try to plug the Chameleon
    ports and see if DUT detects them.
    """
    version = 1

    def run_once(self, host):
        factory = remote_facade_factory.RemoteFacadeFactory(host)
        display_facade = factory.create_display_facade()
        chameleon_board = host.chameleon

        chameleon_board.reset()

        finder = chameleon_port_finder.ChameleonVideoInputFinder(
                chameleon_board, display_facade)
        ports = finder.find_all_ports()

        connected_ports = ports.connected
        dut_failed_ports = ports.failed

        msg = str(finder)
        logging.debug(msg)

        if dut_failed_ports or not connected_ports:
            raise error.TestFail(msg)
