# Copyright (c) 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import urlparse

import belkinF5D8236_ap_configurator
from selenium.common.exceptions import WebDriverException
from selenium.common.exceptions import TimeoutException


class BelkinF9K1001APConfigurator(
        belkinF5D8236_ap_configurator.BelkinF5D8236APConfigurator):
    """Class to configure Belkin F9K1001v5 (01B) router."""

    def __init__(self, ap_config):
        super(BelkinF9K1001APConfigurator, self).__init__(ap_config)
        self._dhcp_delay = 0


    def _security_alert(self, alert):
        text = alert.text
        if 'Selecting WEP Encryption will disable the WPS' in text:
            alert.accept()
        elif 'Selecting WPA/TKIP Encryption will disable the WPS' in text:
            alert.accept()
        elif 'Invalid character' in text:
            alert.accept()
        elif 'It is recommended to use WPA/WPA2 when WPS is enabled' in text:
            alert.accept()
        elif 'Are you sure to configure WPS in Open security?' in text:
            alert.accept()
        else:
            alert.accept()
            raise RuntimeError('Unhandeled modal dialog. %s' % text)


    def navigate_to_page(self, page_number):
        """
        Navigates to the page corresponding to the given page number.

        This method performs the translation between a page number and a url to
        load. This is used internally by apply_settings.

        @param page_number: page number of the page to load

        """
        page_title='Network Name'
        element_xpath='//input[@name="ssid"]'
        page_url = None

        if page_number == 1:
            page_url = urlparse.urljoin(self.admin_interface_url,
                                        'wifi_id.stm')
        elif page_number == 2:
            page_url = urlparse.urljoin(self.admin_interface_url,
                                        'wifi_sc.stm')
            page_title='Security'
            element_xpath=self.security_popup
        else:
            raise RuntimeError('Invalid page number passed. Number of pages '
                               '%d, page value sent was %d' %
                               (self.get_number_of_pages(), page_number))
        try:
            self.get_url(page_url, page_title=page_title,
                         element_xpath=element_xpath)
        except WebDriverException, e:
            if 'Welcome to your Belkin router dashboard!' in self.driver.title:
                self._login_to_dashboard(element_xpath)
        self.wait_for_object_by_xpath(element_xpath)


    def _login_to_dashboard(self, obj_xpath):
        """
        Login and wait for the object with obj_xpath to show up.

        @param obj_xpath: The object that should be searched to confirm that we
                          logged in.

        """
        self.set_content_of_text_field_by_id('password', 'p1210Password',
                                             abort_check=True)
        self.click_button_by_id('p1210a005')
        self.wait_for_object_by_xpath(obj_xpath, wait_time=10)


    def save_page(self, page_number):
        """
        Save changes.

        @param page_number: the page number to save as an integer.

        """
        self.set_wait_time(30)
        if page_number == 1:
            xpath = '//a[text()="Save" and @href="#save" and @id="dnsapply"]'
            self.click_button_by_xpath(xpath,
                                       alert_handler=self._security_alert)
        elif page_number == 2:
            button = self.driver.find_element_by_link_text('Save')
            button.click()
            self._handle_alert(None, alert_handler=self._security_alert)
        dashboard_title = 'Welcome to your Belkin router dashboard!'
        network_title = 'Network Name'
        security_title = 'Security'
        try:
            # This is a dummy wait. We just need to make sure that we give the
            # router enough time to save the changes.
            self.wait.until(lambda _: dashboard_title in self.driver.title)
        except TimeoutException, e:
            if not self.driver.title in [dashboard_title, network_title,
                                    security_title]:
                raise RuntimeError('Error while saving the page. ' + str(e))
        finally:
            self.restore_default_wait_time()


    def set_security_wep(self, key_value, authentication):
        self.add_item_to_command_list(self._set_security_wep,
                                      (key_value, authentication), 2, 1000)


    def _set_security_wep(self, key_value, authentication):
        text_field = '//input[@name="passphrase"]'
        try:
            self.select_item_from_popup_by_xpath('64bit WEP',
                    self.security_popup, wait_for_xpath=text_field,
                    alert_handler=self._security_alert)
        except WebDriverException, e:
            message = str(e)
            if message.find('An open modal dialog blocked') == -1:
               raise RuntimeError(message)
            self._security_alert(self.driver.switch_to_alert())
        self.set_content_of_text_field_by_xpath(key_value, text_field,
                                                abort_check=True)
        self.click_button_by_id('btngen', alert_handler=self._security_alert)
