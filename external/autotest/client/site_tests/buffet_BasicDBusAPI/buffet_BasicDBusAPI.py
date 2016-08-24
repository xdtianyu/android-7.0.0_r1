# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import dbus
import inspect
import json
import logging
import sets

from autotest_lib.client.bin import test
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros.tendo import buffet_config
from autotest_lib.client.common_lib.cros.tendo import buffet_dbus_helper
from autotest_lib.client.common_lib.cros.tendo import privet_helper
from autotest_lib.client.cros.networking import wifi_proxy

def check(expected, value):
    """Check that |value| == |expected|.

    @param expected: expected value
    @param value: actual value we found

    """
    if value != expected:
        frame = inspect.getouterframes(inspect.currentframe())[1]
        raise error.TestFail('%s:%s: "%s" != "%s"' % (frame[1], frame[2],
                                                      expected, value))


class buffet_BasicDBusAPI(test.test):
    """Check that basic buffet daemon DBus APIs are functional."""
    version = 1

    def run_once(self):
        """Test entry point."""
        buffet_config.BuffetConfig(
                disable_pairing_security=True).restart_with_config()
        buffet = buffet_dbus_helper.BuffetDBusHelper()

        check('', buffet.device_id)
        check('Chromium', buffet.oem_name)
        check('Brillo', buffet.model_name)
        check('AATST', buffet.model_id)
        check('', buffet.description)
        check('', buffet.location)

        buffet.manager.UpdateDeviceInfo(dbus.String('A'),
                                        dbus.String('B'),
                                        dbus.String('C'))

        check('A', buffet.name)
        check('B', buffet.description)
        check('C', buffet.location)

        # The test method better work.
        test_message = 'Hello world!'
        echoed_message = buffet.manager.TestMethod(test_message)
        if test_message != echoed_message:
            raise error.TestFail('Expected Manager.TestMethod to return %s '
                                 'but got %s instead.' % (test_message,
                                                          echoed_message))

        # We should get the firmware version right.
        expected_version = None
        with open('/etc/lsb-release') as f:
            for line in f.readlines():
                pieces = line.split('=', 1)
                if len(pieces) != 2:
                    continue
                key = pieces[0].strip()
                if key == 'CHROMEOS_RELEASE_VERSION':
                    expected_version = pieces[1].strip()

        if expected_version is None:
            raise error.TestError('Failed to read version from lsb-release')
        raw_state = buffet.manager.GetState()
        parsed_state = json.loads(raw_state)
        logging.debug('%r', parsed_state)
        actual_version = parsed_state['base']['firmwareVersion']
        if actual_version != expected_version:
            raise error.TestFail('Expected firmwareVersion "%s", but got "%s"' %
                                 (expected_version, actual_version))

        check(raw_state, buffet.state)
        expected_base_keys = sets.Set(
              ['firmwareVersion', 'localDiscoveryEnabled',
               'localAnonymousAccessMaxRole', 'localPairingEnabled'])
        missing_base_keys = sets.Set(expected_base_keys).difference(
              parsed_state['base'].keys())
        if missing_base_keys:
            raise error.TestFail('Missing base keys "%s"' %  missing_base_keys)

        # Privet API
        shill = wifi_proxy.WifiProxy.get_proxy()
        shill.remove_all_wifi_entries()

        check({}, buffet.pairing_info)

        # But we should still be able to pair.
        helper = privet_helper.PrivetHelper()
        data = {'pairing': 'pinCode', 'crypto': 'none'}
        pairing = helper.send_privet_request(privet_helper.URL_PAIRING_START,
                                             request_data=data)
        # And now we should be able to see a pin code in our pairing status.
        pairing_info = buffet.pairing_info
        logging.debug(pairing_info)
        check(pairing_info.get('sessionId', ''), pairing['sessionId'])

        if not 'code' in pairing_info:
            raise error.TestFail('No code in pairing info (%r)' % pairing_info)
        # And if we start a new pairing session, the session ID should change.
        old_session_id = pairing_info['sessionId']
        pairing = helper.send_privet_request(privet_helper.URL_PAIRING_START,
                                             request_data=data)
        if pairing['sessionId'] == old_session_id:
            raise error.TestFail('Session IDs should change on each new '
                                 'pairing attempt.')
        # And if we start and complete a pairing session, we should have no
        # pairing information exposed.
        helper.privet_auth()
        check({}, buffet.pairing_info)

    def cleanup(self):
        """Clean up processes altered during the test."""
        buffet_config.naive_restart()
