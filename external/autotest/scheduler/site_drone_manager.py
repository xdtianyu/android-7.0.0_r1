# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
import logging

from autotest_lib.client.common_lib import global_config, error
from autotest_lib.client.common_lib.cros.graphite import autotest_stats
from autotest_lib.scheduler import drones, scheduler_config

HOSTS_JOB_SUBDIR = 'hosts/'
PARSE_LOG = '.parse.log'
ENABLE_ARCHIVING =  global_config.global_config.get_config_value(
        scheduler_config.CONFIG_SECTION, 'enable_archiving', type=bool)


class SiteDroneManager(object):


    _timer = autotest_stats.Timer('drone_manager')


    def copy_to_results_repository(self, process, source_path,
                                   destination_path=None):
        """
        Copy results from the given process at source_path to destination_path
        in the results repository.

        This site subclassed version will only copy the results back for Special
        Agent Tasks (Cleanup, Verify, Repair) that reside in the hosts/
        subdirectory of results if the copy_task_results_back flag has been set
        to True inside global_config.ini

        It will also only copy .parse.log files back to the scheduler if the
        copy_parse_log_back flag in global_config.ini has been set to True.
        """
        if not ENABLE_ARCHIVING:
            return
        copy_task_results_back = global_config.global_config.get_config_value(
                scheduler_config.CONFIG_SECTION, 'copy_task_results_back',
                type=bool)
        copy_parse_log_back = global_config.global_config.get_config_value(
                scheduler_config.CONFIG_SECTION, 'copy_parse_log_back',
                type=bool)
        special_task = source_path.startswith(HOSTS_JOB_SUBDIR)
        parse_log = source_path.endswith(PARSE_LOG)
        if (copy_task_results_back or not special_task) and (
                copy_parse_log_back or not parse_log):
            super(SiteDroneManager, self).copy_to_results_repository(process,
                    source_path, destination_path)


    def kill_process(self, process):
        """
        Kill the given process.
        """
        logging.info('killing %s', process)
        drone = self._get_drone_for_process(process)
        drone.queue_kill_process(process)


    def _add_drone(self, hostname):
        """
        Forked from drone_manager.py

        Catches AutoservRunError if the drone fails initialization and does not
        add it to the list of usable drones.

        @param hostname: Hostname of the drone we are trying to add.
        """
        logging.info('Adding drone %s' % hostname)
        drone = drones.get_drone(hostname)
        if drone:
            try:
                drone.call('initialize', self.absolute_path(''))
            except error.AutoservRunError as e:
                logging.error('Failed to initialize drone %s with error: %s',
                              hostname, e)
                return
            self._drones[drone.hostname] = drone


    @_timer.decorate
    def refresh(self):
       super(SiteDroneManager, self).refresh()


    @_timer.decorate
    def execute_actions(self):
       super(SiteDroneManager, self).execute_actions()
