# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Feedback implementation for audio with closed-loop cable."""

import logging
import numpy
import os
import tempfile
import wave

import common
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib import site_utils
from autotest_lib.client.common_lib.feedback import client
from autotest_lib.server.brillo import host_utils


# Constants used for updating the audio policy.
#
_DUT_AUDIO_POLICY_PATH = 'system/etc/audio_policy.conf'
_AUDIO_POLICY_ATTACHED_INPUT_DEVICES = 'attached_input_devices'
_AUDIO_POLICY_ATTACHED_OUTPUT_DEVICES = 'attached_output_devices'
_AUDIO_POLICY_DEFAULT_OUTPUT_DEVICE = 'default_output_device'
_WIRED_HEADSET_IN = 'AUDIO_DEVICE_IN_WIRED_HEADSET'
_WIRED_HEADSET_OUT = 'AUDIO_DEVICE_OUT_WIRED_HEADSET'

# Constants used when recording playback.
#
_REC_FILENAME = 'rec_file.wav'
_REC_DURATION = 10

# Number of channels to record.
_DEFAULT_NUM_CHANNELS = 1
# Recording sample rate (48kHz).
_DEFAULT_SAMPLE_RATE = 48000
# Recording sample format is signed 16-bit PCM (two bytes).
_DEFAULT_SAMPLE_WIDTH = 2

# The peak when recording silence is 5% of the max volume.
_SILENCE_THRESHOLD = 0.05

# Thresholds used when comparing files.
#
# The frequency threshold used when comparing files. The frequency of the
# recorded audio has to be within _FREQUENCY_THRESHOLD percent of the frequency
# of the original audio.
_FREQUENCY_THRESHOLD = 0.01
# Noise threshold controls how much noise is allowed as a fraction of the
# magnitude of the peak frequency after taking an FFT. The power of all the
# other frequencies in the signal should be within _FFT_NOISE_THRESHOLD percent
# of the power of the main frequency.
_FFT_NOISE_THRESHOLD = 0.05


def _max_volume(sample_width):
    """Returns the maximum possible volume.

    This is the highest absolute value of an integer of a given width.
    If the sample width is one, then we assume an unsigned intger. For all other
    sample sizes, we assume that the format is signed.

    @param sample_width: The sample width in bytes.
    """
    return (1 << 8) if sample_width == 1 else (1 << (sample_width * 8 - 1))


class Client(client.Client):
    """Audio closed-loop feedback implementation.

    This class (and the queries it instantiates) perform playback and recording
    of audio on the DUT itself, with the assumption that the audio in/out
    connections are cross-wired with a cable. It provides some shared logic
    that queries can use for handling the DUT as well as maintaining shared
    state between queries (such as an audible volume threshold).
    """

    def __init__(self):
        """Construct the client library."""
        super(Client, self).__init__()
        self.host = None
        self.dut_tmp_dir = None
        self.tmp_dir = None
        self.orig_policy = None


    def set_audible_threshold(self, threshold):
        """Sets the audible volume threshold.

        @param threshold: New threshold value.
        """
        self.audible_threshold = threshold


    def _patch_audio_policy(self):
        """Updates the audio_policy.conf file to use the headphone jack.

        Currently, there's no way to update the audio routing if a headset is
        plugged in. This function manually changes the audio routing to play
        through the headset.
        TODO(ralphnathan): Remove this once b/25188354 is resolved.
        """
        # Fetch the DUT's original audio policy.
        _, self.orig_policy = tempfile.mkstemp(dir=self.tmp_dir)
        self.host.get_file(_DUT_AUDIO_POLICY_PATH, self.orig_policy,
                           delete_dest=True)

        # Patch the policy to route audio to a headset.
        _, test_policy = tempfile.mkstemp(dir=self.tmp_dir)
        policy_changed = False
        with open(self.orig_policy) as orig_file:
            with open(test_policy, 'w') as test_file:
                for line in orig_file:
                    if _WIRED_HEADSET_OUT not in line:
                        if _AUDIO_POLICY_ATTACHED_OUTPUT_DEVICES in line:
                            line = '%s|%s\n' % (line.rstrip(),
                                                _WIRED_HEADSET_OUT)
                            policy_changed = True
                        elif _AUDIO_POLICY_DEFAULT_OUTPUT_DEVICE in line:
                            line = '%s %s\n' % (line.rstrip().rsplit(' ', 1)[0],
                                                _WIRED_HEADSET_OUT)
                            policy_changed = True
                    if _WIRED_HEADSET_IN not in line:
                        if _AUDIO_POLICY_ATTACHED_INPUT_DEVICES in line:
                            line = '%s|%s\n' % (line.rstrip(),
                                                _WIRED_HEADSET_IN)
                            policy_changed = True

                    test_file.write(line)

        # Update the DUT's audio policy if changed.
        if policy_changed:
            logging.info('Updating audio policy to route audio to headset')
            self.host.remount()
            self.host.send_file(test_policy, _DUT_AUDIO_POLICY_PATH,
                                delete_dest=True)
            self.host.reboot()
        else:
            os.remove(self.orig_policy)
            self.orig_policy = None

        os.remove(test_policy)


    # Interface overrides.
    #
    def _initialize_impl(self, test, host):
        """Initializes the feedback object.

        @param test: An object representing the test case.
        @param host: An object representing the DUT.
        """
        self.host = host
        self.tmp_dir = test.tmpdir
        self.dut_tmp_dir = host.get_tmp_dir()
        self._patch_audio_policy()


    def _finalize_impl(self):
        """Finalizes the feedback object."""
        if self.orig_policy:
            logging.info('Restoring DUT audio policy')
            self.host.remount()
            self.host.send_file(self.orig_policy, _DUT_AUDIO_POLICY_PATH,
                                delete_dest=True)
            os.remove(self.orig_policy)
            self.orig_policy = None


    def _new_query_impl(self, query_id):
        """Instantiates a new query.

        @param query_id: A query identifier.

        @return A query object.

        @raise error.TestError: Query is not supported.
        """
        if query_id == client.QUERY_AUDIO_PLAYBACK_SILENT:
            return SilentPlaybackAudioQuery(self)
        elif query_id == client.QUERY_AUDIO_PLAYBACK_AUDIBLE:
            return AudiblePlaybackAudioQuery(self)
        elif query_id == client.QUERY_AUDIO_RECORDING:
            return RecordingAudioQuery(self)
        else:
            raise error.TestError('Unsupported query (%s)' % query_id)


