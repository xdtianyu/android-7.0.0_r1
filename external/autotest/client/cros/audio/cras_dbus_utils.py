# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""This module provides cras DBus audio utilities."""

import logging
import multiprocessing
import pprint

from autotest_lib.client.cros.audio import cras_utils


def _set_default_main_loop():
    """Sets the gobject main loop to be the event loop for DBus.

    @raises: ImportError if dbus.mainloop.glib can not be imported.

    """
    try:
        import dbus.mainloop.glib
    except ImportError, e:
        logging.exception(
                'Can not import dbus.mainloop.glib: %s. '
                'This method should only be called on Cros device.', e)
        raise
    dbus.mainloop.glib.DBusGMainLoop(set_as_default=True)


def _get_gobject():
    """Tries to import gobject.

    @returns: The imported gobject module.

    @raises: ImportError if gobject can not be imported.

    """
    try:
        import gobject
    except ImportError, e:
        logging.exception(
                'Can not import gobject: %s. This method should only be '
                'called on Cros device.', e)
        raise
    return gobject


class CrasDBusMonitorError(Exception):
    """Error in CrasDBusMonitor."""
    pass


class CrasDBusMonitor(object):
    """Monitor for DBus signal from Cras."""
    def __init__(self):
        _set_default_main_loop()
        # Acquires a new Cras interface through a new dbus.SystemBus instance
        # which has default main loop.
        self._iface = cras_utils.get_cras_control_interface(private=True)
        self._loop = _get_gobject().MainLoop()
        self._count = 0


class CrasDBusSignalListener(CrasDBusMonitor):
    """Listener for certain signal."""
    def __init__(self):
        super(CrasDBusSignalListener, self).__init__()
        self._target_signal_count = 0


    def wait_for_nodes_changed(self, target_signal_count, timeout_secs):
        """Waits for NodesChanged signal.

        @param target_signal_count: The expected number of signal.
        @param timeout_secs: The timeout in seconds.

        @raises: CrasDBusMonitorError if there is no enough signals before
                 timeout.

        """
        self._target_signal_count = target_signal_count
        signal_match = self._iface.connect_to_signal(
                'NodesChanged', self._nodes_changed_handler)
        _get_gobject().timeout_add(
                timeout_secs * 1000, self._timeout_quit_main_loop)

        # Blocks here until _nodes_changed_handler or _timeout_quit_main_loop
        # quits the loop.
        self._loop.run()

        signal_match.remove()
        if self._count < self._target_signal_count:
            raise CrasDBusMonitorError('Timeout')


    def _nodes_changed_handler(self):
        """Handler for NodesChanged signal."""
        if self._loop.is_running():
            logging.debug('Got NodesChanged signal when loop is running.')
            self._count = self._count + 1
            logging.debug('count = %d', self._count)
            if self._count >= self._target_signal_count:
                logging.debug('Quit main loop')
                self._loop.quit()
        else:
            logging.debug('Got NodesChanged signal when loop is not running.'
                          ' Ignore it')


    def _timeout_quit_main_loop(self):
        """Handler for timeout in main loop.

        @returns: False so this callback will not be called again.

        """
        if self._loop.is_running():
            logging.error('Quit main loop because of timeout')
            self._loop.quit()
        else:
            logging.debug(
                    'Got _quit_main_loop after main loop quits. Ignore it')

        return False


class CrasDBusBackgroundSignalCounter(object):
    """Controls signal counter which runs in background."""
    def __init__(self):
        self._proc = None
        self._signal_name = None
        self._counter = None
        self._parent_conn = None
        self._child_conn = None


    def start(self, signal_name):
        """Starts the signal counter in a subprocess.

        @param signal_name: The name of the signal to count.

        """
        self._signal_name = signal_name
        self._parent_conn, self._child_conn = multiprocessing.Pipe()
        self._proc = multiprocessing.Process(
                target=self._run, args=(self._child_conn,))
        self._proc.daemon = True
        self._proc.start()


    def _run(self, child_conn):
        """Runs CrasDBusCounter.

        This should be called in a subprocess.
        This blocks until parent_conn send stop command to the pipe.

        """
        self._counter = CrasDBusCounter(self._signal_name, child_conn)
        self._counter.run()


    def stop(self):
        """Stops the CrasDBusCounter by sending stop command to parent_conn.

        The result of CrasDBusCounter in its subproces can be obtained by
        reading from parent_conn.

        @returns: The count of the signal of interest.

        """
        self._parent_conn.send(CrasDBusCounter.STOP_CMD)
        return self._parent_conn.recv()


