#!/usr/bin/env python
# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import itertools
import logging
import os
import unittest

import common
from autotest_lib.site_utils import lab_inventory
from autotest_lib.site_utils import status_history


class _FakeHostHistory(object):
    """Class to mock `HostJobHistory` for testing."""

    def __init__(self, board, pool, status):
        self._board = board
        self._pool = pool
        self._status = status


    @property
    def host_board(self):
        """Return the recorded board."""
        return self._board


    @property
    def host_pool(self):
        """Return the recorded host."""
        return self._pool


    def last_diagnosis(self):
        """Return the recorded diagnosis."""
        return self._status, None


class _FakeHostLocation(object):
    """Class to mock `HostJobHistory` for location sorting."""

    _HOSTNAME_FORMAT = 'chromeos%d-row%d-rack%d-host%d'


    def __init__(self, location):
        self.hostname = self._HOSTNAME_FORMAT % location


    @property
    def host(self):
        """Return a fake host object with a hostname."""
        return self


# Status values that may be returned by `HostJobHistory`.
#
# _NON_WORKING_STATUS_LIST - The complete list (as of this writing)
#     of status values that the lab_inventory module treats as
#     "broken".
# _WORKING - A value that counts as "working" for purposes
#     of the lab_inventory module.
# _BROKEN - A value that counts as "broken" for the lab_inventory
#     module.  Since there's more than one valid choice here, we've
#     picked one to stand for all of them.

_NON_WORKING_STATUS_LIST = [
    status_history.UNUSED,
    status_history.BROKEN,
    status_history.UNKNOWN,
]

_WORKING = status_history.WORKING
_BROKEN = _NON_WORKING_STATUS_LIST[0]


class PoolCountTests(unittest.TestCase):
    """Unit tests for class `_PoolCounts`.

    Coverage is quite basic:  mostly just enough to make sure every
    function gets called, and to make sure that the counting knows
    the difference between 0 and 1.

    The testing also ensures that all known status values that
    can be returned by `HostJobHistory` are counted as expected.

    """

    def setUp(self):
        super(PoolCountTests, self).setUp()
        self._pool_counts = lab_inventory._PoolCounts()


    def _add_host(self, status):
        fake = _FakeHostHistory(
                None, lab_inventory._SPARE_POOL, status)
        self._pool_counts.record_host(fake)


    def _check_counts(self, working, broken):
        """Check that pool counts match expectations.

        Checks that `get_working()` and `get_broken()` return the
        given expected values.  Also check that `get_total()` is the
        sum of working and broken devices.

        @param working The expected total of working devices.
        @param broken  The expected total of broken devices.

        """
        self.assertEqual(self._pool_counts.get_working(), working)
        self.assertEqual(self._pool_counts.get_broken(), broken)
        self.assertEqual(self._pool_counts.get_total(),
                         working + broken)


    def test_empty(self):
        """Test counts when there are no DUTs recorded."""
        self._check_counts(0, 0)


    def test_non_working(self):
        """Test counting for all non-working status values."""
        count = 0
        for status in _NON_WORKING_STATUS_LIST:
            self._add_host(status)
            count += 1
            self._check_counts(0, count)


    def test_working_then_broken(self):
        """Test counts after adding a working and then a broken DUT."""
        self._add_host(_WORKING)
        self._check_counts(1, 0)
        self._add_host(_BROKEN)
        self._check_counts(1, 1)


    def test_broken_then_working(self):
        """Test counts after adding a broken and then a working DUT."""
        self._add_host(_BROKEN)
        self._check_counts(0, 1)
        self._add_host(_WORKING)
        self._check_counts(1, 1)


