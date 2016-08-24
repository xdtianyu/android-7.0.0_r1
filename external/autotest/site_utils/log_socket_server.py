# The source code is from following Python documentation:
# https://docs.python.org/2/howto/logging-cookbook.html#network-logging

# Classes in this file are used to create a simple TCP socket-based logging
# receiver. The receiver listens to default logging port (9020) and save log to
# any given log configuration, e.g., a local file. Once the receiver is running,
# client can add a logging handler to write log to the receiver with following
# sample code:
# socketHandler = logging.handlers.SocketHandler('localhost',
#         logging.handlers.DEFAULT_TCP_LOGGING_PORT)
# logging.getLogger().addHandler(socketHandler)

import ctypes
import pickle
import logging
import multiprocessing
import select
import SocketServer
import struct
import time

import common
from autotest_lib.client.common_lib import utils

class LogRecordStreamHandler(SocketServer.StreamRequestHandler):
    """Handler for a streaming logging request.

    This basically logs the record using whatever logging policy is
    configured locally.
    """

    def handle(self):
        """
        Handle multiple requests - each expected to be a 4-byte length,
        followed by the LogRecord in pickle format. Logs the record
        according to whatever policy is configured locally.
        """
        while True:
            chunk = self.connection.recv(4)
            if len(chunk) < 4:
                return
            slen = struct.unpack('>L', chunk)[0]
            chunk = self.connection.recv(slen)
            while len(chunk) < slen:
                chunk = chunk + self.connection.recv(slen - len(chunk))
            obj = self.unpickle(chunk)
            record = logging.makeLogRecord(obj)
            self.handle_log_record(record)


    def unpickle(self, data):
        """Unpickle data received.

        @param data: Received data.
        @returns: unpickled data.
        """
        return pickle.loads(data)


    def handle_log_record(self, record):
        """Process log record.

        @param record: log record.
        """
        # if a name is specified, we use the named logger rather than the one
        # implied by the record.
        if self.server.logname is not None:
            name = self.server.logname
        else:
            name = record.name
        logger = logging.getLogger(name)
        # N.B. EVERY record gets logged. This is because Logger.handle
        # is normally called AFTER logger-level filtering. If you want
        # to do filtering, do it at the client end to save wasting
        # cycles and network bandwidth!
        logger.handle(record)


class LogRecordSocketReceiver(SocketServer.ThreadingTCPServer):
    """Simple TCP socket-based logging receiver.
    """

    allow_reuse_address = 1

    def __init__(self, host='localhost', port=None,
                 handler=LogRecordStreamHandler):
        if not port:
            port = utils.get_unused_port()
        SocketServer.ThreadingTCPServer.__init__(self, (host, port), handler)
        self.abort = 0
        self.timeout = 1
        self.logname = None
        self.port = port


    def serve_until_stopped(self):
        """Run the socket receiver until aborted."""
        print 'Log Record Socket Receiver is started.'
        abort = 0
        while not abort:
            rd, wr, ex = select.select([self.socket.fileno()], [], [],
                                       self.timeout)
            if rd:
                self.handle_request()
            abort = self.abort
        print 'Log Record Socket Receiver is stopped.'


class LogSocketServer:
    """A wrapper class to start and stop a TCP server for logging."""

    process = None
    port = None

    @staticmethod
    def start(**kwargs):
        """Start Log Record Socket Receiver in a new process.

        @param kwargs: log configuration, e.g., format, filename.

        @raise Exception: if TCP server is already running.
        """
        if LogSocketServer.process:
            raise Exception('Log Record Socket Receiver is already running.')
        server_started = multiprocessing.Value(ctypes.c_bool, False)
        port = multiprocessing.Value(ctypes.c_int, 0)
        LogSocketServer.process = multiprocessing.Process(
                target=LogSocketServer._start_server,
                args=(server_started, port),
                kwargs=kwargs)
        LogSocketServer.process.start()
        while not server_started.value:
            time.sleep(0.1)
        LogSocketServer.port = port.value
        print 'Log Record Socket Server is started at port %d.' % port.value


    @staticmethod
    def _start_server(server_started, port, **kwargs):
        """Start the TCP server to receive log.

        @param server_started: True if socket log server is started.
        @param port: Port used by socket log server.
        @param kwargs: log configuration, e.g., format, filename.
        """
        # Clear all existing log handlers.
        logging.getLogger().handlers = []
        if not kwargs:
            logging.basicConfig(
                    format='%(asctime)s - %(levelname)s - %(message)s')
        else:
            logging.basicConfig(**kwargs)

        tcp_server = LogRecordSocketReceiver()
        print('Starting TCP server...')
        server_started.value = True
        port.value = tcp_server.port
        tcp_server.serve_until_stopped()


    @staticmethod
    def stop():
        """Stop Log Record Socket Receiver.
        """
        if LogSocketServer.process:
            LogSocketServer.process.terminate()
            LogSocketServer.process = None
            LogSocketServer.port = None
