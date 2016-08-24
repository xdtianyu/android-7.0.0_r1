"""Scheduler email manager."""


import logging
import os
import re
import socket
import time
import traceback

import common
from autotest_lib.client.common_lib import global_config
from autotest_lib.site_utils  import gmail_lib


CONFIG_SECTION = 'SCHEDULER'


class EmailNotificationManager(object):
    """Scheduler email notification manager."""

    def __init__(self):
        """Initialize the manager."""
        self._emails = []
        self._notify_address = global_config.global_config.get_config_value(
            CONFIG_SECTION, "notify_email", default='')


    def send_email(self, to_string, subject, body):
        """Mails out emails to the addresses listed in to_string.

        @param to_string: is split into a list which can be delimited by any of:
                          ';', ',', ':' or any whitespace.
        @param subject: String, email subject.
        @param body: String, message body
        """
        # Create list from string removing empty strings from the list.
        to_list = [x for x in re.split('\s|,|;|:', to_string) if x]
        if not to_list:
            return
        to_string = ','.join(to_list)
        try:
            gmail_lib.send_email(to_string, subject, body)
        except Exception:
            logging.exception('Sending email failed:')


    def enqueue_notify_email(self, subject, message):
        """Enqueue a message that will be sent later.

        @param subject: String, subject of the message.
        @param message: String, message to enqueue.
        """
        logging.error(subject + '\n' + message)
        if not self._notify_address:
            return

        body = 'Subject: ' + subject + '\n'
        body += "%s / %s / %s\n%s" % (socket.gethostname(),
                                      os.getpid(),
                                      time.strftime("%X %x"), message)
        self._emails.append(body)


    def send_queued_emails(self):
        """Send queued emails."""
        if not self._emails:
            return
        subject = 'Scheduler notifications from ' + socket.gethostname()
        separator = '\n' + '-' * 40 + '\n'
        body = separator.join(self._emails)

        self.send_email(self._notify_address, subject, body)
        self._emails = []


    def log_stacktrace(self, reason):
        """Log an exception and enqueue it.

        @param reason: An exception to log and send.
        """
        logging.exception(reason)
        message = "EXCEPTION: %s\n%s" % (reason, traceback.format_exc())
        self.enqueue_notify_email("monitor_db exception", message)


manager = EmailNotificationManager()
