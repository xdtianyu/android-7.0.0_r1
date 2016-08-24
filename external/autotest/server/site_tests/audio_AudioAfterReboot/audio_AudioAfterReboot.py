# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""This is a server side audio test using the Chameleon board."""

import logging
import os
import time

from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.cros.chameleon import audio_test_utils
from autotest_lib.client.cros.chameleon import audio_widget_link
from autotest_lib.client.cros.chameleon import chameleon_audio_helper
from autotest_lib.client.cros.chameleon import chameleon_audio_ids
from autotest_lib.server.cros.audio import audio_test


class audio_AudioAfterReboot(audio_test.AudioTest):
    """Server side audio test.

    This test talks to a Chameleon board and a Cros device to verify
    audio function of the Cros device after reboot.

    """
    version = 1
    DELAY_BEFORE_RECORD_SECONDS = 0.5
    DELAY_AFTER_BINDING = 0.5
    RECORD_SECONDS = 5
    SHORT_WAIT = 2
    PRC_RECONNECT_TIMEOUT = 60

    def action_plug_jack(self, plug_state):
        """Calls the audio interface API and plugs/unplugs.

        @param plug_state: plug state to switch to

        """
        logging.debug('Plugging' if plug_state else 'Unplugging')
        jack_plugger = self.audio_board.get_jack_plugger()
        if plug_state:
            jack_plugger.plug()
        else:
            jack_plugger.unplug()
        time.sleep(self.SHORT_WAIT)


    def play_and_record(self, source_widget, recorder_widget=None):
        """Plays and records (if needed) audio.

        @param source_widget: widget to do the playback
        @param recorder_widget: widget to do the recording
            None to skip recording.

        """
        self.check_correct_audio_node_selected()

        # Play, wait for some time, and then start recording if needed.
        source_widget.set_playback_data(self.golden_file)
        logging.debug('Start playing %s', self.golden_file.path)
        source_widget.start_playback()

        if recorder_widget != None:
            time.sleep(self.DELAY_BEFORE_RECORD_SECONDS)
            logging.debug('Start recording.')
            recorder_widget.start_recording()

            time.sleep(self.RECORD_SECONDS)

            recorder_widget.stop_recording()
            logging.debug('Stopped recording.')

            audio_test_utils.dump_cros_audio_logs(
                    self.host, self.audio_facade, self.resultsdir,
                    'after_recording')

            recorder_widget.read_recorded_binary()
        else:
            time.sleep(self.RECORD_SECONDS)


    def save_and_check_data(self, recorder_widget):
        """Saves and checks the data from the recorder.

        @param recorder_widget: recorder widget to save data from

        @raise error.TestFail: if comparison fails

        """
        recorded_file = os.path.join(self.resultsdir, "recorded.raw")
        logging.debug('Saving recorded data to %s', recorded_file)
        recorder_widget.save_file(recorded_file)

        # Removes the beginning of recorded data. This is to avoid artifact
        # caused by codec initialization in the beginning of recording.
        recorder_widget.remove_head(2.0)

        # Removes noise by a lowpass filter.
        recorder_widget.lowpass_filter(self.low_pass_freq)
        recorded_file = os.path.join(self.resultsdir,
                                     "recorded_filtered.raw")
        logging.debug('Saving filtered data to %s', recorded_file)
        recorder_widget.save_file(recorded_file)

        # Compares data by frequency.
        audio_test_utils.check_recorded_frequency(
                self.golden_file, recorder_widget,
                second_peak_ratio=self.second_peak_ratio,
                ignore_frequencies=self.ignore_frequencies)


    def check_correct_audio_node_selected(self):
        """Checks the node selected by Cras is correct."""
        audio_test_utils.check_audio_nodes(self.audio_facade, self.audio_nodes)


    def play_reboot_play_and_record (self, source_widget, recorder_widget):
        """Play audio, then reboot, and play and record.

        @param source_widget: source widget to play with
        @param recorder_widget: recorder widget to record with

        """
        self.play_and_record(source_widget)

        # Disconnecs audio bus so Cros device can detects plugger correctly
        # when the test involes plugger.
        # For case where audio bus is used but no plugger is used, it is no
        # harm to disconnect audio bus and reconnect it.
        if self.use_audio_bus:
            logging.info('Disconnecting audio bus before reboot')
            self.widget_link.disconnect_audio_bus()

        self.host.reboot()
        utils.poll_for_condition(condition=self.factory.ready,
                                 timeout=self.PRC_RECONNECT_TIMEOUT,)
        logging.debug('After reboot')

        audio_test_utils.dump_cros_audio_logs(
                self.host, self.audio_facade, self.resultsdir,
                'after_reboot')

        self.check_correct_audio_node_selected()

        if self.use_audio_bus:
            logging.info('Reconnecting audio bus after reboot before playback')
            self.widget_link.reconnect_audio_bus()

        audio_test_utils.dump_cros_audio_logs(
                self.host, self.audio_facade, self.resultsdir,
                'after_bus_reconnect')

        self.play_and_record(source_widget, recorder_widget)


    def run_once(self, host, golden_data, audio_nodes, bind_from=None, bind_to=None,
                 source=None, recorder=None, is_internal=False):
        """Runs the test main workflow.

        @param host: A host object representing the DUT.
        @param golden_data: audio file and low pass filter frequency
           the audio file should be test data defined in audio_test_data
        @param audio_nodes: audio nodes supposed to be selected
        @param bind_from: audio originating entity to be binded
            should be defined in chameleon_audio_ids
        @param bind_to: audio directed_to entity to be binded
            should be defined in chameleon_audio_ids
        @param source: source widget entity
            should be defined in chameleon_audio_ids
        @param recorder: recorder widget entity
            should be defined in chameleon_audio_ids
        @param is_internal: whether internal audio is tested flag

        """
        if (recorder == chameleon_audio_ids.CrosIds.INTERNAL_MIC and
            not audio_test_utils.has_internal_microphone(host)):
            return

        if (source == chameleon_audio_ids.CrosIds.SPEAKER and
            not audio_test_utils.has_internal_speaker(host)):
            return

        self.host = host
        self.audio_nodes = audio_nodes
        self.golden_file, self.low_pass_freq = golden_data
        chameleon_board = self.host.chameleon
        self.factory = self.create_remote_facade_factory(self.host)
        self.audio_facade = self.factory.create_audio_facade()
        chameleon_board.reset()
        widget_factory = chameleon_audio_helper.AudioWidgetFactory(
                self.factory, host)
        self.audio_board = chameleon_board.get_audio_board()
        self.widget_link = None
        self.use_audio_bus = False

        self.second_peak_ratio = audio_test_utils.DEFAULT_SECOND_PEAK_RATIO
        self.ignore_frequencies = None
        if source == chameleon_audio_ids.CrosIds.SPEAKER:
            self.second_peak_ratio = 0.1
            self.ignore_frequencies = [50, 60]
        elif recorder == chameleon_audio_ids.CrosIds.INTERNAL_MIC:
            self.second_peak_ratio = 0.2

        # Two widgets are binded in the factory if necessary
        binder_widget = None
        bind_from_widget = None
        bind_to_widget = None
        if bind_from != None and bind_to != None:
            bind_from_widget = widget_factory.create_widget(bind_from)
            bind_to_widget = widget_factory.create_widget(bind_to)
            binder_widget = widget_factory.create_binder(bind_from_widget,
                                                         bind_to_widget)
            self.widget_link = binder_widget.get_link()
            if isinstance(self.widget_link, audio_widget_link.AudioBusLink):
                self.use_audio_bus = True

        # Additional widgets that could be part of the factory
        if source == None:
            source_widget = bind_from_widget
        else:
            source_widget = widget_factory.create_widget(source)
        if recorder == None:
            recorder_widget = bind_to_widget
        else:
            recorder_widget = widget_factory.create_widget(recorder)

        # Plug for external audio
        self.action_plug_jack(not is_internal)

        # Play only, reboot, then play and record.
        if binder_widget != None:
            with chameleon_audio_helper.bind_widgets(binder_widget):
                time.sleep(self.DELAY_AFTER_BINDING)
                self.play_reboot_play_and_record(source_widget, recorder_widget)
        else:
            self.play_reboot_play_and_record(source_widget, recorder_widget)

        self.save_and_check_data(recorder_widget)
