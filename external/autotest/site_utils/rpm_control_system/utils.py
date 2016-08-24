# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.


"""This file provides util functions used by RPM infrastructure."""


import collections
import csv
import logging
import os
import time

import common

import rpm_infrastructure_exception
from config import rpm_config
from autotest_lib.client.common_lib import enum


MAPPING_FILE = os.path.join(
        os.path.dirname(__file__),
        rpm_config.get('CiscoPOE', 'servo_interface_mapping_file'))


POWERUNIT_HOSTNAME_KEY = 'powerunit_hostname'
POWERUNIT_OUTLET_KEY = 'powerunit_outlet'
HYDRA_HOSTNAME_KEY = 'hydra_hostname'
DEFAULT_EXPIRATION_SECS = 60 * 30

class PowerUnitInfo(object):
    """A class that wraps rpm/poe information of a device."""

    POWERUNIT_TYPES = enum.Enum('POE', 'RPM', string_value=True)

    def __init__(self, device_hostname, powerunit_type,
                 powerunit_hostname, outlet, hydra_hostname=None):
        self.device_hostname = device_hostname
        self.powerunit_type = powerunit_type
        self.powerunit_hostname = powerunit_hostname
        self.outlet = outlet
        self.hydra_hostname = hydra_hostname


    @staticmethod
    def get_powerunit_info(afe_host):
        """Constructe a PowerUnitInfo instance from an afe host.

        @param afe_host: A host object.

        @returns: A PowerUnitInfo object populated with the power management
                  unit information of the host.
        """
        if (not POWERUNIT_HOSTNAME_KEY in afe_host.attributes or
            not POWERUNIT_OUTLET_KEY in afe_host.attributes):
            raise rpm_infrastructure_exception.RPMInfrastructureException(
                    'Can not retrieve complete rpm information'
                    'from AFE for %s, please make sure %s and %s are'
                    ' in the host\'s attributes.' % (afe_host.hostname,
                    POWERUNIT_HOSTNAME_KEY, POWERUNIT_OUTLET_KEY))

        hydra_hostname=(afe_host.attributes[HYDRA_HOSTNAME_KEY]
                        if HYDRA_HOSTNAME_KEY in afe_host.attributes
                        else None)
        return PowerUnitInfo(
                device_hostname=afe_host.hostname,
                powerunit_type=PowerUnitInfo.POWERUNIT_TYPES.RPM,
                powerunit_hostname=afe_host.attributes[POWERUNIT_HOSTNAME_KEY],
                outlet=afe_host.attributes[POWERUNIT_OUTLET_KEY],
                hydra_hostname=hydra_hostname)


class LRUCache(object):
    """A simple implementation of LRU Cache."""


    def __init__(self, size, expiration_secs=DEFAULT_EXPIRATION_SECS):
        """Initialize.

        @param size: Size of the cache.
        @param expiration_secs: The items expire after |expiration_secs|
                                Set to None so that items never expire.
                                Default to DEFAULT_EXPIRATION_SECS.
        """
        self.size = size
        self.cache = collections.OrderedDict()
        self.timestamps = {}
        self.expiration_secs = expiration_secs


    def __getitem__(self, key):
        """Get an item from the cache"""
        # pop and insert the element again so that it
        # is moved to the end.
        value = self.cache.pop(key)
        self.cache[key] = value
        return value


    def __setitem__(self, key, value):
        """Insert an item into the cache."""
        if key in self.cache:
            self.cache.pop(key)
        elif len(self.cache) == self.size:
            removed_key, _ = self.cache.popitem(last=False)
            self.timestamps.pop(removed_key)
        self.cache[key] = value
        self.timestamps[key] = time.time()


    def __contains__(self, key):
        """Check whether a key is in the cache."""
        if (self.expiration_secs is not None and
            key in self.timestamps and
            time.time() - self.timestamps[key] > self.expiration_secs):
            self.cache.pop(key)
            self.timestamps.pop(key)
        return key in self.cache


def load_servo_interface_mapping(mapping_file=MAPPING_FILE):
    """
    Load servo-switch-interface mapping from a CSV file.

    In the file, the first column represents servo hostnames,
    the second column represents switch hostnames, the third column
    represents interface names. Columns are saparated by comma.

    chromeos1-rack3-host12-servo,chromeos1-poe-switch1,fa31
    chromeos1-rack4-host2-servo,chromeos1-poe-switch1,fa32
    ,chromeos1-poe-switch1,fa33
    ...

    A row without a servo hostname indicates that no servo
    has been connected to the corresponding interface.
    This method ignores such rows.

    @param mapping_file: A csv file that stores the mapping.
                         If None, the setting in rpm_config.ini will be used.

    @return a dictionary that maps servo host name to a
              tuple of switch hostname and interface.
              e.g. {
              'chromeos1-rack3-host12-servo': ('chromeos1-poe-switch1', 'fa31')
               ...}

    @raises: rpm_infrastructure_exception.RPMInfrastructureException
             when arg mapping_file is None.
    """
    if not mapping_file:
        raise rpm_infrastructure_exception.RPMInfrastructureException(
                'mapping_file is None.')
    servo_interface = {}
    with open(mapping_file) as csvfile:
        reader = csv.reader(csvfile, delimiter=',')
        for row in reader:
            servo_hostname = row[0].strip()
            switch_hostname = row[1].strip()
            interface = row[2].strip()
            if servo_hostname:
                servo_interface[servo_hostname] = (switch_hostname, interface)
    return servo_interface


def reload_servo_interface_mapping_if_necessary(
        check_point, mapping_file=MAPPING_FILE):
    """Reload the servo-interface mapping file if it is modified.

    This method checks if the last-modified time of |mapping_file| is
    later than |check_point|, if so, it reloads the file.

    @param check_point: A float number representing a time, used to determine
                        whether we need to reload the mapping file.
    @param mapping_file: A csv file that stores the mapping, if none,
                         the setting in rpm_config.ini will be used.

    @return: If the file is reloaded, returns a tuple
             (last_modified_time, servo_interface) where
             the first element is the last_modified_time of the
             mapping file, the second element is a dictionary that
             maps servo hostname to (switch hostname, interface).
             If the file is not reloaded, return None.

    @raises: rpm_infrastructure_exception.RPMInfrastructureException
             when arg mapping_file is None.
    """
    if not mapping_file:
        raise rpm_infrastructure_exception.RPMInfrastructureException(
                'mapping_file is None.')
    last_modified = os.path.getmtime(mapping_file)
    if check_point < last_modified:
        servo_interface = load_servo_interface_mapping(mapping_file)
        logging.info('Servo-interface mapping file %s is reloaded.',
                     mapping_file)
        return (last_modified, servo_interface)
    return None