class BoardCountTests(unittest.TestCase):
    """Unit tests for class `_BoardCounts`.

    Coverage is quite basic:  just enough to make sure every
    function gets called, and to make sure that the counting
    knows the difference between 0 and 1.

    The tests make sure that both individual pool counts and
    totals are counted correctly.

    """

    def setUp(self):
        super(BoardCountTests, self).setUp()
        self._board_counts = lab_inventory._BoardCounts()


    def _add_host(self, pool, status):
        fake = _FakeHostHistory(None, pool, status)
        self._board_counts.record_host(fake)


    def _check_all_counts(self, working, broken):
        """Check that total counts for all pools match expectations.

        Checks that `get_working()` and `get_broken()` return the
        given expected values when called without a pool specified.
        Also check that `get_total()` is the sum of working and
        broken devices.

        Additionally, call the various functions for all the pools
        individually, and confirm that the totals across pools match
        the given expectations.

        @param working The expected total of working devices.
        @param broken  The expected total of broken devices.

        """
        self.assertEqual(self._board_counts.get_working(), working)
        self.assertEqual(self._board_counts.get_broken(), broken)
        self.assertEqual(self._board_counts.get_total(),
                         working + broken)
        count_working = 0
        count_broken = 0
        count_total = 0
        for pool in lab_inventory._MANAGED_POOLS:
            count_working += self._board_counts.get_working(pool)
            count_broken += self._board_counts.get_broken(pool)
            count_total += self._board_counts.get_total(pool)
        self.assertEqual(count_working, working)
        self.assertEqual(count_broken, broken)
        self.assertEqual(count_total, working + broken)


    def _check_pool_counts(self, pool, working, broken):
        """Check that counts for a given pool match expectations.

        Checks that `get_working()` and `get_broken()` return the
        given expected values for the given pool.  Also check that
        `get_total()` is the sum of working and broken devices.

        @param pool    The pool to be checked.
        @param working The expected total of working devices.
        @param broken  The expected total of broken devices.

        """
        self.assertEqual(self._board_counts.get_working(pool),
                         working)
        self.assertEqual(self._board_counts.get_broken(pool),
                         broken)
        self.assertEqual(self._board_counts.get_total(pool),
                         working + broken)


    def test_empty(self):
        """Test counts when there are no DUTs recorded."""
        self._check_all_counts(0, 0)
        for pool in lab_inventory._MANAGED_POOLS:
            self._check_pool_counts(pool, 0, 0)


    def test_all_working_then_broken(self):
        """Test counts after adding a working and then a broken DUT.

        For each pool, add first a working, then a broken DUT.  After
        each DUT is added, check counts to confirm the correct values.

        """
        working = 0
        broken = 0
        for pool in lab_inventory._MANAGED_POOLS:
            self._add_host(pool, _WORKING)
            working += 1
            self._check_pool_counts(pool, 1, 0)
            self._check_all_counts(working, broken)
            self._add_host(pool, _BROKEN)
            broken += 1
            self._check_pool_counts(pool, 1, 1)
            self._check_all_counts(working, broken)


    def test_all_broken_then_working(self):
        """Test counts after adding a broken and then a working DUT.

        For each pool, add first a broken, then a working DUT.  After
        each DUT is added, check counts to confirm the correct values.

        """
        working = 0
        broken = 0
        for pool in lab_inventory._MANAGED_POOLS:
            self._add_host(pool, _BROKEN)
            broken += 1
            self._check_pool_counts(pool, 0, 1)
            self._check_all_counts(working, broken)
            self._add_host(pool, _WORKING)
            working += 1
            self._check_pool_counts(pool, 1, 1)
            self._check_all_counts(working, broken)


class LocationSortTests(unittest.TestCase):
    """Unit tests for `_sort_by_location()`."""

    def setUp(self):
        super(LocationSortTests, self).setUp()


    def _check_sorting(self, *locations):
        """Test sorting a given list of locations.

        The input is an already ordered list of lists of tuples with
        row, rack, and host numbers.  The test converts the tuples
        to hostnames, preserving the original ordering.  Then it
        flattens and scrambles the input, runs it through
        `_sort_by_location()`, and asserts that the result matches
        the original.

        """
        lab = 0
        expected = []
        for tuples in locations:
            lab += 1
            expected.append(
                    [_FakeHostLocation((lab,) + t) for t in tuples])
        scrambled = [e for e in itertools.chain(*expected)]
        scrambled = [e for e in reversed(scrambled)]
        actual = lab_inventory._sort_by_location(scrambled)
        # The ordering of the labs in the output isn't guaranteed,
        # so we can't compare `expected` and `actual` directly.
        # Instead, we create a dictionary keyed on the first host in
        # each lab, and compare the dictionaries.
        self.assertEqual({l[0]: l for l in expected},
                         {l[0]: l for l in actual})


    def test_separate_labs(self):
        """Test that sorting distinguishes labs."""
        self._check_sorting([(1, 1, 1)], [(1, 1, 1)], [(1, 1, 1)])


    def test_separate_rows(self):
        """Test for proper sorting when only rows are different."""
        self._check_sorting([(1, 1, 1), (9, 1, 1), (10, 1, 1)])


    def test_separate_racks(self):
        """Test for proper sorting when only racks are different."""
        self._check_sorting([(1, 1, 1), (1, 9, 1), (1, 10, 1)])


    def test_separate_hosts(self):
        """Test for proper sorting when only hosts are different."""
        self._check_sorting([(1, 1, 1), (1, 1, 9), (1, 1, 10)])


    def test_diagonal(self):
        """Test for proper sorting when all parts are different."""
        self._check_sorting([(1, 1, 2), (1, 2, 1), (2, 1, 1)])


