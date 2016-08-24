# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Spins up a trivial HTTP cgi form listener in a thread.

   This HTTPThread class is a utility for use with test cases that
   need to call back to the Autotest test case with some form value, e.g.
   http://localhost:nnnn/?status="Browser started!"
"""

import cgi, errno, logging, os, posixpath, SimpleHTTPServer, socket, ssl, sys
import threading, urllib, urlparse
from BaseHTTPServer import BaseHTTPRequestHandler, HTTPServer
from SocketServer import BaseServer, ThreadingMixIn


def _handle_http_errors(func):
    """Decorator function for cleaner presentation of certain exceptions."""
    def wrapper(self):
        try:
            func(self)
        except IOError, e:
            if e.errno == errno.EPIPE or e.errno == errno.ECONNRESET:
                # Instead of dumping a stack trace, a single line is sufficient.
                self.log_error(str(e))
            else:
                raise

    return wrapper


class FormHandler(SimpleHTTPServer.SimpleHTTPRequestHandler):
    """Implements a form handler (for POST requests only) which simply
    echoes the key=value parameters back in the response.

    If the form submission is a file upload, the file will be written
    to disk with the name contained in the 'filename' field.
    """

    SimpleHTTPServer.SimpleHTTPRequestHandler.extensions_map.update({
        '.webm': 'video/webm',
    })

    # Override the default logging methods to use the logging module directly.
    def log_error(self, format, *args):
        logging.warning("(httpd error) %s - - [%s] %s\n" %
                     (self.address_string(), self.log_date_time_string(),
                      format%args))

    def log_message(self, format, *args):
        logging.debug("%s - - [%s] %s\n" %
                     (self.address_string(), self.log_date_time_string(),
                      format%args))

    @_handle_http_errors
    def do_POST(self):
        form = cgi.FieldStorage(
            fp=self.rfile,
            headers=self.headers,
            environ={'REQUEST_METHOD': 'POST',
                     'CONTENT_TYPE': self.headers['Content-Type']})
        # You'd think form.keys() would just return [], like it does for empty
        # python dicts; you'd be wrong. It raises TypeError if called when it
        # has no keys.
        if form:
            for field in form.keys():
                field_item = form[field]
                self.server._form_entries[field] = field_item.value
        path = urlparse.urlparse(self.path)[2]
        if path in self.server._url_handlers:
            self.server._url_handlers[path](self, form)
        else:
            # Echo back information about what was posted in the form.
            self.write_post_response(form)
        self._fire_event()


    def write_post_response(self, form):
        """Called to fill out the response to an HTTP POST.

        Override this class to give custom responses.
        """
        # Send response boilerplate
        self.send_response(200)
        self.end_headers()
        self.wfile.write('Hello from Autotest!\nClient: %s\n' %
                         str(self.client_address))
        self.wfile.write('Request for path: %s\n' % self.path)
        self.wfile.write('Got form data:\n')

        # See the note in do_POST about form.keys().
        if form:
            for field in form.keys():
                field_item = form[field]
                if field_item.filename:
                    # The field contains an uploaded file
                    upload = field_item.file.read()
                    self.wfile.write('\tUploaded %s (%d bytes)<br>' %
                                     (field, len(upload)))
                    # Write submitted file to specified filename.
                    file(field_item.filename, 'w').write(upload)
                    del upload
                else:
                    self.wfile.write('\t%s=%s<br>' % (field, form[field].value))


    def translate_path(self, path):
        """Override SimpleHTTPRequestHandler's translate_path to serve
        from arbitrary docroot
        """
        # abandon query parameters
        path = urlparse.urlparse(path)[2]
        path = posixpath.normpath(urllib.unquote(path))
        words = path.split('/')
        words = filter(None, words)
        path = self.server.docroot
        for word in words:
            drive, word = os.path.splitdrive(word)
            head, word = os.path.split(word)
            if word in (os.curdir, os.pardir): continue
            path = os.path.join(path, word)
        logging.debug('Translated path: %s', path)
        return path


    def _fire_event(self):
        wait_urls = self.server._wait_urls
        if self.path in wait_urls:
            _, e = wait_urls[self.path]
            e.set()
            del wait_urls[self.path]
        else:
            logging.debug('URL %s not in watch list' % self.path)


    @_handle_http_errors
    def do_GET(self):
        form = cgi.FieldStorage(
            fp=self.rfile,
            headers=self.headers,
            environ={'REQUEST_METHOD': 'GET'})
        split_url = urlparse.urlsplit(self.path)
        path = split_url[2]
        # Strip off query parameters to ensure that the url path
        # matches any registered events.
        self.path = path
        args = urlparse.parse_qs(split_url[3])
        if path in self.server._url_handlers:
            self.server._url_handlers[path](self, args)
        else:
            SimpleHTTPServer.SimpleHTTPRequestHandler.do_GET(self)
        self._fire_event()


    @_handle_http_errors
    def do_HEAD(self):
        SimpleHTTPServer.SimpleHTTPRequestHandler.do_HEAD(self)


class ThreadedHTTPServer(ThreadingMixIn, HTTPServer):
    def __init__(self, server_address, HandlerClass):
        HTTPServer.__init__(self, server_address, HandlerClass)


class HTTPListener(object):
    # Point default docroot to a non-existent directory (instead of None) to
    # avoid exceptions when page content is served through handlers only.
    def __init__(self, port=0, docroot='/_', wait_urls={}, url_handlers={}):
        self._server = ThreadedHTTPServer(('', port), FormHandler)
        self.config_server(self._server, docroot, wait_urls, url_handlers)

    def config_server(self, server, docroot, wait_urls, url_handlers):
        # Stuff some convenient data fields into the server object.
        self._server.docroot = docroot
        self._server._wait_urls = wait_urls
        self._server._url_handlers = url_handlers
        self._server._form_entries = {}
        self._server_thread = threading.Thread(
            target=self._server.serve_forever)


    def add_wait_url(self, url='/', matchParams={}):
        e = threading.Event()
        self._server._wait_urls[url] = (matchParams, e)
        return e


    def add_url_handler(self, url, handler_func):
        self._server._url_handlers[url] = handler_func


    def clear_form_entries(self):
        self._server._form_entries = {}


    def get_form_entries(self):
        """Returns a dictionary of all field=values recieved by the server.
        """
        return self._server._form_entries


    def run(self):
        logging.debug('http server on %s:%d' %
                      (self._server.server_name, self._server.server_port))
        self._server_thread.start()


    def stop(self):
        self._server.shutdown()
        self._server.socket.close()
        self._server_thread.join()


class SecureHTTPServer(ThreadingMixIn, HTTPServer):
    def __init__(self, server_address, HandlerClass, cert_path, key_path):
        _socket = socket.socket(self.address_family, self.socket_type)
        self.socket = ssl.wrap_socket(_socket,
                                      server_side=True,
                                      ssl_version=ssl.PROTOCOL_TLSv1,
                                      certfile=cert_path,
                                      keyfile=key_path)
        BaseServer.__init__(self, server_address, HandlerClass)
        self.server_bind()
        self.server_activate()


class SecureHTTPRequestHandler(FormHandler):
    def setup(self):
        self.connection = self.request
        self.rfile = socket._fileobject(self.request, 'rb', self.rbufsize)
        self.wfile = socket._fileobject(self.request, 'wb', self.wbufsize)

    # Override the default logging methods to use the logging module directly.
    def log_error(self, format, *args):
        logging.warning("(httpd error) %s - - [%s] %s\n" %
                     (self.address_string(), self.log_date_time_string(),
                      format%args))

    def log_message(self, format, *args):
        logging.debug("%s - - [%s] %s\n" %
                     (self.address_string(), self.log_date_time_string(),
                      format%args))


class SecureHTTPListener(HTTPListener):
    def __init__(self,
                 cert_path='/etc/login_trust_root.pem',
                 key_path='/etc/mock_server.key',
                 port=0,
                 docroot='/_',
                 wait_urls={},
                 url_handlers={}):
        self._server = SecureHTTPServer(('', port),
                                        SecureHTTPRequestHandler,
                                        cert_path,
                                        key_path)
        self.config_server(self._server, docroot, wait_urls, url_handlers)


    def getsockname(self):
        return self._server.socket.getsockname()

