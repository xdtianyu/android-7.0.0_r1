# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros.tendo import buffet_config
from autotest_lib.client.common_lib.cros.tendo import privet_helper
from autotest_lib.server import test

def _assert_equal(expected, actual):
    """Compares objects.

    @param expected: the expected value.
    @param actual: the actual value.

    """
    if expected != actual:
        raise error.TestFail('Expected: %r, actual: %r' % (expected, actual))


def _assert_not_empty(dictionary, key):
    """Compares objects.

    @param expected: the expected value.
    @param actual: the actual value.

    """
    if not key in dictionary:
        raise error.TestFail('Missing key: %s' % key)

    if not dictionary[key]:
        raise error.TestFail('Key "%s" is empty' % key)


class buffet_PrivetInfo(test.test):
    """This test verifies that the buffet responds to /privet/info request and
    returns the expected JSON response object.
    """
    version = 1

    def warmup(self, host):
        config = buffet_config.BuffetConfig(log_verbosity=3)
        config.restart_with_config(host=host)


    def cleanup(self, host):
        buffet_config.naive_restart(host=host)


    def run_once(self, host):
        helper = privet_helper.PrivetHelper(host=host)
        helper.ping_server()  # Make sure the server is up and running.
        info = helper.send_privet_request(privet_helper.URL_INFO)

        # Do some sanity checks on the returned JSON object.
        if info['version'] != '3.0':
            raise error.TestFail('Expected privet version 3.0')

        authentication = info['authentication']
        _assert_not_empty(authentication, 'anonymousMaxScope')
        _assert_equal(['p224_spake2'], authentication['crypto'])
        _assert_equal(['anonymous', 'pairing'], authentication['mode'])
        _assert_equal(['pinCode'], authentication['pairing'])

        _assert_not_empty(info, 'name')
        _assert_not_empty(info, 'id')

        _assert_not_empty(info, 'modelManifestId')
        _assert_equal(5, len(info['modelManifestId']))

        manifest = info['basicModelManifest']
        _assert_not_empty(manifest, 'modelName')
        _assert_not_empty(manifest, 'oemName')
        _assert_not_empty(manifest, 'uiDeviceKind')

        _assert_equal({'id': '', 'status': 'unconfigured'}, info['gcd'])
