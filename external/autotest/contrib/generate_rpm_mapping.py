#! /usr/bin/python

# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""
This script generates a csv file containing the mapping of
(device_hostname, rpm_hostname, outlet, hydra_hostname) for each
host in our lab. The csv file is in the following format.

chromeos-rack2-host1,chromeos-rack2-rpm1,.A1,chromeos-197-hydra1.mtv
chromeos-rack2-host2,chromeos-rack2-rpm1,.A2,chromeos-197-hydra1.mtv
...

The generated csv file can be used as input to add_host_powerunit_info.py

Workflow:
    <Generate the csv file>
    python generate_rpm_mapping.py --csv mapping_file.csv --server cautotest

    <Upload mapping information in csv file to AFE>
    python add_host_powerunit_info.py --csv mapping_file.csv

"""
import argparse
import collections
import logging
import re
import sys

import common

from autotest_lib.client.common_lib import enum
from autotest_lib.server.cros.dynamic_suite import frontend_wrappers

CHROMEOS_LABS = enum.Enum('OysterBay', 'Atlantis', 'Chaos', 'Destiny', start_value=1)
HOST_REGX = 'chromeos(\d+)(-row(\d+))*-rack(\d+)-host(\d+)'
DeviceHostname = collections.namedtuple(
        'DeviceHostname', ['lab', 'row', 'rack', 'host'])


class BaseLabConfig(object):
    """Base class for a lab configuration."""
    RPM_OUTLET_MAP = {}
    LAB_NUMBER = -1

    @classmethod
    def get_rpm_hostname(cls, device_hostname):
        """Get rpm hostname given a device.

        @param device_hostname: A DeviceHostname named tuple.

        @returns: the rpm hostname, default to empty string.

        """
        return ''


    @classmethod
    def get_rpm_outlet(cls, device_hostname):
        """Get rpm outlet given a device.

        @param device_hostname: A DeviceHostname named tuple.

        @returns: the rpm outlet, default to empty string.

        """
        return ''


    @classmethod
    def get_hydra_hostname(cls, device_hostname):
        """Get hydra hostname given a device.

        @param device_hostname: A DeviceHostname named tuple.

        @returns: the hydra hostname, default to empty string.

        """
        return ''


    @classmethod
    def is_device_in_the_lab(cls, device_hostname):
        """Check whether a dut belongs to the lab.

        @param device_hostname: A DeviceHostname named tuple.

        @returns: True if the dut belongs to the lab,
                  False otherwise.

        """
        return device_hostname.lab == cls.LAB_NUMBER


class OysterBayConfig(BaseLabConfig):
    """Configuration for OysterBay"""

    LAB_NUMBER = CHROMEOS_LABS.OYSTERBAY


    @classmethod
    def get_rpm_hostname(cls, device_hostname):
        """Get rpm hostname.

        @param device_hostname: A DeviceHostname named tuple.

        @returns: hostname of the rpm that has the device.

        """
        if not device_hostname.row:
            return ''
        return 'chromeos%d-row%d-rack%d-rpm1' % (
                device_hostname.lab, device_hostname.row,
                device_hostname.rack)


    @classmethod
    def get_rpm_outlet(cls, device_hostname):
        """Get rpm outlet.

        @param device_hostname: A DeviceHostname named tuple.

        @returns: rpm outlet, e.g. '.A1'

        """
        if not device_hostname.row:
            return ''
        return '.A%d' % device_hostname.host


class AtlantisConfig(BaseLabConfig):
    """Configuration for Atlantis lab."""

    LAB_NUMBER = CHROMEOS_LABS.ATLANTIS
    # chromeos2, hostX -> outlet
    RPM_OUTLET_MAP = {
            1: 1,
            7: 2,
            2: 4,
            8: 5,
            3: 7,
            9: 8,
            4: 9,
            10: 10,
            5: 12,
            11: 13,
            6: 15,
            12: 16}

    @classmethod
    def get_rpm_hostname(cls, device_hostname):
        """Get rpm hostname.

        @param device_hostname: A DeviceHostname named tuple.

        @returns: hostname of the rpm that has the device.

        """
        return 'chromeos%d-row%d-rack%d-rpm1' % (
                device_hostname.lab, device_hostname.row,
                device_hostname.rack)


    @classmethod
    def get_rpm_outlet(cls, device_hostname):
        """Get rpm outlet.

        @param device_hostname: A DeviceHostname named tuple.

        @returns: rpm outlet, e.g. '.A1'

        """
        return '.A%d' % cls.RPM_OUTLET_MAP[device_hostname.host]


    @classmethod
    def get_hydra_hostname(cls, device_hostname):
        """Get hydra hostname.

        @param device_hostname: A DeviceHostname named tuple.

        @returns: hydra hostname

        """
        row = device_hostname.row
        rack = device_hostname.rack
        if row >= 1 and row <= 5 and rack >= 1 and rack <= 7:
            return 'chromeos-197-hydra1.cros'
        elif row >= 1 and row <= 5 and rack >= 8 and rack <= 11:
            return 'chromeos-197-hydra2.cros'
        else:
            logging.error('Could not determine hydra for %s',
                          device_hostname)
            return ''


class ChaosConfig(BaseLabConfig):
    """Configuration for Chaos lab."""

    LAB_NUMBER = CHROMEOS_LABS.CHAOS


    @classmethod
    def get_rpm_hostname(cls, device_hostname):
        """Get rpm hostname.

        @param device_hostname: A DeviceHostname named tuple.

        @returns: hostname of the rpm that has the device.

        """
        return 'chromeos%d-row%d-rack%d-rpm1' % (
                device_hostname.lab, device_hostname.row,
                device_hostname.rack)


    @classmethod
    def get_rpm_outlet(cls, device_hostname):
        """Get rpm outlet.

        @param device_hostname: A DeviceHostname named tuple.

        @returns: rpm outlet, e.g. '.A1'

        """
        return '.A%d' % device_hostname.host


class DestinyConfig(BaseLabConfig):
    """Configuration for Desitny lab."""

    LAB_NUMBER = CHROMEOS_LABS.DESTINY
    # None-densified rack: one host per shelf
    # (rowX % 2, hostY) -> outlet
    RPM_OUTLET_MAP = {
            (1, 1): 1,
            (0, 1): 2,
            (1, 2): 4,
            (0, 2): 5,
            (1, 3): 7,
            (0, 3): 8,
            (1, 4): 9,
            (0, 4): 10,
            (1, 5): 12,
            (0, 5): 13,
            (1, 6): 15,
            (0, 6): 16,
    }

    # Densified rack: one shelf can have two chromeboxes or one notebook.
    # (rowX % 2, hostY) -> outlet
    DENSIFIED_RPM_OUTLET_MAP = {
            (1, 2):  1,  (1, 1): 1,
            (0, 1):  2,  (0, 2): 2,
            (1, 4):  3,  (1, 3): 3,
            (0, 3):  4,  (0, 4): 4,
            (1, 6):  5,  (1, 5): 5,
            (0, 5):  6,  (0, 6): 6,
            (1, 8):  7,  (1, 7): 7,
            (0, 7):  8,  (0, 8): 8,
            # outlet 9, 10 are not used
            (1, 10): 11, (1, 9): 11,
            (0, 9):  12, (0, 10): 12,
            (1, 12): 13, (1, 11): 13,
            (0, 11): 14, (0, 12): 14,
            (1, 14): 15, (1, 13): 15,
            (0, 13): 16, (0, 14): 16,
            (1, 16): 17, (1, 15): 17,
            (0, 15): 18, (0, 16): 18,
            (1, 18): 19, (1, 17): 19,
            (0, 17): 20, (0, 18): 20,
            (1, 20): 21, (1, 19): 21,
            (0, 19): 22, (0, 20): 22,
            (1, 22): 23, (1, 21): 23,
            (0, 21): 24, (0, 22): 24,
    }


    @classmethod
    def is_densified(cls, device_hostname):
        """Whether the host is on a densified rack.

        @param device_hostname: A DeviceHostname named tuple.

        @returns: True if on a densified rack, False otherwise.
        """
        return device_hostname.rack in (0, 12, 13)


    @classmethod
    def get_rpm_hostname(cls, device_hostname):
        """Get rpm hostname.

        @param device_hostname: A DeviceHostname named tuple.

        @returns: hostname of the rpm that has the device.

        """
        row = device_hostname.row
        if row == 13:
            logging.warn('Rule not implemented for row 13 in chromeos4')
            return ''

        # rpm row is like chromeos4-row1_2-rackX-rpmY
        rpm_row = ('%d_%d' % (row - 1, row) if row % 2 == 0 else
                   '%d_%d' % (row, row + 1))

        if cls.is_densified(device_hostname):
            # Densified rack has two rpms, decide which one the host belongs to
            # Rule:
            #     odd row number,  even host number -> rpm1
            #     odd row number,  odd host number  -> rpm2
            #     even row number, odd host number  -> rpm1
            #     even row number, even host number -> rpm2
            rpm_number = 1 if (row + device_hostname.host) % 2 == 1 else 2
        else:
            # Non-densified rack only has one rpm
            rpm_number = 1
        return 'chromeos%d-row%s-rack%d-rpm%d' % (
                device_hostname.lab,
                rpm_row, device_hostname.rack, rpm_number)


    @classmethod
    def get_rpm_outlet(cls, device_hostname):
        """Get rpm outlet.

        @param device_hostname: A DeviceHostname named tuple.

        @returns: rpm outlet, e.g. '.A1'

        """
        try:
            outlet_map = (cls.DENSIFIED_RPM_OUTLET_MAP
                          if cls.is_densified(device_hostname) else
                          cls.RPM_OUTLET_MAP)
            outlet_number = outlet_map[(device_hostname.row % 2,
                                        device_hostname.host)]
            return '.A%d' % outlet_number
        except KeyError:
            logging.error('Could not determine outlet for device %s',
                          device_hostname)
            return ''


    @classmethod
    def get_hydra_hostname(cls, device_hostname):
        """Get hydra hostname.

        @param device_hostname: A DeviceHostname named tuple.

        @returns: hydra hostname

        """
        row = device_hostname.row
        rack = device_hostname.rack
        if row >= 1 and row <= 6 and rack >=1 and rack <= 11:
            return 'chromeos-destiny-hydra1.cros'
        elif row >= 7 and row <= 12 and rack >=1 and rack <= 11:
            return 'chromeos-destiny-hydra2.cros'
        elif row >= 1 and row <= 10 and rack >=12 and rack <= 13:
            return 'chromeos-destiny-hydra3.cros'
        elif row in [3, 4, 5, 6, 9, 10] and rack == 0:
            return 'chromeos-destiny-hydra3.cros'
        elif row == 13 and rack >= 0 and rack <= 11:
            return 'chromeos-destiny-hydra3.cros'
        else:
            logging.error('Could not determine hydra hostname for %s',
                          device_hostname)
            return ''


def parse_device_hostname(device_hostname):
    """Parse device_hostname to DeviceHostname object.

    @param device_hostname: A string, e.g. 'chromeos2-row2-rack4-host3'

    @returns: A DeviceHostname named tuple or None if the
              the hostname doesn't follow the pattern
              defined in HOST_REGX.

    """
    m = re.match(HOST_REGX, device_hostname.strip())
    if m:
        return DeviceHostname(
                lab=int(m.group(1)),
                row=int(m.group(3)) if m.group(3) else None,
                rack=int(m.group(4)),
                host=int(m.group(5)))
    else:
        logging.error('Could not parse %s', device_hostname)
        return None


def generate_mapping(hosts, lab_configs):
    """Generate device_hostname-rpm-outlet-hydra mapping.

    @param hosts: hosts objects get from AFE.
    @param lab_configs: A list of configuration classes,
                        each one for a lab.

    @returns: A dictionary that maps device_hostname to
              (rpm_hostname, outlet, hydra_hostname)

    """
    # device hostname -> (rpm_hostname, outlet, hydra_hostname)
    rpm_mapping = {}
    for host in hosts:
        device_hostname = parse_device_hostname(host.hostname)
        if not device_hostname:
            continue
        for lab in lab_configs:
            if lab.is_device_in_the_lab(device_hostname):
                rpm_hostname = lab.get_rpm_hostname(device_hostname)
                rpm_outlet = lab.get_rpm_outlet(device_hostname)
                hydra_hostname = lab.get_hydra_hostname(device_hostname)
                if not rpm_hostname or not rpm_outlet:
                    logging.error(
                            'Skipping device %s: could not determine '
                            'rpm hostname or outlet.', host.hostname)
                    break
                rpm_mapping[host.hostname] = (
                        rpm_hostname, rpm_outlet, hydra_hostname)
                break
        else:
            logging.info(
                    '%s is not in a know lab '
                    '(oyster bay, atlantis, chaos, destiny)',
                    host.hostname)
    return rpm_mapping


def output_csv(rpm_mapping, csv_file):
    """Dump the rpm mapping dictionary to csv file.

    @param rpm_mapping: A dictionary that maps device_hostname to
                        (rpm_hostname, outlet, hydra_hostname)
    @param csv_file: The name of the file to write to.

    """
    with open(csv_file, 'w') as f:
        for hostname, rpm_info in rpm_mapping.iteritems():
            line = ','.join(rpm_info)
            line = ','.join([hostname, line])
            f.write(line + '\n')


if __name__ == '__main__':
    logging.basicConfig(level=logging.DEBUG)
    parser = argparse.ArgumentParser(
            description='Generate device_hostname-rpm-outlet-hydra mapping '
                        'file needed by add_host_powerunit_info.py')
    parser.add_argument('--csv', type=str, dest='csv_file', required=True,
                        help='The path to the csv file where we are going to '
                             'write the mapping information to.')
    parser.add_argument('--server', type=str, dest='server', default=None,
                        help='AFE server that the script will be talking to. '
                             'If not specified, will default to using the '
                             'server in global_config.ini')
    options = parser.parse_args()

    AFE = frontend_wrappers.RetryingAFE(timeout_min=5, delay_sec=10,
                                        server=options.server)
    logging.info('Connected to %s', AFE.server)
    rpm_mapping = generate_mapping(
            AFE.get_hosts(),
            [OysterBayConfig, AtlantisConfig, ChaosConfig, DestinyConfig])
    output_csv(rpm_mapping, options.csv_file)