class _PlaybackAudioQuery(client.OutputQuery):
    """Playback query base class."""

    def __init__(self, client):
        """Constructor.

        @param client: The instantiating client object.
        """
        super(_PlaybackAudioQuery, self).__init__()
        self.client = client
        self.dut_rec_filename = None
        self.local_tmp_dir = None
        self.recording_pid = None


    def _get_local_rec_filename(self):
        """Waits for recording to finish and copies the file to the host.

        @return A string of the local filename containing the recorded audio.

        @raise error.TestError: Error while validating the recording.
        """
        # Wait for recording to finish.
        timeout = _REC_DURATION + 5
        if not host_utils.wait_for_process(self.client.host,
                                           self.recording_pid, timeout):
            raise error.TestError(
                    'Recording did not terminate within %d seconds' % timeout)

        _, local_rec_filename = tempfile.mkstemp(
                prefix='recording-', suffix='.wav', dir=self.local_tmp_dir)
        self.client.host.get_file(self.dut_rec_filename,
                                  local_rec_filename, delete_dest=True)
        return local_rec_filename


    # Implementation overrides.
    #
    def _prepare_impl(self,
                      sample_width=_DEFAULT_SAMPLE_WIDTH,
                      sample_rate=_DEFAULT_SAMPLE_RATE,
                      num_channels=_DEFAULT_NUM_CHANNELS,
                      duration_secs=_REC_DURATION):
        """Implementation of query preparation logic.

        @sample_width: Sample width to record at.
        @sample_rate: Sample rate to record at.
        @num_channels: Number of channels to record at.
        @duration_secs: Duration (in seconds) to record for.
        """
        self.num_channels = num_channels
        self.sample_rate = sample_rate
        self.sample_width = sample_width
        self.dut_rec_filename = os.path.join(self.client.dut_tmp_dir,
                                             _REC_FILENAME)
        self.local_tmp_dir = tempfile.mkdtemp(dir=self.client.tmp_dir)

        # Trigger recording in the background.
        # TODO(garnold) Remove 'su root' once b/25663983 is resolved.
        cmd = ('su root slesTest_recBuffQueue -c%d -d%d -r%d -%d %s' %
               (num_channels, duration_secs, sample_rate, sample_width,
                self.dut_rec_filename))
        logging.info("Recording cmd: %s", cmd)
        self.recording_pid = host_utils.run_in_background(self.client.host, cmd)


