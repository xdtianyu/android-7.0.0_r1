# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.


import datetime, time

from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.cros.video import method_logger


class VideoPlayer(object):
    """
    Provides interface to interact with and control video playback via js.

    Specific players such as VimeoPlayer will inherit from this class and
    customize its behavior.

    """


    def __init__(self, tab, full_url, video_id, video_src_path='',
                 event_timeout=5, polling_wait_time=1):
        """
        @param tab: object, tab to interact with the tab in the browser.
        @param full_url: string, full url pointing to html file to load.
        @param video_src_path: path, complete path to video used for test.
        @param video_id: string, name of the video_id element in the html file.
        @param time_out_for_events_s: integer, how long to wait for an event
                                      before timing out
        @param time_btwn_polling_s: integer, how long to wait between one call
                                    to check a condition and the next.

        """
        self.tab = tab
        self.full_url = full_url
        self.video_id = video_id
        self.video_src_path = video_src_path
        self.event_timeout = event_timeout
        self.polling_wait_time = polling_wait_time


    @method_logger.log
    def load_video(self, wait_for_canplay=True):
        """
        Loads video into browser.
        @param wait_for_canplay: video will be verified before play

        """
        self.tab.Navigate(self.full_url)
        self.tab.WaitForDocumentReadyStateToBeComplete()
        self.inject_source_file()
        if wait_for_canplay:
             self.wait_for_video_ready()


    def inject_source_file(self):
        """
        Injects source file into html file if needed.

        Created for subclasses that need it

        """
        pass


    @method_logger.log
    def wait_for_video_ready(self):
        """
        Waits for video to signal that is ready.

        Each class that inherits from this will define its is_video_ready
        function.

        """
        exception_msg = 'Video did not signal ready in time.'

        self._wait_for_event(self.is_video_ready, exception_msg)


    @method_logger.log
    def verify_video_can_play(self):
        """
        Plays video and ensures that reported video current time is > 0.
        @raises: error.TestError if current time is not > 0 after time > 0s

        """
        exception_msg = 'Expected current > 0 after %ds' %self.event_timeout

        self.play()

        # check that video is playing
        self._wait_for_event(lambda : self.currentTime() > 0.0, exception_msg)

        self.pause()

        # seek back to the beginning of video
        self.seek_to(datetime.timedelta(seconds=0))


    @method_logger.log
    def seek_to(self, timestamp):
        """
        Uses javascript to set currentTime property of video to desired time.

        @param timestamp: timedelta, instance of time to navigate video to.

        """
        self.seek_to(timestamp)


    @method_logger.log
    def wait_for_video_to_seek(self):
        """
        Waits for video's currentTime to equal the time it was seeked to.

        """
        exception_msg = 'Video did not complete seeking in time.'

        self._wait_for_event(self.has_video_finished_seeking, exception_msg)

        # it usually takes a little while before new frame renders, so wait
        time.sleep(1)


    @method_logger.log
    def _wait_for_event(self, predicate_function, exception_msg):
        """
         Helper method to wait for a desired condition.

         @param predicate_function: object, function which returns true when
                                    desired condition is achieved.
         @param exception_msg: string, an exception message to show when desired
                               condition is not achieved in allowed time.

        """
        fullmsg = exception_msg + ' Waited for %ss' % self.event_timeout

        utils.poll_for_condition(predicate_function,
                                 error.TestError(fullmsg),
                                 self.event_timeout,
                                 self.polling_wait_time)
