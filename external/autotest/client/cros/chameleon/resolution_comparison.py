# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Classes to do resolution comparison."""

import logging
import xmlrpclib


class ExactMatchResolutionComparer(object):
    """A class to compare the resolutions by using exact match.

    Calling its member method compare() does the comparison.

    """

    def __init__(self, chameleon_port, display_facade):
        """Initializes the ExactMatchResolutionComparer objects."""
        self._chameleon_port = chameleon_port
        self._display_facade = display_facade


    def compare(self, expected_resolution):
        """Compares the resolutions among the given one, Chameleon's, and CrOS'.

        This method first checks if CrOS is under mirrored mode or not.

        If under extended mode, checks the resolutions of both CrOS and
        Chameleon exactly match the expected one.

        If under mirror mode, only checks the resolution of CrOS exactly
        matches the one of Chameleon.

        @param expected_resolution: A tuple (width, height) for the expected
                                    resolution.
        @return: None if the check passes; otherwise, a string of error message.
        """
        try:
            chameleon_resolution = self._chameleon_port.get_resolution()
        except xmlrpclib.Fault as e:
            logging.exception(e)
            return str(e)
        cros_resolution = self._display_facade.get_external_resolution()

        logging.info('Checking the resolutions of Chameleon and CrOS...')
        if expected_resolution != cros_resolution or (
                chameleon_resolution != cros_resolution):
            message = ('Detected mis-matched resolutions: '
                       'CrOS %r; Chameleon %r; Expected %r.' %
                       (cros_resolution, chameleon_resolution,
                        expected_resolution))
            # Note: In mirrored mode, the device may be in hardware mirror
            # (as opposed to software mirror). If so, the actual resolution
            # could be different from the expected one. So we skip the check
            # in mirrored mode. The resolution of the CrOS and Chameleon
            # should be same no matter the device in mirror mode or not.
            if chameleon_resolution != cros_resolution or (
                    not self._display_facade.is_mirrored_enabled()):
                logging.error(message)
                return message
            else:
                logging.warn(message)
        else:
            logging.info('Resolutions across CrOS and Chameleon match: %dx%d',
                         *expected_resolution)
        return None


class VgaResolutionComparer(object):
    """A class to compare the resolutions for VGA interface.

    Calling its member method compare() does the comparison.

    """

    def __init__(self, chameleon_port, display_facade):
        """Initializes the VgaResolutionComparer objects."""
        self._chameleon_port = chameleon_port
        self._display_facade = display_facade


    def compare(self, expected_resolution):
        """Compares the resolutions among the given one, Chameleon's, and CrOS'.

        There is no DE (data enable) signal in the VGA standard, the captured
        image of Chameleon side is larger than the expected image.

        It checks the resolution of CrOS matches the expected one and
        the resolution of Chameleon is larger than or equal to the one of CrOS.

        @param expected_resolution: A tuple (width, height) of the expected
                                    resolution.
        @return: None if the check passes; otherwise, a string of error message.
        """
        try:
            chameleon_resolution = self._chameleon_port.get_resolution()
        except xmlrpclib.Fault as e:
            logging.exception(e)
            return str(e)
        cros_resolution = self._display_facade.get_external_resolution()

        logging.info('Checking the resolutions of Chameleon and CrOS...')
        if expected_resolution != cros_resolution or (
                chameleon_resolution[0] < cros_resolution[0] or
                chameleon_resolution[1] < cros_resolution[1]):
            message = ('Detected mis-matched VGA resolutions: '
                       'CrOS %r; Chameleon %r; Expected %r.' %
                       (cros_resolution, chameleon_resolution,
                        expected_resolution))
            logging.error(message)
            return message
        else:
            logging.info('Detected VGA resolutions: '
                         'CrOS: %dx%d; Chameleon: %dx%d.',
                         *(cros_resolution + chameleon_resolution))
        return None
