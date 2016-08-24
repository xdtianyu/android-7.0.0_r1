#!/usr/bin/python
# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.


import logging
import numpy
import os
import re
import tempfile
import threading
import time

from glob import glob
from autotest_lib.client.bin import test, utils
from autotest_lib.client.bin.input.input_device import *
from autotest_lib.client.common_lib import error
from autotest_lib.client.cros.audio import alsa_utils
from autotest_lib.client.cros.audio import audio_data
from autotest_lib.client.cros.audio import cmd_utils
from autotest_lib.client.cros.audio import cras_utils
from autotest_lib.client.cros.audio import sox_utils

LD_LIBRARY_PATH = 'LD_LIBRARY_PATH'

_AUDIO_DIAGNOSTICS_PATH = '/usr/bin/audio_diagnostics'

_DEFAULT_NUM_CHANNELS = 2
_DEFAULT_REC_COMMAND = 'arecord -D hw:0,0 -d 10 -f dat'
_DEFAULT_SOX_FORMAT = '-t raw -b 16 -e signed -r 48000 -L'
_DEFAULT_PLAYBACK_VOLUME = 100
_DEFAULT_CAPTURE_GAIN = 2500
_DEFAULT_ALSA_MAX_VOLUME = '100%'
_DEFAULT_ALSA_CAPTURE_GAIN = '25dB'

# Minimum RMS value to pass when checking recorded file.
_DEFAULT_SOX_RMS_THRESHOLD = 0.08

_JACK_VALUE_ON_RE = re.compile('.*values=on')
_HP_JACK_CONTROL_RE = re.compile('numid=(\d+).*Headphone\sJack')
_MIC_JACK_CONTROL_RE = re.compile('numid=(\d+).*Mic\sJack')

_SOX_RMS_AMPLITUDE_RE = re.compile('RMS\s+amplitude:\s+(.+)')
_SOX_ROUGH_FREQ_RE = re.compile('Rough\s+frequency:\s+(.+)')

_AUDIO_NOT_FOUND_RE = r'Audio\snot\sdetected'
_MEASURED_LATENCY_RE = r'Measured\sLatency:\s(\d+)\suS'
_REPORTED_LATENCY_RE = r'Reported\sLatency:\s(\d+)\suS'

# Tools from platform/audiotest
AUDIOFUNTEST_PATH = 'audiofuntest'
AUDIOLOOP_PATH = 'looptest'
LOOPBACK_LATENCY_PATH = 'loopback_latency'
SOX_PATH = 'sox'
TEST_TONES_PATH = 'test_tones'

_MINIMUM_NORM = 0.001
_CORRELATION_INDEX_THRESHOLD = 0.999
# The minimum difference of estimated frequencies between two sine waves.
_FREQUENCY_DIFF_THRESHOLD = 20
# The minimum RMS value of meaningful audio data.
_MEANINGFUL_RMS_THRESHOLD = 0.001

def set_mixer_controls(mixer_settings={}, card='0'):
    """Sets all mixer controls listed in the mixer settings on card.

    @param mixer_settings: Mixer settings to set.
    @param card: Index of audio card to set mixer settings for.
    """
    logging.info('Setting mixer control values on %s', card)
    for item in mixer_settings:
        logging.info('Setting %s to %s on card %s',
                     item['name'], item['value'], card)
        cmd = 'amixer -c %s cset name=%s %s'
        cmd = cmd % (card, item['name'], item['value'])
        try:
            utils.system(cmd)
        except error.CmdError:
            # A card is allowed not to support all the controls, so don't
            # fail the test here if we get an error.
            logging.info('amixer command failed: %s', cmd)

def set_volume_levels(volume, capture):
    """Sets the volume and capture gain through cras_test_client.

    @param volume: The playback volume to set.
    @param capture: The capture gain to set.
    """
    logging.info('Setting volume level to %d', volume)
    utils.system('/usr/bin/cras_test_client --volume %d' % volume)
    logging.info('Setting capture gain to %d', capture)
    utils.system('/usr/bin/cras_test_client --capture_gain %d' % capture)
    utils.system('/usr/bin/cras_test_client --dump_server_info')
    utils.system('/usr/bin/cras_test_client --mute 0')
    utils.system('amixer -c 0 contents')

def loopback_latency_check(**args):
    """Checks loopback latency.

    @param args: additional arguments for loopback_latency.

    @return A tuple containing measured and reported latency in uS.
        Return None if no audio detected.
    """
    noise_threshold = str(args['n']) if args.has_key('n') else '400'

    cmd = '%s -n %s' % (LOOPBACK_LATENCY_PATH, noise_threshold)

    output = utils.system_output(cmd, retain_output=True)

    # Sleep for a short while to make sure device is not busy anymore
    # after called loopback_latency.
    time.sleep(.1)

    measured_latency = None
    reported_latency = None
    for line in output.split('\n'):
        match = re.search(_MEASURED_LATENCY_RE, line, re.I)
        if match:
            measured_latency = int(match.group(1))
            continue
        match = re.search(_REPORTED_LATENCY_RE, line, re.I)
        if match:
            reported_latency = int(match.group(1))
            continue
        if re.search(_AUDIO_NOT_FOUND_RE, line, re.I):
            return None
    if measured_latency and reported_latency:
        return (measured_latency, reported_latency)
    else:
        # Should not reach here, just in case.
        return None

