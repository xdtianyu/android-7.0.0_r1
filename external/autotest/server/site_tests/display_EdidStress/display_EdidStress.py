# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""This is a server side stressing DUT by switching Chameleon EDID."""

import glob
import logging
import os
import xmlrpclib

from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.cros.chameleon import chameleon_port_finder
from autotest_lib.client.cros.chameleon import chameleon_screen_test
from autotest_lib.client.cros.chameleon import edid
from autotest_lib.server import test
from autotest_lib.server.cros.multimedia import remote_facade_factory


class display_EdidStress(test.test):
    """Server side external display test.

    This test switches Chameleon EDID from among a large pool of EDIDs, tests
    DUT recognizes the emulated monitor and emits the correct video signal to
    Chameleon.
    """
    version = 1

    _EDID_TYPES = {'HDMI': {'HDMI', 'MHL', 'DVI'},
                   'DP': {'DP'},
                   'VGA': {'VGA'}}

    def run_once(self, host, edid_set):

        def _get_edid_type(s):
            i = s.rfind('_') + 1
            j = len(s) - len('.txt')
            return s[i:j].upper()

        edid_path = os.path.join(self.bindir, 'test_data', 'edids',
                                 edid_set, '*')

        factory = remote_facade_factory.RemoteFacadeFactory(host)
        display_facade = factory.create_display_facade()
        chameleon_board = host.chameleon

        chameleon_board.reset()
        finder = chameleon_port_finder.ChameleonVideoInputFinder(
                chameleon_board, display_facade)
        for chameleon_port in finder.iterate_all_ports():
            screen_test = chameleon_screen_test.ChameleonScreenTest(
                chameleon_port, display_facade, self.outputdir)

            logging.info('See the display on Chameleon: port %d (%s)',
                         chameleon_port.get_connector_id(),
                         chameleon_port.get_connector_type())

            connector = chameleon_port.get_connector_type()
            supported_types = self._EDID_TYPES[connector]

            failed_edids = []
            for filepath in glob.glob(edid_path):
                filename = os.path.basename(filepath)
                edid_type = _get_edid_type(filename)
                if edid_type not in supported_types:
                    logging.info('Skip EDID: %s...', filename)
                    continue

                logging.info('Use EDID: %s...', filename)
                try:
                    with chameleon_port.use_edid(
                            edid.Edid.from_file(filepath, skip_verify=True)):
                        resolution = utils.wait_for_value_changed(
                                display_facade.get_external_resolution,
                                old_value=None)
                        if resolution is None:
                            raise error.TestFail('No external display detected on DUT')
                        if screen_test.test_resolution(resolution):
                            raise error.TestFail('Resolution test failed')
                except (error.TestFail, xmlrpclib.Fault) as e:
                    logging.warning(e)
                    logging.error('EDID not supported: %s', filename)
                    failed_edids.append(filename)

            if failed_edids:
                message = ('Total %d EDIDs not supported: ' % len(failed_edids)
                           + ', '.join(failed_edids))
                logging.error(message)
                raise error.TestFail(message)
