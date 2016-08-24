# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from autotest_lib.client.cros.video import method_logger


class VideoScreenShotCollector(object):
    """
    Captures and collects screenshots of a video at specified time points.

    """


    @method_logger.log
    def __init__(self, player, screenshot_namer, screenshot_capturer):
        self.player = player
        self.screnshot_namer = screenshot_namer
        self.screnshot_capturer = screenshot_capturer


    @method_logger.log
    def collect_screenshot(self, timestamp):
        """
        Get a screenshot of video at a particular time.

        Navigates player to a given time value, captures and saves a
        screenshot at that time value.

        @param timestamp: time_delta, the time value to capture screenshot for.

        @returns a complete path to the screenshot captured.

        """
        filename = self.screnshot_namer.get_filename(timestamp)

        self.player.seek_to(timestamp)
        self.player.wait_for_video_to_seek()

        return self.screnshot_capturer.capture(filename)


    @method_logger.log
    def ensure_player_is_ready(self):
        """
         Loads video and waits for player to be ready.

         @raises whatever load_video() raises.

        """
        self.player.load_video()


    @method_logger.log
    def collect_multiple_screenshots(self, timestamps):
        """
        Collects screenshots for each timevalue in a list.

        @param timestamps: time_delta list, time values to collect
        screenshots for.

        @returns a list of complete paths for screenshot captured.

        """
        self.ensure_player_is_ready()

        with self.screnshot_capturer:
            return [self.collect_screenshot(t) for t in timestamps]