def get_mixer_jack_status(jack_reg_exp):
    """Gets the mixer jack status.

    @param jack_reg_exp: The regular expression to match jack control name.

    @return None if the control does not exist, return True if jack control
        is detected plugged, return False otherwise.
    """
    output = utils.system_output('amixer -c0 controls', retain_output=True)
    numid = None
    for line in output.split('\n'):
        m = jack_reg_exp.match(line)
        if m:
            numid = m.group(1)
            break

    # Proceed only when matched numid is not empty.
    if numid:
        output = utils.system_output('amixer -c0 cget numid=%s' % numid)
        for line in output.split('\n'):
            if _JACK_VALUE_ON_RE.match(line):
                return True
        return False
    else:
        return None

def get_hp_jack_status():
    """Gets the status of headphone jack."""
    status = get_mixer_jack_status(_HP_JACK_CONTROL_RE)
    if status is not None:
        return status

    # When headphone jack is not found in amixer, lookup input devices
    # instead.
    #
    # TODO(hychao): Check hp/mic jack status dynamically from evdev. And
    # possibly replace the existing check using amixer.
    for evdev in glob('/dev/input/event*'):
        device = InputDevice(evdev)
        if device.is_hp_jack():
            return device.get_headphone_insert()
    else:
        return None

def get_mic_jack_status():
    """Gets the status of mic jack."""
    status = get_mixer_jack_status(_MIC_JACK_CONTROL_RE)
    if status is not None:
        return status

    # When mic jack is not found in amixer, lookup input devices instead.
    for evdev in glob('/dev/input/event*'):
        device = InputDevice(evdev)
        if device.is_mic_jack():
            return device.get_microphone_insert()
    else:
        return None

def log_loopback_dongle_status():
    """Log the status of the loopback dongle to make sure it is equipped."""
    dongle_status_ok = True

    # Check Mic Jack
    mic_jack_status = get_mic_jack_status()
    logging.info('Mic jack status: %s', mic_jack_status)
    dongle_status_ok &= bool(mic_jack_status)

    # Check Headphone Jack
    hp_jack_status = get_hp_jack_status()
    logging.info('Headphone jack status: %s', hp_jack_status)
    dongle_status_ok &= bool(hp_jack_status)

    # Use latency check to test if audio can be captured through dongle.
    # We only want to know the basic function of dongle, so no need to
    # assert the latency accuracy here.
    latency = loopback_latency_check(n=4000)
    if latency:
        logging.info('Got latency measured %d, reported %d',
                latency[0], latency[1])
    else:
        logging.info('Latency check fail.')
        dongle_status_ok = False

    logging.info('audio loopback dongle test: %s',
            'PASS' if dongle_status_ok else 'FAIL')

# Functions to test audio palyback.
def play_sound(duration_seconds=None, audio_file_path=None):
    """Plays a sound file found at |audio_file_path| for |duration_seconds|.

    If |audio_file_path|=None, plays a default audio file.
    If |duration_seconds|=None, plays audio file in its entirety.

    @param duration_seconds: Duration to play sound.
    @param audio_file_path: Path to the audio file.
    """
    if not audio_file_path:
        audio_file_path = '/usr/local/autotest/cros/audio/sine440.wav'
    duration_arg = ('-d %d' % duration_seconds) if duration_seconds else ''
    utils.system('aplay %s %s' % (duration_arg, audio_file_path))

def get_play_sine_args(channel, odev='default', freq=1000, duration=10,
                       sample_size=16):
    """Gets the command args to generate a sine wav to play to odev.

    @param channel: 0 for left, 1 for right; otherwize, mono.
    @param odev: alsa output device.
    @param freq: frequency of the generated sine tone.
    @param duration: duration of the generated sine tone.
    @param sample_size: output audio sample size. Default to 16.
    """
    cmdargs = [SOX_PATH, '-b', str(sample_size), '-n', '-t', 'alsa',
               odev, 'synth', str(duration)]
    if channel == 0:
        cmdargs += ['sine', str(freq), 'sine', '0']
    elif channel == 1:
        cmdargs += ['sine', '0', 'sine', str(freq)]
    else:
        cmdargs += ['sine', str(freq)]

    return cmdargs

