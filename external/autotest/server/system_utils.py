# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""This module provides utility functions to get the servers used by Autotest
system, from server database or global config. The standalone module is needed
to avoid circular imports.

"""

import copy

import common
from autotest_lib.client.common_lib import global_config
from autotest_lib.client.common_lib import utils
from autotest_lib.scheduler import scheduler_config
from autotest_lib.site_utils import server_manager_utils


def get_drones():
    """Get a list of drones from server database or global config.
    """
    if server_manager_utils.use_server_db():
        return server_manager_utils.get_drones()
    else:
        drones = global_config.global_config.get_config_value(
                scheduler_config.CONFIG_SECTION, 'drones', default='localhost')
        return [hostname.strip() for hostname in drones.split(',')]


def get_shards():
    """Get a list of shards from server database or global config.
    """
    if server_manager_utils.use_server_db():
        return server_manager_utils.get_shards()
    else:
        config = global_config.global_config
        shards = config.get_config_value(
                'SERVER', 'shards', default='')
        return [hostname.strip() for hostname in shards.split(',')]


class DroneCache(object):
    """A cache object to store drone list related data.

    The cache is added to avoid repeated calls from each agent task. The cache
    should be refreshed every tick.
    """

    # A cache of unrestricted drones.
    unrestricted_drones = None

    # A cache of a dict of (drone, ip).
    drone_ip_map = None

    @classmethod
    def refresh(cls, restricted_subnets=utils.RESTRICTED_SUBNETS):
        """Refresh the cache.

        @param restricted_subnets: A list of restricted subnet, default is set
                to restricted_subnets in global config.
        """
        new_drone_ip_map = {}
        new_unrestricted_drones = []
        for drone in get_drones():
            new_drone_ip_map[drone] = utils.get_ip_address(drone)
            if (not restricted_subnets or
                not utils.get_restricted_subnet(new_drone_ip_map[drone],
                                                restricted_subnets)):
                new_unrestricted_drones.append(drone)
        cls.drone_ip_map = new_drone_ip_map
        cls.unrestricted_drones = new_unrestricted_drones


    @classmethod
    def get_unrestricted_drones(
                cls, restricted_subnets=utils.RESTRICTED_SUBNETS):
        """Get a list of cached unrestricted drones.

        @param restricted_subnets: A list of restricted subnet, default is set
                to restricted_subnets in global config.
        """
        if not cls.unrestricted_drones:
            cls.refresh(restricted_subnets)

        return copy.copy(cls.unrestricted_drones)


    @classmethod
    def get_drone_ip_map(cls, restricted_subnets=utils.RESTRICTED_SUBNETS):
        """Get a dict of (drone, ip).

        @param restricted_subnets: A list of restricted subnet, default is set
                to restricted_subnets in global config.
        """
        if not cls.drone_ip_map:
            cls.refresh(restricted_subnets)

        return copy.copy(cls.drone_ip_map)
