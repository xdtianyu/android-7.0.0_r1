# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from autotest_lib.client.cros.chameleon import mirror_comparison
from autotest_lib.client.cros.chameleon import resolution_comparison
from autotest_lib.client.cros.chameleon import screen_capture
from autotest_lib.client.cros.chameleon import screen_comparison


class ScreenUtilityFactory(object):
    """A factory to generate utilities for screen comparison test.

    This factory creates the utilities, according to the properties of
    the CrOS. For example, a CrOS connected to VGA can use a VGA specific
    algorithm for screen comparison.

    """

    _PIXEL_DIFF_MARGIN_FOR_ANALOG = 30
    _PIXEL_DIFF_MARGIN_FOR_DIGITAL = 2

    _WRONG_PIXELS_MARGIN_FOR_ANALOG = 0.04  # 4%
    _WRONG_PIXELS_MARGIN_FOR_DIGITAL = 0


    def __init__(self, chameleon_port, display_facade):
        """Initializes the ScreenUtilityFactory objects."""
        self._chameleon_port = chameleon_port
        self._display_facade = display_facade
        self._is_vga = chameleon_port.get_connector_type() == 'VGA'


    def create_resolution_comparer(self):
        """Creates a resolution comparer object."""
        if self._is_vga:
            return resolution_comparison.VgaResolutionComparer(
                    self._chameleon_port, self._display_facade)
        else:
            return resolution_comparison.ExactMatchResolutionComparer(
                    self._chameleon_port, self._display_facade)


    def create_chameleon_screen_capturer(self):
        """Creates a Chameleon screen capturer."""
        if self._is_vga:
            return screen_capture.VgaChameleonScreenCapturer(
                    self._chameleon_port)
        else:
            return screen_capture.CommonChameleonScreenCapturer(
                    self._chameleon_port)


    def create_cros_screen_capturer(self, internal_screen=False):
        """Creates an Chrome OS screen capturer.

        @param internal_screen: True to compare the internal screen on CrOS.
        """
        if internal_screen:
            return screen_capture.CrosInternalScreenCapturer(
                    self._display_facade)
        else:
            return screen_capture.CrosExternalScreenCapturer(
                    self._display_facade)


    def create_screen_comparer(self, output_dir):
        """Creates a screen comparer.

        @param output_dir: The directory the image files output to.
        """
        if self._is_vga:
            pixel_diff_margin = self._PIXEL_DIFF_MARGIN_FOR_ANALOG
            wrong_pixels_margin = self._WRONG_PIXELS_MARGIN_FOR_ANALOG
        else:
            pixel_diff_margin = self._PIXEL_DIFF_MARGIN_FOR_DIGITAL
            wrong_pixels_margin = self._WRONG_PIXELS_MARGIN_FOR_DIGITAL

        capturer1 = self.create_chameleon_screen_capturer()
        capturer2 = self.create_cros_screen_capturer()

        return screen_comparison.ScreenComparer(
                capturer1, capturer2, output_dir,
                pixel_diff_margin, wrong_pixels_margin)


    def create_mirror_comparer(self, output_dir):
        """Creates a comparer for mirrored mode.

        @param output_dir: The directory the image files output to.
        """
        return mirror_comparison.MirrorComparer(self._display_facade, output_dir)