class InventoryScoringTests(unittest.TestCase):
    """Unit tests for `_score_repair_set()`."""

    def setUp(self):
        super(InventoryScoringTests, self).setUp()


    def _make_buffer_counts(self, *counts):
        """Create a dictionary suitable as `buffer_counts`.

        @param counts List of tuples with board count data.

        """
        self._buffer_counts = dict(counts)


    def _make_history_list(self, repair_counts):
        """Create a list suitable as `repair_list`.

        @param repair_counts List of (board, count) tuples.

        """
        pool = lab_inventory._SPARE_POOL
        histories = []
        for board, count in repair_counts:
            for i in range(0, count):
                histories.append(
                    _FakeHostHistory(board, pool, _BROKEN))
        return histories


    def _check_better(self, repair_a, repair_b):
        """Test that repair set A scores better than B.

        Contruct repair sets from `repair_a` and `repair_b`,
        and score both of them using the pre-existing
        `self._buffer_counts`.  Assert that the score for A is
        better than the score for B.

        @param repair_a Input data for repair set A
        @param repair_b Input data for repair set B

        """
        score_a = lab_inventory._score_repair_set(
                self._buffer_counts,
                self._make_history_list(repair_a))
        score_b = lab_inventory._score_repair_set(
                self._buffer_counts,
                self._make_history_list(repair_b))
        self.assertGreater(score_a, score_b)


    def _check_equal(self, repair_a, repair_b):
        """Test that repair set A scores the same as B.

        Contruct repair sets from `repair_a` and `repair_b`,
        and score both of them using the pre-existing
        `self._buffer_counts`.  Assert that the score for A is
        equal to the score for B.

        @param repair_a Input data for repair set A
        @param repair_b Input data for repair set B

        """
        score_a = lab_inventory._score_repair_set(
                self._buffer_counts,
                self._make_history_list(repair_a))
        score_b = lab_inventory._score_repair_set(
                self._buffer_counts,
                self._make_history_list(repair_b))
        self.assertEqual(score_a, score_b)


    def test_improve_worst_board(self):
        """Test that improving the worst board improves scoring.

        Construct a buffer counts dictionary with all boards having
        different counts.  Assert that it is both necessary and
        sufficient to improve the count of the worst board in order
        to improve the score.

        """
        self._make_buffer_counts(('lion', 0),
                                 ('tiger', 1),
                                 ('bear', 2))
        self._check_better([('lion', 1)], [('tiger', 1)])
        self._check_better([('lion', 1)], [('bear', 1)])
        self._check_better([('lion', 1)], [('tiger', 2)])
        self._check_better([('lion', 1)], [('bear', 2)])
        self._check_equal([('tiger', 1)], [('bear', 1)])


    def test_improve_worst_case_count(self):
        """Test that improving the number of worst cases improves the score.

        Construct a buffer counts dictionary with all boards having
        the same counts.  Assert that improving two boards is better
        than improving one.  Assert that improving any one board is
        as good as any other.

        """
        self._make_buffer_counts(('lion', 0),
                                 ('tiger', 0),
                                 ('bear', 0))
        self._check_better([('lion', 1), ('tiger', 1)], [('bear', 2)])
        self._check_equal([('lion', 2)], [('tiger', 1)])
        self._check_equal([('tiger', 1)], [('bear', 1)])


