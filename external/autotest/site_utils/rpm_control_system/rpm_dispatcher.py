#!/usr/bin/python
# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import atexit
import errno
import logging
import re
import sys
import socket
import threading
import xmlrpclib

import rpm_controller
import rpm_logging_config

from config import rpm_config
from MultiThreadedXMLRPCServer import MultiThreadedXMLRPCServer
from rpm_infrastructure_exception import RPMInfrastructureException

import common
from autotest_lib.site_utils.rpm_control_system import utils

LOG_FILENAME_FORMAT = rpm_config.get('GENERAL','dispatcher_logname_format')


class RPMDispatcher(object):
    """
    This class is the RPM dispatcher server and it is responsible for
    communicating directly to the RPM devices to change a DUT's outlet status.

    When an RPMDispatcher is initialized it registers itself with the frontend
    server, who will field out outlet requests to this dispatcher.

    Once a request is received the dispatcher looks up the RPMController
    instance for the given DUT and then queues up the request and blocks until
    it is processed.

    @var _address: IP address or Hostname of this dispatcher server.
    @var _frontend_server: URI of the frontend server.
    @var _lock: Lock used to synchronize access to _worker_dict.
    @var _port: Port assigned to this server instance.
    @var _worker_dict: Dictionary mapping RPM hostname's to RPMController
                       instances.
    """


    def __init__(self, address, port):
        """
        RPMDispatcher constructor.

        Initialized instance vars and registers this server with the frontend
        server.

        @param address: Address of this dispatcher server.
        @param port: Port assigned to this dispatcher server.

        @raise RPMInfrastructureException: Raised if the dispatch server is
                                           unable to register with the frontend
                                           server.
        """
        self._address = address
        self._port = port
        self._lock = threading.Lock()
        self._worker_dict = {}
        self._frontend_server = rpm_config.get('RPM_INFRASTRUCTURE',
                                               'frontend_uri')
        logging.info('Registering this rpm dispatcher with the frontend '
                     'server at %s.', self._frontend_server)
        client = xmlrpclib.ServerProxy(self._frontend_server)
        # De-register with the frontend when the dispatcher exit's.
        atexit.register(self._unregister)
        try:
            client.register_dispatcher(self._get_serveruri())
        except socket.error as er:
            err_msg = ('Unable to register with frontend server. Error: %s.' %
                       errno.errorcode[er.errno])
            logging.error(err_msg)
            raise RPMInfrastructureException(err_msg)


    def _worker_dict_put(self, key, value):
        """
        Private method used to synchronize access to _worker_dict.

        @param key: key value we are using to access _worker_dict.
        @param value: value we are putting into _worker_dict.
        """
        with self._lock:
            self._worker_dict[key] = value


    def _worker_dict_get(self, key):
        """
        Private method used to synchronize access to _worker_dict.

        @param key: key value we are using to access _worker_dict.
        @return: value found when accessing _worker_dict
        """
        with self._lock:
            return self._worker_dict.get(key)


    def is_up(self):
        """
        Allows the frontend server to see if the dispatcher server is up before
        attempting to queue requests.

        @return: True. If connection fails, the client proxy will throw a socket
                 error on the client side.
        """
        return True


    def queue_request(self, powerunit_info_dict, new_state):
        """
        Looks up the appropriate RPMController instance for the device and queues
        up the request.

        @param powerunit_info_dict: A dictionary, containing the attribute/values
                                    of an unmarshalled PowerUnitInfo instance.
        @param new_state: [ON, OFF, CYCLE] state we want to the change the
                          outlet to.
        @return: True if the attempt to change power state was successful,
                 False otherwise.
        """
        powerunit_info = utils.PowerUnitInfo(**powerunit_info_dict)
        logging.info('Received request to set device: %s to state: %s',
                     powerunit_info.device_hostname, new_state)
        rpm_controller = self._get_rpm_controller(
                powerunit_info.powerunit_hostname,
                powerunit_info.hydra_hostname)
        return rpm_controller.queue_request(powerunit_info, new_state)


    def _get_rpm_controller(self, rpm_hostname, hydra_hostname=None):
        """
        Private method that retreives the appropriate RPMController instance
        for this RPM Hostname or calls _create_rpm_controller it if it does not
        already exist.

        @param rpm_hostname: hostname of the RPM whose RPMController we want.

        @return: RPMController instance responsible for this RPM.
        """
        if not rpm_hostname:
            return None
        rpm_controller = self._worker_dict_get(rpm_hostname)
        if not rpm_controller:
            rpm_controller = self._create_rpm_controller(
                    rpm_hostname, hydra_hostname)
            self._worker_dict_put(rpm_hostname, rpm_controller)
        return rpm_controller


    def _create_rpm_controller(self, rpm_hostname, hydra_hostname):
        """
        Determines the type of RPMController required and initializes it.

        @param rpm_hostname: Hostname of the RPM we need to communicate with.

        @return: RPMController instance responsible for this RPM.
        """
        hostname_elements = rpm_hostname.split('-')
        if hostname_elements[-2] == 'poe':
            # POE switch hostname looks like 'chromeos2-poe-switch1'.
            logging.info('The controller is a Cisco POE switch.')
            return rpm_controller.CiscoPOEController(rpm_hostname)
        else:
            # The device is an RPM.
            rack_id = hostname_elements[-2]
            rpm_typechecker = re.compile('rack[0-9]+[a-z]+')
            if rpm_typechecker.match(rack_id):
                logging.info('RPM is a webpowered device.')
                return rpm_controller.WebPoweredRPMController(rpm_hostname)
            else:
                logging.info('RPM is a Sentry CDU device.')
                return rpm_controller.SentryRPMController(
                        hostname=rpm_hostname,
                        hydra_hostname=hydra_hostname)


    def _get_serveruri(self):
        """
        Formats the _address and _port into a meaningful URI string.

        @return: URI of this dispatch server.
        """
        return 'http://%s:%d' % (self._address, self._port)


    def _unregister(self):
        """
        Tells the frontend server that this dispatch server is shutting down and
        to unregister it.

        Called by atexit.

        @raise RPMInfrastructureException: Raised if the dispatch server is
                                           unable to unregister with the
                                           frontend server.
        """
        logging.info('Dispatch server shutting down. Unregistering with RPM '
                     'frontend server.')
        client = xmlrpclib.ServerProxy(self._frontend_server)
        try:
            client.unregister_dispatcher(self._get_serveruri())
        except socket.error as er:
            err_msg = ('Unable to unregister with frontend server. Error: %s.' %
                       errno.errorcode[er.errno])
            logging.error(err_msg)
            raise RPMInfrastructureException(err_msg)