def play_sine(channel, odev='default', freq=1000, duration=10,
              sample_size=16):
    """Generates a sine wave and plays to odev.

    @param channel: 0 for left, 1 for right; otherwize, mono.
    @param odev: alsa output device.
    @param freq: frequency of the generated sine tone.
    @param duration: duration of the generated sine tone.
    @param sample_size: output audio sample size. Default to 16.
    """
    cmdargs = get_play_sine_args(channel, odev, freq, duration, sample_size)
    utils.system(' '.join(cmdargs))

# Functions to compose customized sox command, execute it and process the
# output of sox command.
def get_sox_mixer_cmd(infile, channel,
                      num_channels=_DEFAULT_NUM_CHANNELS,
                      sox_format=_DEFAULT_SOX_FORMAT):
    """Gets sox mixer command to reduce channel.

    @param infile: Input file name.
    @param channel: The selected channel to take effect.
    @param num_channels: The number of total channels to test.
    @param sox_format: Format to generate sox command.
    """
    # Build up a pan value string for the sox command.
    if channel == 0:
        pan_values = '1'
    else:
        pan_values = '0'
    for pan_index in range(1, num_channels):
        if channel == pan_index:
            pan_values = '%s%s' % (pan_values, ',1')
        else:
            pan_values = '%s%s' % (pan_values, ',0')

    return '%s -c 2 %s %s -c 1 %s - mixer %s' % (SOX_PATH,
            sox_format, infile, sox_format, pan_values)

def sox_stat_output(infile, channel,
                    num_channels=_DEFAULT_NUM_CHANNELS,
                    sox_format=_DEFAULT_SOX_FORMAT):
    """Executes sox stat command.

    @param infile: Input file name.
    @param channel: The selected channel.
    @param num_channels: The number of total channels to test.
    @param sox_format: Format to generate sox command.

    @return The output of sox stat command
    """
    sox_mixer_cmd = get_sox_mixer_cmd(infile, channel,
                                      num_channels, sox_format)
    stat_cmd = '%s -c 1 %s - -n stat 2>&1' % (SOX_PATH, sox_format)
    sox_cmd = '%s | %s' % (sox_mixer_cmd, stat_cmd)
    return utils.system_output(sox_cmd, retain_output=True)

def get_audio_rms(sox_output):
    """Gets the audio RMS value from sox stat output

    @param sox_output: Output of sox stat command.

    @return The RMS value parsed from sox stat output.
    """
    for rms_line in sox_output.split('\n'):
        m = _SOX_RMS_AMPLITUDE_RE.match(rms_line)
        if m is not None:
            return float(m.group(1))

def get_rough_freq(sox_output):
    """Gets the rough audio frequency from sox stat output

    @param sox_output: Output of sox stat command.

    @return The rough frequency value parsed from sox stat output.
    """
    for rms_line in sox_output.split('\n'):
        m = _SOX_ROUGH_FREQ_RE.match(rms_line)
        if m is not None:
            return int(m.group(1))

def check_audio_rms(sox_output, sox_threshold=_DEFAULT_SOX_RMS_THRESHOLD):
    """Checks if the calculated RMS value is expected.

    @param sox_output: The output from sox stat command.
    @param sox_threshold: The threshold to test RMS value against.

    @raises error.TestError if RMS amplitude can't be parsed.
    @raises error.TestFail if the RMS amplitude of the recording isn't above
            the threshold.
    """
    rms_val = get_audio_rms(sox_output)

    # In case we don't get a valid RMS value.
    if rms_val is None:
        raise error.TestError(
            'Failed to generate an audio RMS value from playback.')

    logging.info('Got audio RMS value of %f. Minimum pass is %f.',
                 rms_val, sox_threshold)
    if rms_val < sox_threshold:
        raise error.TestFail(
            'Audio RMS value %f too low. Minimum pass is %f.' %
            (rms_val, sox_threshold))

def noise_reduce_file(in_file, noise_file, out_file,
                      sox_format=_DEFAULT_SOX_FORMAT):
    """Runs the sox command to reduce noise.

    Runs the sox command to noise-reduce in_file using the noise
    profile from noise_file.

    @param in_file: The file to noise reduce.
    @param noise_file: The file containing the noise profile.
        This can be created by recording silence.
    @param out_file: The file contains the noise reduced sound.
    @param sox_format: The  sox format to generate sox command.
    """
    prof_cmd = '%s -c 2 %s %s -n noiseprof' % (SOX_PATH,
               sox_format, noise_file)
    reduce_cmd = ('%s -c 2 %s %s -c 2 %s %s noisered' %
            (SOX_PATH, sox_format, in_file, sox_format, out_file))
    utils.system('%s | %s' % (prof_cmd, reduce_cmd))

def record_sample(tmpfile, record_command=_DEFAULT_REC_COMMAND):
    """Records a sample from the default input device.

    @param tmpfile: The file to record to.
    @param record_command: The command to record audio.
    """
    utils.system('%s %s' % (record_command, tmpfile))

