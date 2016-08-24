#!/usr/bin/python

# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Module used to back up the mysql db and upload to Google Storage.

Usage:
  backup_mysql_db.py --type=weekly --gs_bucket=gs://my_bucket --keep 10

  gs_bucket may refer to a local location by omitting gs:// and giving a local
  path if desired for testing. The example usage above creates a dump
  of the autotest db, uploads it to gs://my_bucket/weekly/dump_file.date and
  cleans up older dumps if there are more than 10 in that directory.
"""

import datetime
from distutils import version
import logging
import optparse
import os
import tempfile

import common

from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib import global_config, logging_manager, utils
from autotest_lib.utils import test_importer


_ATTEMPTS = 3
_GSUTIL_BIN = 'gsutil'
_GS_BUCKET = 'gs://chromeos-lab/backup/database'
# TODO(scottz): Should we need to ignore more than one database a general
# function should be designed that lists tables in the database and properly
# creates the --ignore-table= args to be passed to mysqldump.
# Tables to ignore when dumping all databases.
# performance_schema is an internal database that cannot be dumped
IGNORE_TABLES = ['performance_schema.cond_instances',
                 'performance_schema.events_waits_current',
                 'performance_schema.cond_instances',
                 'performance_schema.events_waits_history',
                 'performance_schema.events_waits_history_long',
                 'performance_schema.events_waits_summary_by_instance',
                 ('performance_schema.'
                  'events_waits_summary_by_thread_by_event_name'),
                 'performance_schema.events_waits_summary_global_by_event_name',
                 'performance_schema.file_instances',
                 'performance_schema.file_summary_by_event_name',
                 'performance_schema.file_summary_by_instance',
                 'performance_schema.mutex_instances',
                 'performance_schema.performance_timers',
                 'performance_schema.rwlock_instances',
                 'performance_schema.setup_consumers',
                 'performance_schema.setup_instruments',
                 'performance_schema.setup_timers',
                 'performance_schema.threads']

# Conventional mysqldump schedules.
_DAILY = 'daily'
_WEEKLY = 'weekly'
_MONTHLY = 'monthly'

# Back up server db
_SERVER_DB = 'server_db'

# Contrary to a conventional mysql dump which takes O(hours) on large databases,
# a host dump is the cheapest form of backup possible. We dump the output of a
# of a mysql command showing all hosts and their pool labels to a text file that
# is backed up to google storage.
_ONLY_HOSTS = 'only_hosts'
_ONLY_SHARDS = 'only_shards'
_SCHEDULER_TYPES = [_SERVER_DB, _ONLY_HOSTS, _ONLY_SHARDS, _DAILY, _WEEKLY, _MONTHLY]

class BackupError(Exception):
  """Raised for error occurred during backup."""


class MySqlArchiver(object):
    """Class that archives the Autotest MySQL DB to Google Storage.

    Vars:
      gs_dir:  The path to the directory in Google Storage that this dump file
               will be uploaded to.
      number_to_keep:  The number of dumps we should store.
    """
    _AUTOTEST_DB = "chromeos_autotest_db"
    _SERVER_DB = "chromeos_lab_servers"


    def __init__(self, scheduled_type, number_to_keep, gs_bucket):
        # For conventional scheduled type, we back up all databases.
        # self._db is only used when scheduled_type is not
        # conventional scheduled type.
        self._db = self._get_db_name(scheduled_type)
        self._gs_dir = '/'.join([gs_bucket, scheduled_type])
        self._number_to_keep = number_to_keep
        self._type = scheduled_type


    @classmethod
    def _get_db_name(cls, scheduled_type):
        """Get the db name to backup.

        @param scheduled_type: one of _SCHEDULER_TYPES.

        @returns: The name of the db to backup.
                  Or None for backup all dbs.
        """
        if scheduled_type == _SERVER_DB:
            return cls._SERVER_DB
        elif scheduled_type in [_ONLY_HOSTS, _ONLY_SHARDS]:
            return cls._AUTOTEST_DB
        else:
            return None

    @staticmethod
    def _get_user_pass():
        """Returns a tuple containing the user/pass to use to access the DB."""
        user = global_config.global_config.get_config_value(
                'CROS', 'db_backup_user')
        password = global_config.global_config.get_config_value(
                'CROS', 'db_backup_password')
        return user, password


    def create_mysql_dump(self):
        """Returns the path to a mysql dump of the current autotest DB."""
        user, password = self._get_user_pass()
        _, filename = tempfile.mkstemp('autotest_db_dump')
        logging.debug('Dumping mysql database to file %s', filename)
        extra_dump_args = ''
        for entry in IGNORE_TABLES:
            extra_dump_args += '--ignore-table=%s ' % entry

        if not self._db:
            extra_dump_args += "--all-databases"
        db_name = self._db or ''
        utils.system('set -o pipefail; mysqldump --user=%s '
                     '--password=%s %s %s| gzip - > %s' % (
                     user, password, extra_dump_args, db_name, filename))
        return filename


    def _create_dump_from_query(self, query):
        """Dumps result of a query into a text file.

        @param query: Query to execute.

        @return: The path to a tempfile containing the response of the query.
        """
        if not self._db:
            raise BackupError("_create_dump_from_query requires a specific db.")
        parameters = {'db': self._db, 'query': query}
        parameters['user'], parameters['password'] = self._get_user_pass()
        _, parameters['filename'] = tempfile.mkstemp('autotest_db_dump')
        utils.system(
                'set -o pipefail; mysql -u %(user)s -p%(password)s '
                '%(db)s -e "%(query)s" > %(filename)s' %
                parameters)
        return parameters['filename']


    def create_host_dump(self):
        """Dumps hosts and their labels into a text file.

        @return: The path to a tempfile containing a dump of
                 hosts and their pool labels.
        """
        query = ('SELECT hostname, labels.name FROM afe_hosts AS hosts '
                 'JOIN afe_hosts_labels ON hosts.id = afe_hosts_labels.host_id '
                 'JOIN afe_labels AS labels '
                 'ON labels.id = afe_hosts_labels.label_id '
                 'WHERE labels.name LIKE \'%%pool%%\';')
        return self._create_dump_from_query(query)


    def create_shards_dump(self):
        """Dumps shards and their labels into a text file.

        @return: The path to a tempfile containing a dump of
                 shards and their labels.
        """
        query = ('SELECT hostname, labels.name FROM afe_shards AS shards '
                 'JOIN afe_shards_labels '
                 'ON shards.id = afe_shards_labels.shard_id '
                 'JOIN afe_labels AS labels '
                 'ON labels.id = afe_shards_labels.label_id;')
        return self._create_dump_from_query(query)


    def dump(self):
        """Creates a data dump based on the type of schedule.

        @return: The path to a file containing the dump.
        """
        if self._type == _ONLY_HOSTS:
            return self.create_host_dump()
        if self._type == _ONLY_SHARDS:
            return self.create_shards_dump()
        return self.create_mysql_dump()


    def _get_name(self):
        """Returns the name of the dump as presented to google storage."""
        if self._type in [_ONLY_HOSTS, _ONLY_SHARDS]:
            file_type = 'txt'
        else:
            file_type = 'gz'
        return 'autotest-dump.%s.%s' % (
                datetime.datetime.now().strftime('%y.%m.%d'), file_type)


    @staticmethod
    def _retry_run(cmd):
        """Run the specified |cmd| string, retrying if necessary.

        Args:
          cmd: The command to run.
        """
        for attempt in range(_ATTEMPTS):
            try:
                return utils.system_output(cmd)
            except error.CmdError:
                if attempt == _ATTEMPTS - 1:
                    raise
                else:
                    logging.error('Failed to run %r', cmd)


    def upload_to_google_storage(self, dump_file):
        """Uploads the given |dump_file| to Google Storage.

        @param dump_file: The path to the file containing the dump.
        """
        cmd = '%(gs_util)s cp %(dump_file)s %(gs_dir)s/%(name)s'
        input_dict = dict(gs_util=_GSUTIL_BIN, dump_file=dump_file,
                          name=self._get_name(), gs_dir=self._gs_dir)
        cmd = cmd % input_dict
        logging.debug('Uploading mysql dump to google storage')
        self._retry_run(cmd)
        os.remove(dump_file)


    def _get_gs_command(self, cmd):
        """Returns an array representing the command for rm or ls."""
        # Helpful code to allow us to test without gs.
        assert cmd in ['rm', 'ls']
        gs_bin = _GSUTIL_BIN
        if self._gs_dir.startswith('gs://'):
            cmd_array = [gs_bin, cmd]
        else:
            cmd_array = [cmd]

        return cmd_array


    def _do_ls(self):
        """Returns the output of running ls on the gs bucket."""
        cmd = self._get_gs_command('ls') + [self._gs_dir]
        return self._retry_run(' '.join(cmd))


    def cleanup(self):
        """Cleans up the gs bucket to ensure we don't over archive."""
        logging.debug('Cleaning up previously archived dump files.')
        listing = self._do_ls()
        ordered_listing = sorted(listing.splitlines(), key=version.LooseVersion)
        if len(ordered_listing) < self._number_to_keep:
            logging.debug('Cleanup found nothing to do.')
            return

        to_remove = ordered_listing[:-self._number_to_keep]
        rm_cmd = self._get_gs_command('rm')
        for artifact in to_remove:
            cmd = ' '.join(rm_cmd + [self._gs_dir + '/' + artifact])
            self._retry_run(cmd)


