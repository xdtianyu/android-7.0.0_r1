#!/usr/bin/python

# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""A small wrapper script, iterates through
the known hosts and tries to call get_labels()
to discover host functionality, and adds these
detected labels to host.

Limitations:
 - Does not keep a count of how many labels were
   actually added.
 - If a label is added by this script because it
   is detected as supported by get_labels, but later becomes
   unsupported, this script has no way to know that it
   should be removed, so it will remain attached to the host.
   See crosbug.com/38569
"""


from multiprocessing import pool
import logging
import socket
import argparse
import sys

import common

from autotest_lib.server import hosts
from autotest_lib.server import frontend
from autotest_lib.client.common_lib import error


# A list of label prefix that each dut should only have one of such label with
# the given prefix, e.g., a dut can't have both labels of power:battery and
# power:AC_only.
SINGLETON_LABEL_PREFIX = ['power:']

def add_missing_labels(afe, hostname):
    """
    Queries the detectable labels supported by the given host,
    and adds those labels to the host.

    @param afe: A frontend.AFE() instance.
    @param hostname: The host to query and update.

    @return: True on success.
             False on failure to fetch labels or to add any individual label.
    """
    host = None
    try:
        host = hosts.create_host(hostname)
        labels = host.get_labels()
    except socket.gaierror:
        logging.warning('Unable to establish ssh connection to hostname '
                        '%s. Skipping.', hostname)
        return False
    except error.AutoservError:
        logging.warning('Unable to query labels on hostname %s. Skipping.',
                         hostname)
        return False
    finally:
        if host:
            host.close()

    afe_host = afe.get_hosts(hostname=hostname)[0]

    label_matches = afe.get_labels(name__in=labels)

    for label in label_matches:
        singleton_prefixes = [p for p in SINGLETON_LABEL_PREFIX
                              if label.name.startswith(p)]
        if len(singleton_prefixes) == 1:
            singleton_prefix = singleton_prefixes[0]
            # Delete existing label with `singleton_prefix`
            labels_to_delete = [l for l in afe_host.labels
                                if l.startswith(singleton_prefix) and
                                not l in labels]
            if labels_to_delete:
                logging.warning('Removing label %s', labels_to_delete)
                afe_labels_to_delete = afe.get_labels(name__in=labels_to_delete)
                for afe_label in afe_labels_to_delete:
                    afe_label.remove_hosts(hosts=[hostname])
        label.add_hosts(hosts=[hostname])

    missing_labels = set(labels) - set([l.name for l in label_matches])

    if missing_labels:
        for label in missing_labels:
            logging.warning('Unable to add label %s to host %s. '
                            'Skipping unknown label.', label, hostname)
        return False

    return True


def main():
    """"
    Entry point for add_detected_host_labels script.
    """

    parser = argparse.ArgumentParser()
    parser.add_argument('-s', '--silent', dest='silent', action='store_true',
                        help='Suppress all but critical logging messages.')
    parser.add_argument('-i', '--info', dest='info_only', action='store_true',
                        help='Suppress logging messages below INFO priority.')
    parser.add_argument('-m', '--machines', dest='machines',
                        help='Comma separated list of machines to check.')
    options = parser.parse_args()

    if options.silent and options.info_only:
        print 'The -i and -s flags cannot be used together.'
        parser.print_help()
        return 0


    if options.silent:
        logging.disable(logging.CRITICAL)

    if options.info_only:
        logging.disable(logging.DEBUG)

    threadpool = pool.ThreadPool()
    afe = frontend.AFE()

    if options.machines:
        hostnames = [m.strip() for m in options.machines.split(',')]
    else:
        hostnames = afe.get_hostnames()
    successes = sum(threadpool.imap_unordered(
                        lambda x: add_missing_labels(afe, x),
                        hostnames))
    attempts = len(hostnames)

    logging.info('Label updating finished. Failed update on %d out of %d '
                 'hosts.', attempts-successes, attempts)

    return 0


if __name__ == '__main__':
    sys.exit(main())
