#!/usr/bin/python
# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import argparse
import datetime
import os
import re
import sys
import logging

os.environ['DJANGO_SETTINGS_MODULE'] = 'frontend.settings'

import common
from django.db import connections, transaction


# Format Appears as: [Date] [Time] - [Msg Level] - [Message]
LOGGING_FORMAT = '%(asctime)s - %(levelname)s - %(message)s'
# This regex makes sure the input is in the format of YYYY-MM-DD (2012-02-01)
DATE_FORMAT_REGEX = ('^(19|20)\d\d[- /.](0[1-9]|1[012])[- /.](0[1-9]|[12][0-9]'
                     '|3[01])$')
SELECT_CMD_FORMAT = """
SELECT %(table)s.%(primary_key)s FROM %(table)s
WHERE %(table)s.%(time_column)s <= "%(date)s"
"""
SELECT_JOIN_CMD_FORMAT = """
SELECT %(table)s.%(primary_key)s FROM %(table)s
INNER JOIN %(related_table)s
  ON %(table)s.%(foreign_key)s=%(related_table)s.%(related_primary_key)s
WHERE %(related_table)s.%(time_column)s <= "%(date)s"
"""
SELECT_WITH_INDIRECTION_FORMAT = """
SELECT %(table)s.%(primary_key)s FROM %(table)s
INNER JOIN %(indirection_table)s
  ON %(table)s.%(foreign_key)s =
     %(indirection_table)s.%(indirection_primary_key)s
INNER JOIN %(related_table)s
  ON %(indirection_table)s.%(indirection_foreign_key)s =
  %(related_table)s.%(related_primary_key)s
WHERE %(related_table)s.%(time_column)s <= "%(date)s"
"""
DELETE_ROWS_FORMAT = """
DELETE FROM %(table)s
WHERE %(table)s.%(primary_key)s IN (%(rows)s)
"""


AFE_JOB_ID = 'afe_job_id'
JOB_ID = 'job_id'
JOB_IDX = 'job_idx'
TEST_IDX = 'test_idx'

# CAUTION: Make sure only the 'default' connection is used. Otherwise
# db_cleanup may delete stuff from the global database, which is generally not
# intended.
cursor = connections['default'].cursor()

STEP_SIZE = None  # Threading this through properly is disgusting.

class ProgressBar(object):
    TEXT = "{:<40s} [{:<20s}] ({:>9d}/{:>9d})"

    def __init__(self, name, amount):
        self._name = name
        self._amount = amount
        self._cur = 0

    def __enter__(self):
        return self

    def __exit__(self, a, b, c):
        sys.stdout.write('\n')
        sys.stdout.flush()

    def update(self, x):
        """
        Advance the counter by `x`.

        @param x: An integer of how many more elements were processed.
        """
        self._cur += x

    def show(self):
        """
        Display the progress bar on the current line.  Repeated invocations
        "update" the display.
        """
        if self._amount == 0:
            barlen = 20
        else:
            barlen = int(20 * self._cur / float(self._amount))
        if barlen:
            bartext = '=' * (barlen-1) + '>'
        else:
            bartext = ''
        text = self.TEXT.format(self._name, bartext, self._cur, self._amount)
        sys.stdout.write('\r')
        sys.stdout.write(text)
        sys.stdout.flush()


def grouper(iterable, n):
    """
    Group the elements of `iterable` into groups of maximum size `n`.

    @param iterable: An iterable.
    @param n: Max size of returned groups.
    @returns: Yields iterables of size <= n.

    >>> grouper('ABCDEFG', 3)
    [['A', 'B', C'], ['D', 'E', 'F'], ['G']]
    """
    args = [iter(iterable)] * n
    while True:
        lst = []
        try:
            for itr in args:
                lst.append(next(itr))
            yield lst
        except StopIteration:
            if lst:
                yield lst
            break


