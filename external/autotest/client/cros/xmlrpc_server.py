# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import contextlib
import dbus
import errno
import functools
import logging
import select
import signal
import threading
import SimpleXMLRPCServer


class XmlRpcServer(threading.Thread):
    """Simple XMLRPC server implementation.

    In theory, Python should provide a sane XMLRPC server implementation as
    part of its standard library.  In practice the provided implementation
    doesn't handle signals, not even EINTR.  As a result, we have this class.

    Usage:

    server = XmlRpcServer(('localhost', 43212))
    server.register_delegate(my_delegate_instance)
    server.run()

    """

    def __init__(self, host, port):
        """Construct an XmlRpcServer.

        @param host string hostname to bind to.
        @param port int port number to bind to.

        """
        super(XmlRpcServer, self).__init__()
        logging.info('Binding server to %s:%d', host, port)
        self._server = SimpleXMLRPCServer.SimpleXMLRPCServer((host, port),
                                                             allow_none=True)
        self._server.register_introspection_functions()
        # After python 2.7.10, BaseServer.handle_request automatically retries
        # on EINTR, so handle_request will be blocked at select.select forever
        # if timeout is None. Set a timeout so server can be shut down
        # gracefully. Check issue crbug.com/571737 and
        # https://bugs.python.org/issue7978 for the explanation.
        self._server.timeout = 0.5
        self._keep_running = True
        self._delegates = []
        # Gracefully shut down on signals.  This is how we expect to be shut
        # down by autotest.
        signal.signal(signal.SIGTERM, self._handle_signal)
        signal.signal(signal.SIGINT, self._handle_signal)


    def register_delegate(self, delegate):
        """Register delegate objects with the server.

        The server will automagically look up all methods not prefixed with an
        underscore and treat them as potential RPC calls.  These methods may
        only take basic Python objects as parameters, as noted by the
        SimpleXMLRPCServer documentation.  The state of the delegate is
        persisted across calls.

        @param delegate object Python object to be exposed via RPC.

        """
        self._server.register_instance(delegate)
        self._delegates.append(delegate)


    def run(self):
        """Block and handle many XmlRpc requests."""
        logging.info('XmlRpcServer starting...')
        # TODO(wiley) nested is deprecated, but we can't use the replacement
        #       until we move to Python 3.0.
        with contextlib.nested(*self._delegates):
            while self._keep_running:
                try:
                    self._server.handle_request()
                except select.error as v:
                    # In a cruel twist of fate, the python library doesn't
                    # handle this kind of error.
                    if v[0] != errno.EINTR:
                        raise
        logging.info('XmlRpcServer exited.')


    def _handle_signal(self, _signum, _frame):
        """Handle a process signal by gracefully quitting.

        SimpleXMLRPCServer helpfully exposes a method called shutdown() which
        clears a flag similar to _keep_running, and then blocks until it sees
        the server shut down.  Unfortunately, if you call that function from
        a signal handler, the server will just hang, since the process is
        paused for the signal, causing a deadlock.  Thus we are reinventing the
        wheel with our own event loop.

        """
        self._keep_running = False


def dbus_safe(default_return_value):
    """Catch all DBus exceptions and return a default value instead.

    Wrap a function with a try block that catches DBus exceptions and
    returns default values instead.  This is convenient for simple error
    handling since XMLRPC doesn't understand DBus exceptions.

    @param wrapped_function function to wrap.
    @param default_return_value value to return on exception (usually False).

    """
    def decorator(wrapped_function):
        """Call a function and catch DBus errors.

        @param wrapped_function function to call in dbus safe context.
        @return function return value or default_return_value on failure.

        """
        @functools.wraps(wrapped_function)
        def wrapper(*args, **kwargs):
            """Pass args and kwargs to a dbus safe function.

            @param args formal python arguments.
            @param kwargs keyword python arguments.
            @return function return value or default_return_value on failure.

            """
            logging.debug('%s()', wrapped_function.__name__)
            try:
                return wrapped_function(*args, **kwargs)

            except dbus.exceptions.DBusException as e:
                logging.error('Exception while performing operation %s: %s: %s',
                              wrapped_function.__name__,
                              e.get_dbus_name(),
                              e.get_dbus_message())
                return default_return_value

        return wrapper

    return decorator


class XmlRpcDelegate(object):
    """A super class for XmlRPC delegates used with XmlRpcServer.

    This doesn't add much helpful functionality except to implement the trivial
    status check method expected by autotest's host.xmlrpc_connect() method.
    Subclass this class to add more functionality.

    """


    def __enter__(self):
        logging.debug('Bringing up XmlRpcDelegate: %r.', self)
        pass


    def __exit__(self, exception, value, traceback):
        logging.debug('Tearing down XmlRpcDelegate: %r.', self)
        pass


    def ready(self):
        """Confirm that the XMLRPC server is up and ready to serve.

        @return True (always).

        """
        logging.debug('ready()')
        return True
