# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Module for display info."""

class DisplayInfo(object):
    """The class match displayInfo object from chrome.system.display API.
    """

    class Bounds(object):
        """The class match Bounds object from chrome.system.display API.

        @param left: The x-coordinate of the upper-left corner.
        @param top: The y-coordinate of the upper-left corner.
        @param width: The width of the display in pixels.
        @param height: The height of the display in pixels.
        """
        def __init__(self, d):
            self.left = d['left']
            self.top = d['top']
            self.width = d['width']
            self.height = d['height']


    class Insets(object):
        """The class match Insets object from chrome.system.display API.

        @param left: The x-axis distance from the left bound.
        @param left: The y-axis distance from the top bound.
        @param left: The x-axis distance from the right bound.
        @param left: The y-axis distance from the bottom bound.
        """

        def __init__(self, d):
            self.left = d['left']
            self.top = d['top']
            self.right = d['right']
            self.bottom = d['bottom']


    def __init__(self, d):
        self.display_id = d['id']
        self.name = d['name']
        self.mirroring_source_id = d['mirroringSourceId']
        self.is_primary = d['isPrimary']
        self.is_internal = d['isInternal']
        self.is_enabled = d['isEnabled']
        self.dpi_x = d['dpiX']
        self.dpi_y = d['dpiY']
        self.rotation = d['rotation']
        self.bounds = self.Bounds(d['bounds'])
        self.overscan = self.Insets(d['overscan'])
        self.work_area = self.Bounds(d['workArea'])