class _InventoryTests(unittest.TestCase):
    """Parent class for tests relating to full Lab inventory.

    This class provides a `create_inventory()` method that allows
    construction of a complete `_LabInventory` object from a
    simplified input representation.  The input representation
    is a dictionary mapping board names to tuples of this form:
        `((critgood, critbad), (sparegood, sparebad))`
    where:
        `critgood` is a number of working DUTs in one critical pool.
        `critbad` is a number of broken DUTs in one critical pool.
        `sparegood` is a number of working DUTs in one critical pool.
        `sparebad` is a number of broken DUTs in one critical pool.

    A single 'critical pool' is arbitrarily chosen for purposes of
    testing; there's no coverage for testing arbitrary combinations
    in more than one critical pool.

    """

    _CRITICAL_POOL = lab_inventory._CRITICAL_POOLS[0]
    _SPARE_POOL = lab_inventory._SPARE_POOL

    def setUp(self):
        super(_InventoryTests, self).setUp()
        self.num_duts = 0
        self.inventory = None


    def create_inventory(self, data):
        """Initialize a `_LabInventory` instance for testing.

        @param data  Representation of Lab inventory data, as
                     described above.

        """
        histories = []
        self.num_duts = 0
        status_choices = (_WORKING, _BROKEN)
        pools = (self._CRITICAL_POOL, self._SPARE_POOL)
        for board, counts in data.items():
            for i in range(0, len(pools)):
                for j in range(0, len(status_choices)):
                    for x in range(0, counts[i][j]):
                        history = _FakeHostHistory(board,
                                                   pools[i],
                                                   status_choices[j])
                        histories.append(history)
                        if board is not None:
                            self.num_duts += 1
        self.inventory = lab_inventory._LabInventory(histories)


class LabInventoryTests(_InventoryTests):
    """Tests for the basic functions of `_LabInventory`.

    Contains basic coverage to show that after an inventory is
    created and DUTs with known status are added, the inventory
    counts match the counts of the added DUTs.

    Test inventory objects are created using the `create_inventory()`
    method from the parent class.

    """

    # _BOARD_LIST - A list of sample board names for use in testing.

    _BOARD_LIST = [
        'lion',
        'tiger',
        'bear',
        'aardvark',
        'platypus',
        'echidna',
        'elephant',
        'giraffe',
    ]


    def _check_inventory(self, data):
        """Create a test inventory, and confirm that it's correct.

        Tests these assertions:
          * The counts of working and broken devices for each
            board match the numbers from `data`.
          * That the set of returned boards in the inventory matches
            the set from `data`.
          * That the total number of DUTs matches the number from
            `data`.
          * That the total number of boards matches the number from
            `data`.

        @param data Inventory data as for `self.create_inventory()`.

        """
        working_total = 0
        broken_total = 0
        managed_boards = set()
        for b in self.inventory:
            c = self.inventory[b]
            calculated_counts = (
                (c.get_working(self._CRITICAL_POOL),
                 c.get_broken(self._CRITICAL_POOL)),
                (c.get_working(self._SPARE_POOL),
                 c.get_broken(self._SPARE_POOL)))
            self.assertEqual(data[b], calculated_counts)
            nworking = data[b][0][0] + data[b][1][0]
            nbroken = data[b][0][1] + data[b][1][1]
            self.assertEqual(nworking, len(c.get_working_list()))
            self.assertEqual(nbroken, len(c.get_broken_list()))
            working_total += nworking
            broken_total += nbroken
            ncritical = data[b][0][0] + data[b][0][1]
            nspare = data[b][1][0] + data[b][1][1]
            if ncritical != 0 and nspare != 0:
                managed_boards.add(b)
        self.assertEqual(self.inventory.get_managed_boards(),
                         managed_boards)
        board_list = self.inventory.keys()
        self.assertEqual(set(board_list), set(data.keys()))
        self.assertEqual(self.inventory.get_num_duts(),
                         self.num_duts)
        self.assertEqual(self.inventory.get_num_boards(),
                         len(data))


    def test_empty(self):
        """Test counts when there are no DUTs recorded."""
        self.create_inventory({})
        self._check_inventory({})


    def test_missing_board(self):
        """Test handling when the board is `None`."""
        self.create_inventory({None: ((1, 1), (1, 1))})
        self._check_inventory({})


    def test_board_counts(self):
        """Test counts for various numbers of boards."""
        for nboards in [1, 2, len(self._BOARD_LIST)]:
            counts = ((1, 1), (1, 1))
            slice = self._BOARD_LIST[0 : nboards]
            inventory_data = {
                board: counts for board in slice
            }
            self.create_inventory(inventory_data)
            self._check_inventory(inventory_data)


    def test_single_dut_counts(self):
        """Test counts when there is a single DUT per board."""
        testcounts = [
            ((1, 0), (0, 0)),
            ((0, 1), (0, 0)),
            ((0, 0), (1, 0)),
            ((0, 0), (0, 1)),
        ]
        for counts in testcounts:
            inventory_data = { self._BOARD_LIST[0]: counts }
            self.create_inventory(inventory_data)
            self._check_inventory(inventory_data)


