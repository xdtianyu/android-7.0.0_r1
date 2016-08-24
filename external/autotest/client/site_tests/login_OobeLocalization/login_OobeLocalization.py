# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import json
import logging

from autotest_lib.client.bin import test, utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros import chrome
from autotest_lib.client.cros import cros_ui

class login_OobeLocalization(test.test):
    """Tests different region configurations at OOBE."""
    version = 1

    _LANGUAGE_SELECT = 'language-select'
    _KEYBOARD_SELECT = 'keyboard-select'
    _FALLBACK_KEYBOARD = 'xkb:us::eng'

    # dump_vpd_log reads the VPD cache in lieu of running `vpd -l`.
    _VPD_FILENAME = '/var/cache/vpd/full-v2.txt'
    # The filtered cache is created from the cache by dump_vpd_log. It is read
    # at startup if the device is not owned. (Otherwise /tmp/machine-info is
    # created by dump_vpd_log and read. See
    # /platform/login_manager/init/machine-info.conf.)
    _FILTERED_VPD_FILENAME = '/var/log/vpd_2.0.txt'
    # cros-regions.json has information for each region (locale, input method,
    # etc.) in JSON format.
    _REGIONS_FILENAME = '/usr/share/misc/cros-regions.json'
    # input_methods.txt lists supported input methods.
    _INPUT_METHODS_FILENAME = ('/usr/share/chromeos-assets/input_methods/'
                              'input_methods.txt')


    def initialize(self):
        self._login_keyboards = self._get_login_keyboards()
        self._comp_ime_prefix = self._run_with_chrome(
                self._get_comp_ime_prefix)


    def run_once(self):
        for region in self._get_regions():
            # Unconfirmed regions may have incorrect data. The 'confirm'
            # property is optional when all regions in database are confirmed so
            # we have to check explicit 'False'.
            if region.get('confirmed', True) is False:
                logging.info('Skip unconfirmed region: %s',
                             region['region_code'])
                continue

            # TODO(hungte) When OOBE supports cros-regions.json
            # (crosbug.com/p/34536) we can remove initial_locale,
            # initial_timezone, and keyboard_layout.
            self._set_vpd({'region': region['region_code'],
                           'initial_locale': ','.join(region['locales']),
                           'initial_timezone': ','.join(region['time_zones']),
                           'keyboard_layout': ','.join(region['keyboards'])})
            self._run_with_chrome(self._run_localization_test, region)


    def cleanup(self):
        """Removes cache files so our changes don't persist."""
        cros_ui.stop()
        utils.run('rm /home/chronos/Local\ State', ignore_status=True)
        utils.run('dump_vpd_log --clean')


    def _run_with_chrome(self, func, *args):
        with chrome.Chrome(auto_login=False) as self._chrome:
            utils.poll_for_condition(
                    self._is_oobe_ready,
                    exception=error.TestFail('OOBE not ready'))
            return func(*args)


    def _run_localization_test(self, region):
        """Checks the network screen for the proper dropdown values."""

        # Find the language(s), or acceptable alternate value(s).
        initial_locale = ','.join(region['locales'])
        if not self._verify_initial_options(
                self._LANGUAGE_SELECT,
                initial_locale,
                alternate_values = self._resolve_language(initial_locale),
                check_separator = True):
            raise error.TestFail(
                    'Language not found for region "%s".\n'
                    'Actual value of %s:\n%s' % (
                            region['region_code'],
                            self._LANGUAGE_SELECT,
                            self._dump_options(self._LANGUAGE_SELECT)))

        # We expect to see only login keyboards at OOBE.
        keyboards = region['keyboards']
        keyboards = [kbd for kbd in keyboards if kbd in self._login_keyboards]

        # If there are no login keyboards, expect only the fallback keyboard.
        keyboards = keyboards or [self._FALLBACK_KEYBOARD]

        # Prepend each xkb value with the component extension id.
        keyboard_ids = ','.join(
                [self._comp_ime_prefix + xkb for xkb in keyboards])

        # Find the keyboard layout(s).
        if not self._verify_initial_options(
                self._KEYBOARD_SELECT,
                keyboard_ids):
            raise error.TestFail(
                    'Keyboard not found for region "%s".\n'
                    'Actual value of %s:\n%s' % (
                            region['region_code'],
                            self._KEYBOARD_SELECT,
                            self._dump_options(self._KEYBOARD_SELECT)))

        # Check that the fallback keyboard is present.
        if self._FALLBACK_KEYBOARD not in keyboards:
            if not self._verify_option_exists(
                    self._KEYBOARD_SELECT,
                    self._comp_ime_prefix + self._FALLBACK_KEYBOARD):
                raise error.TestFail(
                        'Fallback keyboard layout not found for region "%s".\n'
                        'Actual value of %s:\n%s' % (
                                region['region_code'],
                                self._KEYBOARD_SELECT,
                                self._dump_options(self._KEYBOARD_SELECT)))


    def _set_vpd(self, vpd_settings):
        """Changes VPD cache on disk.
        @param vpd_settings: Dictionary of VPD key-value pairs.
        """
        cros_ui.stop()

        vpd = {}
        with open(self._VPD_FILENAME, 'r+') as vpd_log:
            # Read the existing VPD info.
            for line in vpd_log:
                # Extract "key"="value" pair.
                key, _, value = line.replace('"', '').partition('=')
                vpd[key] = value

            vpd.update(vpd_settings);

            # Write the new set of settings to disk.
            vpd_log.seek(0)
            for key in vpd:
                vpd_log.write('"%s"="%s"\n' % (key, vpd[key]))
            vpd_log.truncate()

        # Remove filtered cache so dump_vpd_log recreates it from the cache we
        # just updated.
        utils.run('rm ' + self._FILTERED_VPD_FILENAME, ignore_status=True)
        utils.run('dump_vpd_log')

        # Remove cached files to clear initial locale info.
        utils.run('rm /home/chronos/Local\ State', ignore_status=True)
        utils.run('rm /home/chronos/.oobe_completed', ignore_status=True)
        cros_ui.start()


    def _verify_initial_options(self, select_id, values,
                                alternate_values='', check_separator=False):
        """Verifies that |values| are the initial elements of |select_id|.

        @param select_id: ID of the select element to check.
        @param values: Comma-separated list of values that should appear,
                in order, at the top of the select before any options group.
        @param alternate_values: Optional comma-separated list of alternate
                values for the corresponding items in values.
        @param check_separator: If True, also verifies that an options group
                label appears after the initial set of values.

        @returns whether the select fits the given constraints.

        @raises EvaluateException if the JS expression fails to evaluate.
        """
        js_expression = """
                (function () {
                  var select = document.querySelector('#%s');
                  if (!select || select.selectedIndex)
                    return false;
                  var values = '%s'.split(',');
                  var alternate_values = '%s'.split(',');
                  for (var i = 0; i < values.length; i++) {
                    if (select.options[i].value != values[i] &&
                        (!alternate_values[i] ||
                         select.options[i].value != alternate_values[i]))
                      return false;
                  }
                  if (%d) {
                    return select.children[values.length].tagName ==
                        'OPTGROUP';
                  }
                  return true;
                })()""" % (select_id,
                           values,
                           alternate_values,
                           check_separator)

        return self._chrome.browser.oobe.EvaluateJavaScript(js_expression)


    def _verify_option_exists(self, select_id, value):
        """Verifies that |value| exists in |select_id|.

        @param select_id: ID of the select element to check.
        @param value: A single value to find in the select.

        @returns whether the value is found.

        @raises EvaluateException if the JS expression fails to evaluate.
        """
        js_expression = """
                (function () {
                  return !!document.querySelector(
                      '#%s option[value=\\'%s\\']');
                })()""" % (select_id, value)

        return self._chrome.browser.oobe.EvaluateJavaScript(js_expression)


    def _get_login_keyboards(self):
        """Returns the set of login xkbs from the input methods file."""
        login_keyboards = set()
        with open(self._INPUT_METHODS_FILENAME) as input_methods_file:
            for line in input_methods_file:
                columns = line.strip().split()
                # The 5th column will be "login" if this keyboard layout will
                # be used on login.
                if len(columns) == 5 and columns[4] == 'login':
                    login_keyboards.add(columns[0])
        return login_keyboards


    def _get_regions(self):
        regions = {}
        with open(self._REGIONS_FILENAME, 'r') as regions_file:
            return json.load(regions_file).values()


    def _get_comp_ime_prefix(self):
        """Finds the xkb values' component extension id prefix, if any.
        @returns the prefix if found, or an empty string
        """
        return self._chrome.browser.oobe.EvaluateJavaScript("""
                var value = document.getElementById('%s').value;
                value.substr(0, value.lastIndexOf('xkb:'))""" %
                self._KEYBOARD_SELECT)


    def _resolve_language(self, locale):
        """Falls back to an existing locale if the given locale matches a
        language but not the country. Mirrors
        chromium:ui/base/l10n/l10n_util.cc.
        """
        lang, _, region = map(str.lower, str(locale).partition('-'))
        if not region:
            return ''

        # Map from other countries to a localized country.
        if lang == 'es' and region == 'es':
            return 'es-419'
        if lang == 'zh':
            if region in ('hk', 'mo'):
                return 'zh-TW'
            return 'zh-CN'
        if lang == 'en':
            if region in ('au', 'ca', 'nz', 'za'):
                return 'en-GB'
            return 'en-US'

        # No mapping found.
        return ''


    def _is_oobe_ready(self):
        return (self._chrome.browser.oobe and
                self._chrome.browser.oobe.EvaluateJavaScript(
                        "var select = document.getElementById('%s');"
                        "select && select.children.length >= 2" %
                                self._LANGUAGE_SELECT))


    def _dump_options(self, select_id):
        js_expression = """
                (function () {
                  var selector = '#%s';
                  var divider = ',';
                  var select = document.querySelector(selector);
                  if (!select)
                    return 'document.querySelector(\\'' + selector +
                        '\\') failed.';
                  var dumpOptgroup = function(group) {
                    var result = '';
                    for (var i = 0; i < group.children.length; i++) {
                      if (i > 0)
                        result += divider;
                      if (group.children[i].value)
                        result += group.children[i].value;
                      else
                        result += '__NO_VALUE__';
                    }
                    return result;
                  };
                  var result = '';
                  if (select.selectedIndex != 0) {
                    result += '(selectedIndex=' + select.selectedIndex +
                        ', selected \' +
                        select.options[select.selectedIndex].value +
                        '\)';
                  }
                  var children = select.children;
                  for (var i = 0; i < children.length; i++) {
                    if (i > 0)
                      result += divider;
                    if (children[i].value)
                      result += children[i].value;
                    else if (children[i].tagName === 'OPTGROUP')
                      result += '[' + dumpOptgroup(children[i]) + ']';
                    else
                      result += '__NO_VALUE__';
                  }
                  return result;
                })()""" % select_id
        return self._chrome.browser.oobe.EvaluateJavaScript(js_expression)
