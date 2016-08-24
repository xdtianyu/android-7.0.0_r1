# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import errno, os, stat

from autotest_lib.client.bin import test, utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros import chrome
from autotest_lib.client.cros import constants, httpd


def _respond_with_cookies(handler, url_args):
    """set_cookie response.

    Responds with a Set-Cookie header to any GET request, and redirects to a
    chosen URL.

    @param handler: handler for set_cookie.
    @param url_args: arguments passed through the url.

    """
    handler.send_response(303)
    handler.send_header('Set-Cookie', 'name=value')
    handler.send_header('Location', url_args['continue'][0])
    handler.end_headers()
    handler.wfile.write('Got form data:\n')
    handler.wfile.write('%s:\n' % url_args)


class login_ChromeProfileSanitary(test.test):
    """Tests that the browser uses the correct profile after a crash."""
    version = 1


    def __get_cookies_mtime(self):
        try:
            cookies_info = os.stat(constants.LOGIN_PROFILE + '/Cookies')
            return cookies_info[stat.ST_MTIME]
        except OSError as e:
            if e.errno == errno.ENOENT:
                return None
            raise


    def initialize(self):
        spec = 'http://localhost:8000'
        path = '/set_cookie'
        self._wait_path = '/test_over'
        self._test_url = spec + path + '?continue=' + spec + self._wait_path
        self._testServer = httpd.HTTPListener(8000, docroot=self.srcdir)
        self._testServer.add_url_handler(path, _respond_with_cookies)
        self._testServer.run()


    def cleanup(self):
        self._testServer.stop()


    def run_once(self, timeout=10):
        with chrome.Chrome() as cr:
            # Get Default/Cookies mtime. None means no Cookies DB.
            cookies_mtime = self.__get_cookies_mtime()

            # Wait for chrome to show, then "crash" it.
            utils.nuke_process_by_name(constants.BROWSER, with_prejudice=True)

            cr.wait_for_browser_to_come_up()

            latch = self._testServer.add_wait_url(self._wait_path)

            # Navigate to site that leaves cookies.
            cr.browser.tabs[0].Navigate(self._test_url)
            latch.wait(timeout)
            if not latch.is_set():
                raise error.TestError('Never received callback from browser.')

        # Ensure chrome writes state to disk.
        with chrome.Chrome():
            # Check mtime of Default/Cookies.  If changed, KABLOOEY.
            new_cookies_mtime = self.__get_cookies_mtime()

            if cookies_mtime != new_cookies_mtime:
                if not cookies_mtime and new_cookies_mtime:
                    raise error.TestFail('Cookies created in Default profile!')
                raise error.TestFail('Cookies in Default profile changed!')
