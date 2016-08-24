# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""This module provides the test utilities for audio tests using chameleon."""

# TODO (cychiang) Move test utilities from chameleon_audio_helpers
# to this module.

import logging
import multiprocessing
import os
import time
from contextlib import contextmanager

from autotest_lib.client.common_lib import error
from autotest_lib.client.cros import constants
from autotest_lib.client.cros.audio import audio_analysis
from autotest_lib.client.cros.audio import audio_data

def check_audio_nodes(audio_facade, audio_nodes):
    """Checks the node selected by Cros device is correct.

    @param audio_facade: A RemoteAudioFacade to access audio functions on
                         Cros device.

    @param audio_nodes: A tuple (out_audio_nodes, in_audio_nodes) containing
                        expected selected output and input nodes.

    @raises: error.TestFail if the nodes selected by Cros device are not expected.

    """
    curr_out_nodes, curr_in_nodes = audio_facade.get_selected_node_types()
    out_audio_nodes, in_audio_nodes = audio_nodes
    if (in_audio_nodes != None and
        sorted(curr_in_nodes) != sorted(in_audio_nodes)):
        raise error.TestFail('Wrong input node(s) selected %s '
                'instead %s!' % (str(curr_in_nodes), str(in_audio_nodes)))
    if (out_audio_nodes != None and
        sorted(curr_out_nodes) != sorted(out_audio_nodes)):
        raise error.TestFail('Wrong output node(s) selected %s '
                'instead %s!' % (str(curr_out_nodes), str(out_audio_nodes)))


def check_plugged_nodes(audio_facade, audio_nodes):
    """Checks the nodes that are currently plugged on Cros device are correct.

    @param audio_facade: A RemoteAudioFacade to access audio functions on
                         Cros device.

    @param audio_nodes: A tuple (out_audio_nodes, in_audio_nodes) containing
                        expected plugged output and input nodes.

    @raises: error.TestFail if the plugged nodes on Cros device are not expected.

    """
    curr_out_nodes, curr_in_nodes = audio_facade.get_plugged_node_types()
    out_audio_nodes, in_audio_nodes = audio_nodes
    if (in_audio_nodes != None and
        sorted(curr_in_nodes) != sorted(in_audio_nodes)):
        raise error.TestFail('Wrong input node(s) plugged %s '
                'instead %s!' % (str(curr_in_nodes), str(in_audio_nodes)))
    if (out_audio_nodes != None and
        sorted(curr_out_nodes) != sorted(out_audio_nodes)):
        raise error.TestFail('Wrong output node(s) plugged %s '
                'instead %s!' % (str(curr_out_nodes), str(out_audio_nodes)))


def bluetooth_nodes_plugged(audio_facade):
    """Checks bluetooth nodes are plugged.

    @param audio_facade: A RemoteAudioFacade to access audio functions on
                         Cros device.

    @raises: error.TestFail if either input or output bluetooth node is
             not plugged.

    """
    curr_out_nodes, curr_in_nodes = audio_facade.get_plugged_node_types()
    return 'BLUETOOTH' in curr_out_nodes and 'BLUETOOTH' in curr_in_nodes


def _get_board_name(host):
    """Gets the board name.

    @param host: The CrosHost object.

    @returns: The board name.

    """
    return host.get_board().split(':')[1]


def correction_plug_unplug_for_audio(host, port):
    """Plugs/unplugs several times for Cros device to detect audio.

    For issue crbug.com/450101, Exynos HDMI driver has problem recognizing
    HDMI audio, while display can be detected. Do several plug/unplug and wait
    as a workaround. Note that port will be in unplugged state in the end if
    extra plug/unplug is needed.

    @param host: A CrosHost object.
    @param port: A ChameleonVideoInput object.

    """
    board = _get_board_name(host)
    if board in ['peach_pit', 'peach_pi', 'daisy', 'daisy_spring',
                 'daisy_skate']:
        logging.info('Need extra plug/unplug on board %s', board)
        for _ in xrange(3):
            port.plug()
            time.sleep(3)
            port.unplug()
            time.sleep(3)


