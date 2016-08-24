# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.


import base64
import hashlib
import httplib
import json
import logging
import socket
import StringIO
import urllib2
import urlparse

try:
    import pycurl
except ImportError:
    pycurl = None


import common

from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros import retry
from autotest_lib.server import frontend
from autotest_lib.server import site_utils


# Give all our rpcs about six seconds of retry time. If a longer timeout
# is desired one should retry from the caller, this timeout is only meant
# to avoid uncontrolled circumstances like network flake, not, say, retry
# right across a reboot.
BASE_REQUEST_TIMEOUT = 0.1
JSON_HEADERS = {'Content-Type': 'application/json'}
RPC_EXCEPTIONS = (httplib.BadStatusLine, socket.error, urllib2.HTTPError)
MANIFEST_KEY = ('MIGfMA0GCSqGSIb3DQEBAQUAA4GNADCBiQKBgQC+hlN5FB+tjCsBszmBIvI'
                'cD/djLLQm2zZfFygP4U4/o++ZM91EWtgII10LisoS47qT2TIOg4Un4+G57e'
                'lZ9PjEIhcJfANqkYrD3t9dpEzMNr936TLB2u683B5qmbB68Nq1Eel7KVc+F'
                '0BqhBondDqhvDvGPEV0vBsbErJFlNH7SQIDAQAB')
SONIC_BOARD_LABEL = 'board:sonic'


def get_extension_id(pub_key_pem=MANIFEST_KEY):
    """Computes the extension id from the public key.

    @param pub_key_pem: The public key used in the extension.

    @return: The extension id.
    """
    pub_key_der = base64.b64decode(pub_key_pem)
    sha = hashlib.sha256(pub_key_der).hexdigest()
    prefix = sha[:32]
    reencoded = ""
    ord_a = ord('a')
    for old_char in prefix:
        code = int(old_char, 16)
        new_char = chr(ord_a + code)
        reencoded += new_char
    return reencoded


class Url(object):
  """Container for URL information."""

  def __init__(self):
    self.scheme = 'http'
    self.netloc = ''
    self.path = ''
    self.params = ''
    self.query = ''
    self.fragment = ''

  def Build(self):
    """Returns the URL."""
    return urlparse.urlunparse((
        self.scheme,
        self.netloc,
        self.path,
        self.params,
        self.query,
        self.fragment))


# TODO(beeps): Move get and post to curl too, since we have the need for
# custom requests anyway.
@retry.retry(RPC_EXCEPTIONS, timeout_min=BASE_REQUEST_TIMEOUT)
def _curl_request(host, app_path, port, custom_request='', payload=None):
    """Sends a custom request throug pycurl, to the url specified.
    """
    url = Url()
    url.netloc = ':'.join((host, str(port)))
    url.path = app_path
    full_url = url.Build()

    response = StringIO.StringIO()
    conn = pycurl.Curl()
    conn.setopt(conn.URL, full_url)
    conn.setopt(conn.WRITEFUNCTION, response.write)
    if custom_request:
        conn.setopt(conn.CUSTOMREQUEST, custom_request)
    if payload:
        conn.setopt(conn.POSTFIELDS, payload)
    conn.perform()
    conn.close()
    return response.getvalue()


@retry.retry(RPC_EXCEPTIONS, timeout_min=BASE_REQUEST_TIMEOUT)
def _get(url):
    """Get request to the give url.

    @raises: Any of the retry exceptions, if we hit the timeout.
    @raises: error.TimeoutException, if the call itself times out.
        eg: a hanging urlopen will get killed with a TimeoutException while
        multiple retries that hit different Http errors will raise the last
        HttpError instead of the TimeoutException.
    """
    return urllib2.urlopen(url).read()


@retry.retry(RPC_EXCEPTIONS, timeout_min=BASE_REQUEST_TIMEOUT)
def _post(url, data):
    """Post data to the given url.

    @param data: Json data to post.

    @raises: Any of the retry exceptions, if we hit the timeout.
    @raises: error.TimeoutException, if the call itself times out.
        For examples see docstring for _get method.
    """
    request = urllib2.Request(url, json.dumps(data),
                              headers=JSON_HEADERS)
    urllib2.urlopen(request)


