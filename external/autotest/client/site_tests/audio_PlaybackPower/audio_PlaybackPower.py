# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import hashlib, logging, os, time


from autotest_lib.client.bin import test
from autotest_lib.client.common_lib import error, file_utils
from autotest_lib.client.common_lib.cros import chrome
from autotest_lib.client.cros import power_status, power_utils, service_stopper

_DOWNLOAD_BASE = ('http://commondatastorage.googleapis.com/'
                  'chromiumos-test-assets-public/audio_power/')

# Minimum battery charge percentage to run the test
BATTERY_INITIAL_CHARGED_MIN = 10

# Measurement duration in seconds.
MEASUREMENT_DURATION = 150

POWER_DESCRIPTION = 'avg_energy_rate_'

# Time to exclude from calculation after playing audio [seconds].
STABILIZATION_DURATION = 10


class audio_PlaybackPower(test.test):
    """Captures power usage for audio playback."""

    version = 1


    def initialize(self):
        self._service_stopper = None
        self._backlight = None

    def run_power_test(self, audio_type):
        """
        Captures power usage and reports it to the perf dashboard.

        @param audio_type: audio format label to attach with perf keyval.
        """

        self._backlight = power_utils.Backlight()
        self._backlight.set_default()

        self._service_stopper = service_stopper.ServiceStopper(
                service_stopper.ServiceStopper.POWER_DRAW_SERVICES)
        self._service_stopper.stop_services()

        self._power_status = power_status.get_status()
        # Verify that we are running on battery and the battery is sufficiently
        # charged.
        self._power_status.assert_battery_state(BATTERY_INITIAL_CHARGED_MIN)

        measurements = [power_status.SystemPower(
                self._power_status.battery_path)]

        def get_power():
            power_logger = power_status.PowerLogger(measurements)
            power_logger.start()
            time.sleep(STABILIZATION_DURATION)
            start_time = time.time()
            time.sleep(MEASUREMENT_DURATION)
            power_logger.checkpoint('result', start_time)
            keyval = power_logger.calc()
            logging.info('Power output %s', keyval)
            return keyval['result_' + measurements[0].domain + '_pwr']

        energy_rate = get_power()
        perf_keyval = {}
        perf_keyval[POWER_DESCRIPTION + audio_type] = energy_rate
        self.output_perf_value(description=POWER_DESCRIPTION + audio_type,
                               value=energy_rate, units='W',
                               higher_is_better=False)
        self.write_perf_keyval(perf_keyval)


    def run_once(self, test_file, checksum):
        local_path = os.path.join(self.bindir, '%s' % test_file)
        file_utils.download_file(_DOWNLOAD_BASE + test_file, local_path)
        logging.info('Downloaded file: %s. Expected checksum: %s',
                     local_path, checksum)
        with open(local_path, 'r') as r:
            md5sum = hashlib.md5(r.read()).hexdigest()
            if md5sum != checksum:
                raise error.TestError('unmatched md5 sum: %s' % md5sum)
        with chrome.Chrome() as cr:
            cr.browser.platform.SetHTTPServerDirectories(self.bindir)
            url = cr.browser.platform.http_server.UrlOf(local_path)
            self.play_audio(cr.browser.tabs[0], url)
            self.run_power_test(url.split('.')[-1])


    def play_audio(self, tab, url):
        """Navigates to an audio file over http and plays it in loop.

        @param tab: tab to open an audio stream.
        @url: audio/video test url.
        """
        tab.Navigate(url)
        tab.ExecuteJavaScript(
                "document.getElementsByTagName('video')[0].loop=true")
        tab.ExecuteJavaScript(
                "document.getElementsByTagName('video')[0].volume=1")


    def cleanup(self):
        # cleanup() is run by common_lib/test.py.
        if self._backlight:
            self._backlight.restore()
        if self._service_stopper:
            self._service_stopper.restore_services()

        super(audio_PlaybackPower, self).cleanup()