# _BOARD_MESSAGE_TEMPLATE -
# This is a sample of the output text produced by
# _generate_board_inventory_message().  This string is parsed by the
# tests below to construct a sample inventory that should produce
# the output, and then the output is generated and checked against
# this original sample.
#
# Constructing inventories from parsed sample text serves two
# related purposes:
#   - It provides a way to see what the output should look like
#     without having to run the script.
#   - It helps make sure that a human being will actually look at
#     the output to see that it's basically readable.
# This should also help prevent test bugs caused by writing tests
# that simply parrot the original output generation code.

_BOARD_MESSAGE_TEMPLATE = '''
Board                  Avail   Bad  Good Spare Total
lion                      -1    13    11    12    24
tiger                     -1     5     9     4    14
bear                       0     7    10     7    17
aardvark                   1     6     6     7    12
platypus                   2     4    20     6    24
echidna                    6     0    20     6    20
'''


class BoardInventoryTests(_InventoryTests):
    """Tests for `_generate_board_inventory_message()`.

    The tests create various test inventories designed to match the
    counts in `_BOARD_MESSAGE_TEMPLATE`, and asserts that the
    generated message text matches the original message text.

    Message text is represented as a list of strings, split on the
    `'\n'` separator.

    """

    def setUp(self):
        super(BoardInventoryTests, self).setUp()
        # The template string has leading and trailing '\n' that
        # won't be in the generated output; we strip them out here.
        message_lines = _BOARD_MESSAGE_TEMPLATE.split('\n')
        self._header = message_lines[1]
        self._board_lines = message_lines[2:-1]
        self._board_data = []
        for l in self._board_lines:
            items = l.split()
            board = items[0]
            good = int(items[3])
            bad = int(items[2])
            spare = int(items[4])
            self._board_data.append((board, (good, bad, spare)))


    def _make_minimum_spares(self, counts):
        """Create a counts tuple with as few spare DUTs as possible."""
        good, bad, spares = counts
        if spares > bad:
            return ((good + bad - spares, 0),
                    (spares - bad, bad))
        else:
            return ((good, bad - spares), (0, spares))


    def _make_maximum_spares(self, counts):
        """Create a counts tuple with as many spare DUTs as possible."""
        good, bad, spares = counts
        if good > spares:
            return ((good - spares, bad), (spares, 0))
        else:
            return ((0, good + bad - spares),
                    (good, spares - good))


    def _check_board_inventory(self, data):
        """Test that a test inventory creates the correct message.

        Create a test inventory from `data` using
        `self.create_inventory()`.  Then generate the board inventory
        output, and test that the output matches
        `_BOARD_MESSAGE_TEMPLATE`.

        The caller is required to produce data that matches the
        values in `_BOARD_MESSAGE_TEMPLATE`.

        @param data Inventory data as for `self.create_inventory()`.

        """
        self.create_inventory(data)
        message = lab_inventory._generate_board_inventory_message(
                self.inventory).split('\n')
        self.assertIn(self._header, message)
        body = message[message.index(self._header) + 1 :]
        self.assertEqual(body, self._board_lines)


    def test_minimum_spares(self):
        """Test message generation when the spares pool is low."""
        data = {
            board: self._make_minimum_spares(counts)
                for board, counts in self._board_data
        }
        self._check_board_inventory(data)


    def test_maximum_spares(self):
        """Test message generation when the critical pool is low."""
        data = {
            board: self._make_maximum_spares(counts)
                for board, counts in self._board_data
        }
        self._check_board_inventory(data)


    def test_ignore_no_spares(self):
        """Test that messages ignore boards with no spare pool."""
        data = {
            board: self._make_maximum_spares(counts)
                for board, counts in self._board_data
        }
        data['elephant'] = ((5, 4), (0, 0))
        self._check_board_inventory(data)


    def test_ignore_no_critical(self):
        """Test that messages ignore boards with no critical pools."""
        data = {
            board: self._make_maximum_spares(counts)
                for board, counts in self._board_data
        }
        data['elephant'] = ((0, 0), (1, 5))
        self._check_board_inventory(data)


