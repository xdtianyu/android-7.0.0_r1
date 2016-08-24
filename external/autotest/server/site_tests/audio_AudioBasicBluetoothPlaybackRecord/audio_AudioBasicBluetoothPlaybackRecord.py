# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""
This is a server side bluetooth playback/record test using the Chameleon
board.
"""

import logging
import os
import time, threading

from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.cros.audio import audio_test_data
from autotest_lib.client.cros.chameleon import audio_test_utils
from autotest_lib.client.cros.chameleon import chameleon_audio_helper
from autotest_lib.client.cros.chameleon import chameleon_audio_ids
from autotest_lib.server.cros.audio import audio_test


class audio_AudioBasicBluetoothPlaybackRecord(audio_test.AudioTest):
    """Server side bluetooth playback/record audio test.

    This test talks to a Chameleon board and a Cros device to verify
    bluetooth playback/record audio function of the Cros device.

    """
    version = 1
    DELAY_AFTER_DISABLING_MODULE_SECONDS = 30
    DELAY_AFTER_DISCONNECT_SECONDS = 5
    DELAY_AFTER_ENABLING_MODULE_SECONDS = 10
    DELAY_AFTER_RECONNECT_SECONDS = 5
    DELAY_BEFORE_RECORD_SECONDS = 2
    RECORD_SECONDS = 5
    SLEEP_AFTER_RECORD_SECONDS = 5
    SUSPEND_SECONDS = 30
    PRC_RECONNECT_TIMEOUT = 60
    BLUETOOTH_RECONNECT_TIMEOUT_SECS = 30

    def disconnect_connect_bt(self, link):
        """Performs disconnect and connect BT module

        @param link: binder link to control BT adapter

        """

        logging.info("Disconnecting BT module...")
        link.adapter_disconnect_module()
        time.sleep(self.DELAY_AFTER_DISCONNECT_SECONDS)
        audio_test_utils.check_audio_nodes(self.audio_facade,
                                           (['INTERNAL_SPEAKER'],
                                            ['INTERNAL_MIC']))
        logging.info("Connecting BT module...")
        link.adapter_connect_module()
        time.sleep(self.DELAY_AFTER_RECONNECT_SECONDS)


    def disable_enable_bt(self, link):
        """Performs turn off and then on BT module

        @param link: binder playback link to control BT adapter

        """

        logging.info("Turning off BT module...")
        link.disable_bluetooth_module()
        time.sleep(self.DELAY_AFTER_DISABLING_MODULE_SECONDS)
        audio_test_utils.check_audio_nodes(self.audio_facade,
                                           (['INTERNAL_SPEAKER'],
                                            ['INTERNAL_MIC']))
        logging.info("Turning on BT module...")
        link.enable_bluetooth_module()
        time.sleep(self.DELAY_AFTER_ENABLING_MODULE_SECONDS)
        logging.info("Connecting BT module...")
        link.adapter_connect_module()
        time.sleep(self.DELAY_AFTER_RECONNECT_SECONDS)


    def bluetooth_nodes_plugged(self):
        """Checks if bluetooth nodes are plugged.

        @returns: True if bluetooth nodes are plugged. False otherwise.

        """
        return audio_test_utils.bluetooth_nodes_plugged(self.audio_facade)


    def dump_logs_after_nodes_changed(self):
        """Dumps the log after unexpected NodesChanged signal happens."""
        audio_test_utils.dump_cros_audio_logs(
                self.host, self.audio_facade, self.resultsdir,
                'after_nodes_changed')


    def run_once(self, host, suspend=False,
                 disable=False, disconnect=False, check_quality=False):
        """Running Bluetooth basic audio tests

        @param host: device under test host
        @param suspend: suspend flag to enable suspend before play/record
        @param disable: disable flag to disable BT module before play/record
        @param disconnect: disconnect flag to disconnect BT module
            before play/record
        @param check_quality: flag to check audio quality.

        """
        self.host = host

        # Bluetooth HSP/HFP profile only supports one channel
        # playback/recording. So we should use simple frequency
        # test file which contains identical sine waves in two
        # channels.
        golden_file = audio_test_data.SIMPLE_FREQUENCY_TEST_FILE

        factory = self.create_remote_facade_factory(host)
        self.audio_facade = factory.create_audio_facade()

        chameleon_board = host.chameleon
        chameleon_board.reset()

        widget_factory = chameleon_audio_helper.AudioWidgetFactory(
                factory, host)

        playback_source = widget_factory.create_widget(
            chameleon_audio_ids.CrosIds.BLUETOOTH_HEADPHONE)
        playback_bluetooth_widget = widget_factory.create_widget(
            chameleon_audio_ids.PeripheralIds.BLUETOOTH_DATA_RX)
        playback_recorder = widget_factory.create_widget(
            chameleon_audio_ids.ChameleonIds.LINEIN)
        playback_binder = widget_factory.create_binder(
                playback_source, playback_bluetooth_widget, playback_recorder)

        record_source = widget_factory.create_widget(
            chameleon_audio_ids.ChameleonIds.LINEOUT)
        record_bluetooth_widget = widget_factory.create_widget(
            chameleon_audio_ids.PeripheralIds.BLUETOOTH_DATA_TX)
        record_recorder = widget_factory.create_widget(
            chameleon_audio_ids.CrosIds.BLUETOOTH_MIC)
        record_binder = widget_factory.create_binder(
                record_source, record_bluetooth_widget, record_recorder)

        with chameleon_audio_helper.bind_widgets(playback_binder):
            with chameleon_audio_helper.bind_widgets(record_binder):

                audio_test_utils.dump_cros_audio_logs(
                        host, self.audio_facade, self.resultsdir,
                        'after_binding')

                # Checks the input node selected by Cras is internal microphone.
                # Checks crbug.com/495537 for the reason to lower bluetooth
                # microphone priority.
                if audio_test_utils.has_internal_microphone(host):
                    audio_test_utils.check_audio_nodes(self.audio_facade,
                                                       (None, ['INTERNAL_MIC']))

                self.audio_facade.set_selected_output_volume(80)

                # Selecting input nodes needs to be after setting volume because
                # after setting volume, Cras notifies Chrome there is changes
                # in nodes, and Chrome selects the output/input nodes based
                # on its preference again. See crbug.com/535643.

                # Selects bluetooth mic to be the active input node.
                if audio_test_utils.has_internal_microphone(host):
                    self.audio_facade.set_chrome_active_node_type(
                            None, 'BLUETOOTH')

                # Checks the node selected by Cras is correct.
                audio_test_utils.check_audio_nodes(self.audio_facade,
                                                   (['BLUETOOTH'],
                                                    ['BLUETOOTH']))

                # Setup the playback data. This step is time consuming.
                playback_source.set_playback_data(golden_file)
                record_source.set_playback_data(golden_file)

                # Create links to control disconnect and off/on BT adapter.
                link = playback_binder.get_binders()[0].get_link()

                if disable:
                    self.disable_enable_bt(link)
                if disconnect:
                    self.disconnect_connect_bt(link)
                if suspend:
                    audio_test_utils.suspend_resume(host, self.SUSPEND_SECONDS)

                if disable or disconnect or suspend:
                    audio_test_utils.dump_cros_audio_logs(
                            host, self.audio_facade, self.resultsdir,
                            'after_action')

                utils.poll_for_condition(condition=factory.ready,
                                         timeout=self.PRC_RECONNECT_TIMEOUT,
                                         desc='multimedia server reconnect')

                # Gives DUT some time to auto-reconnect bluetooth after resume.
                if suspend:
                    utils.poll_for_condition(
                            condition=self.bluetooth_nodes_plugged,
                            timeout=self.BLUETOOTH_RECONNECT_TIMEOUT_SECS,
                            desc='bluetooth node auto-reconnect after suspend')

                if audio_test_utils.has_internal_microphone(host):
                    # Select again BT input, as default input node is
                    # INTERNAL_MIC.
                    self.audio_facade.set_chrome_active_node_type(
                            None, 'BLUETOOTH')

                with audio_test_utils.monitor_no_nodes_changed(
                        self.audio_facade, self.dump_logs_after_nodes_changed):
                    # Checks the node selected by Cras is correct again.
                    audio_test_utils.check_audio_nodes(self.audio_facade,
                                                       (['BLUETOOTH'],
                                                        ['BLUETOOTH']))

                    # Starts playing, waits for some time, and then starts
                    # recording. This is to avoid artifact caused by codec
                    # initialization.
                    logging.info('Start playing %s on Cros device',
                                 golden_file.path)
                    playback_source.start_playback()
                    logging.info('Start playing %s on Chameleon device',
                                 golden_file.path)
                    record_source.start_playback()

                    time.sleep(self.DELAY_BEFORE_RECORD_SECONDS)
                    logging.info('Start recording from Chameleon.')
                    playback_recorder.start_recording()
                    logging.info('Start recording from Cros device.')
                    record_recorder.start_recording()

                    time.sleep(self.RECORD_SECONDS)

                    playback_recorder.stop_recording()
                    logging.info('Stopped recording from Chameleon.')
                    record_recorder.stop_recording()
                    logging.info('Stopped recording from Cros device.')

                    audio_test_utils.dump_cros_audio_logs(
                            host, self.audio_facade, self.resultsdir,
                            'after_recording')

                    # Sleeps until playback data ends to prevent audio from
                    # going to internal speaker.
                    time.sleep(self.SLEEP_AFTER_RECORD_SECONDS)

                    # Gets the recorded data. This step is time consuming.
                    playback_recorder.read_recorded_binary()
                    logging.info('Read recorded binary from Chameleon.')
                    record_recorder.read_recorded_binary()
                    logging.info('Read recorded binary from Chameleon.')

                    recorded_file = os.path.join(
                            self.resultsdir, "playback_recorded.raw")
                    logging.info('Playback: Saving recorded data to %s',
                                 recorded_file)
                    playback_recorder.save_file(recorded_file)
                    recorded_file = os.path.join(
                            self.resultsdir, "record_recorded.raw")
                    logging.info('Record: Saving recorded data to %s',
                                  recorded_file)
                    record_recorder.save_file(recorded_file)

        # Removes the beginning of recorded data. This is to avoid artifact
        # caused by Chameleon codec initialization in the beginning of
        # recording.
        playback_recorder.remove_head(0.5)

        # Removes the beginning of recorded data. This is to avoid artifact
        # caused by bluetooth module initialization in the beginning of
        # its playback.
        record_recorder.remove_head(0.5)

        recorded_file = os.path.join(self.resultsdir, "playback_clipped.raw")
        logging.info('Saving clipped data to %s', recorded_file)
        playback_recorder.save_file(recorded_file)

        recorded_file = os.path.join(self.resultsdir, "record_clipped.raw")
        logging.info('Saving clipped data to %s', recorded_file)
        record_recorder.save_file(recorded_file)

        # Compares data by frequency. Audio signal recorded by microphone has
        # gone through analog processing and through the air.
        # This suffers from codec artifacts and noise on the path.
        # Comparing data by frequency is more robust than comparing by
        # correlation, which is suitable for fully-digital audio path like USB
        # and HDMI.
        # Use a second peak ratio that can tolerate more noise because HSP
        # is low-quality.
        second_peak_ratio = audio_test_utils.HSP_SECOND_PEAK_RATIO

        error_messages = ''
        try:
            audio_test_utils.check_recorded_frequency(
                    golden_file, playback_recorder, check_anomaly=check_quality,
                    second_peak_ratio=second_peak_ratio)
        except error.TestFail, e:
            error_messages += str(e)

        try:
            audio_test_utils.check_recorded_frequency(
                    golden_file, record_recorder, check_anomaly=check_quality,
                    second_peak_ratio=second_peak_ratio)
        except error.TestFail, e:
            error_messages += str(e)

        if error_messages:
            raise error.TestFail(error_messages)
