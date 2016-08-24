# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""The classes for web elements on a page."""

__author__ = 'cliffordcheng@google.com (Clifford Cheng)'

import re
from selenium.webdriver.common.keys import Keys


class WebElements(object):
  """The base class for all the web element controls."""

  def __init__(self, driver, element_id, name=None):
      """Constructor."""
      self._driver = driver
      self._element_id = element_id
      self._name = name
      self._element = driver.find_element_by_id(element_id)


  def get_name(self):
      """
      Returns the name of a web element.

      @return The name of a web element
      """
      return self._name


  def get_element_id(self):
      """
      Returns the element ID of a web element.

      @return The element ID of a web element
      """
      return self._element_id


  def get_element(self):
      """
      Returns the object of a web element.

      @return The web element object
      """
      return self._element


class TextBox(WebElements):
    """Web element textbox and its controls."""


    def get_value(self):
        """
        Returns the value in the text box.

        @return The value in the text box
        """
        return self._element.get_attribute('value')


    def set_value(self, value):
        """
        Set a value in the text box.

        @param value: The value to be set in the text box
        @raises RuntimeError if an error occurred when setting values
        """
        # Using backspace instead of clear() because of crbug/450812
        # TODO(cliffordcheng): Revert this once the bug is fixed
        original_value = self._element.get_attribute('value')
        for i in range(len(original_value)):
            self._element.send_keys('\b')
        self._element.send_keys(value)
        if self.get_value() != value:
            raise RuntimeError(
                    'Failed to set value "%s"', self._element.get_name())


class Button(WebElements):
    """Web element button and its controls."""

    def click(self):
        """
        Click on the button.
        """
        self._element.click()


class CheckBox(WebElements):
    """Web element checbkbox and its controls."""

    def set_value(self, value):
        """
        Set a value in the check box.

        @param value: The value ('on'/'off') to be set in the check box
        """
        if value == 'on' and not self.get_value():
              self._element.click()
        elif value == 'off' and self.get_value():
              self._element.click()


    def get_value(self):
        """
        Return the value of the check box.

        @return True if the check box is checked, otherwise return False.
        """
        return self._element.is_selected()


class RadioButton(WebElements):
    """Web element radio button and its controls."""

    def click(self):
        """Click the radio button."""
        self._element.send_keys(Keys.SPACE)
        # Click one more time to ensure it's clicked
        # In some corner cases, it needs a second click
        self._element.send_keys(Keys.SPACE)


class ScrollBox(WebElements):
    """Web element scroll box and its controls."""

    def get_value(self):
        """
        Return the text in the scroll box.

        @return The string in the scroll box
        """
        get_inner_html = 'return document.getElementById("%s").innerHTML;'
        get_inner_html %= self._element_id
        inner_html = self._driver.execute_script(get_inner_html)
        inner_text_search = re.search('<.*>(.*)<.*>', inner_html)
        try:
            return inner_text_search.group(1)
        except AttributeError:
            return None


class WebElementBox(WebElements):
    """Web element box and its controls."""

    def get_value(self):
        """
        Return the text in the web element box.

        @return The string in the web element box
        """
        get_inner_html = 'return document.getElementById("%s").innerHTML;'
        get_inner_html %= self._element_id
        inner_html = self._driver.execute_script(get_inner_html)
        inner_text_search = re.search('<.*>(.*)<.*>', inner_html)
        try:
            return inner_text_search.group(1)
        except AttributeError:
            return None
