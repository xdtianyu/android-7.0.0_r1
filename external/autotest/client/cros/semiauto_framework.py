# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import cgi, os

from autotest_lib.client.bin import test
from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib.cros import chrome

_DEFAULT_TIMEOUT = 60  # Seconds a tester has to respond to prompts


class semiauto_test(test.test):
    """Base class for semiauto tests in ChromeOS.

    All these tests use telemetry and a simple interative webpage that
    navigates the user through the test.
    """
    version = 1

    def login_and_open_interactive_tab(self):
        """Log in to machine, open browser, and navigate to dialog template.

        Dialog template is on first tab.  Any other needed tabs can be opened
        using the self._browser object.
        """
        self._browser = chrome.Chrome().browser
        self._tab = self._browser.tabs[0]
        self._browser.platform.SetHTTPServerDirectories(
                os.path.join(self.bindir, '..', '..', 'cros'))
        self._tab.Navigate(self._browser.platform.http_server.UrlOf(
                '/semiauto_shell.html'))

    def close_browser(self):
        """Close browser if open."""
        if self._browser:
            self._browser.Close()

    def set_tab(self, html):
        """Replace the body of self._tab with provided html.

        @param html: the HTML that will replace the body of the dialog tab.
        """
        html_esc = html.replace('"', '\\"')

        # Use JavaScript to set the output.
        self._tab.ExecuteJavaScript('window.__ready = 0; '
                                    'document.body.innerHTML="%s";' % html_esc)
        self._tab.Activate()
        self._tab.WaitForDocumentReadyStateToBeInteractiveOrBetter()

    def clear_output(self):
        """Replace the body of self._tab with a blank screen.
        """
        self.set_tab('')

    def set_tab_with_buttons(self, html, buttons=['OK']):
        """Replace the body of self._tab with provided html and buttons.

        @param html: the HTML that will replace the body of the dialog tab.
        @param buttons: the titles of some number of buttons to add to the
                        page. Each button has an integer value, starting from
                        0 for the first. Defaults to an 'OK' button.
        """
        html_total = html+'<br>'
        index = 0
        for title in buttons:
            onclick = 'submit_button(%d)' % index
            html_total += ('<input type="button" value="%s" onclick="%s">' % (
                           cgi.escape(title), onclick))
            index += 1
        self.set_tab(html_total)

    def set_tab_with_textbox(self, html, title=''):
        """Replace the body of self._tab with provided html and a textbox.

        Adds a textbox and Submit button to the page.  The value returned after
        clicking the button is the text that was entered in the textbox.

        @param html: the HTML that will replace the body of the dialog tab.
        @param title: the title put next to the textbox.
        """
        textbox = '%s<input type="text" id="textinput"/>' % title
        button = '<input type="button" value="SUBMIT" onclick="get_text()"/>'
        html_total = '%s<br>%s<br>%s' % (html, textbox, button)
        self.set_tab(html_total)

    def wait_for_tab_result(self, timeout=_DEFAULT_TIMEOUT):
        """Wait for interactive tab to be ready and get return value.

        @param timeout: Maximum number of seconds to wait for result.

        @return: value of window.__result.
        """
        complete = lambda: self._tab.EvaluateJavaScript('window.__ready') == 1
        utils.poll_for_condition(condition=complete, timeout=timeout,
                                 desc='User response')

        result = self._tab.EvaluateJavaScript('window.__result')
        self._tab.ExecuteJavaScript('window.__ready = 0; '
                                    'window.__result = null;')
        return result
