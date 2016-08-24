# Copyright (c) 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import re

from autotest_lib.client.cros.audio import cmd_utils

SOX_PATH = 'sox'

def _raw_format_args(channels, bits, rate):
    args = ['-t', 'raw', '-e', 'signed']
    args += ['-c', str(channels)]
    args += ['-b', str(bits)]
    args += ['-r', str(rate)]
    return args

def generate_sine_tone_cmd(
        filename, channels=2, bits=16, rate=48000, duration=None, frequence=440,
        gain=None):
    """Gets a command to generate sine tones at specified ferquencies.

    @param filename: The name of the file to store the sine wave in.
    @param channels: The number of channels.
    @param bits: The number of bits of each sample.
    @param rate: The sampling rate.
    @param duration: The length of the generated sine tone (in seconds).
    @param frequence: The frequence of the sine wave.
    @param gain: The gain (in db).
    """
    args = [SOX_PATH, '-n']
    args += _raw_format_args(channels, bits, rate)
    args.append(filename)
    args.append('synth')
    if duration is not None:
        args.append(str(duration))
    args += ['sine', str(frequence)]
    if gain is not None:
        args += ['gain', str(gain)]
    return args


def noise_profile(*arg, **karg):
    """A helper function to execute the noise_profile_cmd."""
    return cmd_utils.execute(noise_profile_cmd(*arg, **karg))


def noise_profile_cmd(input, output, channels=1, bits=16, rate=48000):
    """Gets the noise profile of the input audio.

    @param input: The input audio.
    @param output: The file where the output profile will be stored in.
    @param channels: The number of channels.
    @param bits: The number of bits of each sample.
    @param rate: The sampling rate.
    """
    args = [SOX_PATH]
    args += _raw_format_args(channels, bits, rate)
    args += [input, '-n', 'noiseprof', output]
    return args


def noise_reduce(*args, **kargs):
    """A helper function to execute the noise_reduce_cmd."""
    return cmd_utils.execute(noise_reduce_cmd(*args, **kargs))


def noise_reduce_cmd(
        input, output, noise_profile, channels=1, bits=16, rate=48000):
    """Reduce noise in the input audio by the given noise profile.

    @param input: The input audio file.
    @param ouput: The output file in which the noise reduced audio is stored.
    @param noise_profile: The noise profile.
    @param channels: The number of channels.
    @param bits: The number of bits of each sample.
    @param rate: The sampling rate.
    """
    args = [SOX_PATH]
    format_args = _raw_format_args(channels, bits, rate)
    args += format_args
    args.append(input)
    # Uses the same format for output.
    args += format_args
    args.append(output)
    args.append('noisered')
    args.append(noise_profile)
    return args


def extract_channel_cmd(
        input, output, channel_index, channels=2, bits=16, rate=48000):
    """Extract the specified channel data from the given input audio file.

    @param input: The input audio file.
    @param output: The output file to which the extracted channel is stored
    @param channel_index: The index of the channel to be extracted.
                          Note: 1 for the first channel.
    @param channels: The number of channels.
    @param bits: The number of bits of each sample.
    @param rate: The sampling rate.
    """
    args = [SOX_PATH]
    args += _raw_format_args(channels, bits, rate)
    args.append(input)
    args += ['-t', 'raw', output]
    args += ['remix', str(channel_index)]
    return args


def stat_cmd(input, channels=1, bits=16, rate=44100):
    """Get statistical information about the input audio data.

    The statistics will be output to standard error.

    @param input: The input audio file.
    @param channels: The number of channels.
    @param bits: The number of bits of each sample.
    @param rate: The sampling rate.
    """
    args = [SOX_PATH]
    args += _raw_format_args(channels, bits, rate)
    args += [input, '-n', 'stat']
    return args


def get_stat(*args, **kargs):
    """A helper function to execute the stat_cmd.

    It returns the statistical information (in text) read from the standard
    error.
    """
    p = cmd_utils.popen(stat_cmd(*args, **kargs), stderr=cmd_utils.PIPE)

    #The output is read from the stderr instead of stdout
    stat_output = p.stderr.read()
    cmd_utils.wait_and_check_returncode(p)
    return parse_stat_output(stat_output)


