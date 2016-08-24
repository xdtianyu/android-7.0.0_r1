#pylint: disable-msg=C0111

# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Scheduler helper libraries.
"""
import logging
import os

import common

from autotest_lib.client.common_lib import global_config
from autotest_lib.client.common_lib import logging_config
from autotest_lib.client.common_lib import logging_manager
from autotest_lib.client.common_lib import utils
from autotest_lib.database import database_connection
from autotest_lib.frontend import setup_django_environment
from autotest_lib.frontend.afe import readonly_connection
from autotest_lib.server import utils as server_utils


DB_CONFIG_SECTION = 'AUTOTEST_WEB'

# Translations necessary for scheduler queries to work with SQLite.
# Though this is only used for testing it is included in this module to avoid
# circular imports.
_re_translator = database_connection.TranslatingDatabase.make_regexp_translator
_DB_TRANSLATORS = (
        _re_translator(r'NOW\(\)', 'time("now")'),
        _re_translator(r'LAST_INSERT_ID\(\)', 'LAST_INSERT_ROWID()'),
        # older SQLite doesn't support group_concat, so just don't bother until
        # it arises in an important query
        _re_translator(r'GROUP_CONCAT\((.*?)\)', r'\1'),
        _re_translator(r'TRUNCATE TABLE', 'DELETE FROM'),
        _re_translator(r'ISNULL\(([a-z,_]+)\)',
                       r'ifnull(nullif(\1, NULL), \1) DESC'),
)


class SchedulerError(Exception):
    """Raised by the scheduler when an inconsistent state occurs."""


class ConnectionManager(object):
    """Manager for the django database connections.

    The connection is used through scheduler_models and monitor_db.
    """
    __metaclass__ = server_utils.Singleton

    def __init__(self, readonly=True, autocommit=True):
        """Set global django database options for correct connection handling.

        @param readonly: Globally disable readonly connections.
        @param autocommit: Initialize django autocommit options.
        """
        self.db_connection = None
        # bypass the readonly connection
        readonly_connection.set_globally_disabled(readonly)
        if autocommit:
            # ensure Django connection is in autocommit
            setup_django_environment.enable_autocommit()


    @classmethod
    def open_connection(cls):
        """Open a new database connection.

        @return: An instance of the newly opened connection.
        """
        db = database_connection.DatabaseConnection(DB_CONFIG_SECTION)
        db.connect(db_type='django')
        return db


    def get_connection(self):
        """Get a connection.

        @return: A database connection.
        """
        if self.db_connection is None:
            self.db_connection = self.open_connection()
        return self.db_connection


    def disconnect(self):
        """Close the database connection."""
        try:
            self.db_connection.disconnect()
        except Exception as e:
            logging.debug('Could not close the db connection. %s', e)


    def __del__(self):
        self.disconnect()


class SchedulerLoggingConfig(logging_config.LoggingConfig):
    """Configure timestamped logging for a scheduler."""
    GLOBAL_LEVEL = logging.INFO

    @classmethod
    def get_log_name(cls, timestamped_logfile_prefix):
        """Get the name of a logfile.

        @param timestamped_logfile_prefix: The prefix to apply to the
            a timestamped log. Eg: 'scheduler' will create a logfile named
            scheduler.log.2014-05-12-17.24.02.

        @return: The timestamped log name.
        """
        return cls.get_timestamped_log_name(timestamped_logfile_prefix)


    def configure_logging(self, log_dir=None, logfile_name=None,
                          timestamped_logfile_prefix='scheduler'):
        """Configure logging to a specified logfile.

        @param log_dir: The directory to log into.
        @param logfile_name: The name of the log file.
        @timestamped_logfile_prefix: The prefix to apply to the name of
            the logfile, if a log file name isn't specified.
        """
        super(SchedulerLoggingConfig, self).configure_logging(use_console=True)

        if log_dir is None:
            log_dir = self.get_server_log_dir()
        if not logfile_name:
            logfile_name = self.get_log_name(timestamped_logfile_prefix)

        self.add_file_handler(logfile_name, logging.DEBUG, log_dir=log_dir)
        symlink_path = os.path.join(
                log_dir, '%s.latest' % timestamped_logfile_prefix)
        try:
            os.unlink(symlink_path)
        except OSError:
            pass
        os.symlink(os.path.join(log_dir, logfile_name), symlink_path)


def setup_logging(log_dir, log_name, timestamped_logfile_prefix='scheduler'):
    """Setup logging to a given log directory and log file.

    @param log_dir: The directory to log into.
    @param log_name: Name of the log file.
    @param timestamped_logfile_prefix: The prefix to apply to the logfile.
    """
    logging_manager.configure_logging(
            SchedulerLoggingConfig(), log_dir=log_dir, logfile_name=log_name,
            timestamped_logfile_prefix=timestamped_logfile_prefix)


def check_production_settings(scheduler_options):
    """Check the scheduler option's production settings.

    @param scheduler_options: Settings for scheduler.

    @raises SchedulerError: If a loclhost scheduler is started with
       production settings.
    """
    db_server = global_config.global_config.get_config_value('AUTOTEST_WEB',
                                                             'host')
    if (not scheduler_options.production and
        not utils.is_localhost(db_server)):
        raise SchedulerError('Scheduler is not running in production mode, you '
                             'should not set database to hosts other than '
                             'localhost. It\'s currently set to %s.\nAdd option'
                             ' --production if you want to skip this check.' %
                             db_server)