@retry.retry(RPC_EXCEPTIONS + (error.TestError,), timeout_min=30)
def acquire_sonic(lock_manager, additional_labels=None):
    """Lock a host that has the sonic host labels.

    @param lock_manager: A manager for locking/unlocking hosts, as defined by
        server.cros.host_lock_manager.
    @param additional_labels: A list of additional labels to apply in the search
        for a sonic device.

    @return: A string specifying the hostname of a locked sonic host.

    @raises ValueError: Is no hosts matching the given labels are found.
    """
    sonic_host = None
    afe = frontend.AFE(debug=True)
    labels = [SONIC_BOARD_LABEL]
    if additional_labels:
        labels += additional_labels
    sonic_hostname = utils.poll_for_condition(
            lambda: site_utils.lock_host_with_labels(afe, lock_manager, labels),
            sleep_interval=60,
            exception=SonicProxyException('Timed out trying to find a sonic '
                                          'host with labels %s.' % labels))
    logging.info('Acquired sonic host returned %s', sonic_hostname)
    return sonic_hostname


class SonicProxyException(Exception):
    """Generic exception raised when a sonic rpc fails."""
    pass


class SonicProxy(object):
    """Client capable of making calls to the sonic device server."""
    POLLING_INTERVAL = 5
    SONIC_SERVER_PORT = '8008'

    def __init__(self, hostname):
        """
        @param hostname: The name of the host for this sonic proxy.
        """
        self._sonic_server = 'http://%s:%s' % (hostname, self.SONIC_SERVER_PORT)
        self._hostname = hostname


    def check_server(self):
        """Checks if the sonic server is up and running.

        @raises: SonicProxyException if the server is unreachable.
        """
        try:
            json.loads(_get(self._sonic_server))
        except (RPC_EXCEPTIONS, error.TimeoutException) as e:
            raise SonicProxyException('Could not retrieve information about '
                                      'sonic device: %s' % e)


    def reboot(self, when="now"):
        """
        Post to the server asking for a reboot.

        @param when: The time till reboot. Can be any of:
            now: immediately
            fdr: set factory data reset flag and reboot now
            ota: set recovery flag and reboot now
            ota fdr: set both recovery and fdr flags, and reboot now
            ota foreground: reboot and start force update page
            idle: reboot only when idle screen usage > 10 mins

        @raises SonicProxyException: if we're unable to post a reboot request.
        """
        reboot_url = '%s/%s/%s' % (self._sonic_server, 'setup', 'reboot')
        reboot_params = {"params": when}
        logging.info('Rebooting device through %s.', reboot_url)
        try:
            _post(reboot_url, reboot_params)
        except (RPC_EXCEPTIONS, error.TimeoutException) as e:
            raise SonicProxyException('Could not reboot sonic device through '
                                      '%s: %s' % (self.SETUP_SERVER_PORT, e))


    def stop_app(self, app):
        """Stops the app.

        Performs a hard reboot if pycurl isn't available.

        @param app: An app name, eg YouTube, Fling, Netflix etc.

        @raises pycurl.error: If the DELETE request fails after retries.
        """
        if not pycurl:
            logging.warning('Rebooting sonic host to stop %s, please install '
                            'pycurl if you do not wish to reboot.', app)
            self.reboot()
            return

        _curl_request(self._hostname, 'apps/%s' % app,
                      self.SONIC_SERVER_PORT, 'DELETE')


    def start_app(self, app, payload):
        """Starts an app.

        @param app: An app name, eg YouTube, Fling, Netflix etc.
        @param payload: An url payload for the app, eg: http://www.youtube.com.

        @raises error.TimeoutException: If the call times out.
        """
        url = '%s/apps/%s' % (self._sonic_server, app)
        _post(url, payload)

