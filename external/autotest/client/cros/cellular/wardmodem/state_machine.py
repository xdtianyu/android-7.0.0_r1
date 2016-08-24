# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import keyword
import logging
import re

import task_loop
import wardmodem_exceptions as wme

class StateMachine(object):
    """
    Base class for all state machines in wardmodem.

    All derived objects bundled as part of wardmodem
        (1) Reside in state_machines/
        (2) Have their own module e.g., my_module
        (3) The main state machine class in my_module is called MyModule.

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
        self._state = state
        self._transceiver = transceiver
        self._modem_conf = modem_conf

        self._logger = logging.getLogger(__name__)
        self._task_loop = task_loop.get_instance()

        self._state_update_tag = 0  # Used to tag logs of async updates to
                                    # state.

        # Will raise an exception if this machine should not be instantiated.
        self.get_well_known_name()

        # Add all wardmodem response functions used by this machine.
        self._add_response_function('wm_response_ok')
        self._add_response_function('wm_response_error')
        self._add_response_function('wm_response_ring')
        self._add_response_function('wm_response_text_only')


    # ##########################################################################
    # Subclasses must override these.
    def get_well_known_name(self):
        """
        A well known name of the completely specified state machine.

        The first derived class that completely specifies some state machine
        should implement this function to return the name of the defining module
        as a string.

        """
        # Do not use self._raise_setup_error because it causes infinite
        # recursion.
        raise wme.WardModemSetupException(
                'Attempted to get well known name for a state machine that is '
                'not completely specified.')


    # ##########################################################################
    # Protected convenience methods to be used as is by subclasses.

    def _respond(self, response, response_delay_ms=0, *response_args):
        """
        Respond to the modem after some delay.

        @param reponse: String response. This must be one of the response
                strings recognized by ATTransceiver.

        @param response_delay_ms: Delay in milliseconds after which the response
                should be sent. Type: int.

        @param *response_args: The arguments for the response.

        @requires: response_delay_ms >= 0

        """
        assert response_delay_ms >= 0
        dbgstr = self._tag_with_name(
                'Will respond with "%s(%s)" after %d ms.' %
                (response, str(response_args), response_delay_ms))
        self._logger.debug(dbgstr)
        self._task_loop.post_task_after_delay(
                self._transceiver.process_wardmodem_response,
                response_delay_ms,
                response,
                *response_args)


    def _update_state(self, state_update, state_update_delay_ms=0):
        """
        Post a (delayed) state update.

        @param state_update: The state update to apply. This is a map {string
                --> state enum} that specifies all the state components to be
                updated.

        @param state_update_delay_ms: Delay in milliseconds after which the
                state update should be applied. Type: int.

        @requires: state_update_delay_ms >= 0

        """
        assert state_update_delay_ms >= 0
        dbgstr = self._tag_with_name(
                '[tag:%d] Will update state as %s after %d ms.' %
                (self._state_update_tag, str(state_update),
                 state_update_delay_ms))
        self._logger.debug(dbgstr)
        self._task_loop.post_task_after_delay(
                self._update_state_callback,
                state_update_delay_ms,
                state_update,
                self._state_update_tag)
        self._state_update_tag += 1


    def _respond_ok(self):
        """ Convenience function to respond when everything is OK. """
        self._respond(self.wm_response_ok, response_delay_ms=0)


    def _respond_error(self):
        """ Convenience function to respond when an error occured. """
        self._respond(self.wm_response_error, response_delay_ms=0)


    def _respond_ring(self):
        """ Convenience function to respond with RING. """
        self._respond(self.wm_response_ring, response_delay_ms=0)


    def _respond_with_text(self, text):
        """ Send back just |text| as the response, without any AT prefix. """
        self._respond(self.wm_response_text_only, 0, text)


    def _add_response_function(self, function):
        """
        Add a response used by this state machine to send to the ATTransceiver.

        A state machine should register all the responses it will use in its
        __init__ function by calling
            self._add_response_function('wm_response_dummy')
        The response can then be used to respond to the transceiver thus:
            self._respond(self.wm_response_dummy)

        @param function: The string function name to add. Must be a valid python
                identifier in lowercase.
                Also, these names are conventionally named matching the re
                'wm_response_([a-z0-9]*[_]?)*'

        @raises: WardModemSetupError if the added response function is ill
                formed.

        """
        if not re.match('wm_response_([a-z0-9]*[_]?)*', function) or \
           keyword.iskeyword(function):
            self._raise_setup_error('Response function name ill-formed: |%s|' %
                                    function)
        try:
            getattr(self, function)
            self._raise_setup_error(
                    'Attempted to add response function %s which already '
                    'exists.' % function)
        except AttributeError:  # OK, This is the good case.
            setattr(self, function, function)


    def _raise_setup_error(self, errstring):
        """
        Log the error and raise WardModemSetupException.

        @param errstring: The error string.

        """
        errstring = self._tag_with_name(errstring)
        self._logger.error(errstring)
        raise wme.WardModemSetupException(errstring)


    def _raise_runtime_error(self, errstring):
        """
        Log the error and raise StateMachineException.

        @param errstring: The error string.

        """
        errstring = self._tag_with_name(errstring)
        self._logger.error(errstring)
        raise wme.StateMachineException(errstring)

    def _tag_with_name(self, log_string):
        """
        If possible, prepend the log string with the well know name of the
        object.

        @param log_string: The string to modify.

        @return: The modified string.

        """
        name = self.get_well_known_name()
        log_string = '[' + name + '] ' + log_string
        return log_string

    # ##########################################################################
    # Private methods not to be used by subclasses.

    def _update_state_callback(self, state_update, tag):
        """
        Actually update the state.

        @param state_update: The state update to effect. This is a map {string
                --> state enum} that specifies all the state components to be
                updated.

        @param tag: The tag for this state update.

        @raises: StateMachineException if the state update fails.

        """
        dbgstr = self._tag_with_name('[tag:%d] State update applied.' % tag)
        self._logger.debug(dbgstr)
        for component, value in state_update.iteritems():
            self._state[component] = value