def create_wav_file(wav_dir, prefix=""):
    """Creates a unique name for wav file.

    The created file name will be preserved in autotest result directory
    for future analysis.

    @param wav_dir: The directory of created wav file.
    @param prefix: specified file name prefix.
    """
    filename = "%s-%s.wav" % (prefix, time.time())
    return os.path.join(wav_dir, filename)

def run_in_parallel(*funs):
    """Runs methods in parallel.

    @param funs: methods to run.
    """
    threads = []
    for f in funs:
        t = threading.Thread(target=f)
        t.start()
        threads.append(t)

    for t in threads:
        t.join()

def loopback_test_channels(noise_file_name, wav_dir,
                           playback_callback=None,
                           check_recorded_callback=check_audio_rms,
                           preserve_test_file=True,
                           num_channels = _DEFAULT_NUM_CHANNELS,
                           record_callback=record_sample,
                           mix_callback=None):
    """Tests loopback on all channels.

    @param noise_file_name: Name of the file contains pre-recorded noise.
    @param wav_dir: The directory of created wav file.
    @param playback_callback: The callback to do the playback for
        one channel.
    @param record_callback: The callback to do the recording.
    @param check_recorded_callback: The callback to check recorded file.
    @param preserve_test_file: Retain the recorded files for future debugging.
    @param num_channels: The number of total channels to test.
    @param mix_callback: The callback to do on the one-channel file.
    """
    for channel in xrange(num_channels):
        record_file_name = create_wav_file(wav_dir,
                                           "record-%d" % channel)
        functions = [lambda: record_callback(record_file_name)]

        if playback_callback:
            functions.append(lambda: playback_callback(channel))

        if mix_callback:
            mix_file_name = create_wav_file(wav_dir, "mix-%d" % channel)
            functions.append(lambda: mix_callback(mix_file_name))

        run_in_parallel(*functions)

        if mix_callback:
            sox_output_mix = sox_stat_output(mix_file_name, channel)
            rms_val_mix = get_audio_rms(sox_output_mix)
            logging.info('Got mixed audio RMS value of %f.', rms_val_mix)

        sox_output_record = sox_stat_output(record_file_name, channel)
        rms_val_record = get_audio_rms(sox_output_record)
        logging.info('Got recorded audio RMS value of %f.', rms_val_record)

        reduced_file_name = create_wav_file(wav_dir,
                                            "reduced-%d" % channel)
        noise_reduce_file(record_file_name, noise_file_name,
                          reduced_file_name)

        sox_output_reduced = sox_stat_output(reduced_file_name, channel)

        if not preserve_test_file:
            os.unlink(reduced_file_name)
            os.unlink(record_file_name)
            if mix_callback:
                os.unlink(mix_file_name)

        check_recorded_callback(sox_output_reduced)


def get_channel_sox_stat(
        input_audio, channel_index, channels=2, bits=16, rate=48000):
    """Gets the sox stat info of the selected channel in the input audio file.

    @param input_audio: The input audio file to be analyzed.
    @param channel_index: The index of the channel to be analyzed.
                          (1 for the first channel).
    @param channels: The number of channels in the input audio.
    @param bits: The number of bits of each audio sample.
    @param rate: The sampling rate.
    """
    if channel_index <= 0 or channel_index > channels:
        raise ValueError('incorrect channel_indexi: %d' % channel_index)

    if channels == 1:
        return sox_utils.get_stat(
                input_audio, channels=channels, bits=bits, rate=rate)

    p1 = cmd_utils.popen(
            sox_utils.extract_channel_cmd(
                    input_audio, '-', channel_index,
                    channels=channels, bits=bits, rate=rate),
            stdout=cmd_utils.PIPE)
    p2 = cmd_utils.popen(
            sox_utils.stat_cmd('-', channels=1, bits=bits, rate=rate),
            stdin=p1.stdout, stderr=cmd_utils.PIPE)
    stat_output = p2.stderr.read()
    cmd_utils.wait_and_check_returncode(p1, p2)
    return sox_utils.parse_stat_output(stat_output)


def get_rms(input_audio, channels=1, bits=16, rate=48000):
    """Gets the RMS values of all channels of the input audio.

    @param input_audio: The input audio file to be checked.
    @param channels: The number of channels in the input audio.
    @param bits: The number of bits of each audio sample.
    @param rate: The sampling rate.
    """
    stats = [get_channel_sox_stat(
            input_audio, i + 1, channels=channels, bits=bits,
            rate=rate) for i in xrange(channels)]

    logging.info('sox stat: %s', [str(s) for s in stats])
    return [s.rms for s in stats]


