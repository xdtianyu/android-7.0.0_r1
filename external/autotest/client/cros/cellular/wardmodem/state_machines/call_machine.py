# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Setup wardmodem package root and other autotest paths.
import common

import state_machine

class CallMachine(state_machine.StateMachine):
    """
    This state machine simulates an active call over a registered network.

    """
    # ##########################################################################
    # Functions overriden from base class.
    def get_well_known_name(self):
        """ Returns the well known name for this machine. """
        return 'call_machine'


    # ##########################################################################
    # State machine API functions.
    def connect_call(self):
        """ Connect a call on a reigstered network. """
        power_level = self._state['power_level']
        if power_level == 'FULL':
            self._update_state({'call_status': 'CONNECTED'}, 3000)
            # Update level indicators
            self._update_state({'level_call': 1}, 3000)
        else:
            self._logger.info(self._tag_with_name(
                "Attempted to connect a call at power level: %s. Ignored." %
                power_level))
        self._respond_ok()


    def disconnect_call(self):
        """ Disconnect an active call on a registered network. """
        self._update_state({'call_status': 'DISCONNECTED'}, 3000)
        # Update level indicators
        self._update_state({'level_call': 0}, 3000)
        self._respond_ok()
