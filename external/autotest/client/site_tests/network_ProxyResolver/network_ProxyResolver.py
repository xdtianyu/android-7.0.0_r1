# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.;
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
#
# This is an integration test which ensures that a proxy set on a
# shared network connection is exposed via LibCrosSevice and used
# by tlsdated during time synchronization.

import dbus
import gobject
import logging
import subprocess
import threading
import time

from autotest_lib.client.bin import test, utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.cros import cros_ui
from autotest_lib.client.cros.networking import shill_proxy

from dbus.mainloop.glib import DBusGMainLoop
from SocketServer import ThreadingTCPServer, StreamRequestHandler

class ProxyHandler(StreamRequestHandler):
    """Matching request handler for the ThreadedHitServer
       that notes when an expected request is seen.
    """
    wbufsize = -1
    def handle(self):
        """Reads the first line, up to 40 characters, looking
           for the CONNECT string that tlsdated sends. If it
           is found, the server's hit() method is called.

           All requests receive a HTTP 504 error.
        """
        # Read up to 40 characters
        data = self.rfile.readline(40).strip()
        logging.info('ProxyHandler::handle(): <%s>', data)
        # TODO(wad) Add User-agent check when it lands in tlsdate.
        # Also, abstract the time server and move this code into cros/.
        if data.__contains__('CONNECT clients3.google.com:443 HTTP/1.1'):
          self.server.hit()
        self.wfile.write("HTTP/1.1 504 Gateway Timeout\r\n" +
                         "Connection: close\r\n\r\n")

class ThreadedHitServer(ThreadingTCPServer):
    """A threaded TCP server which services requests
       and allows the handler to track "hits".
    """
    def __init__(self, server_address, HandlerClass):
        """Constructor

        @param server_address: tuple of server IP and port to listen on.
        @param HandlerClass: the RequestHandler class to instantiate per req.
        """
        self._hits = 0
        ThreadingTCPServer.__init__(self, server_address, HandlerClass)

    def hit(self):
        """Increment the hit count. Usually called by the HandlerClass"""
        self._hits += 1

    def reset_hits(self):
        """Set the hit count to 0"""
        self._hits = 0

    def hits(self):
        """Get the number of matched requests
        @return the count of matched requests
        """
        return self._hits

class ProxyListener(object):
    """A fake listener for tracking if an expected CONNECT request is
       seen at the provided server address. Any hits are exposed to be
       consumed by the caller.
    """
    def __init__(self, server_address):
        """Constructor

        @param server_address: tuple of server IP and port to listen on.
        """
        self._server = ThreadedHitServer(server_address, ProxyHandler)
        self._thread = threading.Thread(target=self._server.serve_forever)

    def run(self):
        """Run the server on a thread"""
        self._thread.start()

    def stop(self):
        """Stop the server and its threads"""
        self._server.shutdown()
        self._server.socket.close()
        self._thread.join()

    def reset_hits(self):
        """Reset the number of matched requests to 0"""
        return self._server.reset_hits()

    def hits(self):
        """Get the number of matched requests
        @return the count of matched requests
        """
        return self._server.hits()

