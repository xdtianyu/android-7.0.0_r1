# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Setup wardmodem package root and other autotest paths.
import common

import state_machine

class ModemPowerLevelMachine(state_machine.StateMachine):
    """
    The state machine that determines the functionality level of the modem.

    Setting the funtionality to different levels enables/disables various
    operations one can perform with the modem.

    """

    def __init__(self, state, transceiver, modem_conf):
        """
        @param state: The GlobalState object shared by all state machines.

        @param transceiver: The ATTransceiver object to interact with.

        @param modem_conf: A ModemConfiguration object containing the
                configuration data for the current modem.

        """
        super(ModemPowerLevelMachine, self).__init__(state, transceiver,
                                                     modem_conf)

        # Register all wardmodem responses used.
        self._add_response_function('wm_response_power_level_minimal')
        self._add_response_function('wm_response_power_level_full')
        self._add_response_function('wm_response_power_level_low')
        self._add_response_function('wm_response_power_level_factory_test')
        self._add_response_function('wm_response_power_level_offline')

        # Load configuration for this state machine.
        self._allowed_levels = modem_conf.modem_power_level_allowed_levels
        self._reset_by_default = modem_conf.modem_power_level_reset_by_default

        self._state['power_level'] = modem_conf.modem_power_level_initial_level
        self._logger.debug(self._tag_with_name('Initialized power level to %s' %
                                               self._state['power_level']))


    def get_well_known_name(self):
        """ Returns the well known name for this machine. """
        return 'modem_power_level_machine'


    # ##########################################################################
    # State machine API functions.
    def soft_reset(self):
        """
        Soft reset the modem.
        """
        # In the future, we might want to simulate a reset by hiding the udev
        # device exposed to the modemmanager.
        self._logger.info(self._tag_with_name('Soft reset called.'))
        pass


    def get_current_level(self):
        """ Return the current power level. """
        level = self._state['power_level']
        if level == 'MINIMUM':
            self._respond(self.wm_response_power_level_minimum)
        elif level == 'FULL':
            self._respond(self.wm_response_power_level_full)
        elif level == 'LOW':
            self._respond(self.wm_response_power_level_low)
        elif level == 'FACTORY_TEST':
            self._respond(self.wm_response_power_level_factory_test)
        elif level == 'OFFLINE':
            self._respond(self.wm_response_power_level_offline)
        else:
            self._raise_runtime_error('Read invalid current power level value '
                                      '|%s|', level)
        self._respond_ok()


    def set_level_minimum(self):
        """ Set the power level to MINIMUM. """
        self._set_level('MINIMUM')
        self._task_loop.post_task(
                self._registration_machine().deregister)


    def set_level_full(self):
        """ Set the power level to FULL. """
        self._set_level('FULL')
        if self._state['automatic_registration'] == 'TRUE':
            self._task_loop.post_task(
                    self._registration_machine().register)



    def set_level_low(self):
        """ Set the power level to LOW. """
        self._set_level('LOW')
        self._task_loop.post_task(
                self._registration_machine().deregister)


    # ##########################################################################
    # Helper functions.
    def _set_level(self, level, reset_code=None):
        """
        Set the power/functionality level to the specified value.

        @param level: Integer level to set the power to.

        @param reset_code: If '1', a soft reset is called on the device. Default
                value used is specified in the configuration file.

        """
        if reset_code is None:
            reset = self._reset_by_default
        elif reset_code not in ['0', '1']:
            self._raise_runtime_error("Expected reset to be '0' or '1', found "
                                      "|%s|" % str(reset))
        else:
            reset = (reset_code == '1')

        if level not in self._allowed_levels:
            self._respond_error()
            return

        if reset or level == 'RESET':
            self.soft_reset()
        if level == 'FULL':
            self._update_state({'power_level': level})
        elif level == 'LOW' or level == 'MINIMUM':
            self._update_state({
                'power_level': level,
                'registration_status': 'NOT_REGISTERED',
                'call_status': 'DISCONNECTED',
                'level_call': 0})
        self._respond_ok()


    # ##########################################################################
    # Helper methods.
    def _registration_machine(self):
        # This machine may not have been created when __init__ is executed.
        # Obtain a fresh handle everytime we want to use it.
        return self._transceiver.get_state_machine(
                'network_registration_machine')