def has_internal_speaker(host):
    """Checks if the Cros device has speaker.

    @param host: The CrosHost object.

    @returns: True if Cros device has internal speaker. False otherwise.

    """
    board_name = _get_board_name(host)
    if host.get_board_type() == 'CHROMEBOX' and board_name != 'stumpy':
        logging.info('Board %s does not have speaker.', board_name)
        return False
    return True


def has_internal_microphone(host):
    """Checks if the Cros device has internal microphone.

    @param host: The CrosHost object.

    @returns: True if Cros device has internal microphone. False otherwise.

    """
    board_name = _get_board_name(host)
    if host.get_board_type() == 'CHROMEBOX':
        logging.info('Board %s does not have internal microphone.', board_name)
        return False
    return True


def suspend_resume(host, suspend_time_secs, resume_network_timeout_secs=50):
    """Performs the suspend/resume on Cros device.

    @param suspend_time_secs: Time in seconds to let Cros device suspend.
    @resume_network_timeout_secs: Time in seconds to let Cros device resume and
                                  obtain network.
    """
    def action_suspend():
        """Calls the host method suspend."""
        host.suspend(suspend_time=suspend_time_secs)

    boot_id = host.get_boot_id()
    proc = multiprocessing.Process(target=action_suspend)
    logging.info("Suspending...")
    proc.daemon = True
    proc.start()
    host.test_wait_for_sleep(suspend_time_secs / 3)
    logging.info("DUT suspended! Waiting to resume...")
    host.test_wait_for_resume(
            boot_id, suspend_time_secs + resume_network_timeout_secs)
    logging.info("DUT resumed!")


def dump_cros_audio_logs(host, audio_facade, directory, suffix=''):
    """Dumps logs for audio debugging from Cros device.

    @param host: The CrosHost object.
    @param audio_facade: A RemoteAudioFacade to access audio functions on
                         Cros device.
    @directory: The directory to dump logs.

    """
    def get_file_path(name):
        """Gets file path to dump logs.

        @param name: The file name.

        @returns: The file path with an optional suffix.

        """
        file_name = '%s.%s' % (name, suffix) if suffix else name
        file_path = os.path.join(directory, file_name)
        return file_path

    audio_facade.dump_diagnostics(get_file_path('audio_diagnostics.txt'))

    host.get_file('/var/log/messages', get_file_path('messages'))

    host.get_file(constants.MULTIMEDIA_XMLRPC_SERVER_LOG_FILE,
                  get_file_path('multimedia_xmlrpc_server.log'))


@contextmanager
def monitor_no_nodes_changed(audio_facade, callback=None):
    """Context manager to monitor nodes changed signal on Cros device.

    Starts the counter in the beginning. Stops the counter in the end to make
    sure there is no NodesChanged signal during the try block.

    E.g. with monitor_no_nodes_changed(audio_facade):
             do something on playback/recording

    @param audio_facade: A RemoteAudioFacade to access audio functions on
                         Cros device.
    @param fail_callback: The callback to call before raising TestFail
                          when there is unexpected NodesChanged signals.

    @raises: error.TestFail if there is NodesChanged signal on
             Cros device during the context.

    """
    try:
        audio_facade.start_counting_signal('NodesChanged')
        yield
    finally:
        count = audio_facade.stop_counting_signal()
        if count:
            message = 'Got %d unexpected NodesChanged signal' % count
            logging.error(message)
            if callback:
                callback()
            raise error.TestFail(message)


# The second dominant frequency should have energy less than -26dB of the
# first dominant frequency in the spectrum.
DEFAULT_SECOND_PEAK_RATIO = 0.05

# Tolerate more for bluetooth audio using HSP.
HSP_SECOND_PEAK_RATIO = 0.2

