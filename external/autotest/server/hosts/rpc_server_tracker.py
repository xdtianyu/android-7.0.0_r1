# Copyright (c) 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import httplib
import logging
import socket
import time
import xmlrpclib

import common
from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros import retry

try:
    import jsonrpclib
except ImportError:
    jsonrpclib = None


class RpcServerTracker(object):
    """
    This class keeps track of all the RPC server connections started on a remote
    host. The caller can use either |xmlrpc_connect| or |jsonrpc_connect| to
    start the required type of rpc server on the remote host.
    The host will cleanup all the open RPC server connections on disconnect.
    """

    _RPC_PROXY_URL_FORMAT = 'http://localhost:%d'
    _RPC_SHUTDOWN_POLLING_PERIOD_SECONDS = 2
    _RPC_SHUTDOWN_TIMEOUT_SECONDS = 10


    def __init__(self, host):
        """
        @param port: The host object associated with this instance of
                     RpcServerTracker.
        """
        self._host = host
        self._rpc_proxy_map = {}


    def _setup_rpc(self, port, command_name, remote_pid=None):
        """Sets up a tunnel process and performs rpc connection book keeping.

        Chrome OS on the target closes down most external ports for security.
        We could open the port, but doing that would conflict with security
        tests that check that only expected ports are open.  So, to get to
        the port on the target we use an ssh tunnel.

        This method assumes that xmlrpc and jsonrpc never conflict, since
        we can only either have an xmlrpc or a jsonrpc server listening on
        a remote port. As such, it enforces a single proxy->remote port
        policy, i.e if one starts a jsonrpc proxy/server from port A->B,
        and then tries to start an xmlrpc proxy forwarded to the same port,
        the xmlrpc proxy will override the jsonrpc tunnel process, however:

        1. None of the methods on the xmlrpc proxy will work because
        the server listening on B is jsonrpc.

        2. The xmlrpc client cannot initiate a termination of the JsonRPC
        server, as the only use case currently is goofy, which is tied to
        the factory image. It is much easier to handle a failed xmlrpc
        call on the client than it is to terminate goofy in this scenario,
        as doing the latter might leave the DUT in a hard to recover state.

        With the current implementation newer rpc proxy connections will
        terminate the tunnel processes of older rpc connections tunneling
        to the same remote port. If methods are invoked on the client
        after this has happened they will fail with connection closed errors.

        @param port: The remote forwarding port.
        @param command_name: The name of the remote process, to terminate
                              using pkill.

        @return A url that we can use to initiate the rpc connection.
        """
        self.disconnect(port)
        local_port = utils.get_unused_port()
        tunnel_proc = self._host.rpc_port_forward(port, local_port)
        self._rpc_proxy_map[port] = (command_name, tunnel_proc, remote_pid)
        return self._RPC_PROXY_URL_FORMAT % local_port


    def xmlrpc_connect(self, command, port, command_name=None,
                       ready_test_name=None, timeout_seconds=10,
                       logfile=None):
        """Connect to an XMLRPC server on the host.

        The `command` argument should be a simple shell command that
        starts an XMLRPC server on the given `port`.  The command
        must not daemonize, and must terminate cleanly on SIGTERM.
        The command is started in the background on the host, and a
        local XMLRPC client for the server is created and returned
        to the caller.

        Note that the process of creating an XMLRPC client makes no
        attempt to connect to the remote server; the caller is
        responsible for determining whether the server is running
        correctly, and is ready to serve requests.

        Optionally, the caller can pass ready_test_name, a string
        containing the name of a method to call on the proxy.  This
        method should take no parameters and return successfully only
        when the server is ready to process client requests.  When
        ready_test_name is set, xmlrpc_connect will block until the
        proxy is ready, and throw a TestError if the server isn't
        ready by timeout_seconds.

        If a server is already running on the remote port, this
        method will kill it and disconnect the tunnel process
        associated with the connection before establishing a new one,
        by consulting the rpc_proxy_map in disconnect.

        @param command Shell command to start the server.
        @param port Port number on which the server is expected to
                    be serving.
        @param command_name String to use as input to `pkill` to
            terminate the XMLRPC server on the host.
        @param ready_test_name String containing the name of a
            method defined on the XMLRPC server.
        @param timeout_seconds Number of seconds to wait
            for the server to become 'ready.'  Will throw a
            TestFail error if server is not ready in time.
        @param logfile Logfile to send output when running
            'command' argument.

        """
        # Clean up any existing state.  If the caller is willing
        # to believe their server is down, we ought to clean up
        # any tunnels we might have sitting around.
        self.disconnect(port)
        if logfile:
            remote_cmd = '%s > %s 2>&1' % (command, logfile)
        else:
            remote_cmd = command
        remote_pid = self._host.run_background(remote_cmd)
        logging.debug('Started XMLRPC server on host %s, pid = %s',
                      self._host.hostname, remote_pid)

        # Tunnel through SSH to be able to reach that remote port.
        rpc_url = self._setup_rpc(port, command_name, remote_pid=remote_pid)
        proxy = xmlrpclib.ServerProxy(rpc_url, allow_none=True)

        if ready_test_name is not None:
            # retry.retry logs each attempt; calculate delay_sec to
            # keep log spam to a dull roar.
            @retry.retry((socket.error,
                          xmlrpclib.ProtocolError,
                          httplib.BadStatusLine),
                         timeout_min=timeout_seconds / 60.0,
                         delay_sec=min(max(timeout_seconds / 20.0, 0.1), 1))
            def ready_test():
                """ Call proxy.ready_test_name(). """
                getattr(proxy, ready_test_name)()
            successful = False
            try:
                logging.info('Waiting %d seconds for XMLRPC server '
                             'to start.', timeout_seconds)
                ready_test()
                successful = True
            finally:
                if not successful:
                    logging.error('Failed to start XMLRPC server.')
                    self.disconnect(port)
        logging.info('XMLRPC server started successfully.')
        return proxy


    def jsonrpc_connect(self, port):
        """Creates a jsonrpc proxy connection through an ssh tunnel.

        This method exists to facilitate communication with goofy (which is
        the default system manager on all factory images) and as such, leaves
        most of the rpc server sanity checking to the caller. Unlike
        xmlrpc_connect, this method does not facilitate the creation of a remote
        jsonrpc server, as the only clients of this code are factory tests,
        for which the goofy system manager is built in to the image and starts
        when the target boots.

        One can theoretically create multiple jsonrpc proxies all forwarded
        to the same remote port, provided the remote port has an rpc server
        listening. However, in doing so we stand the risk of leaking an
        existing tunnel process, so we always disconnect any older tunnels
        we might have through disconnect.

        @param port: port on the remote host that is serving this proxy.

        @return: The client proxy.
        """
        if not jsonrpclib:
            logging.warning('Jsonrpclib could not be imported. Check that '
                            'site-packages contains jsonrpclib.')
            return None

        proxy = jsonrpclib.jsonrpc.ServerProxy(self._setup_rpc(port, None))

        logging.info('Established a jsonrpc connection through port %s.', port)
        return proxy


    def disconnect(self, port):
        """Disconnect from an RPC server on the host.

        Terminates the remote RPC server previously started for
        the given `port`.  Also closes the local ssh tunnel created
        for the connection to the host.  This function does not
        directly alter the state of a previously returned RPC
        client object; however disconnection will cause all
        subsequent calls to methods on the object to fail.

        This function does nothing if requested to disconnect a port
        that was not previously connected via _setup_rpc.

        @param port Port number passed to a previous call to
                    `_setup_rpc()`.
        """
        if port not in self._rpc_proxy_map:
            return
        remote_name, tunnel_proc, remote_pid = self._rpc_proxy_map[port]
        if remote_name:
            # We use 'pkill' to find our target process rather than
            # a PID, because the host may have rebooted since
            # connecting, and we don't want to kill an innocent
            # process with the same PID.
            #
            # 'pkill' helpfully exits with status 1 if no target
            # process  is found, for which run() will throw an
            # exception.  We don't want that, so we the ignore
            # status.
            self._host.run("pkill -f '%s'" % remote_name, ignore_status=True)
            if remote_pid:
                logging.info('Waiting for RPC server "%s" shutdown',
                             remote_name)
                start_time = time.time()
                while (time.time() - start_time <
                       self._RPC_SHUTDOWN_TIMEOUT_SECONDS):
                    running_processes = self._host.run(
                            "pgrep -f '%s'" % remote_name,
                            ignore_status=True).stdout.split()
                    if not remote_pid in running_processes:
                        logging.info('Shut down RPC server.')
                        break
                    time.sleep(self._RPC_SHUTDOWN_POLLING_PERIOD_SECONDS)
                else:
                    raise error.TestError('Failed to shutdown RPC server %s' %
                                          remote_name)

        self._host.rpc_port_disconnect(tunnel_proc, port)
        del self._rpc_proxy_map[port]


    def disconnect_all(self):
        """Disconnect all known RPC proxy ports."""
        for port in self._rpc_proxy_map.keys():
            self.disconnect(port)
