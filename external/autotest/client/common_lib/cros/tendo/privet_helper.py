# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import json
import logging
import time

from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib import utils

URL_PING = 'ping'
URL_INFO = 'info'
URL_AUTH = 'v3/auth'
URL_PAIRING_CONFIRM = 'v3/pairing/confirm'
URL_PAIRING_START = 'v3/pairing/start'
URL_SETUP_START = 'v3/setup/start'
URL_SETUP_STATUS = 'v3/setup/status'

DEFAULT_HTTP_PORT = 80
DEFAULT_HTTPS_PORT = 443

class PrivetHelper(object):
    """Delegate class containing logic useful with privetd."""


    def __init__(self, host=None, hostname='localhost',
                 http_port=DEFAULT_HTTP_PORT, https_port=DEFAULT_HTTPS_PORT):
        """Construct a PrivetdHelper

        @param host: host object where we should run the HTTP requests from.
        @param hostname: string hostname of host to issue HTTP requests against.
        @param http_port: int HTTP port to use when making HTTP requests.
        @param https_port: int HTTPS port to use when making HTTPs requests.

        """
        self._host = None
        self._run = utils.run
        if host is not None:
            self._host = host
            self._run = host.run
        self._hostname = hostname
        self._http_port = http_port
        self._https_port = https_port


    def _build_privet_url(self, path_fragment, use_https=True):
        """Builds a request URL for privet.

        @param path_fragment: URL path fragment to be appended to /privet/ URL.
        @param use_https: set to False to use 'http' protocol instead of https.

        @return The full URL to be used for request.

        """
        protocol = 'http'
        port = self._http_port
        if use_https:
            protocol = 'https'
            port = self._https_port
        url = '%s://%s:%s/privet/%s' % (protocol, self._hostname, port,
                                        path_fragment)
        return url


    def _http_request(self, url, request_data=None, retry_count=0,
                      retry_delay=0.3, headers={},
                      timeout_seconds=10,
                      tolerate_failure=False):
        """Sends a GET/POST request to a web server at the given |url|.

        If the request fails due to error 111:Connection refused, try it again
        after |retry_delay| seconds and repeat this to a max |retry_count|.
        This is needed to make sure peerd has a chance to start up and start
        responding to HTTP requests.

        @param url: URL path to send the request to.
        @param request_data: json data to send in POST request.
                If None, a GET request is sent with no data.
        @param retry_count: max request retry count.
        @param retry_delay: retry_delay (in seconds) between retries.
        @param headers: optional dictionary of http request headers
        @param timeout_seconds: int number of seconds for curl to wait
                to complete the request.
        @param tolerate_failure: True iff we should allow curl failures.
        @return The string content of the page requested at url.

        """
        logging.debug('Requesting %s', url)
        args = []
        if request_data is not None:
            headers['Content-Type'] = 'application/json; charset=utf8'
            args.append('--data')
            args.append(request_data)
        for header in headers.iteritems():
            args.append('--header')
            args.append(': '.join(header))
        # TODO(wiley do cert checking
        args.append('--insecure')
        args.append('--max-time')
        args.append('%d' % timeout_seconds)
        # Write the HTTP code to stdout
        args.append('-w')
        args.append('%{http_code}')
        output_file = '/tmp/privetd_http_output'
        args.append('-o')
        args.append(output_file)
        while retry_count >= 0:
            result = self._run('curl %s' % url, args=args,
                               ignore_status=True)
            retry_count -= 1
            raw_response = ''
            success = result.exit_status == 0
            http_code = result.stdout or 'timeout'
            if success:
                raw_response = self._run('cat %s' % output_file).stdout
                logging.debug('Got raw response: %s', raw_response)
            if success and http_code == '200':
                return raw_response
            if retry_count < 0:
                if tolerate_failure:
                    return None
                raise error.TestFail('Failed requesting %s (code=%s)' %
                                     (url, http_code))
            logging.warn('Failed to connect to host. Retrying...')
            time.sleep(retry_delay)


    def send_privet_request(self, path_fragment, request_data=None,
                            auth_token='Privet anonymous',
                            tolerate_failure=False):
        """Sends a privet request over HTTPS.

        @param path_fragment: URL path fragment to be appended to /privet/ URL.
        @param request_data: json data to send in POST request.
                             If None, a GET request is sent with no data.
        @param auth_token: authorization token to be added as 'Authorization'
                           http header using 'Privet' as the auth realm.
        @param tolerate_failure: True iff we should allow curl failures.

        """
        if isinstance(request_data, dict):
                request_data = json.dumps(request_data)
        headers = {'Authorization': auth_token}
        url = self._build_privet_url(path_fragment, use_https=True)
        data = self._http_request(url, request_data=request_data,
                                  headers=headers,
                                  tolerate_failure=tolerate_failure)
        if data is None and tolerate_failure:
            return None
        try:
            json_data = json.loads(data)
            data = json.dumps(json_data)  # Drop newlines, pretty format.
        finally:
            logging.info('Received /privet/%s response: %s',
                         path_fragment, data)
        return json_data


    def ping_server(self, use_https=False):
        """Ping the privetd webserver.

        Reuses port numbers from the last restart request.  The server
        must have been restarted with enable_ping=True for this to work.

        @param use_https: set to True to use 'https' protocol instead of 'http'.

        """
        url = self._build_privet_url(URL_PING, use_https=use_https);
        content = self._http_request(url, retry_delay=5, retry_count=5)
        if content != 'Hello, world!':
            raise error.TestFail('Unexpected response from web server: %s.' %
                                 content)


    def privet_auth(self):
        """Go through pairing and insecure auth.

        @return resulting auth token.

        """
        data = {'pairing': 'pinCode', 'crypto': 'none'}
        pairing = self.send_privet_request(URL_PAIRING_START, request_data=data)

        data = {'sessionId': pairing['sessionId'],
                'clientCommitment': pairing['deviceCommitment']
        }
        self.send_privet_request(URL_PAIRING_CONFIRM, request_data=data)

        data = {'authCode': pairing['deviceCommitment'],
                'mode': 'pairing',
                'requestedScope': 'owner'
        }
        auth = self.send_privet_request(URL_AUTH, request_data=data)
        auth_token = '%s %s' % (auth['tokenType'], auth['accessToken'])
        return auth_token


    def setup_add_wifi_credentials(self, ssid, passphrase, data={}):
        """Add WiFi credentials to the data provided to setup_start().

        @param ssid: string ssid of network to connect to.
        @param passphrase: string passphrase for network.
        @param data: optional dict of information to append to.

        """
        data['wifi'] = {'ssid': ssid, 'passphrase': passphrase}
        return data


    def setup_start(self, data, auth_token):
        """Provide privetd with credentials for various services.

        @param data: dict of information to give to privetd.  Should be
                formed by one or more calls to setup_add_*() above.
        @param auth_token: string auth token returned from privet_auth()
                above.

        """
        # We don't return the response here, because in general, we may not
        # get one.  In many cases, we'll tear down the AP so quickly that
        # the webserver won't have time to respond.
        self.send_privet_request(URL_SETUP_START, request_data=data,
                                 auth_token=auth_token, tolerate_failure=True)


    def wifi_setup_was_successful(self, ssid, auth_token):
        """Detect whether privetd thinks bootstrapping has succeeded.

        @param ssid: string network we expect to connect to.
        @param auth_token: string auth token returned from prviet_auth()
                above.
        @return True iff setup/status reports success in connecting to
                the given network.

        """
        response = self.send_privet_request(URL_SETUP_STATUS,
                                            auth_token=auth_token)
        return (response['wifi']['status'] == 'success' and
                response['wifi']['ssid'] == ssid)
