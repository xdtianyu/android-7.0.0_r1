# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""This is an audio quality test over headphone using the Chameleon board."""

import logging
import os
import time

from autotest_lib.client.common_lib import error
from autotest_lib.client.cros.audio import audio_test_data
from autotest_lib.client.cros.chameleon import audio_test_utils
from autotest_lib.client.cros.chameleon import chameleon_audio_ids
from autotest_lib.client.cros.chameleon import chameleon_audio_helper
from autotest_lib.server.cros.audio import audio_test


class audio_MediaBasicVerification(audio_test.AudioTest):
    """Server side audio quality test over 3.5 headphones.

    This test talks to a Chameleon board and a Cros device to verify
    headphone audio quality of the Cros device.

    """
    version = 1
    DELAY_BEFORE_RECORD_SECONDS = 0.5
    RECORD_SECONDS = 10
    DELAY_AFTER_BINDING = 0.5

    def run_once(self, host, audio_test_file):
        chameleon_board = host.chameleon
        factory = self.create_remote_facade_factory(host)
        chameleon_board.reset()

        widget_factory = chameleon_audio_helper.AudioWidgetFactory(
                factory, host)

        source = widget_factory.create_widget(
            chameleon_audio_ids.CrosIds.HEADPHONE)
        recorder = widget_factory.create_widget(
            chameleon_audio_ids.ChameleonIds.LINEIN)
        binder = widget_factory.create_binder(source, recorder)

        with chameleon_audio_helper.bind_widgets(binder):
            # Checks the node selected by cras is correct.
            time.sleep(self.DELAY_AFTER_BINDING)
            audio_facade = factory.create_audio_facade()

            audio_test_utils.dump_cros_audio_logs(
                    host, audio_facade, self.resultsdir, 'after_binding')

            output_nodes, _ = audio_facade.get_selected_node_types()
            if output_nodes != ['HEADPHONE']:
                raise error.TestFail(
                        '%s rather than headphone is selected on Cros '
                        'device' % output_nodes)

            # Starts playing, waits for some time, and then starts recording.
            # This is to avoid artifact caused by codec initialization.
            logging.info('Start playing %s on Cros device',
                         audio_test_file)
            browser_facade = factory.create_browser_facade()
            tab_descriptor = browser_facade.new_tab(audio_test_file)

            time.sleep(self.DELAY_BEFORE_RECORD_SECONDS)
            logging.info('Start recording from Chameleon.')
            recorder.start_recording()

            time.sleep(self.RECORD_SECONDS)

            recorder.stop_recording()
            logging.info('Stopped recording from Chameleon.')
            browser_facade.close_tab(tab_descriptor)

            audio_test_utils.dump_cros_audio_logs(
                    host, audio_facade, self.resultsdir, 'after_recording')

            recorder.read_recorded_binary()
            logging.info('Read recorded binary from Chameleon.')

        recorded_file = os.path.join(self.resultsdir, 'recorded.raw')
        logging.info('Saving recorded data to %s', recorded_file)
        recorder.save_file(recorded_file)

        # Removes the beginning of recorded data. This is to avoid artifact
        # caused by Chameleon codec initialization in the beginning of
        # recording.
        recorder.remove_head(0.5)

        # Removes noise by a lowpass filter.
        recorder.lowpass_filter(4000)
        recorded_file = os.path.join(self.resultsdir, 'recorded_filtered.raw')
        logging.info('Saving filtered data to %s', recorded_file)
        recorder.save_file(recorded_file)

        # Compares data by frequency. Headphone audio signal has gone through
        # analog processing. This suffers from codec artifacts and noise on the
        # path. Comparing data by frequency is more robust than comparing by
        # correlation, which is suitable for fully-digital audio path like USB
        # and HDMI.
        audio_test_utils.check_recorded_frequency(
                audio_test_data.MEDIA_HEADPHONE_TEST_FILE,
                recorder, check_anomaly=True)
