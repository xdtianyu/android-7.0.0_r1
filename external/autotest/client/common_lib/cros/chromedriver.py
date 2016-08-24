# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import atexit
import logging
import os
import urllib2
import urlparse

try:
    from selenium import webdriver
except ImportError:
    # Ignore import error, as this can happen when builder tries to call the
    # setup method of test that imports chromedriver.
    logging.error('selenium module failed to be imported.')
    pass

from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib.cros import chrome

CHROMEDRIVER_EXE_PATH = '/usr/local/chromedriver/chromedriver'
X_SERVER_DISPLAY = ':0'
X_AUTHORITY = '/home/chronos/.Xauthority'


class chromedriver(object):
    """Wrapper class, a context manager type, for tests to use Chrome Driver."""

    def __init__(self, extra_chrome_flags=[], subtract_extra_chrome_flags=[],
                 extension_paths=[], is_component=True, username=None,
                 password=None, server_port=None, skip_cleanup=False,
                 url_base=None, extra_chromedriver_args=None, *args, **kwargs):
        """Initialize.

        @param extra_chrome_flags: Extra chrome flags to pass to chrome, if any.
        @param subtract_extra_chrome_flags: Remove default flags passed to
                chrome by chromedriver, if any.
        @param extension_paths: A list of paths to unzipped extensions. Note
                                that paths to crx files won't work.
        @param is_component: True if the manifest.json has a key.
        @param username: Log in using this username instead of the default.
        @param password: Log in using this password instead of the default.
        @param server_port: Port number for the chromedriver server. If None,
                            an available port is chosen at random.
        @param skip_cleanup: If True, leave the server and browser running
                             so that remote tests can run after this script
                             ends. Default is False.
        @param url_base: Optional base url for chromedriver.
        @param extra_chromedriver_args: List of extra arguments to forward to
                                        the chromedriver binary, if any.
        """
        self._cleanup = not skip_cleanup
        assert os.geteuid() == 0, 'Need superuser privileges'

        # Log in with telemetry
        self._chrome = chrome.Chrome(extension_paths=extension_paths,
                                     is_component=is_component,
                                     username=username,
                                     password=password,
                                     extra_browser_args=extra_chrome_flags)
        self._browser = self._chrome.browser
        # Close all tabs owned and opened by Telemetry, as these cannot be
        # transferred to ChromeDriver.
        self._browser.tabs[0].Close()

        # Start ChromeDriver server
        self._server = chromedriver_server(CHROMEDRIVER_EXE_PATH,
                                           port=server_port,
                                           skip_cleanup=skip_cleanup,
                                           url_base=url_base,
                                           extra_args=extra_chromedriver_args)

        # Open a new tab using Chrome remote debugging. ChromeDriver expects
        # a tab opened for remote to work. Tabs opened using Telemetry will be
        # owned by Telemetry, and will be inaccessible to ChromeDriver.
        urllib2.urlopen('http://localhost:%i/json/new' %
                        utils.get_chrome_remote_debugging_port())

        chromeOptions = {'debuggerAddress':
                         ('localhost:%d' %
                          utils.get_chrome_remote_debugging_port())}
        capabilities = {'chromeOptions':chromeOptions}
        # Handle to chromedriver, for chrome automation.
        try:
            self.driver = webdriver.Remote(command_executor=self._server.url,
                                           desired_capabilities=capabilities)
        except NameError:
            logging.error('selenium module failed to be imported.')
            raise


    def __enter__(self):
        return self


    def __exit__(self, *args):
        """Clean up after running the test.

        """
        if hasattr(self, 'driver') and self.driver:
            self.driver.close()
            del self.driver

        if not hasattr(self, '_cleanup') or self._cleanup:
            if hasattr(self, '_server') and self._server:
                self._server.close()
                del self._server

            if hasattr(self, '_browser') and self._browser:
                self._browser.Close()
                del self._browser

    def get_extension(self, extension_path):
        """Gets an extension by proxying to the browser.

        @param extension_path: Path to the extension loaded in the browser.

        @return: A telemetry extension object representing the extension.
        """
        return self._chrome.get_extension(extension_path)


    @property
    def chrome_instance(self):
        """ The chrome instance used by this chrome driver instance. """
        return self._chrome


class chromedriver_server(object):
    """A running ChromeDriver server.

    This code is migrated from chrome:
    src/chrome/test/chromedriver/server/server.py
    """

    def __init__(self, exe_path, port=None, skip_cleanup=False,
                 url_base=None, extra_args=None):
        """Starts the ChromeDriver server and waits for it to be ready.

        Args:
            exe_path: path to the ChromeDriver executable
            port: server port. If None, an available port is chosen at random.
            skip_cleanup: If True, leave the server running so that remote
                          tests can run after this script ends. Default is
                          False.
            url_base: Optional base url for chromedriver.
            extra_args: List of extra arguments to forward to the chromedriver
                        binary, if any.
        Raises:
            RuntimeError if ChromeDriver fails to start
        """
        if not os.path.exists(exe_path):
            raise RuntimeError('ChromeDriver exe not found at: ' + exe_path)

        chromedriver_args = [exe_path]
        if port:
            # Allow remote connections if a port was specified
            chromedriver_args.append('--whitelisted-ips')
        else:
            port = utils.get_unused_port()
        chromedriver_args.append('--port=%d' % port)

        self.url = 'http://localhost:%d' % port
        if url_base:
            chromedriver_args.append('--url-base=%s' % url_base)
            self.url = urlparse.urljoin(self.url, url_base)

        if extra_args:
            chromedriver_args.extend(extra_args)

        # TODO(ihf): Remove references to X after M45.
        # Chromedriver will look for an X server running on the display
        # specified through the DISPLAY environment variable.
        os.environ['DISPLAY'] = X_SERVER_DISPLAY
        os.environ['XAUTHORITY'] = X_AUTHORITY

        self.bg_job = utils.BgJob(chromedriver_args, stderr_level=logging.DEBUG)
        if self.bg_job is None:
            raise RuntimeError('ChromeDriver server cannot be started')

        try:
            timeout_msg = 'Timeout on waiting for ChromeDriver to start.'
            utils.poll_for_condition(self.is_running,
                                     exception=utils.TimeoutError(timeout_msg),
                                     timeout=10,
                                     sleep_interval=.1)
        except utils.TimeoutError:
            self.close_bgjob()
            raise RuntimeError('ChromeDriver server did not start')

        logging.debug('Chrome Driver server is up and listening at port %d.',
                      port)
        if not skip_cleanup:
            atexit.register(self.close)


    def is_running(self):
        """Returns whether the server is up and running."""
        try:
            urllib2.urlopen(self.url + '/status')
            return True
        except urllib2.URLError as e:
            return False


    def close_bgjob(self):
        """Close background job and log stdout and stderr."""
        utils.nuke_subprocess(self.bg_job.sp)
        utils.join_bg_jobs([self.bg_job], timeout=1)
        result = self.bg_job.result
        if result.stdout or result.stderr:
            logging.info('stdout of Chrome Driver:\n%s', result.stdout)
            logging.error('stderr of Chrome Driver:\n%s', result.stderr)


    def close(self):
        """Kills the ChromeDriver server, if it is running."""
        if self.bg_job is None:
            return

        try:
            urllib2.urlopen(self.url + '/shutdown', timeout=10).close()
        except:
            pass

        self.close_bgjob()
