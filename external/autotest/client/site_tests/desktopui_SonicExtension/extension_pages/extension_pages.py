# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""The generla class for extension pages."""

import web_elements


class ExtensionPages(object):
    """Contains all the functions of the options page of the extension."""

    def __init__(self, driver, url):
        """Constructor."""
        self._driver = driver
        self._url = url


    def go_to_page(self):
        """Go to options page if current page is not options page."""
        if self._driver.current_url != self._url:
            self._driver.get(self._url)
            self._driver.refresh()


    def _get_button(self, element_id, name=None):
        """Returns the button object on a page.

        @param element_id: The ID of the element
        @param name: The name of the button
        @return The button object
        """
        self.go_to_page()
        return web_elements.Button(self._driver, element_id, name)


    def _get_check_box(self, element_id, name=None):
        """Returns the check box object on a page.

        @param element_id: The ID of the element
        @param name: The name of the check box
        @return The check box object
        """
        self.go_to_page()
        return web_elements.CheckBox(self._driver, element_id, name)


    def _get_text_box(self, element_id, name=None):
        """Returns the text box object on a page.

        @param element_id: The ID of the element
        @param name: The name of the text box
        @return The text box object
        """
        self.go_to_page()
        return web_elements.TextBox(self._driver, element_id, name)


    def _get_radio_button(self, element_id, name=None):
        """Returns the radio button object on a page.

        @param element_id: The ID of the element
        @param name: The name of the radio button
        @return The radio button object
        """
        self.go_to_page()
        return web_elements.RadioButton(self._driver, element_id, name)


    def _get_scroll_box(self, element_id, name=None):
        """Returns the scroll box object on a page.

        Args:
            element_id: The ID of the element
            name: The name of the scroll box
        Returns:
            The scroll box object
        """
        self.go_to_page()
        return web_elements.ScrollBox(self._driver, element_id, name)


    def _get_web_element_box(self, element_id, name=None):
        """Returns the web element box object on a page.

        @param element_id: The ID of the element
        @param name: The name of the web element box
        @return The web element box object
        """
        self.go_to_page()
        return web_elements.WebElementBox(self._driver, element_id, name)


    def get_extension_version(self):
        """Returns the cast extension version based on its extension ID.

        @return The version number, in string, of the Cast extension
        """
        self.go_to_page()
        get_extension_version_js = ('return chrome.runtime.'
                                    'getManifest().version;')
        return self._driver.execute_script(get_extension_version_js)


    def get_extension_name(self):
        """Returns the cast extension name based on its extension ID.

        @return The name of the Cast extension
        """
        self.go_to_page()
        get_extension_name_js = 'return chrome.runtime.getManifest().name;'
        return self._driver.execute_script(get_extension_name_js)


    def execute_script(self, js_script):
        """Executes the javascript code in current page context.

        @param js_script: the javascript code to be executed.
        @return The value returned by javascript code.
        """
        self.go_to_page()
        return self._driver.execute_script(js_script)