def reduce_noise_and_get_rms(
        input_audio, noise_file, channels=1, bits=16, rate=48000):
    """Reduces noise in the input audio by the given noise file and then gets
    the RMS values of all channels of the input audio.

    @param input_audio: The input audio file to be analyzed.
    @param noise_file: The noise file used to reduce noise in the input audio.
    @param channels: The number of channels in the input audio.
    @param bits: The number of bits of each audio sample.
    @param rate: The sampling rate.
    """
    with tempfile.NamedTemporaryFile() as reduced_file:
        p1 = cmd_utils.popen(
                sox_utils.noise_profile_cmd(
                        noise_file, '-', channels=channels, bits=bits,
                        rate=rate),
                stdout=cmd_utils.PIPE)
        p2 = cmd_utils.popen(
                sox_utils.noise_reduce_cmd(
                        input_audio, reduced_file.name, '-',
                        channels=channels, bits=bits, rate=rate),
                stdin=p1.stdout)
        cmd_utils.wait_and_check_returncode(p1, p2)
        return get_rms(reduced_file.name, channels, bits, rate)


def skip_devices_to_test(*boards):
    """Devices to skip due to hardware or test compatibility issues.

    @param boards: the boards to skip testing.
    """
    # TODO(scottz): Remove this when crbug.com/220147 is fixed.
    dut_board = utils.get_current_board()
    if dut_board in boards:
       raise error.TestNAError('This test is not available on %s' % dut_board)


def cras_rms_test_setup():
    """Setups for the cras_rms_tests.

    To make sure the line_out-to-mic_in path is all green.
    """
    # TODO(owenlin): Now, the nodes are choosed by chrome.
    #                We should do it here.
    cras_utils.set_system_volume(_DEFAULT_PLAYBACK_VOLUME)
    cras_utils.set_selected_output_node_volume(_DEFAULT_PLAYBACK_VOLUME)

    cras_utils.set_capture_gain(_DEFAULT_CAPTURE_GAIN)

    cras_utils.set_system_mute(False)
    cras_utils.set_capture_mute(False)


def generate_rms_postmortem():
    """Generates postmortem for rms tests."""
    try:
        logging.info('audio postmortem report')
        log_loopback_dongle_status()
        logging.info(get_audio_diagnostics())
    except Exception:
        logging.exception('Error while generating postmortem report')


def get_audio_diagnostics():
    """Gets audio diagnostic results.

    @returns: a string containing diagnostic results.

    """
    return cmd_utils.execute([_AUDIO_DIAGNOSTICS_PATH], stdout=cmd_utils.PIPE)


def get_max_cross_correlation(signal_a, signal_b):
    """Gets max cross-correlation and best time delay of two signals.

    Computes cross-correlation function between two
    signals and gets the maximum value and time delay.
    The steps includes:
      1. Compute cross-correlation function of X and Y and get Cxy.
         The correlation function Cxy is an array where Cxy[k] is the
         cross product of X and Y when Y is delayed by k.
         Refer to manual of numpy.correlate for detail of correlation.
      2. Find the maximum value C_max and index C_index in Cxy.
      3. Compute L2 norm of X and Y to get norm(X) and norm(Y).
      4. Divide C_max by norm(X)*norm(Y) to get max cross-correlation.

    Max cross-correlation indicates the similarity of X and Y. The value
    is 1 if X equals Y multiplied by a positive scalar.
    The value is -1 if X equals Y multiplied by a negative scaler.
    Any constant level shift will be regarded as distortion and will make
    max cross-correlation value deviated from 1.
    C_index is the best time delay of Y that make Y looks similar to X.
    Refer to http://en.wikipedia.org/wiki/Cross-correlation.

    @param signal_a: A list of numbers which contains the first signal.
    @param signal_b: A list of numbers which contains the second signal.

    @raises: ValueError if any number in signal_a or signal_b is not a float.
             ValueError if norm of any array is less than _MINIMUM_NORM.

    @returns: A tuple (correlation index, best delay). If there are more than
              one best delay, just return the first one.
    """
    def check_list_contains_float(numbers):
        """Checks the elements in a list are all float.

        @param numbers: A list of numbers.

        @raises: ValueError if there is any element which is not a float
                 in the list.
        """
        if any(not isinstance(x, float) for x in numbers):
            raise ValueError('List contains number which is not a float')

    check_list_contains_float(signal_a)
    check_list_contains_float(signal_b)

    norm_a = numpy.linalg.norm(signal_a)
    norm_b = numpy.linalg.norm(signal_b)
    logging.debug('norm_a: %f', norm_a)
    logging.debug('norm_b: %f', norm_b)
    if norm_a <= _MINIMUM_NORM or norm_b <= _MINIMUM_NORM:
        raise ValueError('No meaningful data as norm is too small.')

    correlation = numpy.correlate(signal_a, signal_b, 'full')
    max_correlation = max(correlation)
    best_delays = [i for i, j in enumerate(correlation) if j == max_correlation]
    if len(best_delays) > 1:
        logging.warning('There are more than one best delay: %r', best_delays)
    return max_correlation / (norm_a * norm_b), best_delays[0]


