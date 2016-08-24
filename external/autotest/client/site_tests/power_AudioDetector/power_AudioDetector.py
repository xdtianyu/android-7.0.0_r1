# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging, threading, time
from autotest_lib.client.bin import test, utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros import chrome
from autotest_lib.client.cros import power_utils, rtc
from autotest_lib.client.cros.audio import audio_helper

class power_AudioDetector(test.test):
    """Verifies that audio playback prevents powerd from suspending."""
    version = 1

    def run_once(self, run_time_sec=60):
        if run_time_sec < 10:
            raise error.TestFail('Must run for at least 10 seconds')

        with chrome.Chrome():
            # Audio loop time should be significantly shorter than
            # |run_time_sec| time, so that the total playback time doesn't
            # exceed it by much.
            audio_loop_time_sec = min(10, run_time_sec / 10 + 0.5)

            # Set a low audio volume to avoid annoying people during tests.
            audio_helper.set_volume_levels(10, 100)

            # Start playing audio file.
            self._enable_audio_playback = True
            thread = threading.Thread(target=self._play_audio,
                                      args=(audio_loop_time_sec,))
            thread.start()

            # Restart powerd with timeouts for quick idle events.
            gap_ms = run_time_sec * 1000 / 4
            dim_ms = min(10000, gap_ms)
            off_ms = min(20000, gap_ms * 2)
            suspend_ms = min(30000, gap_ms * 3)
            prefs = { 'disable_idle_suspend'   : 0,
                      'ignore_external_policy' : 1,
                      'plugged_dim_ms'         : dim_ms,
                      'plugged_off_ms'         : off_ms,
                      'plugged_suspend_ms'     : suspend_ms,
                      'unplugged_dim_ms'       : dim_ms,
                      'unplugged_off_ms'       : off_ms,
                      'unplugged_suspend_ms'   : suspend_ms }
            self._pref_change = power_utils.PowerPrefChanger(prefs)

            # Set an alarm to wake up the system in case the audio detector
            # fails and the system suspends.
            alarm_time = rtc.get_seconds() + run_time_sec
            rtc.set_wake_alarm(alarm_time)

            time.sleep(run_time_sec)

            # Stop powerd to avoid suspending when the audio stops.
            utils.system_output('stop powerd')

            # Stop audio and wait for the audio thread to terminate.
            self._enable_audio_playback = False
            thread.join(timeout=(audio_loop_time_sec * 2))
            if thread.is_alive():
                logging.error('Audio thread did not terminate at end of test.')

            # Check powerd's log to make sure that no suspend took place.
            powerd_log_path = '/var/log/power_manager/powerd.LATEST'
            log = open(powerd_log_path, 'r').read()
            if log.find('Starting suspend') != -1:
                err_str = 'System suspended while audio was playing.'
                raise error.TestFail(err_str)


    def cleanup(self):
        utils.restart_job('powerd')


    def _play_audio(self, loop_time):
        """
        Repeatedly plays audio until self._audio_playback_enabled == False.
        """
        # TODO(crosbug.com/33988): Allow for pauses in audio playback to
        # simulate delays in loading the next song.
        while self._enable_audio_playback:
            audio_helper.play_sound(duration_seconds=loop_time)
        logging.info('Done playing audio.')
