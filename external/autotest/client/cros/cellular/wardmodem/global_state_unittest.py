# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import global_state

import logging
import unittest

import wardmodem_exceptions as wme

class GlobalStateSkeletonBadTestCase(unittest.TestCase):
    """
    Test failing derivations of GlobalStateSkeleton.

    """

    def test_duplicate_component_name(self):
        """
        Try (unsuccessfully) to add two components with the same name.

        """
        state = global_state.GlobalStateSkeleton()
        state._add_state_component('common_name', ['THIS_IS_FINE'])
        self.assertRaises(wme.WardModemSetupException,
                          state._add_state_component,
                          'common_name',
                          ['THIS_IS_NOT_FINE'])


    def test_ill_formed_names(self):
        """
        Try (unsuccessfully) to add components with ill formed names or ill
        formed values.

        """
        state = global_state.GlobalStateSkeleton()
        self.assertRaises(TypeError,
                          state._add_state_component,
                          'this_is_fine',
                          'must_have_been_list')
        self.assertRaises(wme.WardModemSetupException,
                          state._add_state_component,
                          'ill formed',
                          ['NO_SPACES'])
        self.assertRaises(wme.WardModemSetupException,
                          state._add_state_component,
                          '',
                          ['CANT_BE_EMPTY'])
        self.assertRaises(wme.WardModemSetupException,
                          state._add_state_component,
                          'ILL_FORMED',
                          ['MUST_BE_LOWERCASE'])
        self.assertRaises(wme.WardModemSetupException,
                          state._add_state_component,
                          'no_spaces',
                          ['ILL FORMED'])
        self.assertRaises(wme.WardModemSetupException,
                          state._add_state_component,
                          'cant_be_empty',
                          [''])
        self.assertRaises(wme.WardModemSetupException,
                          state._add_state_component,
                          'use_int_when_you_want_numbers',
                          ['2'])
        self.assertRaises(wme.WardModemSetupException,
                          state._add_state_component,
                          'must_be_uppercase',
                          ['ill_formed'])


    def test_valid_names(self):
        """
        Some examples of correct component additions.

        """
        state = global_state.GlobalStateSkeleton()

        state._add_state_component('this_is_fine', ['A', 'B', 'C'])
        state._add_state_component('so_is_this', [1, 1, 2, 3, 5, 8, 13])
        state._add_state_component('and_even_this_guy', ['A', 'B3B_CC', 34])


class GlobalStateSkeletonTestCase(unittest.TestCase):
    """
    Test the basic functionality of GlobalStateSkeleton, assuming that it is
    derived without errors.

    """

    class TestGlobalState(global_state.GlobalStateSkeleton):
        """
        This class will correctly derive from GlobalStateSkeleton.

        """

        def __init__(self):
            super(GlobalStateSkeletonTestCase.TestGlobalState, self).__init__()
            # Now, add all state components.
            self._add_state_component('comp1', ['ALLOWED_VALUE_1_1'])
            self._add_state_component('comp2', ['ALLOWED_VALUE_2_1',
                                                'ALLOWED_VALUE_2_2'])
            # No value can ever be assigned to this. Ah, what the heck!
            self._add_state_component('comp3', [])


    def setUp(self):
        self.state = GlobalStateSkeletonTestCase.TestGlobalState()


    def test_successful_read_write(self):
        """
        Test that all values are initialized correctly.

        """
        self.assertEqual(self.state.INVALID_VALUE, self.state['comp1'])
        self.assertEqual(self.state.INVALID_VALUE, self.state['comp2'])
        self.assertEqual(self.state.INVALID_VALUE, self.state['comp3'])

        self.state['comp2'] = 'ALLOWED_VALUE_2_1'
        self.assertEqual('ALLOWED_VALUE_2_1', self.state['comp2'])
        self.state['comp2'] = 'ALLOWED_VALUE_2_2'
        self.assertEqual('ALLOWED_VALUE_2_2', self.state['comp2'])
        self.state['comp1'] = 'ALLOWED_VALUE_1_1'
        self.assertEqual('ALLOWED_VALUE_1_1', self.state['comp1'])


    def _read(self, key):
        """Wrap the read from state to check exceptions raised."""
        return self.state[key]

    def _write(self, key, value):
        """Wrap the assignment to state to check exceptions raised."""
        self.state[key] = value


    def test_failed_read_write(self):
        """
        Attempt to read/write invalid values.

        """
        self.assertRaises(wme.StateMachineException,
                          self._read, 'some_invalid_var')
        self.assertRaises(wme.StateMachineException,
                          self._write, 'some_invalide_var', '')
        self.assertRaises(wme.StateMachineException,
                          self._write, 'comp1', 'DOES_NOT_EXIST')


if __name__ == '__main__':
    logging.basicConfig(level=logging.DEBUG)
    unittest.main()