def launch_server_on_unused_port():
    """
    Looks up an unused port on this host and launches the xmlrpc server.

    Useful for testing by running multiple dispatch servers on the same host.

    @return: server,port - server object and the port that which it is listening
             to.
    """
    address = socket.gethostbyname(socket.gethostname())
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    # Set this socket to allow reuse.
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    sock.bind(('', 0))
    port = sock.getsockname()[1]
    server = MultiThreadedXMLRPCServer((address, port),
                                       allow_none=True)
    sock.close()
    return server, port


if __name__ == '__main__':
    """
    Main function used to launch the dispatch server. Creates an instance of
    RPMDispatcher and registers it to a MultiThreadedXMLRPCServer instance.
    """
    if len(sys.argv) > 1:
      print 'Usage: ./%s, no arguments available.' % sys.argv[0]
      sys.exit(1)
    rpm_logging_config.start_log_server(LOG_FILENAME_FORMAT)
    rpm_logging_config.set_up_logging(use_log_server=True)

    # Get the local ip _address and set the server to utilize it.
    address = socket.gethostbyname(socket.gethostname())
    server, port = launch_server_on_unused_port()
    rpm_dispatcher = RPMDispatcher(address, port)
    server.register_instance(rpm_dispatcher)
    server.serve_forever()