# _POOL_MESSAGE_TEMPLATE -
# This is a sample of the output text produced by
# _generate_pool_inventory_message().  This string is parsed by the
# tests below to construct a sample inventory that should produce
# the output, and then the output is generated and checked against
# this original sample.
#
# See the comments on _BOARD_MESSAGE_TEMPLATE above for the
# rationale on using sample text in this way.

_POOL_MESSAGE_TEMPLATE = '''
Board                    Bad  Good Total
lion                       5     6    11
tiger                      4     5     9
bear                       3     7    10
aardvark                   2     0     2
platypus                   1     1     2
'''

_POOL_ADMIN_URL = 'http://go/cros-manage-duts'



class PoolInventoryTests(unittest.TestCase):
    """Tests for `_generate_pool_inventory_message()`.

    The tests create various test inventories designed to match the
    counts in `_POOL_MESSAGE_TEMPLATE`, and assert that the
    generated message text matches the format established in the
    original message text.

    The output message text is parsed against the following grammar:
        <message> -> <intro> <pool> { "blank line" <pool> }
        <intro> ->
            Instructions to depty mentioning the admin page URL
            A blank line
        <pool> ->
            <description>
            <header line>
            <message body>
        <description> ->
            Any number of lines describing one pool
        <header line> ->
            The header line from `_POOL_MESSAGE_TEMPLATE`
        <message body> ->
            Any number of non-blank lines

    After parsing messages into the parts described above, various
    assertions are tested against the parsed output, including
    that the message body matches the body from
    `_POOL_MESSAGE_TEMPLATE`.

    Parse message text is represented as a list of strings, split on
    the `'\n'` separator.

    """

    def setUp(self):
        message_lines = _POOL_MESSAGE_TEMPLATE.split('\n')
        self._header = message_lines[1]
        self._board_lines = message_lines[2:-1]
        self._board_data = []
        for l in self._board_lines:
            items = l.split()
            board = items[0]
            good = int(items[2])
            bad = int(items[1])
            self._board_data.append((board, (good, bad)))
        self._inventory = None


    def _create_histories(self, pools, board_data):
        """Return a list suitable to create a `_LabInventory` object.

        Creates a list of `_FakeHostHistory` objects that can be
        used to create a lab inventory.  `pools` is a list of strings
        naming pools, and `board_data` is a list of tuples of the
        form
            `(board, (goodcount, badcount))`
        where
            `board` is a board name.
            `goodcount` is the number of working DUTs in the pool.
            `badcount` is the number of broken DUTs in the pool.

        @param pools       List of pools for which to create
                           histories.
        @param board_data  List of tuples containing boards and DUT
                           counts.
        @return A list of `_FakeHostHistory` objects that can be
                used to create a `_LabInventory` object.

        """
        histories = []
        status_choices = (_WORKING, _BROKEN)
        for pool in pools:
            for board, counts in board_data:
                for status, count in zip(status_choices, counts):
                    for x in range(0, count):
                        histories.append(
                            _FakeHostHistory(board, pool, status))
        return histories


    def _parse_pool_summaries(self, histories):
        """Parse message output according to the grammar above.

        Create a lab inventory from the given `histories`, and
        generate the pool inventory message.  Then parse the message
        and return a dictionary mapping each pool to the message
        body parsed after that pool.

        Tests the following assertions:
          * Each <description> contains a mention of exactly one
            pool in the `_CRITICAL_POOLS` list.
          * Each pool is mentioned in exactly one <description>.
        Note that the grammar requires the header to appear once
        for each pool, so the parsing implicitly asserts that the
        output contains the header.

        @param histories  Input used to create the test
                          `_LabInventory` object.
        @return A dictionary mapping board names to the output
                (a list of lines) for the board.

        """
        self._inventory = lab_inventory._LabInventory(histories)
        message = lab_inventory._generate_pool_inventory_message(
                self._inventory).split('\n')
        poolset = set(lab_inventory._CRITICAL_POOLS)
        seen_url = False
        seen_intro = False
        description = ''
        board_text = {}
        current_pool = None
        for line in message:
            if not seen_url:
                if _POOL_ADMIN_URL in line:
                    seen_url = True
            elif not seen_intro:
                if not line:
                    seen_intro = True
            elif current_pool is None:
                if line == self._header:
                    pools_mentioned = [p for p in poolset
                                           if p in description]
                    self.assertEqual(len(pools_mentioned), 1)
                    current_pool = pools_mentioned[0]
                    description = ''
                    board_text[current_pool] = []
                    poolset.remove(current_pool)
                else:
                    description += line
            else:
                if line:
                    board_text[current_pool].append(line)
                else:
                    current_pool = None
        self.assertEqual(len(poolset), 0)
        return board_text


    def _check_inventory_no_shortages(self, text):
        """Test a message body containing no reported shortages.

        The input `text` was created for a pool containing no
        board shortages.  Assert that the text consists of a
        single line starting with '(' and ending with ')'.

        @param text  Message body text to be tested.

        """
        self.assertTrue(len(text) == 1 and
                            text[0][0] == '(' and
                            text[0][-1] == ')')


    def _check_inventory(self, text):
        """Test a message against `_POOL_MESSAGE_TEMPLATE`.

        Test that the given message text matches the parsed
        `_POOL_MESSAGE_TEMPLATE`.

        @param text  Message body text to be tested.

        """
        self.assertEqual(text, self._board_lines)


    def test_no_shortages(self):
        """Test correct output when no pools have shortages."""
        board_text = self._parse_pool_summaries([])
        for text in board_text.values():
            self._check_inventory_no_shortages(text)


    def test_one_pool_shortage(self):
        """Test correct output when exactly one pool has a shortage."""
        for pool in lab_inventory._CRITICAL_POOLS:
            histories = self._create_histories((pool,),
                                               self._board_data)
            board_text = self._parse_pool_summaries(histories)
            for checkpool in lab_inventory._CRITICAL_POOLS:
                text = board_text[checkpool]
                if checkpool == pool:
                    self._check_inventory(text)
                else:
                    self._check_inventory_no_shortages(text)


    def test_all_pool_shortages(self):
        """Test correct output when all pools have a shortage."""
        histories = []
        for pool in lab_inventory._CRITICAL_POOLS:
            histories.extend(
                self._create_histories((pool,),
                                       self._board_data))
        board_text = self._parse_pool_summaries(histories)
        for pool in lab_inventory._CRITICAL_POOLS:
            self._check_inventory(board_text[pool])


    def test_full_board_ignored(self):
        """Test that boards at full strength are not reported."""
        pool = lab_inventory._CRITICAL_POOLS[0]
        full_board = [('echidna', (5, 0))]
        histories = self._create_histories((pool,),
                                           full_board)
        text = self._parse_pool_summaries(histories)[pool]
        self._check_inventory_no_shortages(text)
        board_data = self._board_data + full_board
        histories = self._create_histories((pool,), board_data)
        text = self._parse_pool_summaries(histories)[pool]
        self._check_inventory(text)


    def test_spare_pool_ignored(self):
        """Test that reporting ignores the spare pool inventory."""
        spare_pool = lab_inventory._SPARE_POOL
        spare_data = self._board_data + [('echidna', (0, 5))]
        histories = self._create_histories((spare_pool,),
                                           spare_data)
        board_text = self._parse_pool_summaries(histories)
        for pool in lab_inventory._CRITICAL_POOLS:
            self._check_inventory_no_shortages(board_text[pool])