# The deviation of estimated dominant frequency from golden frequency.
DEFAULT_FREQUENCY_DIFF_THRESHOLD = 5

def check_recorded_frequency(
        golden_file, recorder,
        second_peak_ratio=DEFAULT_SECOND_PEAK_RATIO,
        frequency_diff_threshold=DEFAULT_FREQUENCY_DIFF_THRESHOLD,
        ignore_frequencies=None, check_anomaly=False):
    """Checks if the recorded data contains sine tone of golden frequency.

    @param golden_file: An AudioTestData object that serves as golden data.
    @param recorder: An AudioWidget used in the test to record data.
    @param second_peak_ratio: The test fails when the second dominant
                              frequency has coefficient larger than this
                              ratio of the coefficient of first dominant
                              frequency.
    @param frequency_diff_threshold: The maximum difference between estimated
                                     frequency of test signal and golden
                                     frequency. This value should be small for
                                     signal passed through line.
    @param ignore_frequencies: A list of frequencies to be ignored. The
                               component in the spectral with frequency too
                               close to the frequency in the list will be
                               ignored. The comparison of frequencies uses
                               frequency_diff_threshold as well.
    @param check_anomaly: True to check anomaly in the signal.

    @raises error.TestFail if the recorded data does not contain sine tone of
            golden frequency.

    """
    data_format = recorder.data_format
    recorded_data = audio_data.AudioRawData(
            binary=recorder.get_binary(),
            channel=data_format['channel'],
            sample_format=data_format['sample_format'])

    errors = []

    for test_channel, golden_channel in enumerate(recorder.channel_map):
        if golden_channel is None:
            logging.info('Skipped channel %d', test_channel)
            continue

        signal = recorded_data.channel_data[test_channel]
        saturate_value = audio_data.get_maximum_value_from_sample_format(
                data_format['sample_format'])
        normalized_signal = audio_analysis.normalize_signal(
                signal, saturate_value)
        spectral = audio_analysis.spectral_analysis(
                normalized_signal, data_format['rate'])

        if not spectral:
            errors.append(
                    'Channel %d: Can not find dominant frequency.' %
                            test_channel)

        golden_frequency = golden_file.frequencies[golden_channel]
        logging.debug('Checking channel %s spectral %s against frequency %s',
                test_channel, spectral, golden_frequency)

        dominant_frequency = spectral[0][0]

        if (abs(dominant_frequency - golden_frequency) >
            frequency_diff_threshold):
            errors.append(
                    'Channel %d: Dominant frequency %s is away from golden %s' %
                    (test_channel, dominant_frequency, golden_frequency))

        if check_anomaly:
            detected_anomaly = audio_analysis.anomaly_detection(
                    signal=normalized_signal,
                    rate=data_format['rate'],
                    freq=golden_frequency)
            if detected_anomaly:
                errors.append(
                        'Channel %d: Detect anomaly near these time: %s' %
                        (test_channel, detected_anomaly))
            else:
                logging.info(
                        'Channel %d: Quality is good as there is no anomaly',
                        test_channel)


        def should_be_ignored(frequency):
            """Checks if frequency is close to any frequency in ignore list.

            @param frequency: The frequency to be tested.

            @returns: True if the frequency should be ignored. False otherwise.

            """
            for ignore_frequency in ignore_frequencies:
                if (abs(frequency - ignore_frequency) <
                    frequency_diff_threshold):
                    logging.debug('Ignore frequency: %s', frequency)
                    return True

        # Filter out the frequencies to be ignored.
        if ignore_frequencies:
            spectral = [x for x in spectral if not should_be_ignored(x[0])]

        if len(spectral) > 1:
            first_coeff = spectral[0][1]
            second_coeff = spectral[1][1]
            if second_coeff > first_coeff * second_peak_ratio:
                errors.append(
                        'Channel %d: Found large second dominant frequencies: '
                        '%s' % (test_channel, spectral))

    if errors:
        raise error.TestFail(', '.join(errors))