class CrasDBusCounter(CrasDBusMonitor):
    """Counter for DBus signal sent from Cras"""

    _CHECK_QUIT_PERIOD_SECS = 0.1
    STOP_CMD = 'stop'

    def __init__(self, signal_name, child_conn, ignore_redundant=True):
        """Initializes a CrasDBusCounter.

        @param signal_name: The name of the signal of interest.
        @param child_conn: A multiprocessing.Pipe which is used to receive stop
                     signal and to send the counting result.
        @param ignore_redundant: Ignores signal if GetNodes result stays the
                     same. This happens when there is change in unplugged nodes,
                     which does not affect Cras client.

        """
        super(CrasDBusCounter, self).__init__()
        self._signal_name = signal_name
        self._count = None
        self._child_conn = child_conn
        self._ignore_redundant = ignore_redundant
        self._nodes = None


    def run(self):
        """Runs the gobject main loop and listens for the signal."""
        self._count = 0

        self._nodes = cras_utils.get_cras_nodes()
        logging.debug('Before starting the counter')
        logging.debug('nodes = %s', pprint.pformat(self._nodes))

        signal_match = self._iface.connect_to_signal(
                self._signal_name, self._signal_handler)
        _get_gobject().timeout_add(
                 int(self._CHECK_QUIT_PERIOD_SECS * 1000),
                 self._check_quit_main_loop)

        logging.debug('Start counting for signal %s', self._signal_name)

        # Blocks here until _check_quit_main_loop quits the loop.
        self._loop.run()

        signal_match.remove()

        logging.debug('Count result: %s', self._count)
        self._child_conn.send(self._count)


    def _signal_handler(self):
        """Handler for signal."""
        if self._loop.is_running():
            logging.debug('Got %s signal when loop is running.',
                          self._signal_name)

            logging.debug('Getting nodes.')
            nodes = cras_utils.get_cras_nodes()
            logging.debug('nodes = %s', pprint.pformat(nodes))
            if self._ignore_redundant and self._nodes == nodes:
                logging.debug('Nodes did not change. Ignore redundant signal')
                return

            self._count = self._count + 1
            logging.debug('count = %d', self._count)
        else:
            logging.debug('Got %s signal when loop is not running.'
                          ' Ignore it', self._signal_name)


    def _should_stop(self):
        """Checks if user wants to stop main loop."""
        if self._child_conn.poll():
            if self._child_conn.recv() == self.STOP_CMD:
                logging.debug('Should stop')
                return True
        return False


    def _check_quit_main_loop(self):
        """Handler for timeout in main loop.

        @returns: True so this callback will not be called again.
                  False if user quits main loop.

        """
        if self._loop.is_running():
            logging.debug('main loop is running in _check_quit_main_loop')
            if self._should_stop():
                logging.debug('Quit main loop because of stop command')
                self._loop.quit()
                return False
            else:
                logging.debug('No stop command, keep running')
                return True
        else:
            logging.debug(
                    'Got _quit_main_loop after main loop quits. Ignore it')

            return False


class CrasDBusMonitorUnexpectedNodesChanged(Exception):
    """Error for unexpected nodes changed."""
    pass


def wait_for_unexpected_nodes_changed(timeout_secs):
    """Waits for unexpected nodes changed signal in this blocking call.

    @param timeout_secs: Timeout in seconds for waiting.

    @raises CrasDBusMonitorUnexpectedNodesChanged if there is NodesChanged
            signal

    """
    try:
        CrasDBusSignalListener().wait_for_nodes_changed(1, timeout_secs)
    except CrasDBusMonitorError:
        logging.debug('There is no NodesChanged signal, as expected')
        return
    raise CrasDBusMonitorUnexpectedNodesChanged()
