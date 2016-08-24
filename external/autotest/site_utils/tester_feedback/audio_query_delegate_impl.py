# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Audio query delegates."""

import subprocess

import common
from autotest_lib.client.common_lib import site_utils
from autotest_lib.client.common_lib.feedback import client
from autotest_lib.client.common_lib.feedback import tester_feedback_client

import input_handlers
import query_delegate
import sequenced_request


# Supported WAVE playback commands in decreasing order of preference.
_KNOWN_WAV_PLAYBACK_METHODS = (
        # Alsa command-line tool, most straightforward to use (if available).
        ('aplay', ('aplay', '%(file)s')),
        # Sox's play command.
        ('play', ('play', '-q', '%(file)s')),
        # VLC command-line tool.
        ('cvlc', ('cvlc', '-q', '--play-and-exit', '%(file)s')),
        # Mplayer; might choke when using Alsa and therefore least preferred.
        ('mplayer', ('mplayer', '-quiet', '-novideo', '%(file)s')),
)


class PlaybackMixin(object):
    """Mixin for adding playback capabilities to a query."""

    # TODO(garnold) The provided audio file path is local to the test host,
    # which isn't necessarily the same as the host running the feedback
    # service. To support other use cases (Moblab, client-side testing) we'll
    # need to properly identify such cases and fetch the file (b/26927734).
    def _playback_wav_file(self, msg, audio_file):
        """Plays a WAV file via user selected method.

        Looks for available playback commands and presents them to the user to
        choose from. Also lists "manual playback" as the last option.

        @param msg: Introductory message to present to the user.
        @param audio_file: The audio file to play.

        @return: Whether playback was successful.
        """
        choices = []
        cmds = []
        for tool, cmd in _KNOWN_WAV_PLAYBACK_METHODS:
            if site_utils.which(tool):
                choices.append(tool)
                cmds.append(cmd)
        choices.append('Manual playback')

        msg += (' The audio file is %s. Available playback methods include:' %
                audio_file)
        req = sequenced_request.SequencedFeedbackRequest(self.test, self.dut,
                                                         None)
        req.append_question(
                msg,
                input_handlers.MultipleChoiceInputHandler(choices, default=1),
                prompt='Choose your playback method')
        idx, _ = self._process_request(req)
        if idx < len(choices) - 1:
            cmd = [tok % {'file': audio_file} for tok in cmds[idx]]
            return subprocess.call(cmd) == 0

        return True


class AudiblePlaybackQueryDelegate(query_delegate.OutputQueryDelegate,
                                   PlaybackMixin):
    """Query delegate for validating audible feedback."""

    def _prepare_impl(self):
        """Prepare for audio playback (interface override)."""
        req = sequenced_request.SequencedFeedbackRequest(
                self.test, self.dut, 'Audible playback')
        req.append_question(
                'Device %(dut)s will play a short audible sample. Please '
                'prepare for listening to this playback and hit Enter to '
                'continue...',
                input_handlers.PauseInputHandler())
        self._process_request(req)


    def _validate_impl(self, audio_file=None):
        """Validate playback (interface override).

        @param audio_file: Name of audio file on the test host to validate
                           against.
        """
        req = sequenced_request.SequencedFeedbackRequest(
                self.test, self.dut, None)
        msg = 'Playback finished on %(dut)s.'
        if audio_file is None:
            req.append_question(
                    msg, input_handlers.YesNoInputHandler(default=True),
                    prompt='Did you hear audible sound?')
            err_msg = 'User did not hear audible feedback'
        else:
            if not self._playback_wav_file(msg, audio_file):
                return (tester_feedback_client.QUERY_RET_ERROR,
                        'Failed to playback recorded audio')
            req.append_question(
                    None, input_handlers.YesNoInputHandler(default=True),
                    prompt=('Was the audio produced identical to the refernce '
                            'audio file?'))
            err_msg = ('Audio produced was not identical to the reference '
                       'audio file')

        if not self._process_request(req):
            return (tester_feedback_client.QUERY_RET_FAIL, err_msg)


