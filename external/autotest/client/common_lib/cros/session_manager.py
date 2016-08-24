# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import dbus

from autotest_lib.client.bin import utils
from autotest_lib.client.cros import constants

def connect(bus_loop):
    """Create and return a DBus connection to session_manager.

    Connects to the session manager over the DBus system bus.  Returns
    appropriately configured DBus interface object.

    @param bus_loop: An externally-owned DBusGMainLoop.

    @return a dbus.Interface object connection to the session_manager.
    """
    bus = dbus.SystemBus(mainloop=bus_loop)
    proxy = bus.get_object('org.chromium.SessionManager',
                           '/org/chromium/SessionManager')
    return dbus.Interface(proxy, 'org.chromium.SessionManagerInterface')


class SignalListener(object):
    """A class to listen for DBus signals from the session manager.

    The session_manager emits several DBus signals when different events
    of interest occur. This class provides a framework for derived classes
    to use to listen for certain signals.
    """

    def __init__(self, g_main_loop):
        """Constructor

        @param g_mail_loop: glib main loop object.
        """
        self._main_loop = g_main_loop


    def wait_for_signals(self, desc,
                         timeout=constants.DEFAULT_OWNERSHIP_TIMEOUT):
        """Block for |timeout| seconds waiting for the signals to come in.

        @param desc: string describing the high-level reason you're waiting
                     for the signals.
        @param timeout: maximum seconds to wait for the signals.

        @raises TimeoutError if the timeout is hit.
        """
        utils.poll_for_condition(
            condition=lambda: self.__received_signals(),
            desc=desc,
            timeout=timeout)
        self.reset_signal_state()


    def __received_signals(self):
        """Run main loop until all pending events are done, checks for signals.

        Runs self._main_loop until it says it has no more events pending,
        then returns the state of the internal variables tracking whether
        desired signals have been received.

        @return True if both signals have been handled, False otherwise.
        """
        self.__flush()
        return self.all_signals_received()


    def __flush(self):
        """Runs the main loop until pending events are done."""
        context = self._main_loop.get_context()
        while context.iteration(False):
            pass


    def reset(self):
        """Prepares the listener to receive a new signal.

        This resets the signal state and flushes any pending signals in order to
        avoid picking up stale signals still lingering in the process' input
        queues.
        """
        self.__flush()
        self.reset_signal_state()


    def reset_signal_state(self):
        """Resets internal signal tracking state."""
        raise NotImplementedError()


    def all_signals_received(self):
        """Resets internal signal tracking state."""
        raise NotImplementedError()


    def listen_to_signal(self, callback, signal):
        """Connect a callback to a given session_manager dbus signal.

        Sets up a signal receiver for signal, and calls the provided callback
        when it comes in.

        @param callback: a callable to call when signal is received.
        @param signal: the signal to listen for.
        """
        bus = dbus.SystemBus()
        bus.add_signal_receiver(
            handler_function=callback,
            signal_name=signal,
            dbus_interface='org.chromium.SessionManagerInterface',
            bus_name=None,
            path='/org/chromium/SessionManager')



class OwnershipSignalListener(SignalListener):
    """A class to listen for ownership-related DBus signals.

    The session_manager emits a couple of DBus signals when certain events
    related to device ownership occur.  This class provides a way to
    listen for them and check on their status.
    """

    def __init__(self, g_main_loop):
        """Constructor

        @param g_mail_loop: glib main loop object.
        """
        super(OwnershipSignalListener, self).__init__(g_main_loop)
        self._listen_for_new_key = False
        self._got_new_key = False
        self._listen_for_new_policy = False
        self._got_new_policy = False


    def listen_for_new_key_and_policy(self):
        """Set to listen for signals indicating new owner key and device policy.
        """
        self._listen_for_new_key = self._listen_for_new_policy = True
        self.listen_to_signal(self.__handle_new_key, 'SetOwnerKeyComplete')
        self.listen_to_signal(self.__handle_new_policy,
                              'PropertyChangeComplete')
        self.reset()


    def listen_for_new_policy(self):
        """Set to listen for signal indicating new device policy.
        """
        self._listen_for_new_key = False
        self._listen_for_new_policy = True
        self.listen_to_signal(self.__handle_new_policy,
                              'PropertyChangeComplete')
        self.reset()


    def reset_signal_state(self):
        """Resets internal signal tracking state."""
        self._got_new_key = not self._listen_for_new_key
        self._got_new_policy = not self._listen_for_new_policy


    def all_signals_received(self):
        """Returns true when expected signals are all receieved."""
        return self._got_new_key and self._got_new_policy


    def __handle_new_key(self, success):
        """Callback to be used when a new key signal is received.

        @param success: the string 'success' if the key was generated.
        """
        self._got_new_key = (success == 'success')


    def __handle_new_policy(self, success):
        """Callback to be used when a new policy signal is received.

        @param success: the string 'success' if the policy was stored.
        """
        self._got_new_policy = (success == 'success')



class SessionSignalListener(SignalListener):
    """A class to listen for SessionStateChanged DBus signals.

    The session_manager emits a DBus signal whenever a user signs in, when
    the user session begins termination, and when the session is terminated.
    This class allows this signal to be polled for
    """

    def __init__(self, g_main_loop):
        """Constructor

        @param g_mail_loop: glib main loop object.
        """
        super(SessionSignalListener, self).__init__(g_main_loop)
        self._new_state = None
        self._expected_state = None


    def listen_for_session_state_change(self, expected):
        """Set to listen for state changed signal with payload == |expected|.

        @param expected: string representing the state transition we expect.
                         One of 'started', 'stopping', or 'stopped'.
        """
        if expected not in {'started', 'stopping', 'stopped'}:
            raise ValueError("expected must be one of 'started', 'stopping'," +
                             " or 'stopped'.")
        self.listen_to_signal(self.__handle_signal, 'SessionStateChanged')
        self._expected_state = expected


    def reset_signal_state(self):
        """Resets internal signal tracking state."""
        self._new_state = None


    def all_signals_received(self):
        """Returns true when expected signals are all receieved."""
        return self._new_state == self._expected_state


    def __handle_signal(self, state):
        """Callback to be used when a new state-change signal is received.

        @param state: the state transition being signaled.
        """
        self._new_state = state



class ScreenIsLockedSignalListener(SignalListener):
    """A class to listen for ScreenIsLocked DBus signal.

    The session_manager emits a DBus signal when screen lock operation is
    completed.
    """

    def __init__(self, g_main_loop):
        """Constructor

        @param g_main_loop: glib main loop object.
        """
        super(ScreenIsLockedSignalListener, self).__init__(g_main_loop)
        self._screen_is_locked_received = False
        self.listen_to_signal(self.__handle_signal, 'ScreenIsLocked')


    def reset_signal_state(self):
        """Resets internal signal tracking state."""
        self._screen_is_locked_received = False


    def all_signals_received(self):
        """Returns true when expected signals are all receieved."""
        return self._screen_is_locked_received


    def __handle_signal(self):
        """Callback to be used when ScreenIsLocked signal is received.
        """
        self._screen_is_locked_received = True
