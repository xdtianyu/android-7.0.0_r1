#!/usr/bin/python
# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""
Mail the content of standard input.

Example usage:
  Use pipe:
     $ echo "Some content" |./gmail_lib.py -s "subject" abc@bb.com xyz@gmail.com

  Manually input:
     $ ./gmail_lib.py -s "subject" abc@bb.com xyz@gmail.com
     > Line 1
     > Line 2
     Ctrl-D to end standard input.
"""
import argparse
import base64
import httplib2
import logging
import sys
import os
from email.mime.text import MIMEText

import common
from autotest_lib.client.common_lib import global_config
from autotest_lib.client.common_lib.cros.graphite import autotest_stats
from autotest_lib.server import utils as server_utils
from chromite.lib import retry_util

try:
  from apiclient.discovery import build as apiclient_build
  from apiclient import errors as apiclient_errors
  from oauth2client import file as oauth_client_fileio
except ImportError as e:
  apiclient_build = None
  logging.debug("API client for gmail disabled. %s", e)


EMAIL_COUNT_KEY = 'emails.%s'
DEFAULT_CREDS_FILE = global_config.global_config.get_config_value(
        'NOTIFICATIONS', 'gmail_api_credentials', default=None)
RETRY_DELAY = 5
RETRY_BACKOFF_FACTOR = 1.5
MAX_RETRY = 10
RETRIABLE_MSGS = [
        # User-rate limit exceeded
        r'HttpError 429',]

class GmailApiException(Exception):
    """Exception raised in accessing Gmail API."""


class Message():
    """An email message."""

    def __init__(self, to, subject, message_text):
        """Initialize a message.

        @param to: The recievers saperated by comma.
                   e.g. 'abc@gmail.com,xyz@gmail.com'
        @param subject: String, subject of the message
        @param message_text: String, content of the message.
        """
        self.to = to
        self.subject = subject
        self.message_text = message_text


    def get_payload(self):
        """Get the payload that can be sent to the Gmail API.

        @return: A dictionary representing the message.
        """
        message = MIMEText(self.message_text)
        message['to'] = self.to
        message['subject'] = self.subject
        return {'raw': base64.urlsafe_b64encode(message.as_string())}


class GmailApiClient():
    """Client that talks to Gmail API."""

    def __init__(self, oauth_credentials):
        """Init Gmail API client

        @param oauth_credentials: Path to the oauth credential token.
        """
        if not apiclient_build:
            raise GmailApiException('Cannot get apiclient library.')

        storage = oauth_client_fileio.Storage(oauth_credentials)
        credentials = storage.get()
        if not credentials or credentials.invalid:
            raise GmailApiException('Invalid credentials for Gmail API, '
                                    'could not send email.')
        http = credentials.authorize(httplib2.Http())
        self._service = apiclient_build('gmail', 'v1', http=http)


    def send_message(self, message, ignore_error=True):
        """Send an email message.

        @param message: Message to be sent.
        @param ignore_error: If True, will ignore any HttpError.
        """
        try:
            # 'me' represents the default authorized user.
            message = self._service.users().messages().send(
                    userId='me', body=message.get_payload()).execute()
            logging.debug('Email sent: %s' , message['id'])
        except apiclient_errors.HttpError as error:
            if ignore_error:
                logging.error('Failed to send email: %s', error)
            else:
                raise


def send_email(to, subject, message_text, retry=True, creds_path=None):
    """Send email.

    @param to: The recipients, separated by comma.
    @param subject: Subject of the email.
    @param message_text: Text to send.
    @param retry: If retry on retriable failures as defined in RETRIABLE_MSGS.
    @param creds_path: The credential path for gmail account, if None,
                       will use DEFAULT_CREDS_FILE.
    """
    auth_creds = server_utils.get_creds_abspath(
        creds_path or DEFAULT_CREDS_FILE)
    if not auth_creds or not os.path.isfile(auth_creds):
        logging.error('Failed to send email to %s: Credential file does not'
                      'exist: %s. If this is a prod server, puppet should'
                      'install it. If you need to be able to send email, '
                      'find the credential file from chromeos-admin repo and '
                      'copy it to %s', to, auth_creds, auth_creds)
        return
    client = GmailApiClient(oauth_credentials=auth_creds)
    m = Message(to, subject, message_text)
    retry_count = MAX_RETRY if retry else 0

    def _run():
        """Send the message."""
        client.send_message(m, ignore_error=False)

    def handler(exc):
        """Check if exc is an HttpError and is retriable.

        @param exc: An exception.

        @return: True if is an retriable HttpError.
        """
        if not isinstance(exc, apiclient_errors.HttpError):
            return False

        error_msg = str(exc)
        should_retry = any([msg in error_msg for msg in RETRIABLE_MSGS])
        if should_retry:
            logging.warning('Will retry error %s', exc)
        return should_retry

    autotest_stats.Counter(EMAIL_COUNT_KEY % 'total').increment()
    try:
        retry_util.GenericRetry(
                handler, retry_count, _run, sleep=RETRY_DELAY,
                backoff_factor=RETRY_BACKOFF_FACTOR)
    except Exception:
        autotest_stats.Counter(EMAIL_COUNT_KEY % 'fail').increment()
        raise


if __name__ == '__main__':
    logging.basicConfig(level=logging.DEBUG)
    parser = argparse.ArgumentParser(
            description=__doc__, formatter_class=argparse.RawTextHelpFormatter)
    parser.add_argument('-s', '--subject', type=str, dest='subject',
                        required=True, help='Subject of the mail')
    parser.add_argument('recipients', nargs='*',
                        help='Email addresses separated by space.')
    args = parser.parse_args()
    if not args.recipients or not args.subject:
        print 'Requires both recipients and subject.'
        sys.exit(1)

    message_text = sys.stdin.read()
    send_email(','.join(args.recipients), args.subject , message_text)