class CommandParsingTests(unittest.TestCase):
    """Tests for command line argument parsing in `_parse_command()`."""

    _NULL_NOTIFY = ['--board-notify=', '--pool-notify=']

    def setUp(self):
        dirpath = '/usr/local/fubar'
        self._command_path = os.path.join(dirpath,
                                          'site_utils',
                                          'arglebargle')
        self._logdir = os.path.join(dirpath, lab_inventory._LOGDIR)


    def _parse_arguments(self, argv, notify=_NULL_NOTIFY):
        full_argv = [self._command_path] + argv + notify
        return lab_inventory._parse_command(full_argv)


    def _check_non_notify_defaults(self, notify_option):
        arguments = self._parse_arguments([], notify=[notify_option])
        self.assertEqual(arguments.duration,
                         lab_inventory._DEFAULT_DURATION)
        self.assertFalse(arguments.debug)
        self.assertEqual(arguments.logdir, self._logdir)
        self.assertEqual(arguments.boardnames, [])
        return arguments


    def test_empty_arguments(self):
        """Test that an empty argument list is an error."""
        arguments = self._parse_arguments([], notify=[])
        self.assertIsNone(arguments)


    def test_argument_defaults(self):
        """Test that option defaults match expectations."""
        arguments = self._check_non_notify_defaults(self._NULL_NOTIFY[0])
        self.assertEqual(arguments.board_notify, [''])
        self.assertEqual(arguments.pool_notify, [])
        arguments = self._check_non_notify_defaults(self._NULL_NOTIFY[1])
        self.assertEqual(arguments.board_notify, [])
        self.assertEqual(arguments.pool_notify, [''])


    def test_board_arguments(self):
        """Test that non-option arguments are returned in `boardnames`."""
        boardlist = ['aardvark', 'echidna']
        arguments = self._parse_arguments(boardlist)
        self.assertEqual(arguments.boardnames, boardlist)


    def test_debug_option(self):
        """Test parsing of the `--debug` option."""
        arguments = self._parse_arguments(['--debug'])
        self.assertTrue(arguments.debug)


    def test_duration(self):
        """Test parsing of the `--duration` option."""
        arguments = self._parse_arguments(['--duration', '1'])
        self.assertEqual(arguments.duration, 1)
        arguments = self._parse_arguments(['--duration', '11'])
        self.assertEqual(arguments.duration, 11)
        arguments = self._parse_arguments(['-d', '1'])
        self.assertEqual(arguments.duration, 1)
        arguments = self._parse_arguments(['-d', '11'])
        self.assertEqual(arguments.duration, 11)


    def _check_email_option(self, option, getlist):
        """Test parsing of e-mail address options.

        This is a helper function to test the `--board-notify` and
        `--pool-notify` options.  It tests the following cases:
          * `--option a1` gives the list [a1]
          * `--option ' a1 '` gives the list [a1]
          * `--option a1 --option a2` gives the list [a1, a2]
          * `--option a1,a2` gives the list [a1, a2]
          * `--option 'a1, a2'` gives the list [a1, a2]

        @param option  The option to be tested.
        @param getlist A function to return the option's value from
                       parsed command line arguments.

        """
        a1 = 'mumble@mumbler.com'
        a2 = 'bumble@bumbler.org'
        arguments = self._parse_arguments([option, a1], notify=[])
        self.assertEqual(getlist(arguments), [a1])
        arguments = self._parse_arguments([option, ' ' + a1 + ' '],
                                          notify=[])
        self.assertEqual(getlist(arguments), [a1])
        arguments = self._parse_arguments([option, a1, option, a2],
                                          notify=[])
        self.assertEqual(getlist(arguments), [a1, a2])
        arguments = self._parse_arguments(
                [option, ','.join([a1, a2])], notify=[])
        self.assertEqual(getlist(arguments), [a1, a2])
        arguments = self._parse_arguments(
                [option, ', '.join([a1, a2])], notify=[])
        self.assertEqual(getlist(arguments), [a1, a2])


    def test_board_notify(self):
        """Test parsing of the `--board-notify` option."""
        self._check_email_option('--board-notify',
                                 lambda a: a.board_notify)


    def test_pool_notify(self):
        """Test parsing of the `--pool-notify` option."""
        self._check_email_option('--pool-notify',
                                 lambda a: a.pool_notify)


    def test_pool_logdir(self):
        """Test parsing of the `--logdir` option."""
        logdir = '/usr/local/whatsis/logs'
        arguments = self._parse_arguments(['--logdir', logdir])
        self.assertEqual(arguments.logdir, logdir)


if __name__ == '__main__':
    # Some of the functions we test log messages.  Prevent those
    # messages from showing up in test output.
    logging.getLogger().setLevel(logging.CRITICAL)
    unittest.main()
