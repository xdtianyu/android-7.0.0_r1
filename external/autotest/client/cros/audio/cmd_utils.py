#!/usr/bin/python
# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import atexit
import itertools
import logging
import os
import pipes
import select
import subprocess
import threading

from subprocess import PIPE
from autotest_lib.client.common_lib.utils import TEE_TO_LOGS

_popen_lock = threading.Lock()
_logging_service = None
_command_serial_number = itertools.count(1)

_LOG_BUFSIZE = 4096
_PIPE_CLOSED = -1

class _LoggerProxy(object):

    def __init__(self, logger):
        self._logger = logger

    def fileno(self):
        return self._logger._pipe[1]

    def __del__(self):
        self._logger.close()


class _PipeLogger(object):

    def __init__(self, level, prefix):
        self._pipe = list(os.pipe())
        self._level = level
        self._prefix = prefix

    def close(self):
        if self._pipe[1] != _PIPE_CLOSED:
            os.close(self._pipe[1])
            self._pipe[1] = _PIPE_CLOSED


class _LoggingService(object):

    def __init__(self):
        # Python's list is thread safe
        self._loggers = []

        # Change tuple  to list so that we can change the value when
        # closing the pipe.
        self._pipe = list(os.pipe())
        self._thread = threading.Thread(target=self._service_run)
        self._thread.daemon = True
        self._thread.start()


    def _service_run(self):
        terminate_loop = False
        while not terminate_loop:
            rlist = [l._pipe[0] for l in self._loggers]
            rlist.append(self._pipe[0])
            for r in select.select(rlist, [], [])[0]:
                data = os.read(r, _LOG_BUFSIZE)
                if r != self._pipe[0]:
                    self._output_logger_message(r, data)
                elif len(data) == 0:
                    terminate_loop = True
        # Release resources.
        os.close(self._pipe[0])
        for logger in self._loggers:
            os.close(logger._pipe[0])


    def _output_logger_message(self, r, data):
        logger = next(l for l in self._loggers if l._pipe[0] == r)

        if len(data) == 0:
            os.close(logger._pipe[0])
            self._loggers.remove(logger)
            return

        for line in data.split('\n'):
            logging.log(logger._level, '%s%s', logger._prefix, line)


    def create_logger(self, level=logging.DEBUG, prefix=''):
        logger = _PipeLogger(level=level, prefix=prefix)
        self._loggers.append(logger)
        os.write(self._pipe[1], '\0')
        return _LoggerProxy(logger)


    def shutdown(self):
        if self._pipe[1] != _PIPE_CLOSED:
            os.close(self._pipe[1])
            self._pipe[1] = _PIPE_CLOSED
            self._thread.join()


def create_logger(level=logging.DEBUG, prefix=''):
    global _logging_service
    if _logging_service is None:
        _logging_service = _LoggingService()
        atexit.register(_logging_service.shutdown)
    return _logging_service.create_logger(level=level, prefix=prefix)


def kill_or_log_returncode(*popens):
    '''Kills all the processes of the given Popens or logs the return code.

    @param poopens: The Popens to be killed.
    '''
    for p in popens:
        if p.poll() is None:
            try:
                p.kill()
            except Exception as e:
                logging.warning('failed to kill %d, %s', p.pid, e)
        else:
            logging.warning('command exit (pid=%d, rc=%d): %s',
                            p.pid, p.returncode, p.command)


def wait_and_check_returncode(*popens):
    '''Wait for all the Popens and check the return code is 0.

    If the return code is not 0, it raises an RuntimeError.

    @param popens: The Popens to be checked.
    '''
    error_message = None
    for p in popens:
        if p.wait() != 0:
            error_message = ('Command failed(%d, %d): %s' %
                             (p.pid, p.returncode, p.command))
            logging.error(error_message)
    if error_message:
        raise RuntimeError(error_message)


def execute(args, stdin=None, stdout=TEE_TO_LOGS, stderr=TEE_TO_LOGS):
    '''Executes a child command and wait for it.

    Returns the output from standard output if 'stdout' is subprocess.PIPE.
    Raises RuntimeException if the return code of the child command is not 0.

    @param args: the command to be executed
    @param stdin: the executed program's standard input
    @param stdout: the executed program's stdandrd output
    '''
    ps = popen(args, stdin=stdin, stdout=stdout, stderr=stderr)
    out = ps.communicate()[0] if stdout == subprocess.PIPE else None
    wait_and_check_returncode(ps)
    return out


def popen(args, stdin=None, stdout=TEE_TO_LOGS, stderr=TEE_TO_LOGS, env=None):
    '''Returns a Popen object just as subprocess.Popen does but with the
    executed command stored in Popen.command.
    '''
    command_id = _command_serial_number.next()
    prefix = '[%04d] ' % command_id

    if stdout is TEE_TO_LOGS:
        stdout = create_logger(level=logging.DEBUG, prefix=prefix)
    if stderr is TEE_TO_LOGS:
        stderr = create_logger(level=logging.ERROR, prefix=prefix)

    command = ' '.join(pipes.quote(x) for x in args)
    logging.info('%sRunning: %s', prefix, command)

    # The lock is required for http://crbug.com/323843.
    with _popen_lock:
        ps = subprocess.Popen(args, stdin=stdin, stdout=stdout, stderr=stderr,
            env=env)
    logging.info('%spid is %d', prefix, ps.pid)
    ps.command_id = command_id
    ps.command = command
    return ps
