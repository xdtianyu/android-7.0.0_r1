# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

class _SiteAbstractDrone(object):
    """
    This is a site subclass of _BaseAbstractDrone in drones.py. Any methods
    here automatically overload _BaseAbstractDrone and are used to create
    _AbstractDrone for consumers.
    """


    def __init__(self, timestamp_remote_calls=True):
        """
        Add a new private variable _processes_to_kill to _AbstractDrone

        @param timestamp_remote_calls: If true, drone_utility is invoked with
            the --call_time option and the current time. Currently this is only
            used for testing.
        """
        super(_SiteAbstractDrone, self).__init__(
                timestamp_remote_calls=timestamp_remote_calls)
        self._processes_to_kill = []


    def queue_kill_process(self, process):
        """Queue a process to kill/abort.

        @param process: Process to kill/abort.
        """
        self._processes_to_kill.append(process)


    def clear_processes_to_kill(self):
        """Reset the list of processes to kill for this tick."""
        self._processes_to_kill = []


    def execute_queued_calls(self):
        """Overloads execute_queued_calls().

        If there are any processes queued to kill, kill them then process the
        remaining queued up calls.
        """
        if self._processes_to_kill:
            self.queue_call('kill_processes', self._processes_to_kill)
        self.clear_processes_to_kill()
        return super(_SiteAbstractDrone, self).execute_queued_calls()