class SilentPlaybackQueryDelegate(query_delegate.OutputQueryDelegate):
    """Query delegate for validating silent feedback."""

    def _prepare_impl(self):
        """Prepare for silent playback (interface override)."""
        req = sequenced_request.SequencedFeedbackRequest(
                self.test, self.dut, 'Silent playback')
        req.append_question(
                'Device %(dut)s will play nothing for a short time. Please '
                'prepare for listening to this silence and hit Enter to '
                'continue...',
                input_handlers.PauseInputHandler())
        self._process_request(req)


    def _validate_impl(self, audio_file=None):
        """Validate silence (interface override).

        @param audio_file: Name of audio file on the test host to validate
                           against.
        """
        if audio_file is not None:
            return (tester_feedback_client.QUERY_RET_ERROR,
                    'Not expecting an audio file entry when validating silence')
        req = sequenced_request.SequencedFeedbackRequest(
                self.test, self.dut, None)
        req.append_question(
                'Silence playback finished on %(dut)s.',
                input_handlers.YesNoInputHandler(default=True),
                prompt='Did you hear silence?')
        if not self._process_request(req):
            return (tester_feedback_client.QUERY_RET_FAIL,
                    'User did not hear silence')


class RecordingQueryDelegate(query_delegate.InputQueryDelegate, PlaybackMixin):
    """Query delegate for validating audible feedback."""

    def _prepare_impl(self):
        """Prepare for audio recording (interface override)."""
        req = sequenced_request.SequencedFeedbackRequest(
                self.test, self.dut, 'Audio recording')
        # TODO(ralphnathan) Lift the restriction regarding recording time once
        # the test allows recording for arbitrary periods of time (b/26924426).
        req.append_question(
                'Device %(dut)s will start recording audio for 10 seconds. '
                'Please prepare for producing sound and hit Enter to '
                'continue...',
                input_handlers.PauseInputHandler())
        self._process_request(req)


    def _emit_impl(self):
        """Emit sound for recording (interface override)."""
        req = sequenced_request.SequencedFeedbackRequest(
                self.test, self.dut, None)
        req.append_question(
                'Device %(dut)s is recording audio, hit Enter when done '
                'producing sound...',
                input_handlers.PauseInputHandler())
        self._process_request(req)


    def _validate_impl(self, captured_audio_file, sample_width,
                       sample_rate=None, num_channels=None, peak_percent_min=1,
                       peak_percent_max=100):
        """Validate recording (interface override).

        @param captured_audio_file: Path to the recorded WAV file.
        @param sample_width: The recorded sample width.
        @param sample_rate: The recorded sample rate.
        @param num_channels: The number of recorded channels.
        @peak_percent_min: Lower bound on peak recorded volume as percentage of
                           max molume (0-100). Default is 1%.
        @peak_percent_max: Upper bound on peak recorded volume as percentage of
                           max molume (0-100). Default is 100% (no limit).
        """
        # Check the WAV file properties first.
        try:
            site_utils.check_wav_file(
                    captured_audio_file, num_channels=num_channels,
                    sample_rate=sample_rate, sample_width=sample_width)
        except ValueError as e:
            return (tester_feedback_client.QUERY_RET_FAIL,
                    'Recorded audio file is invalid: %s' % e)

        # Verify playback of the recorded audio.
        props = ['has sample width of %d' % sample_width]
        if sample_rate is not None:
            props.append('has sample rate of %d' % sample_rate)
        if num_channels is not None:
            props.append('has %d recorded channels' % num_channels)
        props_str = '%s%s%s' % (', '.join(props[:-1]),
                                ', and ' if len(props) > 1 else '',
                                props[-1])

        msg = 'Recording finished on %%(dut)s. It %s.' % props_str
        if not self._playback_wav_file(msg, captured_audio_file):
            return (tester_feedback_client.QUERY_RET_ERROR,
                    'Failed to playback recorded audio')

        req = sequenced_request.SequencedFeedbackRequest(
                self.test, self.dut, None)
        req.append_question(
                None,
                input_handlers.YesNoInputHandler(default=True),
                prompt='Did the recording capture the sound produced?')
        if not self._process_request(req):
            return (tester_feedback_client.QUERY_RET_FAIL,
                    'Recorded audio is not identical to what the user produced')


query_delegate.register_delegate_cls(client.QUERY_AUDIO_PLAYBACK_AUDIBLE,
                                     AudiblePlaybackQueryDelegate)

query_delegate.register_delegate_cls(client.QUERY_AUDIO_PLAYBACK_SILENT,
                                     SilentPlaybackQueryDelegate)

query_delegate.register_delegate_cls(client.QUERY_AUDIO_RECORDING,
                                     RecordingQueryDelegate)