def parse_options():
    """Parses given options."""
    parser = optparse.OptionParser()
    parser.add_option('--gs_bucket', default=_GS_BUCKET,
                      help='Google storage bucket to store mysql db dumps.')
    parser.add_option('--keep', default=10, type=int,
                      help='Number of dumps to keep of specified type.')
    parser.add_option('--type', default=_DAILY,
                      help='The type of mysql dump to store.')
    parser.add_option('--verbose', default=False, action='store_true',
                      help='Google storage bucket to store mysql db dumps.')
    options = parser.parse_args()[0]
    if options.type not in _SCHEDULER_TYPES:
        parser.error('Type must be either: %s.' % ', '.join(_SCHEDULER_TYPES))

    return options


def main():
    """Runs the program."""
    options = parse_options()
    logging_manager.configure_logging(test_importer.TestImporterLoggingConfig(),
                                      verbose=options.verbose)
    logging.debug('Start db backup: %s', options.type)
    archiver = MySqlArchiver(options.type, options.keep, options.gs_bucket)
    dump_file = archiver.dump()
    logging.debug('Uploading backup: %s', options.type)
    archiver.upload_to_google_storage(dump_file)
    archiver.cleanup()
    logging.debug('Db backup completed: %s', options.type)


if __name__ == '__main__':
    main()
