# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.


from autotest_lib.client.cros.video import video_player


class VimeoPlayer(video_player.VideoPlayer):
    """
    Provides an interface to interact with vimeo player on a chrome device.

    """


    def is_video_ready(self):
        """
        Determines if a vimeo video is ready by using javascript.

        returns: bool, True if video is ready, else False.

        """
        return self.tab.EvaluateJavaScript('%s.isready' % self.video_id)


    def play(self):
        """
        Plays the vimeo video

        """
        self.tab.ExecuteJavaScript('%s.play()' % self.video_id)


    def seek_to(self, t):
        """
        Seeks a vimeo video to a time stamp.

        @param t: timedelta, time value to seek to.

        """
        self.tab.EvaluateJavaScript('%s.seekTo(%d)' % (self.video_id,
                                                       int(t.total_seconds())))

    def has_video_finished_seeking(self):
        """
        Determines if a vimeo video has finished seeking.

        """
        return self.tab.EvaluateJavaScript('%s.seeked' % self.video_id)