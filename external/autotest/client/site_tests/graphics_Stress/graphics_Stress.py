# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""This is a client side graphics stress test."""

import logging, os, time

from autotest_lib.client.bin import test
from autotest_lib.client.common_lib.cros import chrome
from autotest_lib.client.cros import service_stopper
from autotest_lib.client.cros.graphics import graphics_utils


BIG_BUCK_BUNNY_VM_URL = 'http://vimeo.com/1084537'
BIG_BUCK_BUNNY_YT_URL = 'https://www.youtube.com/watch?v=YE7VzlLtp-4'
GMAPS_MTV_URL = 'https://www.google.com/maps/@37.4249155,-122.072205,13z?force=webgl'
PEACEKEEPER_URL = 'http://peacekeeper.futuremark.com/run.action'
WEBGL_AQUARIUM_URL = \
    'https://webglsamples.googlecode.com/hg/aquarium/aquarium.html'
WEBGL_BLOB_URL = 'https://webglsamples.googlecode.com/hg/blob/blob.html'
WEBGL_SPIRITBOX_URL = \
    'http://www.webkit.org/blog-files/webgl/SpiritBox.html'
VIMEO_COUCHMODE_URL = 'http://vimeo.com/couchmode/'


class graphics_Stress(test.test):
    """Graphics stress test."""
    version = 1


    def initialize(self):
        self.GSC = graphics_utils.GraphicsStateChecker()


    def cleanup(self):
        if self.GSC:
            keyvals = self.GSC.get_memory_keyvals()
            for key, val in keyvals.iteritems():
                self.output_perf_value(description=key, value=val,
                                       units='bytes', higher_is_better=False)
            self.GSC.finalize()
            self.write_perf_keyval(keyvals)


    def setup(self):
        self.job.setup_dep(['graphics'])


    def new_chrome(self):
        return chrome.Chrome(extension_paths=self.ext_paths,
                             logged_in=True,
                             autotest_ext=True)


    def create_window(self, cr, url):
        cmd = 'chrome.windows.create( { url: \'%s\' } )' % ( url )
        cr.autotest_ext.ExecuteJavaScript(cmd)
        tab = cr.browser.tabs[-1]
        return tab


    def open_urls(self, cr, url_list, window=True):
        """Opens a list of the given urls.
        @param browser: The Browser object to run the test with.
        @param url_list: The list of URLs to open.
        """
        tabs = []
        first = True
        cr.browser.tabs[0].WaitForDocumentReadyStateToBeComplete()

        for url in url_list:
            tab = None
            if first:
                tab = cr.browser.tabs[0]
                tab.Navigate(url)
            else:
                if window:
                    tab = self.create_window(cr, url)
                else:
                    tab = cr.browser.tabs.New()
                    tab.Navigate(url)

            logging.info('Opening URL %s', url)
            first = False
            tab.WaitForDocumentReadyStateToBeComplete()
            tab.Activate()
            tabs.append(tab)
        return tabs


    def maps_zoom_cycle(self):
        """Performs one cycle of the maps zooming."""
        # Zoom in on purpose once more than out.
        for _ in range(1, 11):
            graphics_utils.press_keys(['KEY_KPPLUS'])
            time.sleep(0.1)
        time.sleep(0.5)
        for _ in range(1, 10):
            graphics_utils.press_keys(['KEY_KPMINUS'])
            time.sleep(0.1)
        time.sleep(0.5)


    def fifty_spirits_test(self):
        """ Open 50 tabs of WebGL SpiritBox, and let them run for a while. """
        with self.new_chrome() as cr:
            tabs = self.open_urls(cr, [WEBGL_SPIRITBOX_URL] * 50,
                                  window=False)
            time.sleep(self._test_duration_secs - (time.time() - self._start_time))
            for tab in tabs:
                tab.Close()


    def blob_aquarium_yt_test(self):
        """ Open WebGL Blob, WebGL Aquarium, and Youtube video,
        and switch between tabs, pausing for 2 seconds at each tab, for the
        duration of the test. """
        with self.new_chrome() as cr:
            tabs = self.open_urls(cr,
                                  [WEBGL_BLOB_URL,
                                   WEBGL_AQUARIUM_URL,
                                   BIG_BUCK_BUNNY_YT_URL],
                                  window=False)

            tabidx = 0
            while time.time() - self._start_time < self._test_duration_secs:
                cr.browser.tabs[tabidx].Activate()
                tabidx = (tabidx + 1) % len(cr.browser.tabs)
                time.sleep(2)

            for tab in tabs:
                tab.Close()


    def gmaps_test(self):
        """ Google Maps test. Load maps and zoom in and out. """
        with self.new_chrome() as cr:
            tabs = self.open_urls(cr, [GMAPS_MTV_URL])

            # Click into the map area to achieve focus.
            time.sleep(5)
            graphics_utils.click_mouse()

            # Do the stress test.
            cycle = 0
            while time.time() - self._start_time < self._test_duration_secs:
                logging.info('Maps zoom cycle %d', cycle)
                cycle += 1
                self.maps_zoom_cycle()

            for tab in tabs:
                tab.Close()


    def peacekeeper_test(self):
        """ Run Futuremark Peacekeeper benchmark. """
        with self.new_chrome() as cr:
            tabs = self.open_urls(cr, [PEACEKEEPER_URL])
            time.sleep(self._test_duration_secs - (time.time() - self._start_time))
            for tab in tabs:
                tab.Close()


    def restart_test(self):
        """ Restart UI, excercises X server startup and shutdown and related
        kernel paths. """
        # Ui respawn will reboot us if we restart ui too many times, so stop
        # it for the duration of the test.
        stopped_services = service_stopper.ServiceStopper(['ui-respawn'])
        stopped_services.stop_services()

        while time.time() - self._start_time < self._test_duration_secs:
            stopped_ui = service_stopper.ServiceStopper(['ui'])
            stopped_ui.stop_services()
            time.sleep(1)
            stopped_ui.restore_services()

        stopped_services.restore_services()


    def tab_open_close_test(self):
        """ Open 10 tabs of WebGL SpiritBox, close them, repeat. """
        with self.new_chrome() as cr:
            while time.time() - self._start_time < self._test_duration_secs:
                tabs = self.open_urls(cr,
                                      [WEBGL_SPIRITBOX_URL] * 10,
                                      window=False)
                for tab in tabs:
                    tab.Close()
                time.sleep(1)


    def yt_vimeo_webgl_test(self):
        """ Youtube + Vimeo + WebGL, just running at the same time. """
        with self.new_chrome() as cr:
            tabs = self.open_urls(cr,
                                  [BIG_BUCK_BUNNY_YT_URL,
                                   VIMEO_COUCHMODE_URL,
                                   WEBGL_AQUARIUM_URL])
            time.sleep(self._test_duration_secs - (time.time() - self._start_time))
            for tab in tabs:
                tab.Close()


    subtests = {
        '50spirit' : fifty_spirits_test,
        'blob+aquarium+yt' : blob_aquarium_yt_test,
        'gmaps' : gmaps_test,
        'peacekeeper' : peacekeeper_test,
        'restart' : restart_test,
        'tabopenclose' : tab_open_close_test,
        'yt+vimeo+webgl' : yt_vimeo_webgl_test
    }


    def run_once(self, test_duration_secs=600, fullscreen=True, subtest='gmaps'):
        """Finds a browser with telemetry, and runs the test.

        @param test_duration_secs: The test duration in seconds.
        @param fullscreen: Whether to run the test in fullscreen.
        """
        self._start_time = time.time()
        self._test_duration_secs = test_duration_secs
        self._fullscreeen = fullscreen

        self.ext_paths = []
        if fullscreen:
            self.ext_paths.append(
                os.path.join(self.autodir, 'deps', 'graphics',
                             'graphics_test_extension'))

        self.subtests[subtest](self)

