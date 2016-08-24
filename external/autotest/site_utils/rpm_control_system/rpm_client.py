#!/usr/bin/python
# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import argparse
import logging
import sys
import xmlrpclib

import common

from config import rpm_config
from autotest_lib.client.common_lib import global_config
from autotest_lib.client.common_lib.cros import retry


RPM_FRONTEND_URI = global_config.global_config.get_config_value('CROS',
        'rpm_frontend_uri', type=str, default='')
RPM_CALL_TIMEOUT_MINS = rpm_config.getint('RPM_INFRASTRUCTURE',
                                          'call_timeout_mins')


class RemotePowerException(Exception):
    """This is raised when we fail to set the state of the device's outlet."""
    pass


def set_power(hostname, new_state, timeout_mins=RPM_CALL_TIMEOUT_MINS):
    """Sends the power state change request to the RPM Infrastructure.

    @param hostname: host who's power outlet we want to change.
    @param new_state: State we want to set the power outlet to.
    """
    client = xmlrpclib.ServerProxy(RPM_FRONTEND_URI, verbose=False)
    timeout = None
    result = None
    try:
        timeout, result = retry.timeout(client.queue_request,
                                        args=(hostname, new_state),
                                        timeout_sec=timeout_mins * 60,
                                        default_result=False)
    except Exception as e:
        logging.exception(e)
        raise RemotePowerException(
                'Client call exception: ' + str(e))
    if timeout:
        raise RemotePowerException(
                'Call to RPM Infrastructure timed out.')
    if not result:
        error_msg = ('Failed to change outlet status for host: %s to '
                     'state: %s.' % (hostname, new_state))
        logging.error(error_msg)
        raise RemotePowerException(error_msg)


def parse_options():
    """Parse the user supplied options."""
    parser = argparse.ArgumentParser()
    parser.add_argument('-m', '--machine', dest='machine',
                        help='Machine hostname to change outlet state.')
    parser.add_argument('-s', '--state', dest='state',
                        help='Power state to set outlet: ON, OFF, CYCLE')
    parser.add_argument('-d', '--disable_emails', dest='disable_emails',
                        help='Hours to suspend RPM email notifications.')
    parser.add_argument('-e', '--enable_emails', dest='enable_emails',
                        action='store_true',
                        help='Resume RPM email notifications.')
    return parser.parse_args()


def main():
    """Entry point for rpm_client script."""
    options = parse_options()
    if options.machine is not None and options.state is None:
        print 'Need --state to change outlet state.'
    elif options.machine is not None and options.state is not None:
        set_power(options.machine, options.state)

    if options.disable_emails is not None:
        client = xmlrpclib.ServerProxy(RPM_FRONTEND_URI, verbose=False)
        client.suspend_emails(options.disable_emails)
    if options.enable_emails:
        client = xmlrpclib.ServerProxy(RPM_FRONTEND_URI, verbose=False)
        client.resume_emails()


if __name__ == "__main__":
    sys.exit(main())
