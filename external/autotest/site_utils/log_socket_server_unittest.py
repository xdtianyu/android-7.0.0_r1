# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import logging.handlers
import mox
import multiprocessing
import tempfile
import time
import os
import unittest

import log_socket_server


class TestLogSocketServer(mox.MoxTestBase):
    """Test LogSocketServer can start and save logs to a local file.
    """


    def log_call(self, value, port):
        """Method to be called in a new process to log to a socket server.

        @param value: Data to be logged.
        @param port: Port used by log socket server.
        """
        logging.getLogger().handlers = []
        socketHandler = logging.handlers.SocketHandler('localhost', port)
        logging.getLogger().addHandler(socketHandler)
        logging.getLogger().level = logging.INFO
        logging.info(value)


    def testMultiProcessLoggingSuccess(self):
        """Test log can be saved from multiple processes."""
        # Start log TCP server.
        super(TestLogSocketServer, self).setUp()
        log_filename = tempfile.mktemp(suffix='_log_server')
        log_socket_server.LogSocketServer.start(filename=log_filename,
                                                level=logging.INFO)
        processes = []
        process_number = 10
        port = log_socket_server.LogSocketServer.port
        for i in range(process_number):
            process = multiprocessing.Process(target=self.log_call,
                                              args=(i, port))
            process.start()
            processes.append(process)

        for process in processes:
            process.join()

        # Wait for TCP server to finish processing data. If process_number is
        # increased, the wait time should be increased to avoid test flaky.
        time.sleep(1)
        log_socket_server.LogSocketServer.stop()

        # Read log to confirm all logs are written to file.
        num_lines = sum(1 for line in open(log_filename))
        if process_number != num_lines:
            logging.warn('Not all log messages were written to file %s. '
                         'Expected number of logs: %s, Logs found in file: %s',
                         log_filename, process_number, num_lines)
        self.assertNotEqual(0, num_lines, 'No log message was written to file '
                            '%s. Number of logs tried: %s.' %
                            (log_filename, process_number))
        os.remove(log_filename)


if __name__ == "__main__":
    unittest.main()
