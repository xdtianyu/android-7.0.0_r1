#!/usr/bin/python
#
# Copyright (c) 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.


import argparse
import ctypes
import logging
import logging.handlers
import multiprocessing
import signal
import sys
import time

import common
from autotest_lib.client.common_lib.global_config import global_config as config
from autotest_lib.site_utils import log_socket_server


DEFAULT_PORT = 9080
LOGGING_FORMAT = '%(asctime)s.%(msecs)03d %(levelname)-5.5s| %(message)s'
MEGABYTE = 1024 * 1024


class LogServerAlreadyRunningError(Exception):
    pass


class LogServer(object):
    """A wrapper class to start and stop a TCP server for logging."""

    process = None

    @staticmethod
    def start(port, log_handler):
        """Start Log Record Socket Receiver in a new process.

        @param port: Port to listen on.
        @param log_handler: Logging handler.

        @raise Exception: if TCP server is already running.
        """
        if LogServer.process:
            raise LogServerAlreadyRunningError('LogServer is already running.')
        server_started = multiprocessing.Value(ctypes.c_bool, False)
        LogServer.process = multiprocessing.Process(
                target=LogServer._run,
                args=(server_started, port, log_handler))
        LogServer.process.start()
        while not server_started.value:
            time.sleep(0.1)
        print 'LogServer is started at port %d.' % port


    @staticmethod
    def _run(server_started, port, log_handler):
        """Run LogRecordSocketReceiver to receive log.

        @param server_started: True if socket log server is started.
        @param port: Port used by socket log server.
        @param log_handler: Logging handler.
        """
        # Clear all existing log handlers.
        logging.getLogger().handlers = []
        logging.getLogger().addHandler(log_handler)

        tcp_server = log_socket_server.LogRecordSocketReceiver(
                port=port)
        print('Starting LogServer...')
        server_started.value = True
        tcp_server.serve_until_stopped()


    @staticmethod
    def stop():
        """Stop LogServer."""
        if LogServer.process:
            LogServer.process.terminate()
            LogServer.process = None


def signal_handler(signal, frame):
    """Handler for signal SIGINT.

    @param signal: SIGINT
    @param frame: the current stack frame
    """
    LogServer.stop()
    sys.exit(0)


def get_logging_handler():
    """Return a logging handler.

    Configure a RPC logging handler based on global_config and return
    the handler.
    """
    max_log_size = config.get_config_value('SERVER', 'rpc_max_log_size_mb',
                                           type=int)
    number_of_old_logs = config.get_config_value('SERVER', 'rpc_num_old_logs',
                                                 type=int)
    log_path = config.get_config_value('SERVER', 'rpc_log_path')

    formatter = logging.Formatter(
            fmt=LOGGING_FORMAT, datefmt='%m/%d %H:%M:%S')
    handler = logging.handlers.RotatingFileHandler(
            log_path,
            maxBytes=max_log_size*MEGABYTE,
            backupCount=number_of_old_logs)
    handler.setFormatter(formatter)
    return handler


def main():
    parser = argparse.ArgumentParser(
            formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    parser.add_argument('-p', type=int, dest='port',
                        help=('Listening port number'), default=DEFAULT_PORT)
    options = parser.parse_args()

    signal.signal(signal.SIGINT, signal_handler)

    LogServer.start(options.port, get_logging_handler())


if __name__ == '__main__':
    sys.exit(main())
