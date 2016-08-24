#!/usr/bin/python
#
# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Unit tests for server/cros/dynamic_suite/host_spec.py."""

import mox
import unittest

import common

from autotest_lib.server.cros.dynamic_suite import host_spec
from autotest_lib.server.cros.dynamic_suite.fakes import FakeHost


class HostSpecTest(mox.MoxTestBase):
    """Unit tests for dynamic_suite.host_spec module.

    @var _BOARD: fake board to reimage
    """


    _BOARD = 'board'
    _SPECS = [host_spec.HostSpec([_BOARD]),
              host_spec.HostSpec([_BOARD, 'pool:bvt']),
              host_spec.HostSpec([_BOARD], ['label1'])]


    def testOrderSpecsByComplexity(self):
        """Should return new host spec list with simpler entries later."""
        reordered = host_spec.order_by_complexity(self._SPECS)

        for spec in self._SPECS[1:]:
            self.assertTrue(spec in reordered[:-1])
        self.assertEquals(self._SPECS[0], reordered[-1])


    def testSpecSubsets(self):
        """Validate HostSpec subset checks."""
        self.assertTrue(self._SPECS[0].is_subset(self._SPECS[1]))
        self.assertTrue(self._SPECS[0].is_subset(self._SPECS[2]))
        self.assertFalse(self._SPECS[1].is_subset(self._SPECS[2]))
        self.assertFalse(self._SPECS[2].is_subset(self._SPECS[1]))
        self.assertFalse(self._SPECS[1].is_subset(self._SPECS[0]))
        self.assertFalse(self._SPECS[2].is_subset(self._SPECS[0]))


    def testTrivialSpec(self):
        """Validate that HostSpecs are correctly marked as trivial."""
        self.assertTrue(self._SPECS[0].is_trivial)
        self.assertTrue(self._SPECS[1].is_trivial)
        self.assertFalse(self._SPECS[2].is_trivial)