def trim_data(data, threshold=0):
    """Trims a data by removing value that is too small in head and tail.

    Removes elements in head and tail whose absolute value is smaller than
    or equal to threshold.
    E.g. trim_data([0.0, 0.1, 0.2, 0.3, 0.2, 0.1, 0.0], 0.2) =
    ([0.2, 0.3, 0.2], 2)

    @param data: A list of numbers.
    @param threshold: The threshold to compare against.

    @returns: A tuple (trimmed_data, end_trimmed_length), where
              end_trimmed_length is the length of original data being trimmed
              from the end.
              Returns ([], None) if there is no valid data.
    """
    indice_valid = [
            i for i, j in enumerate(data) if abs(j) > threshold]
    if not indice_valid:
        logging.warning(
                'There is no element with absolute value greater '
                'than threshold %f', threshold)
        return [], None
    logging.debug('Start and end of indice_valid: %d, %d',
                  indice_valid[0], indice_valid[-1])
    end_trimmed_length = len(data) - indice_valid[-1] - 1
    logging.debug('Trimmed length in the end: %d', end_trimmed_length)
    return (data[indice_valid[0] : indice_valid[-1] + 1], end_trimmed_length)


def get_one_channel_correlation(test_data, golden_data):
    """Gets max cross-correlation of test_data and golden_data.

    Trims test data and compute the max cross-correlation against golden_data.
    Signal can be trimmed because those zero values in the head and tail of
    a signal will not affect correlation computation.

    @param test_data: A list containing the data to compare against golden data.
    @param golden_data: A list containing the golden data.

    @returns: A tuple (max cross-correlation, best_delay) if data is valid.
              Otherwise returns (None, None). Refer to docstring of
              get_max_cross_correlation.
    """
    trimmed_test_data, end_trimmed_length = trim_data(test_data)

    def to_float(samples):
      """Casts elements in the list to float.

      @param samples: A list of numbers.

      @returns: A list of original numbers casted to float.
      """
      samples_float = [float(x) for x in samples]
      return samples_float

    max_cross_correlation, best_delay =  get_max_cross_correlation(
            to_float(golden_data),
            to_float(trimmed_test_data))

    # The reason to add back the trimmed length in the end.
    # E.g.:
    # golden data:
    #
    # |-----------vvvv----------------|  vvvv is the signal of interest.
    #       a                 b
    #
    # test data:
    #
    # |---x----vvvv--------x----------------|  x is the place to trim.
    #   c   d         e            f
    #
    # trimmed test data:
    #
    # |----vvvv--------|
    #   d         e
    #
    # The first output of cross correlation computation :
    #
    #                  |-----------vvvv----------------|
    #                       a                 b
    #
    # |----vvvv--------|
    #   d         e
    #
    # The largest output of cross correlation computation happens at
    # delay a + e.
    #
    #                  |-----------vvvv----------------|
    #                       a                 b
    #
    #                         |----vvvv--------|
    #                           d         e
    #
    # Cross correlation starts computing by aligning the last sample
    # of the trimmed test data to the first sample of golden data.
    # The best delay calculated from trimmed test data and golden data
    # cross correlation is e + a. But the real best delay that should be
    # identical on two channel should be e + a + f.
    # So we need to add back the length being trimmed in the end.

    if max_cross_correlation:
        return max_cross_correlation, best_delay + end_trimmed_length
    else:
        return None, None


def compare_one_channel_correlation(test_data, golden_data, parameters):
    """Compares two one-channel data by correlation.

    @param test_data: A list containing the data to compare against golden data.
    @param golden_data: A list containing the golden data.
    @param parameters: A dict containing parameters for method.

    @returns: A dict containing:
              index: The index of similarity where 1 means they are different
                  only by a positive scale.
              best_delay: The best delay of test data in relative to golden
                  data.
              equal: A bool containing comparing result.
    """
    if 'correlation_threshold' in parameters:
        threshold = parameters['correlation_threshold']
    else:
        threshold = _CORRELATION_INDEX_THRESHOLD

    result_dict = dict()
    max_cross_correlation, best_delay = get_one_channel_correlation(
            test_data, golden_data)
    result_dict['index'] = max_cross_correlation
    result_dict['best_delay'] = best_delay
    result_dict['equal'] = True if (
        max_cross_correlation and
        max_cross_correlation > threshold) else False
    logging.debug('result_dict: %r', result_dict)
    return result_dict