class SilentPlaybackAudioQuery(_PlaybackAudioQuery):
    """Implementation of a silent playback query."""

    def __init__(self, client):
        super(SilentPlaybackAudioQuery, self).__init__(client)


    # Implementation overrides.
    #
    def _validate_impl(self):
        """Implementation of query validation logic."""
        local_rec_filename = self._get_local_rec_filename()
        try:
              silence_peaks = site_utils.check_wav_file(
                      local_rec_filename,
                      num_channels=self.num_channels,
                      sample_rate=self.sample_rate,
                      sample_width=self.sample_width)
        except ValueError as e:
            raise error.TestFail('Invalid file attributes: %s' % e)

        silence_peak = max(silence_peaks)
        # Fail if the silence peak volume exceeds the maximum allowed.
        max_vol = _max_volume(self.sample_width) * _SILENCE_THRESHOLD
        if silence_peak > max_vol:
            logging.error('Silence peak level (%d) exceeds the max allowed '
                          '(%d)', silence_peak, max_vol)
            raise error.TestFail('Environment is too noisy')

        # Update the client audible threshold, if so instructed.
        audible_threshold = silence_peak * 15
        logging.info('Silent peak level (%d) is below the max allowed (%d); '
                     'setting audible threshold to %d',
                     silence_peak, max_vol, audible_threshold)
        self.client.set_audible_threshold(audible_threshold)


class AudiblePlaybackAudioQuery(_PlaybackAudioQuery):
    """Implementation of an audible playback query."""

    def __init__(self, client):
        super(AudiblePlaybackAudioQuery, self).__init__(client)


    def _check_peaks(self):
        """Ensure that peak recording volume exceeds the threshold."""
        local_rec_filename = self._get_local_rec_filename()
        try:
              audible_peaks = site_utils.check_wav_file(
                      local_rec_filename,
                      num_channels=self.num_channels,
                      sample_rate=self.sample_rate,
                      sample_width=self.sample_width)
        except ValueError as e:
            raise error.TestFail('Invalid file attributes: %s' % e)

        min_channel, min_audible_peak = min(enumerate(audible_peaks),
                                            key=lambda p: p[1])
        if min_audible_peak < self.client.audible_threshold:
            logging.error(
                    'Audible peak level (%d) is less than expected (%d) for '
                    'channel %d', min_audible_peak,
                    self.client.audible_threshold, min_channel)
            raise error.TestFail(
                    'The played audio peak level is below the expected '
                    'threshold. Either playback did not work, or the volume '
                    'level is too low. Check the audio connections and '
                    'settings on the DUT.')

        logging.info('Audible peak level (%d) exceeds the threshold (%d)',
                     min_audible_peak, self.client.audible_threshold)


    def _is_outside_frequency_threshold(self, freq_golden, freq_rec):
        """Compares the frequency of the recorded audio with the golden audio.

        This function checks to see if the frequencies corresponding to the peak
        FFT values are similiar meaning that the dominant frequency in the audio
        signal is the same for the recorded audio as that in the audio played.

        @freq_golden: The dominant frequency in the reference audio file.
        @freq_rec: The dominant frequency in the recorded audio file.

        @returns: True is freq_rec is with _FREQUENCY_THRESHOLD percent of
                  freq_golden.
        """
        ratio = float(freq_rec) / freq_golden
        if ratio > 1 + _FREQUENCY_THRESHOLD or ratio < 1 - _FREQUENCY_THRESHOLD:
            return True
        return False


    def _compare_file(self, audio_file):
        """Compares the recorded audio file to the golden audio file.

        This method checks for two things:
          1. That the main frequency is the same in both the files. This is done
             using the FFT and observing the frequency corresponding to the
             peak.
          2. That there is no other dominant frequency in the recorded file.
             This is done by sweeping the frequency domain and checking that the
             frequency is always less than _FFT_NOISE_THRESHOLD percentage of
             the peak.

        The key assumption here is that the reference audio file contains only
        one frequency.

        @param audio_file: Reference audio file containing the golden signal.

        @raise error.TestFail: The frequency of the recorded signal doesn't
                               match that of the golden signal.
        @raise error.TestFail: There is too much noise in the recorded signal.
        """
        local_rec_filename = self._get_local_rec_filename()

        # Open both files and extract data.
        golden_file = wave.open(audio_file, 'rb')
        golden_file_frames = site_utils.extract_wav_frames(golden_file)
        rec_file = wave.open(local_rec_filename, 'rb')
        rec_file_frames = site_utils.extract_wav_frames(rec_file)

        num_channels = golden_file.getnchannels()
        for channel in range(num_channels):
            golden_data = golden_file_frames[channel::num_channels]
            rec_data = rec_file_frames[channel::num_channels]

            # Get fft and frequencies corresponding to the fft values.
            fft_golden = numpy.fft.rfft(golden_data)
            fft_rec = numpy.fft.rfft(rec_data)
            fft_freqs_golden = numpy.fft.rfftfreq(
                    len(golden_data), 1.0 / golden_file.getframerate())
            fft_freqs_rec = numpy.fft.rfftfreq(len(rec_data),
                                               1.0 / rec_file.getframerate())

            # Get frequency at highest peak.
            freq_golden = fft_freqs_golden[numpy.argmax(numpy.abs(fft_golden))]
            abs_fft_rec = numpy.abs(fft_rec)
            freq_rec = fft_freqs_rec[numpy.argmax(abs_fft_rec)]

            # Compare the two frequencies.
            logging.info('Golden frequency = %f', freq_golden)
            logging.info('Recorded frequency = %f', freq_rec)
            if self._is_outside_frequency_threshold(freq_golden, freq_rec):
                raise error.TestFail('The recorded audio frequency does not '
                                     'match that of the audio played.')

            # Check for noise in the frequency domain.
            fft_rec_peak_val = numpy.max(abs_fft_rec)
            noise_detected = False
            for fft_index, fft_val in enumerate(abs_fft_rec):
                if self._is_outside_frequency_threshold(freq_golden, freq_rec):
                    # If the frequency exceeds _FFT_NOISE_THRESHOLD, then fail
                    # the test.
                    if fft_val > _FFT_NOISE_THRESHOLD * fft_rec_peak_val:
                        logging.warning('Unexpected frequency peak detected at '
                                        '%f Hz.', fft_freqs_rec[fft_index])
                        noise_detected = True

            if noise_detected:
                raise error.TestFail('Signal is noiser than expected.')


    # Implementation overrides.
    #
    def _validate_impl(self, audio_file=None):
        """Implementation of query validation logic.

        @audio_file: File to compare recorded audio to.
        """
        self._check_peaks()
        # If the reference audio file is available, then perform an additional
        # check.
        if audio_file:
            self._compare_file(audio_file)