class HostGroupTest(mox.MoxTestBase):
    """Unit tests for dynamic_suite.host_spec.HostGroup derived classes.
    """


    def testCanConstructExplicit(self):
        """Should be able to make an ExplicitHostGroup."""
        host_list = [FakeHost('h1'), FakeHost('h2'), FakeHost('h3')]
        hosts_per_spec = {host_spec.HostSpec(['l1']): host_list[:1],
                          host_spec.HostSpec(['l2']): host_list[1:]}
        group = host_spec.ExplicitHostGroup(hosts_per_spec)
        for host in host_list:
            self.assertTrue(host.hostname in group.as_args()['hosts'])


    def testExplicitEnforcesHostUniqueness(self):
        """Should fail to make ExplicitHostGroup with duplicate hosts."""
        host_list = [FakeHost('h1'), FakeHost('h2'), FakeHost('h3')]
        hosts_per_spec = {host_spec.HostSpec(['l1']): host_list[:1],
                          host_spec.HostSpec(['l2']): host_list}
        self.assertRaises(ValueError,
                          host_spec.ExplicitHostGroup, hosts_per_spec)


    def testCanConstructByMetahostsWithDependencies(self):
        """Should be able to make a HostGroup from labels."""
        labels = ['meta_host', 'dep1', 'dep2']
        num = 3
        group = host_spec.MetaHostGroup(labels, num)
        args = group.as_args()
        self.assertEquals(labels[:1] * num, args['meta_hosts'])
        self.assertEquals(labels[1:], args['dependencies'])


    def testExplicitCanTrackSuccess(self):
        """Track success/failure in an ExplicitHostGroup."""
        host_list = [FakeHost('h1'), FakeHost('h2'), FakeHost('h3')]
        specs = [host_spec.HostSpec(['l1']), host_spec.HostSpec(['l2'])]
        hosts_per_spec = {specs[0]: host_list[:1], specs[1]: host_list[1:]}
        group = host_spec.ExplicitHostGroup(hosts_per_spec)

        # Reimage just the one host that satisfies specs[0].
        group.mark_host_success(host_list[0].hostname)
        self.assertTrue(group.enough_hosts_succeeded())
        self.assertTrue(specs[1] in group.doomed_specs)

        # Reimage some host that satisfies specs[1].
        group.mark_host_success(host_list[2].hostname)
        self.assertTrue(group.enough_hosts_succeeded())
        self.assertFalse(group.doomed_specs)


    def testExplicitCanTrackSuccessWithSupersets(self):
        """Track success/failure in an ExplicitHostGroup with supersets."""
        host_list = [FakeHost('h1'), FakeHost('h2'), FakeHost('h3')]
        specs = [host_spec.HostSpec(['l1']),
                 host_spec.HostSpec(['l2']),
                 host_spec.HostSpec(['l2', 'l1'])]
        hosts_per_spec = {specs[0]: host_list[:1],
                          specs[1]: host_list[1:2],
                          specs[2]: host_list[2:]}
        group = host_spec.ExplicitHostGroup(hosts_per_spec)

        # Reimage just the one host that satisfies specs[2].
        # Because satisfying specs[2] statisfies all the specs, we should have
        # no doomed specs.
        group.mark_host_success(host_list[2].hostname)
        self.assertTrue(group.enough_hosts_succeeded())
        self.assertFalse(group.doomed_specs)


    def testExplicitCanTrackUnsatisfiedSpecs(self):
        """Track unsatisfiable HostSpecs in ExplicitHostGroup."""
        group = host_spec.ExplicitHostGroup()
        satisfiable_spec = host_spec.HostSpec(['l2'])
        unsatisfiable_spec = host_spec.HostSpec(['l1'], ['e1'])
        group.add_host_for_spec(unsatisfiable_spec, None)
        group.add_host_for_spec(satisfiable_spec, FakeHost('h1'))
        self.assertTrue(unsatisfiable_spec in group.unsatisfied_specs)
        self.assertTrue(satisfiable_spec not in group.unsatisfied_specs)


    def testExplicitOneHostEnoughToSatisfySpecs(self):
        """One host is enough to satisfy a HostSpec in ExplicitHostGroup."""
        satisfiable_spec = host_spec.HostSpec(['l1'])
        group = host_spec.ExplicitHostGroup()
        group.add_host_for_spec(satisfiable_spec, FakeHost('h1'))
        group.add_host_for_spec(satisfiable_spec, None)
        self.assertTrue(satisfiable_spec not in group.unsatisfied_specs)

        group = host_spec.ExplicitHostGroup()
        group.add_host_for_spec(satisfiable_spec, None)
        group.add_host_for_spec(satisfiable_spec, FakeHost('h1'))
        self.assertTrue(satisfiable_spec not in group.unsatisfied_specs)


    def testExplicitSubsetSpecSatisfiedIfAnyAre(self):
        """Ensures that any satisfied spec also satisfies a subset HostSpec."""
        specs = [host_spec.HostSpec(['l1'], ['l3']),
                 host_spec.HostSpec(['l1'], ['l3', 'l4']),
                 host_spec.HostSpec(['l1'], ['l5', 'l4']),
                 host_spec.HostSpec(['l1'], ['l2', 'l3', 'l4'])]
        group = host_spec.ExplicitHostGroup()
        group.add_host_for_spec(specs[0], None)
        group.add_host_for_spec(specs[1], FakeHost('h1'))
        group.add_host_for_spec(specs[2], FakeHost('h2'))
        group.add_host_for_spec(specs[3], None)

        self.assertTrue(specs[0] not in group.unsatisfied_specs)
        self.assertTrue(specs[1] not in group.unsatisfied_specs)
        self.assertTrue(specs[2] not in group.unsatisfied_specs)
        self.assertTrue(specs[3] in group.unsatisfied_specs)


    def testMetaCanTrackSuccess(self):
        """Track success/failure in a MetaHostGroup."""
        labels = ['meta_host', 'dep1', 'dep2']
        num = 3
        group = host_spec.MetaHostGroup(labels, num)

        self.assertFalse(group.enough_hosts_succeeded())

        group.mark_host_success('h1')
        self.assertTrue(group.enough_hosts_succeeded())


if __name__ == '__main__':
    unittest.main()
