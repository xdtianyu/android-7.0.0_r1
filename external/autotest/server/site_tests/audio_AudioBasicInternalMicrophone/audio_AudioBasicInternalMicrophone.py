# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""This is a server side internal microphone test using the Chameleon board."""

import logging
import os
import time

from autotest_lib.client.common_lib import error
from autotest_lib.client.cros.audio import audio_test_data
from autotest_lib.client.cros.chameleon import audio_test_utils
from autotest_lib.client.cros.chameleon import chameleon_audio_helper
from autotest_lib.client.cros.chameleon import chameleon_audio_ids
from autotest_lib.server.cros.audio import audio_test


class audio_AudioBasicInternalMicrophone(audio_test.AudioTest):
    """Server side internal microphone audio test.

    This test talks to a Chameleon board and a Cros device to verify
    internal mic audio function of the Cros device.

    """
    version = 1
    DELAY_BEFORE_RECORD_SECONDS = 0.5
    RECORD_SECONDS = 9
    DELAY_AFTER_BINDING = 0.5

    def run_once(self, host):
        if not audio_test_utils.has_internal_microphone(host):
            return

        golden_file = audio_test_data.SIMPLE_FREQUENCY_TEST_FILE

        chameleon_board = host.chameleon
        factory = self.create_remote_facade_factory(host)

        chameleon_board.reset()

        widget_factory = chameleon_audio_helper.AudioWidgetFactory(
                factory, host)

        source = widget_factory.create_widget(
            chameleon_audio_ids.ChameleonIds.LINEOUT)
        sink = widget_factory.create_widget(
            chameleon_audio_ids.PeripheralIds.SPEAKER)
        binder = widget_factory.create_binder(source, sink)

        recorder = widget_factory.create_widget(
            chameleon_audio_ids.CrosIds.INTERNAL_MIC)

        with chameleon_audio_helper.bind_widgets(binder):
            # Checks the node selected by cras is correct.
            time.sleep(self.DELAY_AFTER_BINDING)
            audio_facade = factory.create_audio_facade()

            audio_test_utils.dump_cros_audio_logs(
                    host, audio_facade, self.resultsdir, 'after_binding')

            _, input_nodes = audio_facade.get_selected_node_types()
            if input_nodes != ['INTERNAL_MIC']:
                raise error.TestFail(
                        '%s rather than internal mic is selected on Cros '
                        'device' % input_nodes)

            logging.info('Setting playback data on Chameleon')
            source.set_playback_data(golden_file)

            # Starts playing, waits for some time, and then starts recording.
            # This is to avoid artifact caused by chameleon codec initialization
            # in the beginning of playback.
            logging.info('Start playing %s from Chameleon',
                         golden_file.path)
            source.start_playback()

            time.sleep(self.DELAY_BEFORE_RECORD_SECONDS)
            logging.info('Start recording from Cros device.')
            recorder.start_recording()

            time.sleep(self.RECORD_SECONDS)

            recorder.stop_recording()
            logging.info('Stopped recording from Cros device.')

            audio_test_utils.dump_cros_audio_logs(
                    host, audio_facade, self.resultsdir, 'after_recording')

            recorder.read_recorded_binary()
            logging.info('Read recorded binary from Cros device.')

        recorded_file = os.path.join(self.resultsdir, "recorded.raw")
        logging.info('Saving recorded data to %s', recorded_file)
        recorder.save_file(recorded_file)

        # Removes the beginning of recorded data. This is to avoid artifact
        # caused by Cros device codec initialization in the beginning of
        # recording.
        recorder.remove_head(1.0)

        # Removes noise by a lowpass filter.
        recorder.lowpass_filter(1500)
        recorded_file = os.path.join(self.resultsdir, "recorded_filtered.raw")
        logging.info('Saving filtered data to %s', recorded_file)
        recorder.save_file(recorded_file)

        # Cros device only records one channel data. Here we set the channel map
        # of the recorder to compare the recorded data with left channel of the
        # test data. This is fine as left and right channel of test data are
        # identical.
        recorder.channel_map = [0]

        # Compares data by frequency. Audio signal from Chameleon Line-Out to
        # speaker and finally recorded on Cros device using internal microphone
        # has gone through analog processing and through the air.
        # This suffers from codec artifacts and noise on the path.
        # Comparing data by frequency is more robust than comparing them by
        # correlation, which is suitable for fully-digital audio path like USB
        # and HDMI.
        audio_test_utils.check_recorded_frequency(golden_file, recorder,
                                                  second_peak_ratio=0.2)
