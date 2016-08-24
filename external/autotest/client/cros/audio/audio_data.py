#!/usr/bin/env python
# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""This module provides abstraction of audio data."""

import contextlib
import copy
import struct
import StringIO


"""The dict containing information on how to parse sample from raw data.

Keys: The sample format as in aplay command.
Values: A dict containing:
    message: Human-readable sample format.
    struct_format: Format used in struct.unpack.
    size_bytes: Number of bytes for one sample.
"""
SAMPLE_FORMATS = dict(
        S32_LE=dict(
                message='Signed 32-bit integer, little-endian',
                struct_format='<i',
                size_bytes=4),
        S16_LE=dict(
                message='Signed 16-bit integer, little-endian',
                struct_format='<h',
                size_bytes=2))


def get_maximum_value_from_sample_format(sample_format):
    """Gets the maximum value from sample format.

    @param sample_format: A key in SAMPLE_FORMAT.

    @returns: The maximum value the sample can hold + 1.

    """
    size_bits = SAMPLE_FORMATS[sample_format]['size_bytes'] * 8
    return 1 << (size_bits - 1)


class AudioRawDataError(Exception):
    """Error in AudioRawData."""
    pass


class AudioRawData(object):
    """The abstraction of audio raw data.

    @property channel: The number of channels.
    @property channel_data: A list of lists containing samples in each channel.
                            E.g., The third sample in the second channel is
                            channel_data[1][2].
    @property sample_format: The sample format which should be one of the keys
                             in audio_data.SAMPLE_FORMATS.
    """
    def __init__(self, binary, channel, sample_format):
        """Initializes an AudioRawData.

        @param binary: A string containing binary data. If binary is not None,
                       The samples in binary will be parsed and be filled into
                       channel_data.
        @param channel: The number of channels.
        @param sample_format: One of the keys in audio_data.SAMPLE_FORMATS.
        """
        self.channel = channel
        self.channel_data = [[] for _ in xrange(self.channel)]
        self.sample_format = sample_format
        if binary:
            self.read_binary(binary)


    def read_one_sample(self, handle):
        """Reads one sample from handle.

        @param handle: A handle that supports read() method.

        @return: A number read from file handle based on sample format.
                 None if there is no data to read.
        """
        data = handle.read(SAMPLE_FORMATS[self.sample_format]['size_bytes'])
        if data == '':
            return None
        number, = struct.unpack(
                SAMPLE_FORMATS[self.sample_format]['struct_format'], data)
        return number


    def read_binary(self, binary):
        """Reads samples from binary and fills channel_data.

        Reads one sample for each channel and repeats until the end of
        input binary.

        @param binary: A string containing binary data.
        """
        channel_index = 0
        with contextlib.closing(StringIO.StringIO(binary)) as f:
            number = self.read_one_sample(f)
            while number is not None:
                self.channel_data[channel_index].append(number)
                channel_index = (channel_index + 1) % self.channel
                number = self.read_one_sample(f)


    def copy_channel_data(self, channel_data):
        """Copies channel data and updates channel number.

        @param channel_data: A list of list. The channel data to be copied.

        """
        self.channel_data = copy.deepcopy(channel_data)
        self.channel = len(self.channel_data)


    def write_to_file(self, file_path):
        """Writes channel data to file.

        Writes samples in each channel into file in index-first sequence.
        E.g. (index_0, ch_0), (index_0, ch_1), ... ,(index_0, ch_N),
             (index_1, ch_0), (index_1, ch_1), ... ,(index_1, ch_N).

        @param file_path: The path to the file.

        """
        lengths = [len(self.channel_data[ch])
                   for ch in xrange(self.channel)]
        if len(set(lengths)) != 1:
            raise AudioRawDataError(
                    'Channel lengths are not the same: %r' % lengths)
        length = lengths[0]

        with open(file_path, 'wb') as f:
            for index in xrange(length):
                for ch in xrange(self.channel):
                    f.write(struct.pack(
                            SAMPLE_FORMATS[self.sample_format]['struct_format'],
                            self.channel_data[ch][index]))
