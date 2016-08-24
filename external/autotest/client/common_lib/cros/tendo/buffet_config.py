# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import random
import string

from autotest_lib.client.common_lib import utils
from autotest_lib.client.common_lib.cros.fake_device_server import oauth
from autotest_lib.client.common_lib.cros.fake_device_server import server

TEST_CONFIG_PATH = '/tmp/buffet.fake.conf'
TEST_STATE_PATH = '/tmp/buffet.fake.state'

LOCAL_SERVER_PORT = server.PORT
LOCAL_OAUTH_URL = 'http://localhost:%d/%s/' % (LOCAL_SERVER_PORT,
                                               oauth.OAUTH_PATH)
LOCAL_SERVICE_URL = 'http://localhost:%d/' % LOCAL_SERVER_PORT
TEST_API_KEY = oauth.TEST_API_KEY

def build_unique_device_name():
    """@return a test-unique name for a device."""
    RAND_CHARS = string.ascii_lowercase + string.digits
    NUM_RAND_CHARS = 16
    rand_token = ''.join([random.choice(RAND_CHARS)
                          for _ in range(NUM_RAND_CHARS)])
    name = 'CrOS_%s' % rand_token
    logging.debug('Generated unique device name %s', name)
    return name


TEST_CONFIG = {
    'client_id': 'this_is_my_client_id',
    'client_secret': 'this_is_my_client_secret',
    'api_key': TEST_API_KEY,
    'oauth_url': LOCAL_OAUTH_URL,
    'service_url': LOCAL_SERVICE_URL,
    'model_id': 'AATST',
    'wifi_auto_setup_enabled': 'false',
    'name': build_unique_device_name()
}


def bool_to_flag(value):
    """Converts boolean value into lowercase string

    @param value: Boolean value.
    @return lower case string: 'true' or 'false'.

    """
    return ('%s' % value).lower()


def format_options(options, separator):
    """Format dictionary as key1=value1{separator}key2=value2{separator}..

    @param options: Dictionary with options.
    @param separator: String to be used as separator between key=value strings.
    @return formated string.

    """
    return separator.join(['%s=%s' % (k, v) for (k, v) in options.iteritems()])


def naive_restart(host=None):
    """Restart Buffet without configuring it in any way.

    @param host: Host object if we're interested in a remote host.

    """
    run = utils.run if host is None else host.run
    run('stop buffet', ignore_status=True)
    run('start buffet')



class BuffetConfig(object):
    """An object that knows how to restart buffet in various configurations."""

    def __init__(self,
                 log_verbosity=None,
                 test_definitions_dir=None,
                 enable_xmpp=False,
                 enable_ping=True,
                 disable_pairing_security=False,
                 device_whitelist=None,
                 options=None):
        self.enable_xmpp = enable_xmpp
        self.log_verbosity = log_verbosity
        self.test_definitions_dir = test_definitions_dir
        self.enable_ping = enable_ping
        self.disable_pairing_security = disable_pairing_security
        self.device_whitelist = device_whitelist
        self.options = TEST_CONFIG.copy()
        if options:
            self.options.update(options)


    def restart_with_config(self,
                            host=None,
                            clean_state=True):
        """Restart Buffet with this configuration.

        @param host: Host object if we're interested in a remote host.
        @param clean_state: boolean True to remove all existing state.

        """
        run = utils.run if host is None else host.run
        run('stop buffet', ignore_status=True)
        flags = {
            'BUFFET_ENABLE_XMPP': 'true' if self.enable_xmpp else 'false',
            'BUFFET_CONFIG_PATH': TEST_CONFIG_PATH,
            'BUFFET_STATE_PATH': TEST_STATE_PATH,
            'BUFFET_ENABLE_PING': bool_to_flag(self.enable_ping),
            'BUFFET_DISABLE_SECURITY':
                    bool_to_flag(self.disable_pairing_security),
        }
        if self.log_verbosity:
            flags['BUFFET_LOG_LEVEL'] = self.log_verbosity

        # Go through this convoluted shell magic here because we need to
        # create this file on both remote and local hosts (see how run() is
        # defined).
        run('cat <<EOF >%s\n%s\nEOF\n' % (TEST_CONFIG_PATH,
                                          format_options(self.options, '\n')))

        if clean_state:
            run('echo > %s' % TEST_STATE_PATH)
            run('chown buffet:buffet %s' % TEST_STATE_PATH)

        if self.test_definitions_dir:
            flags['BUFFET_TEST_DEFINITIONS_PATH'] = self.test_definitions_dir

        if self.device_whitelist:
            flags['BUFFET_DEVICE_WHITELIST'] = ','.join(self.device_whitelist)

        run('start buffet %s' % format_options(flags, ' '))

