#!/usr/bin/env python
# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Create e-mail reports of the Lab's DUT inventory.

Gathers a list of all DUTs of interest in the Lab, segregated by
board and pool, and determines whether each DUT is working or
broken.  Then, send one or more e-mail reports summarizing the
status to e-mail addresses provided on the command line.

usage:  lab_inventory.py [ options ] [ board ... ]

Options:
--duration / -d <hours>
    How far back in time to search job history to determine DUT
    status.

--board-notify <address>[,<address>]
    Send the "board status" e-mail to all the specified e-mail
    addresses.

--pool-notify <address>[,<address>]
    Send the "pool status" e-mail to all the specified e-mail
    addresses.

--recommend <number>
    When generating the "board status" e-mail, included a list of
    <number> specific DUTs to be recommended for repair.

--logdir <directory>
    Log progress and actions in a file under this directory.  Text
    of any e-mail sent will also be logged in a timestamped file in
    this directory.

--debug
    Suppress all logging and sending e-mail.  Instead, write the
    output that would be generated onto stdout.

<board> arguments:
    With no arguments, gathers the status for all boards in the lab.
    With one or more named boards on the command line, restricts
    reporting to just those boards.

"""


import argparse
import logging
import logging.handlers
import os
import re
import sys
import time

import common
from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib import time_utils
from autotest_lib.server.cros.dynamic_suite import frontend_wrappers
from autotest_lib.server.hosts import servo_host
from autotest_lib.site_utils import gmail_lib
from autotest_lib.site_utils import status_history
from autotest_lib.site_utils.suite_scheduler import constants


# The pools in the Lab that are actually of interest.
#
# These are general purpose pools of DUTs that are considered
# identical for purposes of testing.  That is, a device in one of
# these pools can be shifted to another pool at will for purposes
# of supplying test demand.
#
# Devices in these pools are not allowed to have special-purpose
# attachments, or to be part of in any kind of custom fixture.
# Devices in these pools are also required to reside in areas
# managed by the Platforms team (i.e. at the time of this writing,
# only in "Atlantis" or "Destiny").
#
# _CRITICAL_POOLS - Pools that must be kept fully supplied in order
#     to guarantee timely completion of tests from builders.
# _SPARE_POOL - A low priority pool that is allowed to provide
#     spares to replace broken devices in the critical pools.
# _MANAGED_POOLS - The set of all the general purpose pools
#     monitored by this script.

_CRITICAL_POOLS = ['bvt', 'cq', 'continuous']
_SPARE_POOL = 'suites'
_MANAGED_POOLS = _CRITICAL_POOLS + [_SPARE_POOL]

# _DEFAULT_DURATION:
#     Default value used for the --duration command line option.
#     Specifies how far back in time to search in order to determine
#     DUT status.

_DEFAULT_DURATION = 24

# _LOGDIR:
#     Relative path used in the calculation of the default setting
#     for the --logdir option.  The full path path is relative to
#     the root of the autotest directory, as determined from
#     sys.argv[0].
# _LOGFILE:
#     Basename of a file to which general log information will be
#     written.
# _LOG_FORMAT:
#     Format string for log messages.

_LOGDIR = os.path.join('logs', 'dut-data')
_LOGFILE = 'lab-inventory.log'
_LOG_FORMAT = '%(asctime)s | %(levelname)-10s | %(message)s'

# Pattern describing location-based host names in the Chrome OS test
# labs.  Each DUT hostname designates the DUT's location:
#   * A lab (room) that's physically separated from other labs
#     (i.e. there's a door).
#   * A row (or aisle) of DUTs within the lab.
#   * A vertical rack of shelves on the row.
#   * A specific host on one shelf of the rack.

_HOSTNAME_PATTERN = re.compile(
        r'(chromeos\d+)-row(\d+)-rack(\d+)-host(\d+)')


class _PoolCounts(object):
    """Maintains a set of `HostJobHistory` objects for a pool.

    The collected history objects are nominally all part of a single
    scheduling pool of DUTs.  The collection maintains a list of
    working DUTs, a list of broken DUTs, and a list of all DUTs.

    Performance note:  Certain methods in this class are potentially
    expensive:
      * `get_working()`
      * `get_working_list()`
      * `get_broken()`
      * `get_broken_list()`
    The first time any one of these methods is called, it causes
    multiple RPC calls with a relatively expensive set of database
    queries.  However, the results of the queries are cached in the
    individual `HostJobHistory` objects, so only the first call
    actually pays the full cost.

    Additionally, `get_working_list()` and `get_broken_list()` both
    cache their return values to avoid recalculating lists at every
    call; this caching is separate from the caching of RPC results
    described above.

    This class is deliberately constructed to delay the RPC cost
    until the accessor methods are called (rather than to query in
    `record_host()`) so that it's possible to construct a complete
    `_LabInventory` without making the expensive queries at creation
    time.  `_populate_board_counts()`, below, assumes this behavior.

    """

    def __init__(self):
        self._histories = []
        self._working_list = None
        self._broken_list = None


    def record_host(self, host_history):
        """Add one `HostJobHistory` object to the collection.

        @param host_history The `HostJobHistory` object to be
                            remembered.

        """
        self._working_list = None
        self._broken_list = None
        self._histories.append(host_history)


    def get_working_list(self):
        """Return a list of all working DUTs in the pool.

        Filter `self._histories` for histories where the last
        diagnosis is `WORKING`.

        Cache the result so that we only cacluate it once.

        @return A list of HostJobHistory objects.

        """
        if self._working_list is None:
            self._working_list = [h for h in self._histories
                    if h.last_diagnosis()[0] == status_history.WORKING]
        return self._working_list


    def get_working(self):
        """Return the number of working DUTs in the pool."""
        return len(self.get_working_list())


    def get_broken_list(self):
        """Return a list of all broken DUTs in the pool.

        Filter `self._histories` for histories where the last
        diagnosis is not `WORKING`.

        Cache the result so that we only cacluate it once.

        @return A list of HostJobHistory objects.

        """
        if self._broken_list is None:
            self._broken_list = [h for h in self._histories
                    if h.last_diagnosis()[0] != status_history.WORKING]
        return self._broken_list


    def get_broken(self):
        """Return the number of broken DUTs in the pool."""
        return len(self.get_broken_list())


    def get_total(self):
        """Return the total number of DUTs in the pool."""
        return len(self._histories)


class _BoardCounts(object):
    """Maintains a set of `HostJobHistory` objects for a board.

    The collected history objects are nominally all of the same
    board.  The collection maintains a count of working DUTs, a
    count of broken DUTs, and a total count.  The counts can be
    obtained either for a single pool, or as a total across all
    pools.

    DUTs in the collection must be assigned to one of the pools
    in `_MANAGED_POOLS`.

    The `get_working()` and `get_broken()` methods rely on the
    methods of the same name in _PoolCounts, so the performance
    note in _PoolCounts applies here as well.

    """

    def __init__(self):
        self._pools = {
            pool: _PoolCounts() for pool in _MANAGED_POOLS
        }

    def record_host(self, host_history):
        """Add one `HostJobHistory` object to the collection.

        @param host_history The `HostJobHistory` object to be
                            remembered.

        """
        pool = host_history.host_pool
        self._pools[pool].record_host(host_history)


    def _count_pool(self, get_pool_count, pool=None):
        """Internal helper to count hosts in a given pool.

        The `get_pool_count` parameter is a function to calculate
        the exact count of interest for the pool.

        @param get_pool_count  Function to return a count from a
                               _PoolCount object.
        @param pool            The pool to be counted.  If `None`,
                               return the total across all pools.

        """
        if pool is None:
            return sum([get_pool_count(counts)
                            for counts in self._pools.values()])
        else:
            return get_pool_count(self._pools[pool])


    def get_working_list(self):
        """Return a list of all working DUTs for the board.

        Go through all HostJobHistory objects in the board's pools,
        selecting the ones where the last diagnosis is `WORKING`.

        @return A list of HostJobHistory objects.

        """
        l = []
        for p in self._pools.values():
            l.extend(p.get_working_list())
        return l


    def get_working(self, pool=None):
        """Return the number of working DUTs in a pool.

        @param pool  The pool to be counted.  If `None`, return the
                     total across all pools.

        @return The total number of working DUTs in the selected
                pool(s).
        """
        return self._count_pool(_PoolCounts.get_working, pool)


    def get_broken_list(self):
        """Return a list of all broken DUTs for the board.

        Go through all HostJobHistory objects in the board's pools,
        selecting the ones where the last diagnosis is not
        `WORKING`.

        @return A list of HostJobHistory objects.

        """
        l = []
        for p in self._pools.values():
            l.extend(p.get_broken_list())
        return l


    def get_broken(self, pool=None):
        """Return the number of broken DUTs in a pool.

        @param pool  The pool to be counted.  If `None`, return the
                     total across all pools.

        @return The total number of broken DUTs in the selected pool(s).
        """
        return self._count_pool(_PoolCounts.get_broken, pool)


    def get_spares_buffer(self):
        """Return the the nominal number of working spares.

        Calculates and returns how many working spares there would
        be in the spares pool if all broken DUTs were in the spares
        pool.  This number may be negative, indicating a shortfall
        in the critical pools.

        @return The total number DUTs in the spares pool, less the total
                number of broken DUTs in all pools.
        """
        return self.get_total(_SPARE_POOL) - self.get_broken()


    def get_total(self, pool=None):
        """Return the total number of DUTs in a pool.

        @param pool  The pool to be counted.  If `None`, return the
                     total across all pools.

        @return The total number of DUTs in the selected pool(s).
        """
        return self._count_pool(_PoolCounts.get_total, pool)


class _LabInventory(dict):
    """Collection of `HostJobHistory` objects for the Lab's inventory.

    The collection is indexed by board.  Indexing returns the
    _BoardCounts object associated with the board.

    The collection is also iterable.  The iterator returns all the
    boards in the inventory, in unspecified order.

    """

    @classmethod
    def create_inventory(cls, afe, start_time, end_time, boardlist=[]):
        """Return a Lab inventory with specified parameters.

        By default, gathers inventory from `HostJobHistory` objects
        for all DUTs in the `_MANAGED_POOLS` list.  If `boardlist`
        is supplied, the inventory will be restricted to only the
        given boards.

        @param afe         AFE object for constructing the
                           `HostJobHistory` objects.
        @param start_time  Start time for the `HostJobHistory`
                           objects.
        @param end_time    End time for the `HostJobHistory`
                           objects.
        @param boardlist   List of boards to include.  If empty,
                           include all available boards.
        @return A `_LabInventory` object for the specified boards.

        """
        label_list = [constants.Labels.POOL_PREFIX + l
                          for l in _MANAGED_POOLS]
        afehosts = afe.get_hosts(labels__name__in=label_list)
        if boardlist:
            boardhosts = []
            for board in boardlist:
                board_label = constants.Labels.BOARD_PREFIX + board
                host_list = [h for h in afehosts
                                  if board_label in h.labels]
                boardhosts.extend(host_list)
            afehosts = boardhosts
        create = lambda host: (
                status_history.HostJobHistory(afe, host,
                                              start_time, end_time))
        return cls([create(host) for host in afehosts])


    def __init__(self, histories):
        # N.B. The query that finds our hosts is restricted to those
        # with a valid pool: label, but doesn't check for a valid
        # board: label.  In some (insufficiently) rare cases, the
        # AFE hosts table has been known to (incorrectly) have DUTs
        # with a pool: but no board: label.  We explicitly exclude
        # those here.
        histories = [h for h in histories
                     if h.host_board is not None]
        boards = set([h.host_board for h in histories])
        initval = { board: _BoardCounts() for board in boards }
        super(_LabInventory, self).__init__(initval)
        self._dut_count = len(histories)
        self._managed_boards = None
        for h in histories:
            self[h.host_board].record_host(h)


    def get_managed_boards(self):
        """Return the set of "managed" boards.

        Operationally, saying a board is "managed" means that the
        board will be included in the "board" and "repair
        recommendations" reports.  That is, if there are failures in
        the board's inventory then lab techs will be asked to fix
        them without a separate ticket.

        For purposes of implementation, a board is "managed" if it
        has DUTs in both the spare and a non-spare (i.e. critical)
        pool.

        @return A set of all the boards that have both spare and
                non-spare pools.
        """
        if self._managed_boards is None:
            self._managed_boards = set()
            for board, counts in self.items():
                spares = counts.get_total(_SPARE_POOL)
                total = counts.get_total()
                if spares != 0 and spares != total:
                    self._managed_boards.add(board)
        return self._managed_boards


    def get_num_duts(self):
        """Return the total number of DUTs in the inventory."""
        return self._dut_count


    def get_num_boards(self):
        """Return the total number of boards in the inventory."""
        return len(self)


def _sort_by_location(inventory_list):
    """Return a list of DUTs, organized by location.

    Take the given list of `HostJobHistory` objects, separate it
    into a list per lab, and sort each lab's list by location.  The
    order of sorting within a lab is
      * By row number within the lab,
      * then by rack number within the row,
      * then by host shelf number within the rack.

    Return a list of the sorted lists.

    Implementation note: host locations are sorted by converting
    each location into a base 100 number.  If row, rack or
    host numbers exceed the range [0..99], then sorting will
    break down.

    @return A list of sorted lists of DUTs.

    """
    BASE = 100
    lab_lists = {}
    for history in inventory_list:
        location = _HOSTNAME_PATTERN.match(history.host.hostname)
        if location:
            lab = location.group(1)
            key = 0
            for idx in location.group(2, 3, 4):
                key = BASE * key + int(idx)
            lab_lists.setdefault(lab, []).append((key, history))
    return_list = []
    for dut_list in lab_lists.values():
        dut_list.sort(key=lambda t: t[0])
        return_list.append([t[1] for t in dut_list])
    return return_list


def _score_repair_set(buffer_counts, repair_list):
    """Return a numeric score rating a set of DUTs to be repaired.

    `buffer_counts` is a dictionary mapping board names to the
    size of the board's spares buffer.

    `repair_list` is a list of DUTs to be repaired.

    This function calculates the new set of buffer counts that would
    result from the proposed repairs, and scores the new set using
    two numbers:
      * Worst case buffer count for any board (higher is better).
        This is the more siginficant number for comparison.
      * Number of boards at the worst case (lower is better).  This
        is the less significant number.

    Implementation note:  The score could fail to reflect the
    intended criteria if there are more than 1000 boards in the
    inventory.

    @param spare_counts A dictionary mapping boards to buffer counts.
    @param repair_list  A list of boards to be repaired.
    @return A numeric score.

    """
    # Go through `buffer_counts`, and create a list of new counts
    # that records the buffer count for each board after repair.
    # The new list of counts discards the board names, as they don't
    # contribute to the final score.
    _NBOARDS = 1000
    repair_inventory = _LabInventory(repair_list)
    new_counts = []
    for b, c in buffer_counts.items():
        if b in repair_inventory:
            newcount = repair_inventory[b].get_total()
        else:
            newcount = 0
        new_counts.append(c + newcount)
    # Go through the new list of counts.  Find the worst available
    # spares count, and count how many times that worst case occurs.
    worst_count = new_counts[0]
    num_worst = 1
    for c in new_counts[1:]:
        if c == worst_count:
            num_worst += 1
        elif c < worst_count:
            worst_count = c
            num_worst = 1
    # Return the calculated score
    return _NBOARDS * worst_count - num_worst


def _generate_repair_recommendation(inventory, num_recommend):
    """Return a summary of selected DUTs needing repair.

    Returns a message recommending a list of broken DUTs to be
    repaired.  The list of DUTs is selected based on these
    criteria:
      * No more than `num_recommend` DUTs will be listed.
      * All DUTs must be in the same lab.
      * DUTs should be selected for some degree of physical
        proximity.
      * DUTs for boards with a low spares buffer are more important
        than DUTs with larger buffers.

    The algorithm used will guarantee that at least one DUT from a
    board with the smallest spares buffer will be recommended.  If
    the worst spares buffer number is shared by more than one board,
    the algorithm will tend to prefer repair sets that include more
    of those boards over sets that cover fewer boards.

    @param inventory      Inventory for generating recommendations.
    @param num_recommend  Number of DUTs to recommend for repair.

    """
    logging.debug('Creating DUT repair recommendations')
    board_buffer_counts = {}
    broken_list = []
    for board in inventory.get_managed_boards():
        logging.debug('Listing failed DUTs for %s', board)
        counts = inventory[board]
        if counts.get_broken() != 0:
            board_buffer_counts[board] = counts.get_spares_buffer()
            broken_list.extend(counts.get_broken_list())
    # N.B. The logic inside this loop may seem complicated, but
    # simplification is hard:
    #   * Calculating an initial recommendation outside of
    #     the loop likely would make things more complicated,
    #     not less.
    #   * It's necessary to calculate an initial lab slice once per
    #     lab _before_ the while loop, in case the number of broken
    #     DUTs in a lab is less than `num_recommend`.
    recommendation = None
    best_score = None
    for lab_duts in _sort_by_location(broken_list):
        start = 0
        end = num_recommend
        lab_slice = lab_duts[start : end]
        lab_score = _score_repair_set(board_buffer_counts,
                                      lab_slice)
        while end < len(lab_duts):
            start += 1
            end += 1
            new_slice = lab_duts[start : end]
            new_score = _score_repair_set(board_buffer_counts,
                                          new_slice)
            if new_score > lab_score:
                lab_slice = new_slice
                lab_score = new_score
        if recommendation is None or lab_score > best_score:
            recommendation = lab_slice
            best_score = lab_score
    message = ['Repair recommendations:\n',
               '%-30s %-16s %s' % (
                       'Hostname', 'Board', 'Servo instructions')]
    for h in recommendation:
        servo_name = servo_host.make_servo_hostname(h.host.hostname)
        if utils.host_is_in_lab_zone(servo_name):
            servo_message = 'Repair servo first'
        else:
            servo_message = 'No servo present'
        line = '%-30s %-16s %s' % (
                h.host.hostname, h.host_board, servo_message)
        message.append(line)
    return '\n'.join(message)


def _generate_board_inventory_message(inventory):
    """Generate the "board inventory" e-mail message.

    The board inventory is a list by board summarizing the number
    of working and broken DUTs, and the total shortfall or surplus
    of working devices relative to the minimum critical pool
    requirement.

    The report omits boards with no DUTs in the spare pool or with
    no DUTs in a critical pool.

    N.B. For sample output text formattted as users can expect to
    see it in e-mail and log files, refer to the unit tests.

    @param inventory  _LabInventory object with the inventory to
                      be reported on.
    @return String with the inventory message to be sent.

    """
    logging.debug('Creating board inventory')
    nworking = 0
    nbroken = 0
    nbroken_boards = 0
    summaries = []
    for board in inventory.get_managed_boards():
        logging.debug('Counting board inventory for %s', board)
        counts = inventory[board]
        # Summary elements laid out in the same order as the text
        # headers:
        #     Board Avail   Bad  Good Spare Total
        #      e[0]  e[1]  e[2]  e[3]  e[4]  e[5]
        element = (board,
                   counts.get_spares_buffer(),
                   counts.get_broken(),
                   counts.get_working(),
                   counts.get_total(_SPARE_POOL),
                   counts.get_total())
        summaries.append(element)
        nbroken += element[2]
        nworking += element[3]
        if element[2]:
            nbroken_boards += 1
    ntotal = nworking + nbroken
    summaries = sorted(summaries, key=lambda e: (e[1], -e[2]))
    broken_percent = int(round(100.0 * nbroken / ntotal))
    working_percent = 100 - broken_percent
    message = ['Summary of DUTs in inventory:',
               '%10s %10s %6s' % ('Bad', 'Good', 'Total'),
               '%5d %3d%% %5d %3d%% %6d' % (
                   nbroken, broken_percent,
                   nworking, working_percent,
                   ntotal),
               '',
               'Boards with failures: %d' % nbroken_boards,
               'Boards in inventory:  %d' % len(summaries),
               '', '',
               'Full board inventory:\n',
               '%-22s %5s %5s %5s %5s %5s' % (
                   'Board', 'Avail', 'Bad', 'Good',
                   'Spare', 'Total')]
    message.extend(
            ['%-22s %5d %5d %5d %5d %5d' % e for e in summaries])
    return '\n'.join(message)


_POOL_INVENTORY_HEADER = '''\
Notice to Infrastructure deputies:  All boards shown below are at
less than full strength, please take action to resolve the issues.
Once you're satisified that failures won't recur, failed DUTs can
be replaced with spares by running `balance_pool`.  Detailed
instructions can be found here:
    http://go/cros-manage-duts
'''


def _generate_pool_inventory_message(inventory):
    """Generate the "pool inventory" e-mail message.

    The pool inventory is a list by pool and board summarizing the
    number of working and broken DUTs in the pool.  Only boards with
    at least one broken DUT are included in the list.

    N.B. For sample output text formattted as users can expect to
    see it in e-mail and log files, refer to the unit tests.

    @param inventory  _LabInventory object with the inventory to
                      be reported on.
    @return String with the inventory message to be sent.

    """
    logging.debug('Creating pool inventory')
    message = [_POOL_INVENTORY_HEADER]
    newline = ''
    for pool in _CRITICAL_POOLS:
        message.append(
            '%sStatus for pool:%s, by board:' % (newline, pool))
        message.append(
            '%-20s   %5s %5s %5s' % (
                'Board', 'Bad', 'Good', 'Total'))
        data_list = []
        for board, counts in inventory.items():
            logging.debug('Counting inventory for %s, %s',
                          board, pool)
            broken = counts.get_broken(pool)
            if broken == 0:
                continue
            working = counts.get_working(pool)
            total = counts.get_total(pool)
            data_list.append((board, broken, working, total))
        if data_list:
            data_list = sorted(data_list, key=lambda d: -d[1])
            message.extend(
                ['%-20s   %5d %5d %5d' % t for t in data_list])
        else:
            message.append('(All boards at full strength)')
        newline = '\n'
    return '\n'.join(message)


def _send_email(arguments, tag, subject, recipients, body):
    """Send an inventory e-mail message.

    The message is logged in the selected log directory using `tag`
    for the file name.

    If the --print option was requested, the message is neither
    logged nor sent, but merely printed on stdout.

    @param arguments   Parsed command-line options.
    @param tag         Tag identifying the inventory for logging
                       purposes.
    @param subject     E-mail Subject: header line.
    @param recipients  E-mail addresses for the To: header line.
    @param body        E-mail message body.

    """
    logging.debug('Generating email: "%s"', subject)
    all_recipients = ', '.join(recipients)
    report_body = '\n'.join([
            'To: %s' % all_recipients,
            'Subject: %s' % subject,
            '', body, ''])
    if arguments.debug:
        print report_body
    else:
        filename = os.path.join(arguments.logdir, tag)
        try:
            report_file = open(filename, 'w')
            report_file.write(report_body)
            report_file.close()
        except EnvironmentError as e:
            logging.error('Failed to write %s:  %s', filename, e)
        try:
            gmail_lib.send_email(all_recipients, subject, body)
        except Exception as e:
            logging.error('Failed to send e-mail to %s:  %s',
                          all_recipients, e)


def _separate_email_addresses(address_list):
    """Parse a list of comma-separated lists of e-mail addresses.

    @param address_list  A list of strings containing comma
                         separate e-mail addresses.
    @return A list of the individual e-mail addresses.

    """
    newlist = []
    for arg in address_list:
        newlist.extend([email.strip() for email in arg.split(',')])
    return newlist


def _verify_arguments(arguments):
    """Validate command-line arguments.

    Join comma separated e-mail addresses for `--board-notify` and
    `--pool-notify` in separate option arguments into a single list.

    For non-debug uses, require that notification be requested for
    at least one report.  For debug, if notification isn't specified,
    treat it as "run all the reports."

    The return value indicates success or failure; in the case of
    failure, we also write an error message to stderr.

    @param arguments  Command-line arguments as returned by
                      `ArgumentParser`
    @return True if the arguments are semantically good, or False
            if the arguments don't meet requirements.

    """
    arguments.board_notify = _separate_email_addresses(
            arguments.board_notify)
    arguments.pool_notify = _separate_email_addresses(
            arguments.pool_notify)
    if not arguments.board_notify and not arguments.pool_notify:
        if not arguments.debug:
            sys.stderr.write('Must specify at least one of '
                             '--board-notify or --pool-notify\n')
            return False
        else:
            # We want to run all the reports.  An empty notify list
            # will cause a report to be skipped, so make sure the
            # lists are non-empty.
            arguments.board_notify = ['']
            arguments.pool_notify = ['']
    return True


def _get_logdir(script):
    """Get the default directory for the `--logdir` option.

    The default log directory is based on the parent directory
    containing this script.

    @param script  Path to this script file.
    @return A path to a directory.

    """
    basedir = os.path.dirname(os.path.abspath(script))
    basedir = os.path.dirname(basedir)
    return os.path.join(basedir, _LOGDIR)


def _parse_command(argv):
    """Parse the command line arguments.

    Create an argument parser for this command's syntax, parse the
    command line, and return the result of the ArgumentParser
    parse_args() method.

    @param argv Standard command line argument vector; argv[0] is
                assumed to be the command name.
    @return Result returned by ArgumentParser.parse_args().

    """
    parser = argparse.ArgumentParser(
            prog=argv[0],
            description='Gather and report lab inventory statistics')
    parser.add_argument('-d', '--duration', type=int,
                        default=_DEFAULT_DURATION, metavar='HOURS',
                        help='number of hours back to search for status'
                             ' (default: %d)' % _DEFAULT_DURATION)
    parser.add_argument('--board-notify', action='append',
                        default=[], metavar='ADDRESS',
                        help='Generate board inventory message, '
                        'and send it to the given e-mail address(es)')
    parser.add_argument('--pool-notify', action='append',
                        default=[], metavar='ADDRESS',
                        help='Generate pool inventory message, '
                             'and send it to the given address(es)')
    parser.add_argument('-r', '--recommend', type=int, default=None,
                        help=('Specify how many DUTs should be '
                              'recommended for repair (default: no '
                              'recommendation)'))
    parser.add_argument('--debug', action='store_true',
                        help='Print e-mail messages on stdout '
                             'without sending them.')
    parser.add_argument('--logdir', default=_get_logdir(argv[0]),
                        help='Directory where logs will be written.')
    parser.add_argument('boardnames', nargs='*',
                        metavar='BOARD',
                        help='names of boards to report on '
                             '(default: all boards)')
    arguments = parser.parse_args(argv[1:])
    if not _verify_arguments(arguments):
        return None
    return arguments


def _configure_logging(arguments):
    """Configure the `logging` module for our needs.

    How we log depends on whether the `--print` option was
    provided on the command line.  Without the option, we log all
    messages at DEBUG level or above, and write them to a file in
    the directory specified by the `--logdir` option.  With the
    option, we write log messages to stdout; messages below INFO
    level are discarded.

    The log file is configured to rotate once a week on Friday
    evening, preserving ~3 months worth of history.

    @param arguments  Command-line arguments as returned by
                      `ArgumentParser`

    """
    root_logger = logging.getLogger()
    if arguments.debug:
        root_logger.setLevel(logging.INFO)
        handler = logging.StreamHandler(sys.stdout)
        handler.setFormatter(logging.Formatter())
    else:
        root_logger.setLevel(logging.DEBUG)
        logfile = os.path.join(arguments.logdir, _LOGFILE)
        handler = logging.handlers.TimedRotatingFileHandler(
                logfile, when='W4', backupCount=13)
        formatter = logging.Formatter(_LOG_FORMAT,
                                      time_utils.TIME_FMT)
        handler.setFormatter(formatter)
    # TODO(jrbarnette) This is gross.  Importing client.bin.utils
    # implicitly imported logging_config, which calls
    # logging.basicConfig() *at module level*.  That gives us an
    # extra logging handler that we don't want.  So, clear out all
    # the handlers here.
    for h in root_logger.handlers:
        root_logger.removeHandler(h)
    root_logger.addHandler(handler)


def _populate_board_counts(inventory):
    """Gather board counts while providing interactive feedback.

    Gathering the status of all individual DUTs in the lab can take
    considerable time (~30 minutes at the time of this writing).

    Normally, we pay that cost by querying as we go.  However, with
    the `--print` option, a human being may be watching the
    progress.  So, we force the first (expensive) queries to happen
    up front, and provide a small ASCII progress bar to give an
    indicator of how many boards have been processed.

    @param inventory  _LabInventory object with the inventory to
                      be gathered.

    """
    n = 0
    total_broken = 0
    for counts in inventory.values():
        n += 1
        if n % 10 == 5:
            c = '+'
        elif n % 10 == 0:
            c = '%d' % ((n / 10) % 10)
        else:
            c = '.'
        sys.stdout.write(c)
        sys.stdout.flush()
        # This next call is where all the time goes - it forces all
        # of a board's HostJobHistory objects to query the database
        # and cache their results.
        total_broken += counts.get_broken()
    sys.stdout.write('\n')
    sys.stdout.write('Found %d broken DUTs\n' % total_broken)


def main(argv):
    """Standard main routine.
    @param argv  Command line arguments including `sys.argv[0]`.
    """
    arguments = _parse_command(argv)
    if not arguments:
        sys.exit(1)
    _configure_logging(arguments)
    try:
        end_time = int(time.time())
        start_time = end_time - arguments.duration * 60 * 60
        timestamp = time.strftime('%Y-%m-%d.%H',
                                  time.localtime(end_time))
        logging.debug('Starting lab inventory for %s', timestamp)
        if arguments.board_notify:
            if arguments.recommend:
                logging.debug('Will include repair recommendations')
            logging.debug('Will include board inventory')
        if arguments.pool_notify:
            logging.debug('Will include pool inventory')

        afe = frontend_wrappers.RetryingAFE(server=None)
        inventory = _LabInventory.create_inventory(
                afe, start_time, end_time, arguments.boardnames)
        logging.info('Found %d hosts across %d boards',
                         inventory.get_num_duts(),
                         inventory.get_num_boards())

        if arguments.debug:
            _populate_board_counts(inventory)

        if arguments.board_notify:
            if arguments.recommend:
                recommend_message = _generate_repair_recommendation(
                        inventory, arguments.recommend) + '\n\n\n'
            else:
                recommend_message = ''
            board_message = _generate_board_inventory_message(inventory)
            _send_email(arguments,
                        'boards-%s.txt' % timestamp,
                        'DUT board inventory %s' % timestamp,
                        arguments.board_notify,
                        recommend_message + board_message)

        if arguments.pool_notify:
            _send_email(arguments,
                        'pools-%s.txt' % timestamp,
                        'DUT pool inventory %s' % timestamp,
                        arguments.pool_notify,
                        _generate_pool_inventory_message(inventory))
    except KeyboardInterrupt:
        pass
    except EnvironmentError as e:
        logging.exception('Unexpected OS error: %s', e)
    except Exception as e:
        logging.exception('Unexpected exception: %s', e)


def get_managed_boards(afe):
    end_time = int(time.time())
    start_time = end_time - 24 * 60 * 60
    inventory = _LabInventory.create_inventory(
            afe, start_time, end_time)
    return inventory.get_managed_boards()


if __name__ == '__main__':
    main(sys.argv)
