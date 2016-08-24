# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Handle gdata spreadsheet authentication."""

import BaseHTTPServer
import httplib
import os
import Queue
import threading
import socket
import time

import gdata.gauth
import gdata.spreadsheets.client

# Local port of http daemon for receiving the refresh token from the redirected
# url returned by authentication server.
START_PORT = 12345
ACCESS_CODE_Q = Queue.Queue()

# Token storage
TOKEN_STORAGE_NAME = '%s/.spreadsheets.oauth2.dat' % os.getenv('HOME')

# Application's CLIENT_ID and SECRET for OAuthToken which is created from
# google's api console's API access - https://code.google.com/apis/console
CLIENT_ID = '657833351030.apps.googleusercontent.com'
CLIENT_SECRET = 'h72FzPdzfbN3I4U3M3l1DSiT'

USER_AGENT = 'Pressure Calibration Data Collector'
SCOPES = 'https://spreadsheets.google.com/feeds/ https://docs.google.com/feeds/'
RETRY = 10


class AuthenticationHandler(BaseHTTPServer.BaseHTTPRequestHandler):
    """Authentication handler class."""

    def do_QUIT(self):
        """Do QUIT reuqest."""
        self.send_response(200)
        self.end_headers()
        self.server.stop = True

    def do_GET(self):  # pylint: disable=g-bad-name
        """Do GET request."""
        self.send_response(200)
        self.send_header('Content-type', 'text/html')
        self.end_headers()
        self.port = START_PORT
        query_str = self.path[2:]
        if query_str.startswith('code='):
            self.wfile.write('Spreadsheet authentication complete, '
                             'please back to command prompt.')
            ACCESS_CODE_Q.put(query_str[5:])
            return
        if query_str.startswith('error='):
            print "Ouch, error: '%s'." % query_str[6:]
            raise Exception("Exception during approval process: '%s'." %
                            query_str[6:])

    def log_message(self, format, *args):  # pylint: disable=redefined-builtin
        pass


class AuthenticationHTTPD(BaseHTTPServer.HTTPServer):
    """ Handle redirected response from authentication server."""
    def serve_forever(self, poll_interval=0.5):
        poll_interval = poll_interval
        self.stop = False
        while not self.stop:
            self.handle_request()


class AuthenticationServer(threading.Thread):
    """Authentication http server thread."""

    def __init__(self):
        self.authserver = None
        self.started = False
        threading.Thread.__init__(self)

    def run(self):
        for ports in range(START_PORT, START_PORT + RETRY):
            self.port = ports
            try:
                self.authserver = AuthenticationHTTPD(('', self.port),
                                                      AuthenticationHandler)
                self.started = True
                self.authserver.serve_forever()
                return

            except socket.error, se:
                # port is busy, there must be another instance running...
                if self.port == START_PORT + RETRY - 1:
                    raise se
                else:
                    continue  # keep trying new ports

            except KeyboardInterrupt:
                print '^C received, shutting down authentication server.'
                self.stop = True
                return  # out of retry loop

            except Exception:
                self.stop = True
                return  # out of retry loop

    def get_port(self):
        """Get the running port number."""
        for retry in xrange(RETRY):
            if self.started and self.authserver:
                return self.port
            else:
                time.sleep(retry * 2)
                continue


class SpreadsheetAuthorizer:
    """ Handle gdata api authentication for spreadsheet client."""
    def __init__(self):
        self.lock = threading.Lock()
        self.refresh_token = None
        self.redirect_url = None
        self.httpd_auth = None
        self.port = None

    def _start_server(self):
        """Start http daemon for handling refresh token."""
        if not self.httpd_auth or not self.httpd_auth.isAlive():
            ### Starting webserver if necessary
            self.httpd_auth = AuthenticationServer()
            self.httpd_auth.start()
            self.port = self.httpd_auth.get_port()

    def _stop_server(self):
        """Stop http daemon."""
        if self.httpd_auth:
            try:
                conn = httplib.HTTPConnection('localhost:%s' % self.port)
                conn.request('QUIT', '/')
                conn.getresponse()
                time.sleep(1)
                del self.httpd_auth
                self.httpd_auth = None
            except Exception, e:
                print "Failed to quit local auth server...'%s'." % e
        return

    def authorize(self, ss_client):
        """Authorize the spreadsheet client.

        @param ss_client: spreadsheet client
        """
        self._read_refresh_token()
        token = gdata.gauth.OAuth2Token(CLIENT_ID,
                                        CLIENT_SECRET,
                                        SCOPES,
                                        USER_AGENT,
                                        refresh_token = self.refresh_token)
        try:
            if not self.refresh_token:
                self._start_server()
                token = gdata.gauth.OAuth2Token(CLIENT_ID,
                                                CLIENT_SECRET,
                                                SCOPES,
                                                USER_AGENT)
                redirect_url = 'http://localhost:' + str(self.port)
                url = token.generate_authorize_url(redirect_url)
                print ('\nPlease open the following URL and use @chromium.org'
                       'account for authentication and authorization of the'
                       'spreadsheet access:\n' + url)
                print 'Waiting for you to authenticate...'
                while ACCESS_CODE_Q.empty():
                    time.sleep(.25)
                access_code = ACCESS_CODE_Q.get()
                print 'ACCESS CODE is ' + access_code
                token.get_access_token(access_code)
                self.refresh_token = token.refresh_token
                print 'REFRESH TOKEN is ' + self.refresh_token
                self._write_refresh_token()
                self._stop_server()
            token.authorize(ss_client)
            return True
        except IOError:
            print "ERROR!!!!!!!!!!!!!!!!"
        return False

    def _read_refresh_token(self):
        """Read refresh token from storage."""
        try:
            self.lock.acquire()
            token_file = open(TOKEN_STORAGE_NAME, 'r')
            self.refresh_token = token_file.readline().strip()
            token_file.close()
            self.lock.release()
            return self.refresh_token
        except IOError:
            self.lock.release()
            return None

    def _write_refresh_token(self):
        """Write refresh token into storage."""
        try:
            self.lock.acquire()
            token_descriptor = os.open(TOKEN_STORAGE_NAME,
                                       os.O_CREAT | os.O_WRONLY | os.O_TRUNC,
                                       0600)
            token_file = os.fdopen(token_descriptor, 'w')
            token_file.write(self.refresh_token + '\n')
            token_file.close()
            self.lock.release()
        except (IOError, OSError):
            self.lock.release()
            print 'Error, can not write refresh token\n'
