# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""This is a server side bluetooth connection test using the Chameleon board."""

import logging
import os
import time, threading

from autotest_lib.client.bin import utils
from autotest_lib.client.cros.chameleon import audio_test_utils
from autotest_lib.client.cros.chameleon import chameleon_audio_helper
from autotest_lib.client.cros.chameleon import chameleon_audio_ids
from autotest_lib.server.cros.audio import audio_test


class audio_AudioBluetoothConnectionStability(audio_test.AudioTest):
    """Server side bluetooth connection audio test.

    This test talks to a Chameleon board and a Cros device to verify
    bluetooth connection between Cros device and bluetooth module is stable.

    """
    version = 1
    CONNECTION_TEST_TIME_SECS = 300

    def dump_logs_after_nodes_changed(self):
        """Dumps the log after unexpected NodesChanged signal happens."""
        audio_test_utils.dump_cros_audio_logs(
                self.host, self.audio_facade, self.resultsdir,
                'after_nodes_changed')


    def run_once(self, host, suspend=False,
                 disable=False, disconnect=False):
        """Runs bluetooth audio connection test."""
        self.host = host

        factory = self.create_remote_facade_factory(host)
        self.audio_facade = factory.create_audio_facade()

        chameleon_board = host.chameleon
        chameleon_board.reset()

        widget_factory = chameleon_audio_helper.AudioWidgetFactory(
                factory, host)

        source = widget_factory.create_widget(
            chameleon_audio_ids.CrosIds.BLUETOOTH_HEADPHONE)
        bluetooth_widget = widget_factory.create_widget(
            chameleon_audio_ids.PeripheralIds.BLUETOOTH_DATA_RX)
        recorder = widget_factory.create_widget(
            chameleon_audio_ids.ChameleonIds.LINEIN)

        binder = widget_factory.create_binder(
                source, bluetooth_widget, recorder)

        with chameleon_audio_helper.bind_widgets(binder):

            audio_test_utils.dump_cros_audio_logs(
                    host, self.audio_facade, self.resultsdir, 'after_binding')

            if audio_test_utils.has_internal_microphone(host):
                self.audio_facade.set_chrome_active_node_type(None, 'BLUETOOTH')

            audio_test_utils.check_audio_nodes(self.audio_facade,
                                               (['BLUETOOTH'], ['BLUETOOTH']))

            # Monitors there is no node change in this time period.
            with audio_test_utils.monitor_no_nodes_changed(
                    self.audio_facade, self.dump_logs_after_nodes_changed):
                logging.debug('Monitoring NodesChanged signal for %s seconds',
                              self.CONNECTION_TEST_TIME_SECS)
                time.sleep(self.CONNECTION_TEST_TIME_SECS)
