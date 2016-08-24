# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.


"""This module provides the level control for audio widgets."""


from autotest_lib.client.cros.chameleon import chameleon_audio_ids as ids


class _AudioLevel(object):
    """Audio signal level on audio widgets."""
    # Line level signal on consumer equipment is typically -10 dBV, or
    # 0.316 Volt RMS.
    LINE_LEVEL = 'Line level'
    # Mic level signal on microphone is typically -60 dBV, or
    # 1 mV RMS.
    MIC_LEVEL = 'Mic level'
    # Digital signal, e.g., USB, HDMI. is not subjected to bias level or
    # full swing constraints. The signal is guranteed to be transmitted to the
    # other end without noise introduced on the path.
    # Signal level is relative to full swing of data width.
    # E.g. 2^12 is 1/8 of maximum amplitude, that is, 2^15 - 1, of signed
    # 16 bit data format.
    # TODO(cychiang) Check if we need to do level scaling for digital signal.
    DIGITAL = 'Digital'
    # The signal level of input of bluetooth module on the audio board is
    # slightly higher than mic level.
    BLUETOOTH_SIGNAL_INPUT_LEVEL = 'Bluetooth signal input level'


# The relative level of audio levels. This is used to compute scale between
# two levels.
_RELATIVE_LEVEL = {
    _AudioLevel.LINE_LEVEL: 1.0,
    _AudioLevel.MIC_LEVEL: 0.033,
    _AudioLevel.BLUETOOTH_SIGNAL_INPUT_LEVEL: 0.05,
}

_SOURCE_LEVEL_TABLE = {
        ids.ChameleonIds.LINEOUT: _AudioLevel.LINE_LEVEL,
        ids.ChameleonIds.USBOUT: _AudioLevel.DIGITAL,
        ids.CrosIds.HDMI: _AudioLevel.DIGITAL,
        ids.CrosIds.HEADPHONE: _AudioLevel.LINE_LEVEL,
        ids.CrosIds.SPEAKER: _AudioLevel.LINE_LEVEL,
        ids.CrosIds.BLUETOOTH_HEADPHONE: _AudioLevel.DIGITAL,
        ids.CrosIds.USBOUT: _AudioLevel.DIGITAL,
        ids.PeripheralIds.MIC: _AudioLevel.MIC_LEVEL,
        ids.PeripheralIds.BLUETOOTH_DATA_RX: _AudioLevel.LINE_LEVEL,
        ids.PeripheralIds.BLUETOOTH_DATA_TX: _AudioLevel.DIGITAL,
}

_SINK_LEVEL_TABLE = {
        ids.ChameleonIds.HDMI: _AudioLevel.DIGITAL,
        ids.ChameleonIds.LINEIN: _AudioLevel.LINE_LEVEL,
        ids.ChameleonIds.USBIN: _AudioLevel.DIGITAL,
        ids.CrosIds.EXTERNAL_MIC: _AudioLevel.MIC_LEVEL,
        ids.CrosIds.INTERNAL_MIC: _AudioLevel.MIC_LEVEL,
        ids.CrosIds.BLUETOOTH_MIC: _AudioLevel.DIGITAL,
        ids.CrosIds.USBIN: _AudioLevel.DIGITAL,
        ids.PeripheralIds.SPEAKER: _AudioLevel.LINE_LEVEL,
        ids.PeripheralIds.BLUETOOTH_DATA_RX: _AudioLevel.DIGITAL,
        ids.PeripheralIds.BLUETOOTH_DATA_TX:
                _AudioLevel.BLUETOOTH_SIGNAL_INPUT_LEVEL,
}


class LevelController(object):
    """The controller which sets scale between widgets of different levels."""
    def __init__(self, source, sink):
        """Initializes a LevelController.

        @param source: An AudioWidget for source.
        @param sink: An AudioWidget for sink.

        """
        self._source = source
        self._sink = sink


    def _get_needed_scale(self):
        """Gets the needed scale for _source and _sink to balance the level.

        @returns: A number for scaling on source widget.

        """
        source_level = _SOURCE_LEVEL_TABLE[self._source.port_id]
        sink_level = _SINK_LEVEL_TABLE[self._sink.port_id]
        if source_level == sink_level:
            return 1
        else:
            return _RELATIVE_LEVEL[sink_level] / _RELATIVE_LEVEL[source_level]


    def reset(self):
        """Resets scale of _source."""
        self._source.handler.scale = 1


    def set_scale(self):
        """Sets scale of _source to balance the level."""
        self._source.handler.scale = self._get_needed_scale()
        self._sink.handler.scale = 1
