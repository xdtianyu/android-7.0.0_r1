# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import time

from selenium.common.exceptions import NoSuchElementException
from selenium.common.exceptions import TimeoutException as \
    SeleniumTimeoutException
from selenium.common.exceptions import WebDriverException
from selenium.webdriver.support.ui import WebDriverWait

class WebDriverCoreHelpers(object):
    """Base class for manipulating web pages using webdriver."""

    def __init__(self):
        super(WebDriverCoreHelpers, self).__init__()
        self.driver = None
        self.wait = WebDriverWait(self.driver, timeout=5)


    def _check_for_alert_in_message(self, message, alert_handler):
        """Check for an alert in error message and handle it.

        @param message: The error message.
        @param alert_handler: The handler method to call.

        """
        if (message.find('An open modal dialog blocked') != -1 and
            message.find('unexpected alert open') != -1):
            alert = self.driver.switch_to_alert()
            alert_handler(alert)
        else:
            raise RuntimeError(message)


    def _handle_alert(self, xpath, alert_handler):
        """Calls the alert handler if there is an alert.

        @param xpath: The xpath that could raise the alert.
        @param alert_handler: the handler method to call.

        """
        try:
            self.driver.find_element_by_xpath(xpath)
            return
        except WebDriverException, e:
            message = str(e)
            # The messages differ based on the webdriver version
            if (message.find('An open modal dialog blocked') == -1 and
                message.find('unexpected alert open') == -1):
                return
            self._handler(alert_handler)
        # Sometimes routers put out multiple alert statements on the same page.
        self._handle_alert(xpath, alert_handler)


    def _handler(self, alert_handler):
        """Handles the alert.

        @param alert_handler: The custom handler method to call.

        """
        alert = self.driver.switch_to_alert()
        if not alert_handler:
            # The caller did not provide us with a handler, dismiss and raise.
            try:
                alert_text = alert.text
            except WebDriverException:
                # There is a bug in selenium where the alert object will exist
                # but you can't get to the text object right away.
                time.sleep(1)
            alert_text = alert.text
            alert.accept()
            raise RuntimeError('An alert was encountered and no handler was '
                               'specified.  The text from the alert was: %s'
                               % alert_text)
        alert_handler(alert)


    def set_wait_time(self, time):
        """Sets the wait time of webdriver commands.

        @param time: the time to wait in seconds.

        """
        self.wait = WebDriverWait(self.driver, timeout=time)


    def restore_default_wait_time(self):
        """Restores the default webdriver wait time."""
        self.wait = WebDriverWait(self.driver, timeout=5)


    def wait_for_objects_by_id(self, element_ids, wait_time=5):
        """Wait for one of the element_ids to show up.

        @param element_ids: A list of all the element ids to find.
        @param wait_time: The time to wait before giving up.

        @return The id that was found first.

        """
        xpaths = []
        for element_id in element_ids:
            xpaths.append('id("%s")' % element_id)
        xpath_found = self.wait_for_objects_by_xpath(xpaths, wait_time)
        for element_id in element_ids:
            if element_id in xpath_found:
                return element_id


    def wait_for_objects_by_xpath(self, xpaths, wait_time=5):
        """Wait for one of the items in the xpath to show up.

        @param xpaths: A list of all the xpath's of elements to find.
        @param wait_time: The time to wait before giving up.

        @return The xpath that was found first.

        """
        excpetion = None
        if wait_time < len(xpaths):
            wait_time = len(xpaths)
        start_time = int(time.time())
        while (int(time.time()) - start_time) < wait_time:
            for xpath in xpaths:
                try:
                    element = self.wait_for_object_by_xpath(xpath,
                                                            wait_time=0.25)
                    if element and element.is_displayed():
                        return xpath
                except SeleniumTimeoutException, e:
                    exception = str(e)
                    pass
        raise SeleniumTimeoutException(exception)


    def click_button_by_id(self, element_id, alert_handler=None):
        """Clicks a button by id.

        @param element_id: the id of the button
        @param alert_handler: method invoked if an alert is detected. The method
                              must take one parameter, a webdriver alert object

        """
        xpath = 'id("%s")' % element_id
        return self.click_button_by_xpath(xpath, alert_handler)


    def click_button_by_xpath(self, xpath, alert_handler=None):
        """Clicks a button by xpath.

        @param xpath: the xpath of the button
        @param alert_handler: method invoked if an alert is detected. The method
                              must take one parameter, a webdriver alert object

        """
        button = self.wait_for_object_by_xpath(xpath)
        button.click()
        self._handle_alert(xpath, alert_handler)


    def get_url(self, page_url, page_title=None, element_xpath=None):
        """Load page and check if the page loads completely, if not, reload.

        @param page_url: The url to load.
        @param page_title: The complete/partial title of the page after loaded.
        @param element_xpath: The element that we search for to confirm that
                              the page loaded.

        """
        self.driver.get(page_url)
        if page_title:
            try:
                self.wait.until(lambda _: page_title in self.driver.title)
            except SeleniumTimeoutException, e:
                self.driver.get(page_url)
                self.wait.until(lambda _: self.driver.title)
            finally:
                if not page_title in self.driver.title:
                    raise WebDriverException('Page did not load. Expected %s in'
                                             'title, but got %s as title.' %
                                             (page_title, self.driver.title))
        if element_xpath:
            self.wait_for_object_by_xpath(element_xpath)


    def wait_for_object_by_id(self, element_id, wait_time=5):
        """Waits for an element to become available; returns a reference to it.

        @param element_id: the id of the element to wait for
        @param wait_time: the time to wait for the object

        @returns a reference to the element if found before a timeout.

        """
        xpath = 'id("%s")' % element_id
        return self.wait_for_object_by_xpath(xpath, wait_time=wait_time)


    def wait_for_object_by_xpath_to_vanish(self, xpath, wait_time=5):
        """Wait for the item in xpath to disappear from page.

        @param xpath: The xpath of the object to wait on.
        @param wait_time: The time to wait before giving up.

        @return void or raise exception if object does not vanish.

        """
        start_time = int(time.time())
        while (int(time.time()) - start_time) < wait_time:
            if self.object_by_xpath_exist(xpath):
                time.sleep(0.5)
            else:
                return
        raise SeleniumTimeoutException('The object with xpath %s failed to'
                                       ' vanish.' % xpath)


    def wait_for_object_by_id_to_vanish(self, element_id, wait_time=5):
        """Wait for the item in xpath to disappear from page.

        @param element_id: The id of the object to wait on.
        @param wait_time: The time to wait before giving up.

        @return void or raise exception if object does not vanish.

        """
        xpath = 'id("%s")' % element_id
        return self.wait_for_object_by_xpath_to_vanish(xpath,
                                                       wait_time=wait_time)


    def object_by_id_exist(self, element_id):
        """Finds if an object exist in this particular page.

        @param element_id: the id of the element to find

        @returns True if the element exists. False if the element does not.

        """
        xpath = 'id("%s")' % element_id
        return self.object_by_xpath_exist(xpath)


    def object_by_xpath_exist(self, xpath):
        """Finds if an object exist in this particular page.

        @param xpath: the xpath of the element to find

        @returns True if the xpath exists. False if the xpath does not.

        """
        try:
            self.wait_for_object_by_xpath(xpath)
        except SeleniumTimeoutException:
            return False
        return True


    def wait_for_object_by_xpath(self, xpath, wait_time=5):
        """Waits for an element to become available; returns a reference to it.

        @param xpath: the xpath of the element to wait for
        @param wait_time: the time to wait for the object.

        @returns reference to the element if found before a timeout.

        """
        self.set_wait_time(wait_time)
        try:
            self.wait.until(lambda _: self.driver.find_element_by_xpath(xpath))
            element = self.driver.find_element_by_xpath(xpath)
        except (SeleniumTimeoutException, NoSuchElementException) as e:
            self.restore_default_wait_time()
            raise SeleniumTimeoutException('Unable to find the object by '
                                           'xpath: %s\n WebDriver exception: '
                                           '%s' % (xpath, str(e)))
        self.restore_default_wait_time()
        return element


    def item_in_popup_by_id_exist(self, item, element_id):
        """Returns if an item exists in a popup given a id

        @param item: name of the item
        @param element_id: the id of the popup

        @returns True if the item exists; False otherwise.

        """
        xpath = 'id("%s")' % element_id
        return self.item_in_popup_by_xpath_exist(item, xpath)


    def item_in_popup_by_xpath_exist(self, item, xpath):
        """Returns if an item exists in a popup given an xpath

        @param item: name of the item
        @param xpath: the xpath of the popup

        @returns True if the item exists; False otherwise.

        """
        if self.number_of_items_in_popup_by_xpath(xpath) == 0:
            raise SeleniumTimeoutException('The popup at xpath %s has no items.'
                                           % xpath)
        popup = self.driver.find_element_by_xpath(xpath)
        for option in popup.find_elements_by_tag_name('option'):
            if option.text == item:
                return True
        return False


    def number_of_items_in_popup_by_id(self, element_id, alert_handler=None):
        """Returns the number of items in a popup given the element ID.

        @param element_id: the html ID of the item
        @param alert_handler: method invoked if an alert is detected. The method
                              must take one parameter, a webdriver alert object

        @returns the number of items in the popup.

        """
        xpath = 'id("%s")' % element_id
        return self.number_of_items_in_popup_by_xpath(xpath, alert_handler)


    def number_of_items_in_popup_by_xpath(self, xpath, alert_handler=None):
        """Returns the number of items in a popup given a xpath

        @param xpath: the xpath of the popup
        @param alert_handler: method invoked if an alert is detected. The method
                         must take one parameter, a webdriver alert object

        @returns the number of items in the popup.

        """
        popup = self.driver.find_element_by_xpath(xpath)
        try:
            self.wait.until(lambda _:
                            len(popup.find_elements_by_tag_name('option')))
        except SeleniumTimeoutException, e:
            return 0
        return len(popup.find_elements_by_tag_name('option'))


    def select_item_from_popup_by_id(self, item, element_id,
                                     wait_for_xpath=None, alert_handler=None):
        """Selects an item from a popup, by passing the element ID.

        @param item: the string of the item to select from the popup
        @param element_id: the html ID of the item
        @param wait_for_xpath: an item to wait for before returning, if not
                               specified the method does not wait.
        @param alert_handler: method invoked if an alert is detected. The method
                              must take one parameter, a webdriver alert object

        """
        xpath = 'id("%s")' % element_id
        self.select_item_from_popup_by_xpath(item, xpath, wait_for_xpath,
                                             alert_handler)


    def select_item_from_popup_by_xpath(self, item, xpath, wait_for_xpath=None,
                                        alert_handler=None):
        """Selects an item from a popup, by passing the xpath of the popup.

        @param item: the string of the item to select from the popup
        @param xpath: the xpath of the popup
        @param wait_for_xpath: an item to wait for before returning, if not
                               specified the method does not wait.
        @param alert_handler: method invoked if an alert is detected. The method
                              must take one parameter, a webdriver alert object

        """
        if self.number_of_items_in_popup_by_xpath(xpath) == 0:
            raise SeleniumTimeoutException('The popup at xpath %s has no items.'
                                           % xpath)
        if not self.item_in_popup_by_xpath_exist(item, xpath):
            raise SeleniumTimeoutException('The popup at xpath %s does not '
                                           'contain the item %s.' % (xpath,
                                           item))
        popup = self.driver.find_element_by_xpath(xpath)
        for option in popup.find_elements_by_tag_name('option'):
            if option.text == item:
                option.click()
                break
        self._handle_alert(xpath, alert_handler)
        if wait_for_xpath:
            self.wait_for_object_by_xpath(wait_for_xpath)


    def set_content_of_text_field_by_id(self, content, text_field_id,
                                        wait_for_xpath=None,
                                        abort_check=False):
        """Sets the content of a textfield, by passing the element ID.

        @param content: the content to apply to the textfield
        @param text_field_id: the html ID of the textfield
        @param wait_for_xpath: an item to wait for before returning, if not
                               specified the method does not wait.

        """
        xpath = 'id("%s")' % text_field_id
        self.set_content_of_text_field_by_xpath(content, xpath,
                                                wait_for_xpath=wait_for_xpath,
                                                abort_check=abort_check)


    def set_content_of_text_field_by_xpath(self, content, xpath,
                                           wait_for_xpath=None,
                                           abort_check=False):
        """Sets the content of a textfield, by passing the xpath.

        @param content: the content to apply to the textfield
        @param xpath: the xpath of the textfield
        @param wait_for_xpath: an item to wait for before returning, if not
                               specified the method does not wait.
        @param abort_check: do not get the current value before setting

        """
        # When we can get the value we know the text field is ready.
        text_field = self.driver.find_element_by_xpath(xpath)
        if text_field.get_attribute('type') != 'password' and not abort_check:
            try:
                self.wait.until(lambda _: text_field.get_attribute('value'))
            except SeleniumTimeoutException, e:
                raise SeleniumTimeoutException('Unable to obtain the value of '
                                               'the text field %s.\nWebDriver '
                                               'exception:%s' % (xpath, str(e)))
        text_field.clear()
        text_field.send_keys(content)
        if wait_for_xpath: self.wait_for_object_by_xpath(wait_for_xpath)


    def set_check_box_selected_by_id(self, check_box_id, selected=True,
                                     wait_for_xpath=None, alert_handler=None):
        """Sets the state of a checkbox, by passing the ID.

        @param check_box_id: the html id of the checkbox
        @param selected: True to check the checkbox; False to uncheck it
        @param wait_for_xpath: an item to wait for before returning, if not
                               specified the method does not wait.
        @param alert_handler: method invoked if an alert is detected. The method
                              must take one parameter, a webdriver alert object

        """
        xpath = 'id("%s")' % check_box_id
        self.set_check_box_selected_by_xpath(xpath, selected, wait_for_xpath,
                                             alert_handler)


    def set_check_box_selected_by_xpath(self, xpath, selected=True,
                                        wait_for_xpath=None,
                                        alert_handler=None):
        """Sets the state of a checkbox, by passing the xpath.

        @param xpath: the xpath of the checkbox
        @param selected: True to check the checkbox; False to uncheck it
        @param wait_for_xpath: an item to wait for before returning, if not
                               specified the method does not wait.
        @param alert_handler: method invoked if an alert is detected. The method
                              must take one parameter, a webdriver alert object
        """
        check_box = self.wait_for_object_by_xpath(xpath)
        value = check_box.get_attribute('value')
        if (value == '1' and not selected) or (value == '0' and selected):
            check_box.click()
        self._handle_alert(xpath, alert_handler)
        if wait_for_xpath:
            self.wait_for_object_by_xpath(wait_for_xpath)
