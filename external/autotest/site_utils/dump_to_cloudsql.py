#!/usr/bin/python
#
# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Module to upload a MySQL dump file to Cloud SQL.

Usage:
  dump_to_cloudsql.py [-h] [--resume NUM] [--user USER] [--passwd PASSWD] FILE
                      [REMOTE]

  Uploads MySQL dump file to a MySQL database or Cloud SQL. With no optional
  arguments will connect to localhost as root with an empty password.

  positional arguments:
    FILE             text dump file containing MySQL commands
    REMOTE           Cloud SQL instance name or MySQL hostname

  optional arguments:
    -h, --help       show this help message and exit
    --resume NUM     resume dump at command NUM
    --user USER      user (ignored for CloudSQL)
    --passwd PASSWD  passwd (ignored for CloudSQL)
"""

from __future__ import division
import argparse
import collections
import datetime
import os
import re
import sys
import time


BYTES_PER_GB = 2**30


class MySQLConnectionManager(object):
    """Manages connections to a MySQL database.

    Vars:
      factory: A *ConnectionFactory.
      connected: Whether we currently hold a live DB connection.
      cmd_num: The number of commands executed.
    """
    def __init__(self, connection_factory):
        self.factory = connection_factory
        self.connected = False
        self.cmd_num = 0

    def write(self, data, execute_cmd=True, increment_cmd=False):
        """Buffers writes to command boundaries.

        Args:
          data: A line of data from the MySQL dump.
          execute_cmd: Whether to execute the command, defaults to True.
          increment_cmd: Whether to increment cmd_num, defaults to False.
          """
        if not data or not data.strip() or data == '\n' or data[:2] == '--':
            return
        self._cmd += data[:-1] if data[-1] == '\n' else data
        if self._cmd[-1] != ';':
            return
        # Execute command.
        if execute_cmd:
            self._cursor.execute(self._cmd.decode('utf-8'))
        self._cmd = ''
        if increment_cmd:
            self.cmd_num += 1

    def disconnect(self):
      """Closes the current database connection."""
      if self.connected:
          self.connected = False
          self._cursor.close()
          self._db.close()

    def connect(self):
      """Creates a new database connection."""
      self.disconnect()
      self._db = self.factory.connect()
      self.connected = True
      self._cursor = self._db.cursor()
      self._cmd = ''


class CloudSQLConnectionFactory(object):
    """Creates Cloud SQL database connections."""
    def __init__(self, cloudsql_instance):
        self._instance = cloudsql_instance

    def connect(self):
        """Connects to the Cloud SQL database and returns the connection.

        Returns:
          A MySQLdb compatible database connection to the Cloud SQL instance.
        """
        print 'Connecting to Cloud SQL instance %s.' % self._instance
        try:
            from google.storage.speckle.python.api import rdbms_googleapi
        except ImportError:
            sys.exit('Unable to import rdbms_googleapi. Add the AppEngine SDK '
                     'directory to your PYTHONPATH. Download the SDK from: '
                     'https://developers.google.com/appengine/downloads')
        return rdbms_googleapi.connect(None, instance=self._instance)


class LocalSQLConnectionFactory(object):
    """Creates local MySQL database connections."""
    def __init__(self, host=None, user='root', passwd=''):
        if not host:
          host = 'localhost'
        self._host = host
        self._user = user
        self._passwd = passwd

    def connect(self):
        """Connects to the local MySQL database and returns the connection.

        Returns:
          A MySQLdb database connection to the local MySQL database.
        """
        print 'Connecting to mysql at localhost as %s.' % self._user
        try:
            import MySQLdb
        except ImportError:
            sys.exit('Unable to import MySQLdb. To install on Ubuntu: '
                     'apt-get install python-mysqldb')
        return MySQLdb.connect(host=self._host, user=self._user,
                               passwd=self._passwd)


class MySQLState(object):
    """Maintains the MySQL global state.

    This is a hack that keeps record of all MySQL lines that set global state.
    These are needed to reconstruct the MySQL state on resume.
    """
    _set_regex = re.compile('\S*\s*SET(.*)[\s=]')

    def __init__(self):
        self._db_line = ''
        self._table_lock = []
        self._sets = collections.OrderedDict()

    def process(self, line):
        """Check and save lines that affect the global state.

        Args:
          line: A line from the MySQL dump file.
        """
        # Most recent USE line.
        if line[:3] == 'USE':
            self._db_line = line
        # SET variables.
        m = self._set_regex.match(line)
        if m:
            self._sets[m.group(1).strip()] = line
        # Maintain LOCK TABLES
        if (line[:11] == 'LOCK TABLES' or
            ('ALTER TABLE' in line and 'DISABLE KEYS' in line)):
            self._table_lock.append(line)
        if (line[:14] == 'UNLOCK TABLES;'):
            self._table_lock = []

    def write(self, out):
        """Print lines to recreate the saved state.

        Args:
          out: A File-like object to write out saved state.
        """
        out.write(self._db_line)
        for v in self._sets.itervalues():
            out.write(v)
        for l in self._table_lock:
            out.write(l)

    def breakpoint(self, line):
      """Returns true if we can handle breaking after this line.

      Args:
        line: A line from the MySQL dump file.

      Returns:
        Boolean indicating whether we can break after |line|.
      """
      return (line[:28] == '-- Table structure for table' or
              line[:11] == 'INSERT INTO')


def dump_to_cloudsql(dumpfile, manager, cmd_offset=0):
    """Dumps a MySQL dump file to a database through a MySQLConnectionManager.

    Args:
      dumpfile: Path to a file from which to read the MySQL dump.
      manager: An instance of MySQLConnectionManager.
      cmd_offset: No commands will be executed on the database before this count
        is reached. Used to continue an uncompleted dump. Defaults to 0.
    """
    state = MySQLState()
    total = os.path.getsize(dumpfile)
    start_time = time.time()
    line_num = 0
    with open(dumpfile, 'r') as dump:
        for line in dump:
            line_num += 1
            if not manager.connected:
                manager.connect()
            try:
                # Construct commands from lines and execute them.
                state.process(line)
                if manager.cmd_num == cmd_offset and cmd_offset != 0:
                    print '\nRecreating state at line: %d' % line_num
                    state.write(manager)
                manager.write(line, manager.cmd_num >= cmd_offset, True)
                # Print status.
                sys.stdout.write(
                    '\rstatus:  %.3f%%     %0.2f GB     %d commands ' %
                    (100 * dump.tell() / total, dump.tell() / BYTES_PER_GB,
                     manager.cmd_num))
                sys.stdout.flush()
            # Handle interrupts and connection failures.
            except KeyboardInterrupt:
                print ('\nInterrupted while executing command: %d' %
                       manager.cmd_num)
                raise
            except:
                print '\nFailed while executing command: %d' % manager.cmd_num
                delta = int(time.time() - start_time)
                print 'Total time: %s' % str(datetime.timedelta(seconds=delta))
                if state.breakpoint(line):
                    # Attempt to resume.
                    print ('Execution can resume from here (line = %d)' %
                           line_num)
                    manager.cmd_num += 1
                    cmd_offset = manager.cmd_num
                    print ('Will now attempt to auto-resume at command: %d' %
                           cmd_offset)
                    manager.disconnect()
                else:
                    print 'Execution may fail to resume correctly from here.'
                    print ('Use --resume=%d to attempt to resume the dump.' %
                           manager.cmd_num)
                    raise
    print '\nDone.'


if __name__ == '__main__':
    """Imports a MySQL database from a dump file.

    Interprets command line arguments and calls dump_to_cloudsql appropriately.
    """
    description = """Uploads MySQL dump file to a MySQL database or Cloud SQL.
                  With no optional arguments will connect to localhost as root
                  with an empty password."""
    parser = argparse.ArgumentParser(description=description)
    parser.add_argument('mysqldump', metavar='FILE',
                        help='text dump file containing MySQL commands')
    parser.add_argument('remote', default=None, nargs='?', metavar='REMOTE',
        help='either a Cloud SQL account:instance or a hostname')
    parser.add_argument('--resume', default=0, type=int, metavar='NUM',
                        help='resume dump at command NUM')
    parser.add_argument('--user', default='root', metavar='USER',
                        help='user (ignored for Cloud SQL)')
    parser.add_argument('--passwd', default='', metavar='PASSWD',
                        help='passwd (ignored for Cloud SQL)')
    args = parser.parse_args()
    if args.remote and ':' in args.remote:
        connection = CloudSQLConnectionFactory(args.remote)
    else:
        connection = LocalSQLConnectionFactory(args.remote, args.user,
                                               args.passwd)
    if args.resume:
        print 'Resuming execution at command: %d' % options.resume
    dump_to_cloudsql(args.mysqldump, MySQLConnectionManager(connection),
                     args.resume)