_SOX_STAT_ATTR_MAP = {
        'Samples read': ('sameple_count', int),
        'Length (seconds)': ('length', float),
        'RMS amplitude': ('rms', float),
        'Rough frequency': ('rough_frequency', float)}

_RE_STAT_LINE = re.compile('(.*):(.*)')

class _SOX_STAT:
    def __str__(self):
        return str(vars(self))


def _remove_redundant_spaces(value):
    return ' '.join(value.split()).strip()


def parse_stat_output(stat_output):
    """A helper function to parses the stat_cmd's output to get a python object
    for easy access to the statistics.

    It returns a python object with the following attributes:
      .sample_count: The number of the audio samples.
      .length: The length of the audio (in seconds).
      .rms: The RMS value of the audio.
      .rough_frequency: The rough frequency of the audio (in Hz).

    @param stat_output: The statistics ouput to be parsed.
    """
    stat = _SOX_STAT()

    for line in stat_output.splitlines():
        match = _RE_STAT_LINE.match(line)
        if not match:
            continue
        key, value = (_remove_redundant_spaces(x) for x in match.groups())
        attr, convfun = _SOX_STAT_ATTR_MAP.get(key, (None, None))
        if attr:
            setattr(stat, attr, convfun(value))

    if not all(hasattr(stat, x[0]) for x in _SOX_STAT_ATTR_MAP.values()):
        logging.error('stat_output: %s', stat_output)
        raise RuntimeError('missing entries: ' + str(stat))

    return stat


def convert_raw_file(path_src, channels_src, bits_src, rate_src,
                     path_dst):
    """Converts a raw file to a new format.

    @param path_src: The path to the source file.
    @param channels_src: The channel number of the source file.
    @param bits_src: The size of sample in bits of the source file.
    @param rate_src: The sampling rate of the source file.
    @param path_dst: The path to the destination file. The file name determines
                     the new file format.

    """
    sox_cmd = [SOX_PATH]
    sox_cmd += _raw_format_args(channels_src, bits_src, rate_src)
    sox_cmd += [path_src]
    sox_cmd += [path_dst]
    cmd_utils.execute(sox_cmd)


def convert_format(path_src, channels_src, bits_src, rate_src,
                   path_dst, channels_dst, bits_dst, rate_dst,
                   volume_scale):
    """Converts a raw file to a new format.

    @param path_src: The path to the source file.
    @param channels_src: The channel number of the source file.
    @param bits_src: The size of sample in bits of the source file.
    @param rate_src: The sampling rate of the source file.
    @param path_dst: The path to the destination file.
    @param channels_dst: The channel number of the destination file.
    @param bits_dst: The size of sample in bits of the destination file.
    @param rate_dst: The sampling rate of the destination file.
    @param volume_scale: A float for volume scale used in sox command.
                         E.g. 1.0 is the same. 0.5 to scale volume by
                         half. -1.0 to invert the data.

    """
    sox_cmd = [SOX_PATH]
    sox_cmd += _raw_format_args(channels_src, bits_src, rate_src)
    sox_cmd += ['-v', '%f' % volume_scale]
    sox_cmd += [path_src]
    sox_cmd += _raw_format_args(channels_dst, bits_dst, rate_dst)
    sox_cmd += [path_dst]
    cmd_utils.execute(sox_cmd)


def lowpass_filter(path_src, channels_src, bits_src, rate_src,
                   path_dst, frequency):
    """Passes a raw file to a lowpass filter.

    @param path_src: The path to the source file.
    @param channels_src: The channel number of the source file.
    @param bits_src: The size of sample in bits of the source file.
    @param rate_src: The sampling rate of the source file.
    @param path_dst: The path to the destination file.
    @param frequency: A float for frequency used in sox command. The 3dB
                      frequency of the lowpass filter. Checks manual of sox
                      command for detail.

    """
    sox_cmd = [SOX_PATH]
    sox_cmd += _raw_format_args(channels_src, bits_src, rate_src)
    sox_cmd += [path_src]
    sox_cmd += _raw_format_args(channels_src, bits_src, rate_src)
    sox_cmd += [path_dst]
    sox_cmd += ['lowpass', '-2', str(frequency)]
    cmd_utils.execute(sox_cmd)
