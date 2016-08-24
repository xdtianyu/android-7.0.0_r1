# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""
This is a server side USB playback/record audio test using the
Chameleon board.
"""

import logging
import os
import time

from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.cros.audio import audio_test_data
from autotest_lib.client.cros.chameleon import audio_test_utils
from autotest_lib.client.cros.chameleon import chameleon_audio_ids
from autotest_lib.client.cros.chameleon import chameleon_audio_helper
from autotest_lib.server.cros.audio import audio_test


class audio_AudioBasicUSBPlaybackRecord(audio_test.AudioTest):
    """Server side USB playback/record audio test.

    This test talks to a Chameleon board and a Cros device to verify
    USB audio playback/record function of the Cros device.

    """
    version = 1
    RECORD_SECONDS = 5
    SUSPEND_SECONDS = 30
    RPC_RECONNECT_TIMEOUT = 60

    def run_once(self, host, suspend=False):
        golden_file = audio_test_data.SWEEP_TEST_FILE

        chameleon_board = host.chameleon
        factory = self.create_remote_facade_factory(host)

        chameleon_board.reset()

        widget_factory = chameleon_audio_helper.AudioWidgetFactory(
                factory, host)

        playback_source = widget_factory.create_widget(
                chameleon_audio_ids.CrosIds.USBOUT)
        playback_recorder = widget_factory.create_widget(
                chameleon_audio_ids.ChameleonIds.USBIN)
        playback_binder = widget_factory.create_binder(
                playback_source, playback_recorder)

        record_source = widget_factory.create_widget(
                chameleon_audio_ids.ChameleonIds.USBOUT)
        record_recorder = widget_factory.create_widget(
                chameleon_audio_ids.CrosIds.USBIN)
        record_binder = widget_factory.create_binder(
                record_source, record_recorder)

        with chameleon_audio_helper.bind_widgets(playback_binder):
            with chameleon_audio_helper.bind_widgets(record_binder):
                # Checks the node selected by cras is correct.
                audio_facade = factory.create_audio_facade()

                audio_test_utils.dump_cros_audio_logs(
                        host, audio_facade, self.resultsdir, 'after_binding')

                audio_test_utils.check_audio_nodes(
                        audio_facade, (['USB'], ['USB']))

                logging.info('Setting playback data on Cros device')

                audio_facade.set_selected_output_volume(70)

                playback_source.set_playback_data(golden_file)
                record_source.set_playback_data(golden_file)

                if suspend:
                    audio_test_utils.suspend_resume(host, self.SUSPEND_SECONDS)
                    utils.poll_for_condition(condition=factory.ready,
                                             timeout=self.RPC_RECONNECT_TIMEOUT,
                                             desc='multimedia server reconnect')
                    audio_test_utils.check_audio_nodes(audio_facade,
                                                       (['USB'], ['USB']))

                logging.info('Start recording from Chameleon.')
                playback_recorder.start_recording()
                logging.info('Start recording from Cros device.')
                record_recorder.start_recording()

                logging.info('Start playing %s on Cros device',
                             golden_file.path)
                playback_source.start_playback()
                logging.info('Start playing %s on Chameleon',
                             golden_file.path)
                record_source.start_playback()

                time.sleep(self.RECORD_SECONDS)

                playback_recorder.stop_recording()
                logging.info('Stopped recording from Chameleon.')
                record_recorder.stop_recording()
                logging.info('Stopped recording from Cros device.')

                audio_test_utils.dump_cros_audio_logs(
                        host, audio_facade, self.resultsdir, 'after_recording')

                playback_recorder.read_recorded_binary()
                logging.info('Read recorded binary from Chameleon.')
                record_recorder.read_recorded_binary()
                logging.info('Read recorded binary from Cros device.')

        playback_recorded_file = os.path.join(
                self.resultsdir, "playback_recorded.raw")
        logging.info('Saving Cros playback recorded data to %s',
                     playback_recorded_file)
        playback_recorder.save_file(playback_recorded_file)

        record_recorded_file = os.path.join(
                self.resultsdir, "record_recorded.raw")
        logging.info('Saving Cros record recorded data to %s',
                     record_recorded_file)
        record_recorder.save_file(record_recorded_file)

        error_messages = ''
        if not chameleon_audio_helper.compare_recorded_result(
                golden_file, playback_recorder, 'correlation'):
            error_messages += ('Record: Recorded file does not match'
                               ' playback file.')
        if not chameleon_audio_helper.compare_recorded_result(
                golden_file, record_recorder, 'correlation'):
            error_messages += ('Playback: Recorded file does not match'
                               ' playback file.')
        if error_messages:
            raise error.TestFail(error_messages)
