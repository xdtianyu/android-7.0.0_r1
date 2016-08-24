# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Setup wardmodem package root and other autotest paths.
import common

import state_machine

class LevelIndicatorsMachine(state_machine.StateMachine):
    """
    Machine that maintains the information about different indications that are
    usually shown by the phone on UI.

    """
    def __init__(self, state, transceiver, modem_conf):
        """
        @param state: The GlobalState object shared by all state machines.

        @param transceiver: The ATTransceiver object to interact with.

        @param modem_conf: A modem configuration object that contains
                configuration data for different state machines.

        @raises: SetupException if we attempt to create an instance of a machine
        that has not been completely specified (see get_well_known_name).

        """
        super(LevelIndicatorsMachine, self).__init__(state, transceiver,
                                                     modem_conf)

        # Add all wardmodem response functions used by this machine.
        self._add_response_function('wm_response_level_indicators')

        # Load configuration for this machine and initialize relevant
        # GlobalState components.
        self._supported_indicators = []
        for item in self._modem_conf.level_indicators_items:
            self._supported_indicators.append('level_' + item)

        items = self._supported_indicators
        defaults = self._modem_conf.level_indicators_defaults
        if len(items) != len(defaults):
            self._raise_setup_error(
                    'Indicator list and its defaults must be of the same '
                    'length: Items:|%s|, Defaults:|%s|' % (str(items),
                                                           str(defaults)))
        for index in range(len(items)):
            self._state[items[index]] = defaults[index]


    def get_well_known_name(self):
        """ Returns the well known name for this machine. """
        return 'level_indicators_machine'


    # ##########################################################################
    # State machine API functions.
    def get_current_levels(self):
        levels = []
        for indicator in self._supported_indicators:
            levels.append(str(self._state[indicator]))
        self._respond(self.wm_response_level_indicators, 0, *levels)
        self._respond_ok()