def _delete_table_data_before_date(table_to_delete_from, primary_key,
                                   related_table, related_primary_key,
                                   date, foreign_key=None,
                                   time_column="started_time",
                                   indirection_table=None,
                                   indirection_primary_key=None,
                                   indirection_foreign_key=None):
    """
    We want a delete statement that will only delete from one table while
    using a related table to find the rows to delete.

    An example mysql command:
    DELETE FROM tko_iteration_result USING tko_iteration_result INNER JOIN
    tko_tests WHERE tko_iteration_result.test_idx=tko_tests.test_idx AND
    tko_tests.started_time <= '2012-02-01';

    There are also tables that require 2 joins to determine which rows we want
    to delete and we determine these rows by joining the table we want to
    delete from with an indirection table to the actual jobs table.

    @param table_to_delete_from: Table whose rows we want to delete.
    @param related_table: Table with the date information we are selecting by.
    @param foreign_key: Foreign key used in table_to_delete_from to reference
                        the related table. If None, the primary_key is used.
    @param primary_key: Primary key in the related table.
    @param date: End date of the information we are trying to delete.
    @param time_column: Column that we want to use to compare the date to.
    @param indirection_table: Table we use to link the data we are trying to
                              delete with the table with the date information.
    @param indirection_primary_key: Key we use to connect the indirection table
                                    to the table we are trying to delete rows
                                    from.
    @param indirection_foreign_key: Key we use to connect the indirection table
                                    to the table with the date information.
    """
    if not foreign_key:
        foreign_key = primary_key

    if not related_table:
        # Deleting from a table directly.
        variables = dict(table=table_to_delete_from, primary_key=primary_key,
                         time_column=time_column, date=date)
        sql = SELECT_CMD_FORMAT % variables
    elif not indirection_table:
        # Deleting using a single JOIN to get the date information.
        variables = dict(primary_key=primary_key, table=table_to_delete_from,
                         foreign_key=foreign_key, related_table=related_table,
                         related_primary_key=related_primary_key,
                         time_column=time_column, date=date)
        sql = SELECT_JOIN_CMD_FORMAT % variables
    else:
        # There are cases where we need to JOIN 3 TABLES to determine the rows
        # we want to delete.
        variables = dict(primary_key=primary_key, table=table_to_delete_from,
                         indirection_table=indirection_table,
                         foreign_key=foreign_key,
                         indirection_primary_key=indirection_primary_key,
                         related_table=related_table,
                         related_primary_key=related_primary_key,
                         indirection_foreign_key=indirection_foreign_key,
                         time_column=time_column, date=date)
        sql = SELECT_WITH_INDIRECTION_FORMAT % variables

    logging.debug('SQL: %s', sql)
    cursor.execute(sql, [])
    rows = [x[0] for x in cursor.fetchall()]
    logging.debug(rows)

    if not rows or rows == [None]:
        with ProgressBar(table_to_delete_from, 0) as pb:
            pb.show()
        logging.debug('Noting to delete for %s', table_to_delete_from)
        return

    with ProgressBar(table_to_delete_from, len(rows)) as pb:
        for row_keys in grouper(rows, STEP_SIZE):
            variables['rows'] = ','.join([str(x) for x in row_keys])
            sql = DELETE_ROWS_FORMAT % variables
            logging.debug('SQL: %s', sql)
            cursor.execute(sql, [])
            transaction.commit_unless_managed(using='default')
            pb.update(len(row_keys))
            pb.show()


def _subtract_days(date, days_to_subtract):
    """
    Return a date (string) that is 'days' before 'date'

    @param date: date (string) we are subtracting from.
    @param days_to_subtract: days (int) we are subtracting.
    """
    date_obj = datetime.datetime.strptime(date, '%Y-%m-%d')
    difference = date_obj - datetime.timedelta(days=days_to_subtract)
    return difference.strftime('%Y-%m-%d')


