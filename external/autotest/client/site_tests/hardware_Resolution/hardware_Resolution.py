#!/usr/bin/python
#
# Copyright (c) 2010 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

__author__ = 'kdlucas@chromium.org (Kelly Lucas)'

import logging, re

from autotest_lib.client.bin import test, utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.cros.graphics import graphics_utils

_SUPPORTED_LVDS_RESOLUTIONS = ['1280x800', '1366x768']

_MODETEST_COMMAND = 'modetest -c'
_MODETEST_CONNECTED = 'connected'
_MODETEST_CONNECTOR_LVDS = 'LVDS'
# The list of connectors in this regex pattern comes from an array called
# connector_type_names in the libdrm file caled modetest.c .
_MODETEST_CONNECTOR_PATTERN = (r'\d+\s+\d+\s+(connected|disconnected)\s+'
                               r'(unknown|VGA|DVI-I|DVI-D|DVI-A|composite|'
                               r's-video|LVDS|component|9-pin DIN|HDMI-A|'
                               r'HDMI-B|TV|eDP)\s+\d+x\d+\s+\d+\s+\d+')
_MODETEST_MODE_PATTERN = (r'\s+.+\d+\s+(\d+)\s+\d+\s+\d+\s+\d+\s+(\d+)\s+\d+\s+'
                          r'\d+\s+\d+\s+flags:')

_LVDS_UNSUPPORTED_MESSAGE = '%s is not a supported LVDS resolution'

class hardware_Resolution(test.test):
    """
    Verify the current screen resolution is supported.
    """
    version = 1

    def is_lvds_res(self, res, xrandr_output):
        """
        Returns True if the supplied resolution is associated with
        an LVDS connection.
        """
        search_str = r'LVDS\d+ connected ' + res
        for line in xrandr_output:
            if re.match(search_str, line):
                return True

        return False

    def get_current_res(self, xrandr_output):
        """
        Get the current video resolution.
        Returns:
            string: represents the video resolution.
        """
        for line in xrandr_output:
            if 'Screen 0' in line:
                sections = line.split(',')
                for item in sections:
                    if 'current' in item:
                        res = item.split()
                        return '%s%s%s' % (res[1], res[2], res[3])

        return None

    def run_x(self):
        xrandr_output = graphics_utils.call_xrandr().split('\n')

        res = self.get_current_res(xrandr_output)
        if not res or not re.match(r'\d+x\d+$', res):
            raise error.TestFail('%s is not a valid resolution' % res)

        if self.is_lvds_res(res, xrandr_output) and \
           res not in _SUPPORTED_LVDS_RESOLUTIONS:
            raise error.TestFail(_LVDS_UNSUPPORTED_MESSAGE % res)

    def run_freon(self):
        modetest_output = utils.system_output(_MODETEST_COMMAND)
        logging.info('modetest output: \n{0}'.format(modetest_output))

        # True if the information being read is about a connected LVDS
        # connector, False otherwise
        connected_lvds = False

        for line in modetest_output.splitlines():
            connector_match = re.match(_MODETEST_CONNECTOR_PATTERN, line)
            if connector_match is not None:
                connected_lvds = False
                if connector_match.group(1) == _MODETEST_CONNECTED:
                    if connector_match.group(2) == _MODETEST_CONNECTOR_LVDS:
                        connected_lvds = True

            if connected_lvds:
                mode_match = re.match(_MODETEST_MODE_PATTERN, line)
                if mode_match is not None:
                    res = '{0}x{1}'.format(int(mode_match.group(1)),
                                           int(mode_match.group(2)))
                    if res not in _SUPPORTED_LVDS_RESOLUTIONS:
                        raise error.TestFail(_LVDS_UNSUPPORTED_MESSAGE % res)

    def run_once(self):
        if utils.is_freon():
            self.run_freon()
        else:
            self.run_x()
