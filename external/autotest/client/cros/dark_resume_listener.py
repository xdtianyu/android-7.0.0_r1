# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import threading

import dbus
import dbus.mainloop.glib
import gobject


class DarkResumeListener(object):
    """Server which listens for dark resume-related DBus signals to count how
    many dark resumes we have seen since instantiation."""

    SIGNAL_NAME = 'DarkSuspendImminent'


    def __init__(self):
        dbus.mainloop.glib.threads_init()
        gobject.threads_init()

        dbus.mainloop.glib.DBusGMainLoop(set_as_default=True)
        self._bus = dbus.SystemBus()
        self._count = 0

        self._bus.add_signal_receiver(handler_function=self._saw_dark_resume,
                                      signal_name=self.SIGNAL_NAME)

        def loop_runner():
            """Handles DBus events on the system bus using the mainloop."""
            # If we just call run on this loop, the listener will hang and the test
            # will never finish. Instead, we process events as they come in. This
            # thread is set to daemon below, which means that the program will exit
            # when the main thread exits.
            loop = gobject.MainLoop()
            context = loop.get_context()
            while True:
                context.iteration(True)
        thread = threading.Thread(None, loop_runner)
        thread.daemon = True
        thread.start()
        logging.debug('Dark resume listener started')


    @property
    def count(self):
        """Number of DarkSuspendImminent events this listener has seen since its
        creation."""
        return self._count


    def _saw_dark_resume(self, unused):
        self._count += 1
