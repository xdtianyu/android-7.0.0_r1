#! /usr/bin/python

# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""
Manage power unit information for autotest hosts.

  We store rpm hostname, outlet, hydra information for a host in cautotest
  as host attributes. This tool allows you to add/modify/view/backup
  rpm attributes for hosts.

* Add/Modify power unit attributes:
  Step 1: create csv:
    Put attributes in a csv file, e.g. mapping.csv.
    Each line in mapping.csv consists of
        device_hostname, powerunit_hostname, powerunit_outlet, hydra_hostname,
    seperated by comma. For example

    chromeos-rack2-host1,chromeos-rack2-rpm1,.A1,chromeos-197-hydra1.mtv,
    chromeos-rack2-host2,chromeos-rack2-rpm1,.A2,chromeos-197-hydra1.mtv,

  Step 2: run
    ./manage_powerunit_info.py upload --csv mapping_file.csv

* View power unit attributes:
    ./manage_powerunit_info.py list
        -m "chromeos-rack2-host1,chromeos-rack2-host2"

* Backup existing attributes for all hosts to a csv file:
    ./manage_powerunit_info.py backup --csv backup.csv
"""
import argparse
import csv
import logging
import os
import sys

import common

from autotest_lib.client.common_lib import global_config
from autotest_lib.server.cros.dynamic_suite import frontend_wrappers
from autotest_lib.site_utils.rpm_control_system import utils as rpm_utils


# The host attribute key name for get rpm hostname.
POWERUNIT_KEYS = [rpm_utils.POWERUNIT_HOSTNAME_KEY,
                  rpm_utils.POWERUNIT_OUTLET_KEY,
                  rpm_utils.HYDRA_HOSTNAME_KEY]
DEFAULT_SERVER = global_config.global_config.get_config_value(
        'SERVER', 'hostname', default=None)


def add_powerunit_info_to_host(afe, device, keyvals):
    """Add keyvals to the host's attributes in AFE.

    @param afe: AFE server to talk to.
    @param device: the device hostname, e.g. 'chromeos1-rack1-host1'
    @param keyvals: A dictionary where keys are the values in POWERUNIT_KEYS.
                    These are the power unit info about the devcie that we
                    are going to insert to AFE as host attributes.
    """
    if not afe.get_hosts(hostname=device):
        logging.debug('No host named %s', device)
        return

    logging.info('Adding host attribues to %s: %s', device, keyvals)
    for key, val in keyvals.iteritems():
        afe.set_host_attribute(key, val, hostname=device)


def add_from_csv(afe, csv_file):
    """Read power unit information from csv and add to host attributes.

    @param afe: AFE server to talk to.
    @param csv_file: A csv file, each line consists of device_hostname,
                     powerunit_hostname powerunit_outlet, hydra_hostname
                     separated by comma.
    """
    with open(csv_file) as f:
        reader = csv.reader(f, delimiter=',')
        for row in reader:
            device = row[0].strip()
            hydra = row[3].strip()
            if not hydra:
                hydra = None
            keyvals = dict(zip(
                    POWERUNIT_KEYS,
                    [row[1].strip(), row[2].strip(), hydra]))
            add_powerunit_info_to_host(afe, device, keyvals)


def dump_to_csv(afe, csv_file):
    """Dump power unit info of all hosts to a csv file.

    @param afe: AFE server to talk to.
    @param csv_file: A file to store the power unit information.

    """
    logging.info('Back up host attribues to %s', csv_file)
    with open(csv_file, 'w') as f:
        hosts = afe.get_hosts()
        for h in hosts:
            logging.info('Proccessing %s', h.hostname)
            f.write(h.hostname + ',')
            for key in POWERUNIT_KEYS:
                f.write(h.attributes.get(key, '') + ',')
            f.write('\n')


def list_powerunit_info(afe, devices):
    """List power unit info for a list of hosts.

    @param afe: AFE server to talk to.
    @param devices: a list of device hostnames.
    """
    hosts = afe.get_hosts(hostname__in = devices)
    if not hosts:
        logging.error('No host found.')
    for h in hosts:
        info = h.hostname + ','
        for key in POWERUNIT_KEYS:
            info += h.attributes.get(key, '') + ','
        print info


def parse_options():
    """Parse options"""
    parser = argparse.ArgumentParser(
            description=__doc__,
            formatter_class=argparse.RawDescriptionHelpFormatter)
    action_help = (
            'upload: read rpm attributes from csv file and set the attributes. '
            'list: list current attributes for a list of hosts. '
            'backup: dump existing rpm attributes to a csv file (for backup).')
    parser.add_argument(
            'action', choices=('upload', 'list', 'backup'), help=action_help)
    parser.add_argument('-f', '--csv_file', type=str, dest='csv_file',
                        help='A path to a csv file. When upload, each line '
                             'should consist of device_name, powerunit_hostname, '
                             'powerunit_outlet, hydra_hostname, separated '
                             'by comma. When dump, the file will be generated.')
    parser.add_argument('-m', type=str, dest='hostnames', default='',
                        help='A list of machine hostnames seperated by comma, '
                             'applicable to "list" command')
    parser.add_argument('-s', '--server', type=str, dest='server',
                        default=DEFAULT_SERVER,
                        help='AFE server that the script will be talking to. '
                             'If not speicified, will default to using the '
                             'server in global_config.ini')
    options = parser.parse_args()
    if options.action == 'upload' or options.action =='backup':
        if not options.csv_file:
            logging.error('Please specifiy a file with -f/--csv')
            sys.exit(1)
        file_exists = os.path.exists(options.csv_file)
        if options.action == 'upload' and not file_exists:
            logging.error('%s is not a valid file.', options.csv_file)
            sys.exit(1)
        if options.action == 'backup' and file_exists:
            logging.error('%s already exists.', options.csv_file)
            sys.exit(1)
    if options.action == 'list' and not options.hostnames:
       logging.error('Please specify hostnames with -m')
       sys.exit(1)
    return options


if __name__ == '__main__':
    logging.basicConfig(level=logging.DEBUG)
    options = parse_options()
    afe = frontend_wrappers.RetryingAFE(timeout_min=5, delay_sec=10,
                                        server=options.server)
    logging.info('Connected to %s', afe.server)
    if options.action =='backup':
        dump_to_csv(afe, options.csv_file)
    elif options.action == 'upload':
        confirm_msg = ('Upload rpm mapping from %s, are you sure?'
                       % options.csv_file)
        confirm = raw_input("%s (y/N) " % confirm_msg).lower() == 'y'
        if confirm:
            add_from_csv(afe, options.csv_file)
    elif options.action == 'list':
        list_powerunit_info(afe, [h.strip() for h in options.hostnames.split(',')])
