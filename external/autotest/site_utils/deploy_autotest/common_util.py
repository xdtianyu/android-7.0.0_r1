#!/usr/bin/python

# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Module storing common utilities used by the deploy_autotest package.

This python module is shared between the two programs in deploy_autotest.
Specifically it contains shared constants and their shared arg parser.
"""

import argparse
import logging

def setup_logging():
    """Setup basic logging with all logging info stripped."""
    screen_handler = logging.StreamHandler()
    screen_handler.setFormatter(logging.Formatter('%(message)s'))
    logging.getLogger().addHandler(screen_handler)
    logging.getLogger().setLevel(logging.INFO)


SYNC = 'sync'
RESTART = 'restart'
PRINT = 'print'
VALID_COMMANDS = [SYNC, RESTART, PRINT]

DEVS = 'devservers'
DRONES = 'drones'
SCHEDULER = 'scheduler'
VALID_TARGETS = [DEVS, DRONES, SCHEDULER]


def parse_args(argv):
    parser = argparse.ArgumentParser()
    parser.add_argument('operation',
                        help='Operation to perform. Must be one of: %s' %
                        ' '.join(VALID_COMMANDS))
    parser.add_argument('servers', nargs='+',
                        help='Any set of items from the list: %s' %
                        ' '.join(VALID_TARGETS))
    parsed_args = parser.parse_args(argv)

    # Some sanity checks.
    if not parsed_args.operation in VALID_COMMANDS:
        parser.error('Invalid operation specified. Must be one of: %s' %
                     ' '.join(VALID_COMMANDS))

    if not set(parsed_args.servers).issubset(set(VALID_TARGETS)):
        parser.error('All servers must be one of %s' % ' '.join(VALID_TARGETS))

    return parsed_args