def _delete_all_data_before_date(date):
    """
    Delete all the database data before a given date.

    This function focuses predominately on the data for jobs in tko_jobs.
    However not all jobs in afe_jobs are also in tko_jobs.

    Therefore we delete all the afe_job and foreign key relations prior to two
    days before date. Then we do the queries using tko_jobs and these
    tables to ensure all the related information is gone. Even though we are
    repeating deletes on these tables, the second delete will be quick and
    completely thorough in ensuring we clean up all the foreign key
    dependencies correctly.

    @param date: End date of the information we are trying to delete.
    @param step: Rows to delete per SQL query.
    """
    # First cleanup all afe_job related data (prior to 2 days before date).
    # The reason for this is not all afe_jobs may be in tko_jobs.
    afe_date = _subtract_days(date, 2)
    logging.info('Cleaning up all afe_job data prior to %s.', afe_date)
    _delete_table_data_before_date('afe_aborted_host_queue_entries',
                                   'queue_entry_id',
                                   'afe_jobs', 'id', afe_date,
                                   time_column= 'created_on',
                                   foreign_key='queue_entry_id',
                                   indirection_table='afe_host_queue_entries',
                                   indirection_primary_key='id',
                                   indirection_foreign_key='job_id')
    _delete_table_data_before_date('afe_special_tasks', 'id',
                                   'afe_jobs', 'id',
                                   afe_date, time_column='created_on',
                                   foreign_key='queue_entry_id',
                                   indirection_table='afe_host_queue_entries',
                                   indirection_primary_key='id',
                                   indirection_foreign_key='job_id')
    _delete_table_data_before_date('afe_host_queue_entries', 'id',
                                   'afe_jobs', 'id',
                                   afe_date, time_column='created_on',
                                   foreign_key=JOB_ID)
    _delete_table_data_before_date('afe_job_keyvals', 'id',
                                   'afe_jobs', 'id',
                                   afe_date, time_column='created_on',
                                   foreign_key=JOB_ID)
    _delete_table_data_before_date('afe_jobs_dependency_labels', 'id',
                                   'afe_jobs', 'id',
                                   afe_date, time_column='created_on',
                                   foreign_key=JOB_ID)
    _delete_table_data_before_date('afe_jobs', 'id',
                                   None, None,
                                   afe_date, time_column='created_on')
    # Special tasks that aren't associated with an HQE
    # Since we don't do the queue_entry_id=NULL check, we might wipe out a bit
    # more than we should, but I doubt anyone will notice or care.
    _delete_table_data_before_date('afe_special_tasks', 'id',
                                   None, None,
                                   afe_date, time_column='time_requested')

    # Now go through and clean up all the rows related to tko_jobs prior to
    # date.
    logging.info('Cleaning up all data related to tko_jobs prior to %s.',
                  date)
    _delete_table_data_before_date('tko_test_attributes', 'id',
                                   'tko_tests', TEST_IDX,
                                   date, foreign_key=TEST_IDX)
    _delete_table_data_before_date('tko_test_labels_tests', 'id',
                                   'tko_tests', TEST_IDX,
                                   date, foreign_key= 'test_id')
    _delete_table_data_before_date('tko_iteration_result', TEST_IDX,
                                   'tko_tests', TEST_IDX,
                                   date)
    _delete_table_data_before_date('tko_iteration_perf_value', TEST_IDX,
                                   'tko_tests', TEST_IDX,
                                   date)
    _delete_table_data_before_date('tko_iteration_attributes', TEST_IDX,
                                   'tko_tests', TEST_IDX,
                                   date)
    _delete_table_data_before_date('tko_job_keyvals', 'id',
                                   'tko_jobs', JOB_IDX,
                                   date, foreign_key='job_id')
    _delete_table_data_before_date('afe_aborted_host_queue_entries',
                                   'queue_entry_id',
                                   'tko_jobs', AFE_JOB_ID, date,
                                   foreign_key='queue_entry_id',
                                   indirection_table='afe_host_queue_entries',
                                   indirection_primary_key='id',
                                   indirection_foreign_key='job_id')
    _delete_table_data_before_date('afe_special_tasks', 'id',
                                   'tko_jobs', AFE_JOB_ID,
                                   date, foreign_key='queue_entry_id',
                                   indirection_table='afe_host_queue_entries',
                                   indirection_primary_key='id',
                                   indirection_foreign_key='job_id')
    _delete_table_data_before_date('afe_host_queue_entries', 'id',
                                   'tko_jobs', AFE_JOB_ID,
                                   date, foreign_key='job_id')
    _delete_table_data_before_date('afe_job_keyvals', 'id',
                                   'tko_jobs', AFE_JOB_ID,
                                   date, foreign_key='job_id')
    _delete_table_data_before_date('afe_jobs_dependency_labels', 'id',
                                   'tko_jobs', AFE_JOB_ID,
                                   date, foreign_key='job_id')
    _delete_table_data_before_date('afe_jobs', 'id',
                                   'tko_jobs', AFE_JOB_ID,
                                   date, foreign_key='id')
    _delete_table_data_before_date('tko_tests', TEST_IDX,
                                   'tko_jobs', JOB_IDX,
                                   date, foreign_key=JOB_IDX)
    _delete_table_data_before_date('tko_jobs', JOB_IDX,
                                   None, None, date)


def parse_args():
    """Parse command line arguments"""
    parser = argparse.ArgumentParser()
    parser.add_argument('-v', '--verbose', action='store_true',
                        help='Print SQL commands and results')
    parser.add_argument('--step', type=int, action='store',
                        default=1000,
                        help='Number of rows to delete at once')
    parser.add_argument('date', help='Keep results newer than')
    return parser.parse_args()


def main():
    """main"""
    args = parse_args()

    level = logging.DEBUG if args.verbose else logging.INFO
    logging.basicConfig(level=level, format=LOGGING_FORMAT)
    logging.info('Calling: %s', sys.argv)

    if not re.match(DATE_FORMAT_REGEX, args.date):
        print 'DATE must be in yyyy-mm-dd format!'
        return

    global STEP_SIZE
    STEP_SIZE = args.step
    _delete_all_data_before_date(args.date)


if __name__ == '__main__':
    main()
