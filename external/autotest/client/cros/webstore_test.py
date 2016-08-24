# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""
This module allows tests to interact with the Chrome Web Store (CWS)
using ChromeDriver. They should inherit from the webstore_test class,
and should override the run() method.
"""

import logging
import time

from autotest_lib.client.bin import test
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros import chromedriver
from autotest_lib.client.common_lib.global_config import global_config
from selenium.webdriver.common.by import By
from selenium.webdriver.support import expected_conditions
from selenium.webdriver.support.ui import WebDriverWait

# How long to wait, in seconds, for an app to launch. This is larger
# than it needs to be, because it might be slow on older Chromebooks
_LAUNCH_DELAY = 4

# How long to wait before entering the password when logging in to the CWS
_ENTER_PASSWORD_DELAY = 2

# How long to wait before entering payment info
_PAYMENT_DELAY = 5

def enum(*enumNames):
    """
    Creates an enum. Returns an enum object with a value for each enum
    name, as well as from_string and to_string mappings.

    @param enumNames: The strings representing the values of the enum
    """
    enums = dict(zip(enumNames, range(len(enumNames))))
    reverse = dict((value, key) for key, value in enums.iteritems())
    enums['from_string'] = enums
    enums['to_string'] = reverse
    return type('Enum', (), enums)

# TODO: staging and PNL don't work in these tests (crbug/396660)
TestEnv = enum('staging', 'pnl', 'prod', 'sandbox')

ItemType = enum(
    'hosted_app',
    'packaged_app',
    'chrome_app',
    'extension',
    'theme',
)

# NOTE: paid installs don't work right now
InstallType = enum(
    'free',
    'free_trial',
    'paid',
)

def _labeled_button(label):
    """
    Returns a button with the class webstore-test-button-label and the
    specified label

    @param label: The label on the button
    """
    return ('//div[contains(@class,"webstore-test-button-label") '
            'and text()="' + label + '"]')

def _install_type_click_xpath(item_type, install_type):
    """
    Returns the XPath of the button to install an item of the given type.

    @param item_type: The type of the item to install
    @param install_type: The type of installation being used
    """
    if install_type == InstallType.free:
        return _labeled_button('Free')
    elif install_type == InstallType.free_trial:
        # Both of these cases return buttons that say "Add to Chrome",
        # but they are actually different buttons with only one being
        # visible at a time.
        if item_type == ItemType.hosted_app:
            return ('//div[@id="cxdialog-install-paid-btn" and '
                    '@aria-label="Add to Chrome"]')
        else:
            return _labeled_button('Add to Chrome')
    else:
        return ('//div[contains(@aria-label,"Buy for") '
                'and not(contains(@style,"display: none"))]')

def _get_chrome_flags(test_env):
    """
    Returns the Chrome flags for the given test environment.
    """
    flags = ['--apps-gallery-install-auto-confirm-for-tests=accept']
    if test_env == TestEnv.prod:
        return flags

    url_middle = {
            TestEnv.staging: 'staging.corp',
            TestEnv.sandbox: 'staging.sandbox',
            TestEnv.pnl: 'prod-not-live.corp'
            }[test_env]
    download_url_middle = {
            TestEnv.staging: 'download-staging.corp',
            TestEnv.sandbox: 'download-staging.sandbox',
            TestEnv.pnl: 'omaha.sandbox'
            }[test_env]
    flags.append('--apps-gallery-url=https://webstore-' + url_middle +
            '.google.com')
    flags.append('--apps-gallery-update-url=https://' + download_url_middle +
            '.google.com/service/update2/crx')
    logging.info('Using flags %s', flags)
    return flags


class webstore_test(test.test):
    """
    The base class for tests that interact with the web store.

    Subclasses must define run(), but should not override run_once().
    Subclasses should use methods in this module such as install_item,
    but they can also use the driver directly if they need to.
    """

    def initialize(self, test_env=TestEnv.sandbox,
                   account='cwsbotdeveloper1@gmail.com'):
        """
        Initialize the test.

        @param test_env: The test environment to use
        """
        super(webstore_test, self).initialize()

        self.username = account
        self.password = global_config.get_config_value(
                'CLIENT', 'webstore_test_password', type=str)

        self.test_env = test_env
        self._chrome_flags = _get_chrome_flags(test_env)
        self.webstore_url = {
                TestEnv.staging:
                    'https://webstore-staging.corp.google.com',
                TestEnv.sandbox:
                    'https://webstore-staging.sandbox.google.com/webstore',
                TestEnv.pnl:
                    'https://webstore-prod-not-live.corp.google.com/webstore',
                TestEnv.prod:
                    'https://chrome.google.com/webstore'
                }[test_env]


    def build_url(self, page):
        """
        Builds a webstore URL for the specified page.

        @param page: the page to build a URL for
        """
        return self.webstore_url + page + "?gl=US"


    def detail_page(self, item_id):
        """
        Returns the URL of the detail page for the given item

        @param item_id: The item ID
        """
        return self.build_url("/detail/" + item_id)


    def wait_for(self, xpath):
        """
        Waits until the element specified by the given XPath is visible

        @param xpath: The xpath of the element to wait for
        """
        self._wait.until(expected_conditions.visibility_of_element_located(
                (By.XPATH, xpath)))


    def run_once(self, **kwargs):
        with chromedriver.chromedriver(
                username=self.username,
                password=self.password,
                extra_chrome_flags=self._chrome_flags) \
                as chromedriver_instance:
            self.driver = chromedriver_instance.driver
            self.driver.implicitly_wait(15)
            self._wait = WebDriverWait(self.driver, 20)
            logging.info('Running test on test environment %s',
                    TestEnv.to_string[self.test_env])
            self.run(**kwargs)


    def run(self):
        """
        Runs the test. Should be overridden by subclasses.
        """
        raise error.TestError('The test needs to override run()')


    def install_item(self, item_id, item_type, install_type):
        """
        Installs an item from the CWS.

        @param item_id: The ID of the item to install
                (a 32-char string of letters)
        @param item_type: The type of the item to install
        @param install_type: The type of installation
                (free, free trial, or paid)
        """
        logging.info('Installing item %s of type %s with install_type %s',
                item_id, ItemType.to_string[item_type],
                InstallType.to_string[install_type])

        # We need to go to the CWS home page before going to the detail
        # page due to a bug in the CWS
        self.driver.get(self.webstore_url)
        self.driver.get(self.detail_page(item_id))

        install_type_click_xpath = _install_type_click_xpath(
                item_type, install_type)
        if item_type == ItemType.extension or item_type == ItemType.theme:
            post_install_xpath = (
                '//div[@aria-label="Added to Chrome" '
                ' and not(contains(@style,"display: none"))]')
        else:
            post_install_xpath = _labeled_button('Launch app')

        # In this case we need to sign in again
        if install_type != InstallType.free:
            button_xpath = _labeled_button('Sign in to add')
            logging.info('Clicking button %s', button_xpath)
            self.driver.find_element_by_xpath(button_xpath).click()
            time.sleep(_ENTER_PASSWORD_DELAY)
            password_field = self.driver.find_element_by_xpath(
                    '//input[@id="Passwd"]')
            password_field.send_keys(self.password)
            self.driver.find_element_by_xpath('//input[@id="signIn"]').click()

        logging.info('Clicking %s', install_type_click_xpath)
        self.driver.find_element_by_xpath(install_type_click_xpath).click()

        if install_type == InstallType.paid:
            handle = self.driver.current_window_handle
            iframe = self.driver.find_element_by_xpath(
                '//iframe[contains(@src, "sandbox.google.com/checkout")]')
            self.driver.switch_to_frame(iframe)
            self.driver.find_element_by_id('purchaseButton').click()
            time.sleep(_PAYMENT_DELAY) # Wait for animation to finish
            self.driver.find_element_by_id('finishButton').click()
            self.driver.switch_to_window(handle)

        self.wait_for(post_install_xpath)


    def launch_app(self, app_id):
        """
        Launches an app. Verifies that it launched by verifying that
        a new tab/window was opened.

        @param app_id: The ID of the app to run
        """
        logging.info('Launching app %s', app_id)
        num_handles_before = len(self.driver.window_handles)
        self.driver.get(self.webstore_url)
        self.driver.get(self.detail_page(app_id))
        launch_button = self.driver.find_element_by_xpath(
            _labeled_button('Launch app'))
        launch_button.click();
        time.sleep(_LAUNCH_DELAY) # Wait for the app to launch
        num_handles_after = len(self.driver.window_handles)
        if num_handles_after <= num_handles_before:
            raise error.TestError('App failed to launch')
