# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""This is a server side audio test using the Chameleon board."""

import logging
import os
import tempfile
import time
import threading

from autotest_lib.client.common_lib import error, file_utils
from autotest_lib.client.cros.chameleon import audio_test_utils
from autotest_lib.client.cros.chameleon import chameleon_audio_helper
from autotest_lib.client.cros.chameleon import chameleon_audio_ids
from autotest_lib.server.cros.audio import audio_test


class audio_AudioQualityAfterSuspend(audio_test.AudioTest):
    """Server side audio test.

    This test talks to a Chameleon board and a Cros device to verify
    audio function of the Cros device after suspend/resume.

    """
    version = 1
    RECORD_SECONDS = 10
    RESUME_TIMEOUT_SECS = 60
    SHORT_WAIT = 4
    SUSPEND_SECONDS = 30


    def action_suspend(self, suspend_time=SUSPEND_SECONDS):
        """Calls the host method suspend.

        @param suspend_time: time to suspend the device for.

        """
        self.host.suspend(suspend_time=suspend_time)


    def suspend_resume(self):
        """Performs suspend/resume."""

        # Suspend
        boot_id = self.host.get_boot_id()
        thread = threading.Thread(target=self.action_suspend)
        thread.start()

        logging.info('Suspend start....')
        self.host.test_wait_for_sleep(self.SUSPEND_SECONDS / 3)
        logging.info('Waiting for resume....')
        self.host.test_wait_for_resume(boot_id, self.RESUME_TIMEOUT_SECS)
        logging.info('Resume complete....')


    def check_correct_audio_node_selected(self):
        """Checks the node selected by Cras is correct."""
        audio_test_utils.check_audio_nodes(self.audio_facade, self.audio_nodes)


    def play_and_record(self, recorder_widget):
        """Plays and records audio

        @param recorder_widget: widget to do the recording

        """
        audio_test_utils.dump_cros_audio_logs(
                self.host, self.audio_facade, self.resultsdir,
                'before_playback')

        self.check_correct_audio_node_selected()

        browser_facade = self.factory.create_browser_facade()

        host_file = os.path.join('/tmp',
                os.path.basename(self.test_playback_file))
        with tempfile.NamedTemporaryFile() as tmpfile:
            file_utils.download_file(self.test_playback_file, tmpfile.name)
            os.chmod(tmpfile.name, 0444)
            self.host.send_file(tmpfile.name, host_file)
            logging.debug('Copied the file on the DUT at %s', host_file)

        # Play, wait for some time, and then start recording.
        # This is to avoid artifact caused by codec initialization.
        browser_facade.new_tab('file://' + host_file)
        logging.info('Start playing %s on Cros device', host_file)

        time.sleep(self.SHORT_WAIT)
        logging.debug('Suspend.')
        self.suspend_resume()
        logging.debug('Resume.')
        time.sleep(self.SHORT_WAIT)
        logging.debug('Start recording.')
        recorder_widget.start_recording()

        time.sleep(self.RECORD_SECONDS)

        recorder_widget.stop_recording()
        logging.debug('Stopped recording.')

        audio_test_utils.dump_cros_audio_logs(
                self.host, self.audio_facade, self.resultsdir,
                'after_recording')

        recorder_widget.read_recorded_binary()


    def save_and_check_data(self, recorder_widget):
        """Saves and checks the data from the recorder

        @param recorder_widget: recorder widget to save data from

        """
        recorded_file = os.path.join(self.resultsdir, 'recorded.raw')
        logging.debug('Saving recorded data to %s', recorded_file)
        recorder_widget.save_file(recorded_file)

        # Removes the beginning of recorded data. This is to avoid artifact
        # caused by codec initialization in the beginning of
        # recording.
        recorder_widget.remove_head(2.0)

        # Removes noise by a lowpass filter.
        recorder_widget.lowpass_filter(self.lowpass_freq)
        recorded_file = os.path.join(self.resultsdir, 'recorded_filtered.raw')
        logging.debug('Saving filtered data to %s', recorded_file)
        recorder_widget.save_file(recorded_file)

        # Compares data by frequency and returns the result.
        audio_test_utils.check_recorded_frequency(
                self.audio_test_data, recorder_widget,
                second_peak_ratio=self.second_peak_ratio,
                ignore_frequencies=self.ignore_frequencies,
                check_anomaly=True)


    def run_once(self, host, audio_nodes, audio_test_data, test_playback_file,
                 lowpass_freq=None,
                 bind_from=None, bind_to=None,
                 source=None, recorder=None,
                 tag=None):
        """Runs the test main workflow

        @param host: A host object representing the DUT.
        @param audio_nodes: audio nodes supposed to be selected.
        @param audio_test_data: audio test frequency defined in audio_test_data
        @param test_playback_file: audio media file(wav, mp3,...) to be used
            for testing
        @param lowpass_freq: frequency noise filter.
        @param bind_from: audio originating entity to be binded
            should be defined in chameleon_audio_ids
        @param bind_to: audio directed_to entity to be binded
            should be defined in chameleon_audio_ids
        @param source: source widget entity
            should be defined in chameleon_audio_ids
        @param recorder: recorder widget entity
            should be defined in chameleon_audio_ids

        """
        self.host = host
        self.audio_nodes = audio_nodes

        if (not audio_test_utils.has_internal_speaker(host) and
                tag == "internal_speaker"):
            return

        self.second_peak_ratio = audio_test_utils.DEFAULT_SECOND_PEAK_RATIO
        self.ignore_frequencies = None
        if source == chameleon_audio_ids.CrosIds.SPEAKER:
            self.second_peak_ratio = 0.1
            self.ignore_frequencies = [50, 60]

        self.audio_test_data = audio_test_data
        self.lowpass_freq = lowpass_freq
        self.test_playback_file = test_playback_file
        chameleon_board = self.host.chameleon
        self.factory = self.create_remote_facade_factory(self.host)
        self.audio_facade = self.factory.create_audio_facade()
        chameleon_board.reset()
        widget_factory = chameleon_audio_helper.AudioWidgetFactory(
                self.factory, host)

        # Two widgets are binded in the factory if necessary.
        binder_widget = None
        source_widget = None
        recorder_widget = None
        if bind_from != None and bind_to != None:
            source_widget = widget_factory.create_widget(bind_from)
            recorder_widget = widget_factory.create_widget(bind_to)
            binder_widget = widget_factory.create_binder(source_widget,
                                                         recorder_widget)
        elif source != None and recorder != None:
            source_widget = widget_factory.create_widget(source)
            recorder_widget = widget_factory.create_widget(recorder)
        else:
            raise error.TestError('Test configuration or setup problem.')

        self.audio_board = chameleon_board.get_audio_board()

        if binder_widget:
            # Headphone test which requires Chameleon LINEIN and DUT headphones
            # binding.
            with chameleon_audio_helper.bind_widgets(binder_widget):
                self.play_and_record(recorder_widget)
        else:
            # Internal speakers test.
            self.play_and_record(recorder_widget)

        self.save_and_check_data(recorder_widget)
