# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Classes to do comparison for mirrored mode."""

import logging

from autotest_lib.client.cros.chameleon import screen_capture
from autotest_lib.client.cros.chameleon import screen_comparison


class MirrorComparer(object):
    """A class to compare the resolutions and screens for mirrored mode.

    Calling its member method compare() does the comparison.

    """

    def __init__(self, display_facade, output_dir):
        """Initializes the MirrorComparer objects.

        @param display_facade: A display facade object
        @param output_dir: The directory for output images.
        """
        self._display_facade = display_facade
        int_capturer = screen_capture.CrosInternalScreenCapturer(display_facade)
        ext_capturer = screen_capture.CrosExternalScreenCapturer(display_facade)
        # The frame buffers of screens should be perfectly matched.
        self._screen_comparer = screen_comparison.ScreenComparer(
                int_capturer, ext_capturer, output_dir, 0, 0)


    def compare(self):
        """Compares the resolutions and screens on all CrOS screens.

        This method first checks if CrOS is under mirrored mode or not. Skip
        the following tests if not in mirrored mode.

        Then it checks all the resolutions identical and also all the screens
        identical.

        @return: None if the check passes; otherwise, a string of error message.
        """
        if not self._display_facade.is_mirrored_enabled():
            message = 'Do mirror comparison but not in mirrored mode.'
            logging.error(message)
            return message

        logging.info('Checking the resolutions of all screens identical...')
        # TODO(waihong): Support the case without internal screen.
        internal_resolution = self._display_facade.get_internal_resolution()
        # TODO(waihong): Support multiple external screens.
        external_resolution = self._display_facade.get_external_resolution()

        if internal_resolution != external_resolution:
            logging.info('Sofware-based mirroring, skip the screen comparison. '
                         'Resolutions: Internal %r; External %r',
                         internal_resolution, external_resolution)
            return None
        else:
            logging.info('Resolutions across all CrOS screens match: %dx%d',
                         *internal_resolution)

        logging.info('Checking all the screens mirrored...')
        return self._screen_comparer.compare()