class RecordingAudioQuery(client.InputQuery):
    """Implementation of a recording query."""

    def __init__(self, client):
        super(RecordingAudioQuery, self).__init__()
        self.client = client


    def _prepare_impl(self, **kwargs):
        """Implementation of query preparation logic (no-op)."""
        pass


    def _emit_impl(self):
        """Implementation of query emission logic."""
        self.client.host.run('slesTest_sawtoothBufferQueue')


    def _validate_impl(self, captured_audio_file, sample_width,
                       sample_rate=None, num_channels=None,
                       peak_percent_min=1, peak_percent_max=100):
        """Implementation of query validation logic.

        @param captured_audio_file: Path to the recorded WAV file.
        @peak_percent_min: Lower bound on peak recorded volume as percentage of
            max molume (0-100). Default is 1%.
        @peak_percent_max: Upper bound on peak recorded volume as percentage of
            max molume (0-100). Default is 100% (no limit).
        """
        # TODO(garnold) Currently, we just test whether anything audible was
        # recorded. We should compare the captured audio to the one produced.
        try:
            recorded_peaks = site_utils.check_wav_file(
                    captured_audio_file, num_channels=num_channels,
                    sample_rate=sample_rate, sample_width=sample_width)
        except ValueError as e:
            raise error.TestFail('Recorded audio file is invalid: %s' % e)

        max_volume = _max_volume(sample_width)
        peak_min = max_volume * peak_percent_min / 100
        peak_max = max_volume * peak_percent_max / 100
        for channel, recorded_peak in enumerate(recorded_peaks):
            if recorded_peak < peak_min:
                logging.error(
                        'Recorded audio peak level (%d) is less than expected '
                        '(%d) for channel %d', recorded_peak, peak_min, channel)
                raise error.TestFail(
                        'The recorded audio peak level is below the expected '
                        'threshold. Either recording did not capture the '
                        'produced audio, or the recording level is too low. '
                        'Check the audio connections and settings on the DUT.')

            if recorded_peak > peak_max:
                logging.error(
                        'Recorded audio peak level (%d) is more than expected '
                        '(%d) for channel %d', recorded_peak, peak_max, channel)
                raise error.TestFail(
                        'The recorded audio peak level exceeds the expected '
                        'maximum. Either recording captured much background '
                        'noise, or the recording level is too high. Check the '
                        'audio connections and settings on the DUT.')
