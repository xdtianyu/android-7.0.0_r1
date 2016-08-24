# Copyright (c) 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import operator
import os
import time

from autotest_lib.client.bin import test
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros import chrome


class video_YouTubePage(test.test):
    """The main test class of this test.

    """


    version = 1

    PSEUDO_RANDOM_TIME_1 = 20.25
    PSEUDO_RANDOM_TIME_2 = 5.47

    # Minimum number of timeupdates required to fire in the last second.
    MIN_LAST_SECOND_UPDATES = 3

    NO_DELAY = 0
    MINIMAL_DELAY = 1
    MAX_REBUFFER_DELAY = 10

    PLAYING_STATE = 'playing'
    PAUSED_STATE = 'paused'
    ENDED_STATE = 'ended'
    TEST_PAGE = 'http://web-release-qa.youtube.com/watch?v=zuzaxlddWbk&html5=1'

    DISABLE_COOKIES = False

    tab = None


    def initialize_test(self, chrome, player_page):
        """Initializes the test.

        @param chrome: An Autotest Chrome instance.
        @param player_page: The URL (string) of the YouTube player page to test.

        """
        self.tab = chrome.browser.tabs[0]

        self.tab.Navigate(player_page)
        self.tab.WaitForDocumentReadyStateToBeComplete()
        time.sleep(2)

        with open(
                os.path.join(os.path.dirname(__file__),
                'files/video_YouTubePageCommon.js')) as f:
            js = f.read()
            if not self.tab.EvaluateJavaScript(js):
                raise error.TestFail('YouTube page failed to load.')
            logging.info('Loaded accompanying .js script.')


    def get_player_state(self):
        """Simple wrapper to get the JS player state.

        @returns: The state of the player (string).

        """
        return self.tab.EvaluateJavaScript('window.__getVideoState();')


    def play_video(self):
        """Simple wrapper to play the video.

        """
        self.tab.ExecuteJavaScript('window.__playVideo();')


    def pause_video(self):
        """Simple wrapper to pause the video.

        """
        self.tab.ExecuteJavaScript('window.__pauseVideo();')


    def seek_video(self, new_time):
        """Simple wrapper to seek the video to a new time.

        @param new_time: Time to seek to.

        """
        self.tab.ExecuteJavaScript('window.__seek(%f);' % new_time)


    def seek_to_almost_end(self, seconds_before_end):
        """Simple wrapper to seek to almost the end of the video.

        @param seconds_before_end: How many seconds (a float, not integer)
                before end of video.

        """
        self.tab.ExecuteJavaScript(
                'window.__seekToAlmostEnd(%f);' % seconds_before_end)


    def get_current_time(self):
        """Simple wrapper to get the current time in the video.

        @returns: The current time (float).

        """
        return self.tab.EvaluateJavaScript('window.__getCurrentTime();')


    def assert_event_state(self, event, op, error_str):
        """Simple wrapper to get the status of a state in the video.

        @param event: A string denoting the event. Check the accompanying JS
                file for the possible values.
        @param op: truth or not_ operator from the standard Python operator
                module.
        @param error_str: A string for the error output.

        @returns: Whether or not the input event has fired.

        """
        result = self.tab.EvaluateJavaScript(
                'window.__getEventHappened("%s");' % event)
        if not op(result):
            raise error.TestError(error)


    def clear_event_state(self, event):
        """Simple wrapper to clear the status of a state in the video.

        @param event: A string denoting the event. Check the accompanying JS
                file for the possible vlaues.

        """
        self.tab.ExecuteJavaScript('window.__clearEventHappened("%s");' % event)


    def verify_last_second_playback(self):
        """Simple wrapper to check the playback of the last second.

        """
        result = self.tab.EvaluateJavaScript(
                'window.__getLastSecondTimeupdates()')
        if result < self.MIN_LAST_SECOND_UPDATES:
            raise error.TestError(
                    'Last second did not play back correctly (%d events).' %
                    result)


    def assert_player_state(self, state, max_wait_secs):
        """Simple wrapper to busy wait and test the current state of the player.

        @param state: A string denoting the expected state of the player.
        @param max_wait_secs: Maximum amount of time to wait before failing.

        @raises: A error.TestError if the state is not as expected.

        """
        start_time = time.time()
        while True:
            current_state = self.get_player_state()
            if current_state == state:
                return
            elif time.time() < start_time + max_wait_secs:
                time.sleep(0.5)
            else:
                raise error.TestError(
                        'Current player state "%s" is not the expected state '
                        '"%s".' % (current_state, state))


    def perform_test(self):
        """Base method for derived classes to run their test.

        """
        raise error.TestFail('Derived class did not specify a perform_test.')


    def perform_playing_test(self):
        """Test to check if the YT page starts off playing.

        """
        self.assert_player_state(self.PLAYING_STATE, self.NO_DELAY)
        if self.get_current_time() <= 0.0:
            raise error.TestError('perform_playing_test failed.')


    def perform_pausing_test(self):
        """Test to check if the video is in the 'paused' state.

        """
        self.assert_player_state(self.PLAYING_STATE, self.NO_DELAY)
        self.pause_video()
        self.assert_player_state(self.PAUSED_STATE, self.MINIMAL_DELAY)


    def perform_resuming_test(self):
        """Test to check if the video responds to resumption.

        """
        self.assert_player_state(self.PLAYING_STATE, self.NO_DELAY)
        self.pause_video()
        self.assert_player_state(self.PAUSED_STATE, self.MINIMAL_DELAY)
        self.play_video()
        self.assert_player_state(self.PLAYING_STATE, self.MINIMAL_DELAY)


    def perform_seeking_test(self):
        """Test to check if seeking works.

        """
        # Test seeking while playing.
        self.assert_player_state(self.PLAYING_STATE, self.NO_DELAY)
        self.seek_video(self.PSEUDO_RANDOM_TIME_1)
        time.sleep(self.MINIMAL_DELAY)
        if not self.tab.EvaluateJavaScript(
                'window.__getCurrentTime() >= %f;' % self.PSEUDO_RANDOM_TIME_1):
            raise error.TestError(
                    'perform_seeking_test failed because player time is not '
                    'the expected time during playing seeking.')
        self.assert_event_state(
                'seeking', operator.truth,
                'perform_seeking_test failed: "seeking" state did not fire.')
        self.assert_event_state(
                'seeked', operator.truth,
                'perform_seeking_test failed: "seeked" state did not fire.')

        # Now make sure the video is still playing.

        # Let it buffer/play for at most 10 seconds before continuing.
        self.assert_player_state(self.PLAYING_STATE, self.MAX_REBUFFER_DELAY)

        self.clear_event_state('seeking');
        self.clear_event_state('seeked');
        self.assert_event_state(
                'seeking', operator.not_,
                'perform_seeking_test failed: '
                '"seeking" state did not get cleared.')
        self.assert_event_state(
                'seeked', operator.not_,
                'perform_seeking_test failed: '
                '"seeked" state did not get cleared.')

        # Test seeking while paused.
        self.pause_video()
        self.assert_player_state(self.PAUSED_STATE, self.MINIMAL_DELAY)

        self.seek_video(self.PSEUDO_RANDOM_TIME_2)
        time.sleep(self.MINIMAL_DELAY)
        if not self.tab.EvaluateJavaScript(
                'window.__getCurrentTime() === %f;' %
                self.PSEUDO_RANDOM_TIME_2):
            raise error.TestError(
                    'perform_seeking_test failed because player time is not '
                    'the expected time.')
        self.assert_event_state(
                'seeking', operator.truth,
                'perform_seeking_test failed: "seeking" state did not fire '
                'again.')
        self.assert_event_state(
                'seeked', operator.truth,
                'perform_seeking_test failed: "seeked" state did not fire '
                'again.')

        # Make sure the video is paused.
        self.assert_player_state(self.PAUSED_STATE, self.NO_DELAY)


    def perform_frame_drop_test(self):
        """Test to check if there are too many dropped frames.

        """
        self.assert_player_state(self.PLAYING_STATE, self.NO_DELAY)
        time.sleep(15)
        dropped_frames_percentage = self.tab.EvaluateJavaScript(
                'window.__videoElement.webkitDroppedFrameCount /'
                'window.__videoElement.webkitDecodedFrameCount')
        if dropped_frames_percentage > 0.01:
            raise error.TestError((
                    'perform_frame_drop_test failed due to too many dropped '
                    'frames (%f%%)') % (dropped_frames_percentage * 100))


    def perform_ending_test(self):
        """Test to check if the state is 'ended' at the end of a video.

        """
        ALMOST_END = 0.1
        self.assert_player_state(self.PLAYING_STATE, self.NO_DELAY)
        self.seek_to_almost_end(ALMOST_END)
        self.assert_player_state(self.ENDED_STATE, self.MAX_REBUFFER_DELAY)


    def perform_last_second_test(self):
        """Test to check if the last second is played.

        """
        NEAR_END = 2.0
        self.assert_player_state(self.PLAYING_STATE, self.NO_DELAY)
        self.seek_to_almost_end(NEAR_END)
        self.assert_player_state(
                self.ENDED_STATE, self.MAX_REBUFFER_DELAY + NEAR_END)
        self.verify_last_second_playback()


    def run_once(self, subtest_name):
        """Main runner for the test.

        @param subtest_name: The name of the test to run, given below.

        """
        extension_paths = []
        if self.DISABLE_COOKIES:
            # To stop the system from erasing the previous profile, enable:
            #  options.dont_override_profile = True
            extension_path = os.path.join(
                    os.path.dirname(__file__),
                    'files/cookie-disabler')
            extension_paths.append(extension_path)


        with chrome.Chrome(extension_paths=extension_paths) as cr:
            self.initialize_test(cr, self.TEST_PAGE)

            if subtest_name is 'playing':
                self.perform_playing_test()
            elif subtest_name is 'pausing':
                self.perform_pausing_test()
            elif subtest_name is 'resuming':
                self.perform_resuming_test()
            elif subtest_name is 'seeking':
                self.perform_seeking_test()
            elif subtest_name is 'frame_drop':
                self.perform_frame_drop_test()
            elif subtest_name is 'ending':
                self.perform_ending_test()
            elif subtest_name is 'last_second':
                self.perform_last_second_test()
