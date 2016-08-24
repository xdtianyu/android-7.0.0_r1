# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
import datetime
import logging
import logging.handlers
import os
import socket
import time

from config import rpm_config

import common
from autotest_lib.site_utils import log_socket_server
from autotest_lib.site_utils.rpm_control_system import rpm_infrastructure_exception

LOGGING_FORMAT = rpm_config.get('GENERAL', 'logging_format')
RECEIVERS = rpm_config.get('RPM_INFRASTRUCTURE',
                           'email_notification_recipients').split(',')
SUBJECT_LINE = (rpm_config.get('GENERAL', 'email_subject_line_format') %
                socket.gethostname())


class SuspendableSMTPHandler(logging.handlers.SMTPHandler):
    """SMTPHandler that can have it's emails suspended."""
    _suspend_start_time = datetime.datetime.now()
    _suspend_time_hrs = 0


    def suspend_emails(self, hours):
        """Suspend email notifications.

        @param hours: How many hours to suspend email notifications.
        """
        self._suspend_start_time = datetime.datetime.now()
        self._suspend_time_hrs = int(hours, 0)


    def resume_emails(self):
        """Resume email notifications."""
        self._suspend_time_hrs = 0


    def emit(self, record):
        """Emit a log record.

        This subclassed version only emits the log record if emails are not
        suspended.

        @param record: Log record object we want to emit/record.
        """
        if datetime.datetime.now() < (self._suspend_start_time +
                datetime.timedelta(hours=self._suspend_time_hrs)):
            return
        record.msg += ('\n\nTo disable these emails use rpm_client from your '
                       'local checkout. For a 12 hour suspension run: '
                       'site_utils/rpm_control_system/rpm_client.py -d 12')
        return super(SuspendableSMTPHandler, self).emit(record)


def get_log_filename(log_filename_format):
    """Get file name of log based on given log_filename_format.

    @param log_filename_format: Format to use to create the log file.

    @raise Exception: If log_filename_format is None.
    """
    if not log_filename_format:
            raise Exception('log_filename_format must be set.')

    log_filename = os.path.abspath(time.strftime(log_filename_format))
    log_dir = os.path.dirname(log_filename)
    if not os.path.isdir(log_dir):
        os.makedirs(log_dir)
    return log_filename


def set_up_logging(log_filename_format=None, use_log_server=False):
    """
    Correctly set up logging to have the correct format/level, log to a file,
    and send out email notifications in case of error level messages.

    @param log_filename_format: Format to use to create the log file.
    @param use_log_server: True if log to a TCP server.

    @returns email_handler: Logging handler used to send out email alerts.
    """
    if log_socket_server.LogSocketServer.port is None and use_log_server:
        # Port is unknown, can't log to the server.
        raise rpm_infrastructure_exception.RPMLoggingSetupError(
                'set_up_logging failed: Log server port is unknown.')
    if use_log_server:
        socketHandler = logging.handlers.SocketHandler(
                'localhost', log_socket_server.LogSocketServer.port)
        logging.getLogger().addHandler(socketHandler)
    else:
        log_filename = get_log_filename(log_filename_format)
        logging.basicConfig(filename=log_filename, level=logging.INFO,
                            format=LOGGING_FORMAT)

    if rpm_config.getboolean('GENERAL', 'debug'):
        logging.getLogger().setLevel(logging.DEBUG)

    email_handler = SuspendableSMTPHandler('localhost', 'rpm@google.com',
                                           RECEIVERS, SUBJECT_LINE, None)
    email_handler.setLevel(logging.ERROR)
    email_handler.setFormatter(logging.Formatter(LOGGING_FORMAT))
    logging.getLogger('').addHandler(email_handler)
    return email_handler


def start_log_server(log_filename_format):
    """Start log server to accept logging through a TCP server.

    @param log_filename_format: Format to use to create the log file.
    """
    log_filename = get_log_filename(log_filename_format)
    log_socket_server.LogSocketServer.start(filename=log_filename,
                                            level=logging.INFO,
                                            format=LOGGING_FORMAT)
