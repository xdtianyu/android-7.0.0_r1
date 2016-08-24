# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
import logging
import time

import common

from autotest_lib.client.common_lib import logging_config


class DroneLoggingConfig(logging_config.LoggingConfig):
    """This class sets up logging for the Drone Machines.

    Drone_utility is kicked off on each tick, so this logging config sets up
    the log file to timestamp by day and will create a daily log file.
    """


    @classmethod
    def get_timestamped_log_name(cls, base_name):
        """Generate a log file name based off of Today's Date.

        Normally the other processes in the infrastructure (like the scheduler)
        are kicked off once for long periods of time. However drone_utility is
        kicked off once per tick. Therefore get_timestamped_log_name is
        overloaded so the returned log name just includes the current date.

        @param base_name: String to start the log's filename with.

        @returns String of the base_name suffixed with a timestamp of today's
                 date.
        """
        return '%s.log.%s' % (base_name, time.strftime('%Y-%m-%d'))


    def configure_logging(self, log_dir=None, logfile_name=None):
        """Configure logging for the Drones.

        If log_dir and logfile_name are not provided, it will request a
        timestamped log name with prefix 'drone'. Both the stdout and stderr
        logging handlers are disabled because drone_utility's output is parsed
        by the caller.

        This function is called by client/common_lib/logging_manager.py which
        manages a logging_config. For example if any modules want to adjust
        logging (enabling and/or disabling loggers) after drone_utility has
        started they will do so through the logging_manager.

        @param log_dir: Directory to store the log in. If none will use
                        /usr/local/autotest/logs
        @param logfile_name: Name of the log file. If none it will be in the
                             format of 'drone.log.YEAR-MONTH-DAY'

        """
        # Disable the default stdout/stderr handlers.
        self._clear_all_handlers()
        if log_dir is None:
            log_dir = self.get_server_log_dir()
        if not logfile_name:
            logfile_name = self.get_timestamped_log_name('drone')

        for level in (logging.DEBUG, logging.INFO, logging.WARNING,
                      logging.ERROR, logging.CRITICAL):
            self.add_file_handler(logfile_name, level, log_dir=log_dir)