def get_one_channel_stat(data, data_format):
    """Gets statistic information of data.

    @param data: A list containing one channel data.
    @param data_format: A dict containing data format of data.

    @return: The sox stat parsed result. An object containing
             sameple_count: An int. Samples read.
             length: A float. Length in seconds.
             rms: A float. RMS amplitude.
             rough_frequency: A float. Rough frequency.
    """
    if not data:
        raise ValueError('Data is empty. Can not get stat')
    raw_data = audio_data.AudioRawData(
            binary=None, channel=1,
            sample_format=data_format['sample_format'])
    raw_data.copy_channel_data([data])
    with tempfile.NamedTemporaryFile() as raw_data_file:
        raw_data_path = raw_data_file.name
        raw_data.write_to_file(raw_data_path)

        bits = 8 * (audio_data.SAMPLE_FORMATS[
                    data_format['sample_format']]['size_bytes'])
        stat = sox_utils.get_stat(raw_data_path, channels=1, bits=bits,
                                  rate=data_format['rate'])
        return stat


def compare_one_channel_frequency(test_data, test_data_format,
                                  golden_data, golden_data_format):
    """Compares two one-channel data by frequency.

    @param test_data: A list containing the data to compare against golden data.
    @param test_data_format: A dict containing data format of test data.
    @param golden_data: A list containing the golden data.
    @param golden_data_format: A dict containing data format of golden data.

    @returns: A dict containing:
              test_data_frequency: test data frequency.
              golden_data_frequency: golden data frequency.
              equal: A bool containing comparing result.

    @raises: ValueError if the test data RMS is too small to be meaningful.

    """
    result_dict = dict()
    golden_data_stat = get_one_channel_stat(golden_data, golden_data_format)
    logging.info('Get golden data one channel stat: %s', golden_data_stat)
    test_data_stat = get_one_channel_stat(test_data, test_data_format)
    logging.info('Get test data one channel stat: %s', test_data_stat)

    result_dict['golden_data_frequency'] = golden_data_stat.rough_frequency
    result_dict['test_data_frequency'] = test_data_stat.rough_frequency
    result_dict['equal'] = True if (
            abs(result_dict['test_data_frequency'] -
                result_dict['golden_data_frequency']) < _FREQUENCY_DIFF_THRESHOLD
            ) else False
    logging.debug('result_dict: %r', result_dict)
    if test_data_stat.rms < _MEANINGFUL_RMS_THRESHOLD:
        raise ValueError('Recorded RMS %f is too small to be meaningful.',
                         test_data_stat.rms)
    return result_dict


def compare_one_channel_data(test_data, test_data_format,
                             golden_data, golden_data_format, method,
                             parameters):
    """Compares two one-channel data.

    @param test_data: A list containing the data to compare against golden data.
    @param test_data_format: The data format of test data.
    @param golden_data: A list containing the golden data.
    @param golden_data_format: The data format of golden data.
    @param method: The comparing method. Currently only 'correlation' is
                   supported.
    @param parameters: A dict containing parameters for method.

    @returns: A dict containing:
              index: The index of similarity where 1 means they are different
                  only by a positive scale.
              best_delay: The best delay of test data in relative to golden
                  data.
              equal: A bool containing comparing result.

    @raises: NotImplementedError if method is not supported.
    """
    if method == 'correlation':
        return compare_one_channel_correlation(test_data, golden_data,
                parameters)
    if method == 'frequency':
        return compare_one_channel_frequency(
                test_data, test_data_format, golden_data, golden_data_format)
    raise NotImplementedError('method %s is not implemented' % method)


def compare_data(golden_data_binary, golden_data_format,
                 test_data_binary, test_data_format,
                 channel_map, method, parameters=None):
    """Compares two raw data.

    @param golden_data_binary: The binary containing golden data.
    @param golden_data_format: The data format of golden data.
    @param test_data_binary: The binary containing test data.
    @param test_data_format: The data format of test data.
    @param channel_map: A list containing channel mapping.
                        E.g. [1, 0, None, None, None, None, None, None] means
                        channel 0 of test data should map to channel 1 of
                        golden data. Channel 1 of test data should map to
                        channel 0 of golden data. Channel 2 to 7 of test data
                        should be skipped.
    @param method: The method to compare data. Use 'correlation' to compare
                   general data. Use 'frequency' to compare data containing
                   sine wave.

    @param parameters: A dict containing parameters for method, if needed.

    @returns: A boolean for compare result.

    @raises: NotImplementedError if file type is not raw.
             NotImplementedError if sampling rates of two data are not the same.
    """
    if parameters is None:
        parameters = dict()

    if (golden_data_format['file_type'] != 'raw' or
        test_data_format['file_type'] != 'raw'):
        raise NotImplementedError('Only support raw data in compare_data.')
    if (golden_data_format['rate'] != test_data_format['rate']):
        raise NotImplementedError(
                'Only support comparing data with the same sampling rate')
    golden_data = audio_data.AudioRawData(
            binary=golden_data_binary,
            channel=golden_data_format['channel'],
            sample_format=golden_data_format['sample_format'])
    test_data = audio_data.AudioRawData(
            binary=test_data_binary,
            channel=test_data_format['channel'],
            sample_format=test_data_format['sample_format'])
    compare_results = []
    for test_channel, golden_channel in enumerate(channel_map):
        if golden_channel is None:
            logging.info('Skipped channel %d', test_channel)
            continue
        test_data_one_channel = test_data.channel_data[test_channel]
        golden_data_one_channel = golden_data.channel_data[golden_channel]
        result_dict = dict(test_channel=test_channel,
                           golden_channel=golden_channel)
        result_dict.update(
                compare_one_channel_data(
                        test_data_one_channel, test_data_format,
                        golden_data_one_channel, golden_data_format, method,
                        parameters))
        compare_results.append(result_dict)
    logging.info('compare_results: %r', compare_results)
    return_value = False if not compare_results else True
    for result in compare_results:
        if not result['equal']:
            logging.error(
                    'Failed on test channel %d and golden channel %d',
                    result['test_channel'], result['golden_channel'])
            return_value = False
    # Also checks best delay are exactly the same.
    if method == 'correlation':
        best_delays = set([result['best_delay'] for result in compare_results])
        if len(best_delays) > 1:
            logging.error('There are more than one best delay.')
            return_value = False
    return return_value


