# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Classes to do screen capture."""

import logging

from PIL import Image

from autotest_lib.client.cros.multimedia import image_generator


def _unlevel(p):
    """Unlevel a color value from TV level back to PC level

    @param p: The color value in one character byte

    @return: The color value in integer in PC level
    """
    # TV level: 16~236; PC level: 0~255
    p = (p - 126) * 128 / 110 + 128
    if p < 0:
        p = 0
    elif p > 255:
        p = 255
    return p


class CommonChameleonScreenCapturer(object):
    """A class to capture the screen on Chameleon.

    Calling its member method capture() captures the screen.

    """
    TAG = 'Chameleon'

    def __init__(self, chameleon_port):
        """Initializes the CommonChameleonScreenCapturer objects."""
        self._chameleon_port = chameleon_port


    def capture(self):
        """Captures the screen.

        @return An Image object.
        """
        logging.info('Capturing the screen on Chameleon...')
        image = self._chameleon_port.capture_screen()

        # unleveling from TV level [16, 235]
        pmin, pmax = image_generator.ImageGenerator.get_extrema(image)
        if pmin > 10 and pmax < 240:
            logging.info(' (TV level: %d %d)', pmin, pmax)
            image = Image.eval(image, _unlevel)
        return image


class VgaChameleonScreenCapturer(object):
    """A class to capture the screen on a VGA port of Chameleon.

    Calling its member method capture() captures the screen.

    """
    TAG = 'Chameleon'

    def __init__(self, chameleon_port):
        """Initializes the VgaChameleonScreenCapturer objects."""
        self._chameleon_port = chameleon_port


    def capture(self):
        """Captures the screen.

        @return An Image object.
        """
        logging.info('Capturing the screen on a VGA port of Chameleon...')
        image = self._chameleon_port.capture_screen()

        # Find the box containing white points on its boundary.
        boundary = image.convert('L').point(
                lambda x: 255 if x >= 220 else 0).getbbox()
        logging.info('Boundary: %r', boundary)
        image = image.crop(boundary)
        return image


class CrosExternalScreenCapturer(object):
    """A class to capture the external screen on Chrome OS.

    Calling its member method capture() captures the screen.

    """
    TAG = 'CrOS-Ext'

    def __init__(self, display_facade):
        """Initializes the CrosExternalScreenCapturer objects."""
        self._display_facade = display_facade


    def capture(self):
        """Captures the screen.

        @return An Image object.
        """
        logging.info('Capturing the external screen on CrOS...')
        return self._display_facade.capture_external_screen()


class CrosInternalScreenCapturer(object):
    """A class to capture the internal screen on Chrome OS.

    Calling its member method capture() captures the screen.

    """
    TAG = 'CrOS-Int'

    def __init__(self, display_facade):
        """Initializes the CrosInternalScreenCapturer objects."""
        self._display_facade = display_facade


    def capture(self):
        """Captures the screen.

        @return An Image object.
        """
        logging.info('Capturing the internal screen on CrOS...')
        return self._display_facade.capture_internal_screen()
