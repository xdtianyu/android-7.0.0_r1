# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""This module provides the utilities for USB audio using chameleon."""


def set_usb_playback_configs_on_chameleon(widget_link, playback_configs):
    """Sets the playback configurations for the USB gadget driver on Chameleon.

    This method also sets the channel map of the link based on the channel
    number specified in the configs dictionary.

    @param widget_link: The USBWidgetLink object to be handled.
    @param playback_configs: A 4-entry dictionary with following fields:
                             'file_type', 'sample_format', 'channel' and 'rate'.
                             For e.g.,
                             format = {
                                 'file_type': 'wav',
                                 'sample_format': 'S16_LE',
                                 'channel': 2,
                                 'rate': 48000
                             }
                             However, the 'file_type' field will be ignored
                             since file type for playback is determined by the
                             file type of the playback file passed in by user.

    """
    channel_number = playback_configs['channel']
    widget_link.channel_map = \
            _convert_channel_number_to_channel_map(channel_number)
    usb_ctrl = widget_link.usb_ctrl
    usb_ctrl.set_playback_configs(playback_configs)


def set_usb_capture_configs_on_chameleon(widget_link, capture_configs):
    """Sets the capture configurations for the USB gadget driver on Chameleon.

    This method also sets the channel map of the link based on the channel
    number specified in the configs dictionary.

    @param widget_link: The USBWidgetLink object to be handled.
    @param capture_configs: A 4-entry dictionary with following fields:
                            'file_type', 'sample_format', 'channel' and 'rate'.
                            For e.g.,
                            format = {
                                'file_type': 'wav',
                                'sample_format': 'S16_LE',
                                'channel': 2,
                                'rate': 48000
                            }
                            'file_type' field is used to specify the file type
                            in which captured audio data should be saved.

    """
    channel_number = capture_configs['channel']
    widget_link.channel_map = \
            _convert_channel_number_to_channel_map(channel_number)
    usb_ctrl = widget_link.usb_ctrl
    usb_ctrl.set_capture_configs(capture_configs)


def _convert_channel_number_to_channel_map(channel_number):
    """Converts the channel number passed into a list representing channel map.

    @param channel_number: A number representing the number of channels.

    @return: A list representing the corresponding channel map.

    """
    return range(channel_number)