class _base_rms_test(test.test):
    """Base class for all rms_test """

    def postprocess(self):
        super(_base_rms_test, self).postprocess()

        # Sum up the number of failed constraints in each iteration
        if sum(len(x) for x in self.failed_constraints):
            generate_rms_postmortem()


class chrome_rms_test(_base_rms_test):
    """Base test class for audio RMS test with Chrome.

    The chrome instance can be accessed by self.chrome.
    """
    def warmup(self):
        skip_devices_to_test('x86-mario')
        super(chrome_rms_test, self).warmup()

        # Not all client of this file using telemetry.
        # Just do the import here for those who really need it.
        from autotest_lib.client.common_lib.cros import chrome

        self.chrome = chrome.Chrome()

        # The audio configuration could be changed when we
        # restart chrome.
        try:
            cras_rms_test_setup()
        except Exception:
            self.chrome.browser.Close()
            raise


    def cleanup(self, *args):
        try:
            self.chrome.browser.Close()
        finally:
            super(chrome_rms_test, self).cleanup()

class cras_rms_test(_base_rms_test):
    """Base test class for CRAS audio RMS test."""

    def warmup(self):
        skip_devices_to_test('x86-mario')
        super(cras_rms_test, self).warmup()
        cras_rms_test_setup()


def alsa_rms_test_setup():
    """Setup for alsa_rms_test.

    Different boards/chipsets have different set of mixer controls.  Even
    controls that have the same name on different boards might have different
    capabilities.  The following is a general idea to setup a given class of
    boards, and some specialized setup for certain boards.
    """
    card_id = alsa_utils.get_first_soundcard_with_control('Mic Jack', 'Mic')
    arch = utils.get_arch()
    board = utils.get_board()
    uses_max98090 = os.path.exists('/sys/module/snd_soc_max98090')
    if board in ['daisy_spring', 'daisy_skate']:
        # The MIC controls of the boards do not support dB syntax.
        alsa_utils.mixer_cmd(card_id,
                             'sset Headphone ' + _DEFAULT_ALSA_MAX_VOLUME)
        alsa_utils.mixer_cmd(card_id, 'sset MIC1 ' + _DEFAULT_ALSA_MAX_VOLUME)
        alsa_utils.mixer_cmd(card_id, 'sset MIC2 ' + _DEFAULT_ALSA_MAX_VOLUME)
    elif arch in ['armv7l', 'aarch64'] or uses_max98090:
        # ARM platforms or Intel platforms that uses max98090 codec driver.
        alsa_utils.mixer_cmd(card_id,
                             'sset Headphone ' + _DEFAULT_ALSA_MAX_VOLUME)
        alsa_utils.mixer_cmd(card_id, 'sset MIC1 ' + _DEFAULT_ALSA_CAPTURE_GAIN)
        alsa_utils.mixer_cmd(card_id, 'sset MIC2 ' + _DEFAULT_ALSA_CAPTURE_GAIN)
    else:
        # The rest of Intel platforms.
        alsa_utils.mixer_cmd(card_id, 'sset Master ' + _DEFAULT_ALSA_MAX_VOLUME)
        alsa_utils.mixer_cmd(card_id,
                             'sset Capture ' + _DEFAULT_ALSA_CAPTURE_GAIN)


class alsa_rms_test(_base_rms_test):
    """Base test class for ALSA audio RMS test."""

    def warmup(self):
        skip_devices_to_test('x86-mario')
        super(alsa_rms_test, self).warmup()

        alsa_rms_test_setup()
