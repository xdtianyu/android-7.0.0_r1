#!/usr/bin/python
#
# Copyright 2010 Google Inc. All Rights Reserved.

"""Tool to check the data consistency between master autotest db and replica.

This tool will issue 'show master status;' and 'show slave status;' commands to
two replicated databases to compare its log position.

It will also take a delta command line argument to allow certain time delay
between master and slave. If the delta of two log positions falls into the
defined range, it will be treated as synced.

It will send out an email notification upon any problem if specified an --to
argument.
"""

import getpass
import MySQLdb
import optparse
import os
import socket
import sys

import common
from autotest_lib.client.common_lib import global_config


c = global_config.global_config
_section = 'AUTOTEST_WEB'
DATABASE_HOST = c.get_config_value(_section, "host")
REPLICA_DATABASE_HOST = c.get_config_value(_section, "readonly_host")
DATABASE_NAME = c.get_config_value(_section, "database")
DATABASE_USER = c.get_config_value(_section, "user")
DATABASE_PASSWORD = c.get_config_value(_section, "password")
SYSTEM_USER = 'chromeos-test'


def ParseOptions():
  parser = optparse.OptionParser()
  parser.add_option('-d', '--delta', help='Difference between master and '
                    'replica db', type='int', dest='delta', default=0)
  parser.add_option('--to', help='Comma separated Email notification TO '
                    'recipients.', dest='to', type='string', default='')
  parser.add_option('--cc', help='Comma separated Email notification CC '
                    'recipients.', dest='cc', type='string', default='')
  parser.add_option('-t', '--test-mode', help='skip common group email',
                    dest='testmode', action='store_true', default=False)
  options, _ = parser.parse_args()
  return options


def FetchMasterResult():
  master_conn = MySQLdb.connect(host=DATABASE_HOST,
                                user=DATABASE_USER,
                                passwd=DATABASE_PASSWORD,
                                db=DATABASE_NAME )
  cursor = master_conn.cursor(MySQLdb.cursors.DictCursor)
  cursor.execute ("show master status;")
  master_result = cursor.fetchone()
  master_conn.close()
  return master_result


def FetchSlaveResult():
  replica_conn = MySQLdb.connect(host=REPLICA_DATABASE_HOST,
                                 user=DATABASE_USER,
                                 passwd=DATABASE_PASSWORD,
                                 db=DATABASE_NAME )
  cursor = replica_conn.cursor(MySQLdb.cursors.DictCursor)
  cursor.execute ("show slave status;")
  slave_result = cursor.fetchone()
  replica_conn.close()
  return slave_result


def RunChecks(options, master_result, slave_result):
  master_pos = master_result['Position']
  slave_pos = slave_result['Read_Master_Log_Pos']
  if (master_pos - slave_pos) > options.delta:
    return 'DELTA EXCEEDED: master=%s, slave=%s' % (master_pos, slave_pos)
  if slave_result['Last_SQL_Error'] != '':
    return 'SLAVE Last_SQL_Error'
  if slave_result['Slave_IO_State'] != 'Waiting for master to send event':
    return 'SLAVE Slave_IO_State'
  if slave_result['Last_IO_Error'] != '':
    return 'SLAVE Last_IO_Error'
  if slave_result['Slave_SQL_Running'] != 'Yes':
    return 'SLAVE Slave_SQL_Running'
  if slave_result['Slave_IO_Running'] != 'Yes':
    return 'SLAVE Slave_IO_Running'
  return None


def ShowStatus(options, master_result, slave_result, msg):
  summary = 'Master (%s) and slave (%s) databases are out of sync.' % (
      DATABASE_HOST, REPLICA_DATABASE_HOST) + msg
  if not options.to:
    print summary
    print 'Master status:'
    print str(master_result)
    print 'Slave status:'
    print str(slave_result)
  else:
    email_to = ['%s@google.com' % to.strip() for to in options.to.split(',')]
    email_cc = []
    if options.cc:
      email_cc.extend(
          '%s@google.com' % cc.strip() for cc in options.cc.split(','))
    if getpass.getuser() == SYSTEM_USER and not options.testmode:
      email_cc.append('chromeos-test-cron@google.com')
    body = ('%s\n\n'
            'Master (%s) status:\n%s\n\n'
            'Slave (%s) status:\n%s' % (summary, DATABASE_HOST, master_result,
                                        REPLICA_DATABASE_HOST, slave_result))
    p = os.popen('/usr/sbin/sendmail -t', 'w')
    p.write('To: %s\n' % ','.join(email_to))
    if email_cc:
      p.write('Cc: %s\n' % ','.join(email_cc))

    p.write('Subject: Inconsistency detected in cautotest DB replica on %s.\n'
            % socket.gethostname())
    p.write('Content-Type: text/plain')
    p.write('\n')  # blank line separating headers from body
    p.write(body)
    p.write('\n')
    return_code = p.close()
    if return_code is not None:
      print 'Sendmail exit status %s' % return_code


def main():
  options = ParseOptions()
  master_result = FetchMasterResult()
  slave_result = FetchSlaveResult()
  problem_msg = RunChecks(options, master_result, slave_result)
  if problem_msg:
    ShowStatus(options, master_result, slave_result, problem_msg)
    sys.exit(-1)

if __name__ == '__main__':
  main()
