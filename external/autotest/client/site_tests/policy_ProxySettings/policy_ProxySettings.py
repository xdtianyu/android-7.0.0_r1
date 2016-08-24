# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import threading

from autotest_lib.client.common_lib import error
from autotest_lib.client.cros import enterprise_policy_base
from SocketServer import ThreadingTCPServer, StreamRequestHandler

POLICY_NAME = 'ProxySettings'
PROXY_HOST = 'localhost'
PROXY_PORT = 3128
FIXED_PROXY = '''{
  "ProxyBypassList": "www.google.com,www.googleapis.com",
  "ProxyMode": "fixed_servers",
  "ProxyServer": "localhost:%s"
}''' % PROXY_PORT
DIRECT_PROXY = '''{
  "ProxyMode": "direct"
}'''
TEST_URL = 'http://www.wired.com/'


class ProxyHandler(StreamRequestHandler):
    """Provide request handler for the Threaded Proxy Listener."""

    def handle(self):
        """Get URL of request from first line.

        Read the first line of the request, up to 40 characters, and look
        for the URL of the request. If found, save it to the URL list.

        Note: All requests are sent an HTTP 504 error.
        """
        # Capture URL in first 40 chars of request.
        data = self.rfile.readline(40).strip()
        logging.info('ProxyHandler::handle(): <%s>', data)
        self.server.store_requests_recieved(data)
        self.wfile.write('HTTP/1.1 504 Gateway Timeout\r\n'
                         'Connection: close\r\n\r\n')


class ThreadedProxyServer(ThreadingTCPServer):
    """Provide a Threaded Proxy Server to service and save requests.

    Define a Threaded Proxy Server which services requests, and allows the
    handler to save all requests.
    """

    def __init__(self, server_address, HandlerClass):
        """Constructor.

        @param server_address: tuple of server IP and port to listen on.
        @param HandlerClass: the RequestHandler class to instantiate per req.
        """
        self.reset_requests_received()
        ThreadingTCPServer.__init__(self, server_address, HandlerClass)

    def store_requests_recieved(self, request):
        """Add receieved request to list.

        @param request: request received by the proxy server.
        """
        self._requests_recieved.append(request)

    def get_requests_recieved(self):
        """Get list of received requests."""
        return self._requests_recieved

    def reset_requests_received(self):
        """Clear list of received requests."""
        self._requests_recieved = []


class ProxyListener(object):
    """Provide a Proxy Listener to detect connect requests.

    Define a proxy listener to detect when a CONNECT request is seen at the
    given |server_address|, and record all requests received. Requests
    recieved are exposed to the caller.
    """

    def __init__(self, server_address):
        """Constructor.

        @param server_address: tuple of server IP and port to listen on.
        """
        self._server = ThreadedProxyServer(server_address, ProxyHandler)
        self._thread = threading.Thread(target=self._server.serve_forever)

    def run(self):
        """Start the server by activating it's thread."""
        self._thread.start()

    def stop(self):
        """Stop the server and its threads."""
        self._server.shutdown()
        self._server.socket.close()
        self._thread.join()

    def store_requests_recieved(self, request):
        """Add receieved request to list.

        @param request: request received by the proxy server.
        """
        self._requests_recieved.append(request)

    def get_requests_recieved(self):
        """Get list of received requests."""
        return self._server.get_requests_recieved()

    def reset_requests_received(self):
        """Clear list of received requests."""
        self._server.reset_requests_received()


class policy_ProxySettings(enterprise_policy_base.EnterprisePolicyTest):
    """Test effect of ProxySettings policy on Chrome OS behavior.

    This test verifies the behavior of Chrome OS for specific configurations
    of the ProxySettings use policy: None (undefined), ProxyMode=direct,
    ProxyMode=fixed_servers. None means that the policy value is not set. This
    induces the default behavior, equivalent to what is seen by an un-managed
    user.

    When ProxySettings is None (undefined), or ProxyMode=direct, then no proxy
    server should be used. When ProxyMode=fixed_servers, then the proxy server
    address specified by the ProxyServer entry should be used.
    """
    version = 1
    TEST_CASES = {
        'FixedProxy_UseFixedProxy': FIXED_PROXY,
        'DirectProxy_UseNoProxy': DIRECT_PROXY,
        'NotSet_UseNoProxy': None
    }

    def initialize(self, args=()):
        super(policy_ProxySettings, self).initialize(args)
        self._proxy_server = ProxyListener(('', PROXY_PORT))
        self._proxy_server.run()

    def cleanup(self):
        self._proxy_server.stop()
        super(policy_ProxySettings, self).cleanup()

    def _test_proxy_configuration(self, policy_value, policies_json):
        """Verify CrOS enforces the specified ProxySettings configuration.

        @param policy_value: policy value expected on chrome://policy page.
        @param policies_json: policy JSON data to send to the fake DM server.
        """
        logging.info('Running _test_proxy_configuration(%s, %s)',
                     policy_value, policies_json)
        self.setup_case(POLICY_NAME, policy_value, policies_json)

        self._proxy_server.reset_requests_received()
        self.navigate_to_url(TEST_URL)
        proxied_requests = self._proxy_server.get_requests_recieved()

        # Determine whether TEST_URL is in |proxied_requests|. Comprehension
        # is conceptually equivalent to `TEST_URL in proxied_requests`;
        # however, we must do partial matching since TEST_URL and the
        # elements inside |proxied_requests| are not necessarily equal, i.e.,
        # TEST_URL is a substring of the received request.
        matching_requests = [request for request in proxied_requests
                             if TEST_URL in request]
        logging.info('matching_requests: %s', matching_requests)

        if policy_value is None or 'direct' in policy_value:
            if matching_requests:
                raise error.TestFail('Requests should not have been sent '
                                     'through the proxy server.')
        elif 'fixed_servers' in policy_value:
            if not matching_requests:
                raise error.TestFail('Requests should have been sent '
                                     'through the proxy server.')

    def run_test_case(self, case):
        """Setup and run the test configured for the specified test case.

        Set the expected |policy_value| and |policies_json| data based on the
        test |case|. If the user gave an expected |value| on the command line,
        then set |policy_value| to |value|, and |policies_json| to None.

        @param case: Name of the test case to run.

        """
        if self.is_value_given:
            # If |value| was given in the command line args, then set expected
            # |policy_value| to the given value, and |policies_json| to None.
            policy_value = self.value
            policies_json = None
        else:
            # Otherwise, set expected |policy_value| and setup |policies_json|
            # data to the values required by the specified test |case|.
            policy_value = self.TEST_CASES[case]
            policies_json = {POLICY_NAME: self.TEST_CASES[case]}

        self._test_proxy_configuration(policy_value, policies_json)