class SignalListener(object):
    """A class to listen for a DBus signal
    """
    DEFAULT_TIMEOUT = 60
    _main_loop = None
    _signals = { }

    def __init__(self, g_main_loop):
        """Constructor

        @param g_mail_loop: glib main loop object.
        """
        self._main_loop = g_main_loop


    def listen_for_signal(self, signal, interface, path):
        """Listen with a default handler
        @param signal: signal name to listen for
        @param interface: DBus interface to expect it from
        @param path: DBus path associated with the signal
        """
        self.__listen_to_signal(self.__handle_signal, signal, interface, path)


    def wait_for_signals(self, desc,
                         timeout=DEFAULT_TIMEOUT):
        """Block for |timeout| seconds waiting for the signals to come in.

        @param desc: string describing the high-level reason you're waiting
                     for the signals.
        @param timeout: maximum seconds to wait for the signals.

        @raises TimeoutError if the timeout is hit.
        """
        utils.poll_for_condition(
            condition=lambda: self.__received_signals(),
            desc=desc,
            timeout=self.DEFAULT_TIMEOUT)
        all_signals = self._signals.copy()
        self.__reset_signal_state()
        return all_signals


    def __received_signals(self):
        """Run main loop until all pending events are done, checks for signals.

        Runs self._main_loop until it says it has no more events pending,
        then returns the state of the internal variables tracking whether
        desired signals have been received.

        @return True if both signals have been handled, False otherwise.
        """
        context = self._main_loop.get_context()
        while context.iteration(False):
            pass
        return len(self._signals) > 0


    def __reset_signal_state(self):
        """Resets internal signal tracking state."""
        self._signals = { }


    def __listen_to_signal(self, callback, signal, interface, path):
        """Connect a callback to a given session_manager dbus signal.

        Sets up a signal receiver for signal, and calls the provided callback
        when it comes in.

        @param callback: a callable to call when signal is received.
        @param signal: the signal to listen for.
        """
        bus = dbus.SystemBus(mainloop=self._main_loop)
        bus.add_signal_receiver(
            handler_function=callback,
            signal_name=signal,
            dbus_interface=interface,
            bus_name=None,
            path=path,
            member_keyword='signal_name')


    def __handle_signal(self, *args, **kwargs):
        """Callback to be used when a new key signal is received."""
        signal_name = kwargs.pop('signal_name', '')
        #signal_data = str(args[0])
        logging.info("SIGNAL: " + signal_name + ", " + str(args));
        if self._signals.has_key(signal_name):
          self._signals[signal_name].append(args)
        else:
          self._signals[signal_name] = [args]


