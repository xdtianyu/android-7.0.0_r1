# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""This module provides the USB Controller interface."""


class USBController(object):
    """An abstraction of USB audio gadget driver controller on Chameleon.

    It provides methods to control the USB gadget driver on Chameleon.

    A ChameleonConnection object is passed to the construction.

    """
    def __init__(self, chameleon_connection):
        """Constructs an USBController.

        @param chameleon_connection: A ChameleonConnection object.

        """
        self._chameleond_proxy = chameleon_connection.chameleond_proxy


    def set_playback_configs(self, playback_data_format):
        """Sets the playback configurations for the USB gadget driver.

        @param playback_data_format: A 4-entry dictionary with following fields:
                                     'file_type', 'sample_format', 'channel' and
                                     'rate'. For e.g.,
                                     format = {
                                         'file_type': 'raw',
                                         'sample_format': 'S16_LE',
                                         'channel': 2,
                                         'rate': 48000
                                     }

        """
        self._chameleond_proxy.SetUSBDriverPlaybackConfigs(playback_data_format)


    def set_capture_configs(self, port_id, capture_data_foramt):
        """Sets the capture configurations for the USB gadget driver.

        @param capture_data_format: A 4-entry dictionary with following fields:
                                     'file_type', 'sample_format', 'channel' and
                                     'rate'. For e.g.,
                                     format = {
                                         'file_type': 'raw',
                                         'sample_format': 'S16_LE',
                                         'channel': 2,
                                         'rate': 48000
                                     }

        """
        self._chameleond_proxy.SetUSBDriverCaptureConfigs(capture_data_foramt)