class network_ProxyResolver(test.test):
    """A test fixture for validating the integration of
       shill, Chrome, and tlsdated's proxy resolution.
    """
    version = 1
    auto_login = False
    service_settings = { }

    TIMEOUT = 360

    def initialize(self):
       """Constructor
          Sets up the test such that all DBus signals can be
          received and a fake proxy server can be instantiated.
          Additionally, the UI is restarted to ensure consistent
          shared network use.
       """
       super(network_ProxyResolver, self).initialize()
       cros_ui.stop()
       cros_ui.start()
       DBusGMainLoop(set_as_default=True)
       self._listener = SignalListener(gobject.MainLoop())
       self._shill = shill_proxy.ShillProxy.get_proxy()
       if self._shill is None:
         raise error.TestFail('Could not connect to shill')
       # Listen for ProxyResolve responses
       self._listener.listen_for_signal('ProxyChange',
                                        'org.chromium.AutotestProxyInterface',
                                        '/org/chromium/LibCrosService')
       # Listen for network property changes
       self._listener.listen_for_signal('PropertyChanged',
                                        'org.chromium.flimflam.Service',
                                        '/')
       # Listen on the proxy port.
       self._proxy_server = ProxyListener(('', 3128))

    # Set the proxy with Shill. This only works for shared connections
    # (like Eth).
    def set_proxy(self, service_name, proxy_config):
        """Changes the ProxyConfig property on the specified shill service.

        @param service_name: the name, as a str, of the shill service
        @param proxy_config: the ProxyConfig property value string

        @raises TestFail if the service is not found.
        """
        shill = self._shill
        service = shill.find_object('Service', { 'Name' : service_name })
        if not service:
            raise error.TestFail('Service ' + service_name +
                                 ' not found to test proxy with.')
        props = service.GetProperties()
        old_proxy = ''
        if props.has_key('ProxyConfig'):
          old_proxy = props['ProxyConfig']
        if self.service_settings.has_key(service_name) == False:
          logging.info('Preexisting ProxyConfig: ' + service_name +
                       ' -> ' + old_proxy)
          self.service_settings[service_name] = old_proxy
        logging.info('Setting proxy to ' + proxy_config)
        service.SetProperties({'ProxyConfig': proxy_config})


    def reset_services(self):
        """Walks the dict of service->ProxyConfig values and sets the
           proxy back to the originally observed value.
        """
        if len(self.service_settings) == 0:
          return
        for k,v in self.service_settings.items():
          logging.info('Resetting ProxyConfig: ' + k + ' -> ' + v)
          self.set_proxy(k, v)


    def check_chrome(self, proxy_type, proxy_config, timeout):
        """Check that Chrome has acknowledged the supplied proxy config
           by asking for resolution over DBus.

        @param proxy_type: PAC-style string type (e.g., 'PROXY', 'SOCKS')
        @param proxy_config: PAC-style config string (e.g., 127.0.0.1:1234)
        @param timeout: time in seconds to wait for Chrome to issue a signal.

        @return True if a matching response is seen and False otherwise
        """
        bus = dbus.SystemBus()
        dbus_proxy = bus.get_object('org.chromium.LibCrosService',
                                    '/org/chromium/LibCrosService')
        cros_service = dbus.Interface(dbus_proxy,
                                      'org.chromium.LibCrosServiceInterface')
        attempts = timeout
        while attempts > 0:
          cros_service.ResolveNetworkProxy(
                                       'https://clients3.google.com',
                                       'org.chromium.AutotestProxyInterface',
                                       'ProxyChange')
          signals = self._listener.wait_for_signals(
                        'waiting for proxy resolution from Chrome')
          if signals['ProxyChange'][0][1] == proxy_type + ' ' + proxy_config:
            return True
          attempts -= 1
          time.sleep(1)
        logging.error('Last DBus signal seen before giving up: ' + str(signals))
        return False

    def check_tlsdated(self, timeout):
        """Check that tlsdated uses the set proxy.
        @param timeout: time in seconds to wait for tlsdate to restart and query
        @return True if tlsdated hits the proxy server and False otherwise
        """
        # Restart tlsdated to force a network resync
        # (The other option is to force it to think there is no network sync.)
        try:
            self._proxy_server.run()
        except Exception as e:
            logging.error("Proxy error =>" + str(e))
            return False
        logging.info("proxy started!")
        status = subprocess.call(['initctl', 'restart', 'tlsdated'])
        if status != 0:
          logging.info("failed to restart tlsdated")
          return False
        attempts = timeout
        logging.info("waiting for hits on the proxy server")
        while attempts > 0:
          if self._proxy_server.hits() > 0:
            self._proxy_server.reset_hits()
            return True
          time.sleep(1)
          attempts -= 1
        logging.info("no hits")
        return False


    def cleanup(self):
        """Reset all the service data and teardown the proxy."""
        self.reset_services()
        logging.info("tearing down the proxy server")
        self._proxy_server.stop()
        logging.info("proxy server down")
        super(network_ProxyResolver, self).cleanup()


    def test_same_ip_proxy_at_signin_chrome_system_tlsdated(
                                                        self,
                                                        service_name,
                                                        test_timeout=TIMEOUT):
        """ Set the user policy, waits for condition, then logs out.

        @param service_name: shill service name to test on
        @param test_timeout: the total time in seconds split among all timeouts.
        """
        proxy_type = 'http'
        proxy_port = '3128'
        proxy_host = '127.0.0.1'
        proxy_url = proxy_type + '://' + proxy_host + ':' + proxy_port
        # TODO(wad) Only do the below if it was a single protocol proxy.
        # proxy_config = proxy_type + '=' + proxy_host + ':' + proxy_port
        proxy_config = proxy_host + ':' + proxy_port
        self.set_proxy(service_name, '{"mode":"fixed_servers","server":"' +
                                     proxy_config + '"}')

        logging.info("checking chrome")
        if self.check_chrome('PROXY', proxy_config, test_timeout/3) == False:
          raise error.TestFail('Chrome failed to resolve the proxy')

        # Restart tlsdate to force a network fix
        logging.info("checking tlsdated")
        if self.check_tlsdated(test_timeout/3) == False:
          raise error.TestFail('tlsdated never tried the proxy')
        logging.info("done!")

    def run_once(self, test_type, **params):
        logging.info('client: Running client test %s', test_type)
        getattr(self, test_type)(**params)
