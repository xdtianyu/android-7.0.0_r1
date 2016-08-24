# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.


class DroneTaskQueueException(Exception):
    """Generic task queue exception."""
    pass


class DroneTaskQueue(object):
    """A manager to run queued tasks in drones and gather results from them."""

    def __init__(self):
        self.results = dict()


    def get_results(self):
        """Get a results dictionary keyed on drones.

        @return: A dictionary of return values from drones.
        """
        results_copy = self.results.copy()
        self.results.clear()
        return results_copy


    def execute(self, drones, wait=True):
        """Invoke methods via SSH to a drone.

        @param drones: A list of drones with calls to execute.
        @param wait: If True, this method will only return when all the drones
            have returned the result of their respective invocations of
            drone_utility. The `results` map will be cleared.
            If False, the caller must clear the map before the next invocation
            of `execute`, by calling `get_results`.

        @return: A dictionary keyed on the drones, containing a list of return
            values from the execution of drone_utility.

        @raises DroneTaskQueueException: If the results map isn't empty at the
            time of invocation.
        """
        if self.results:
            raise DroneTaskQueueException(
                    'Cannot clobber results map: %s, it should be cleared '
                    'through get_results.' % self.results)
        for drone in drones:
            if not drone.get_calls():
                continue
            drone_results = drone.execute_queued_calls()
            if drone in self.results:
                raise DroneTaskQueueException(
                        'Task queue has recorded results for drone %s: %s' %
                        (drone, self.results))
            self.results[drone] = drone_results
        return self.get_results() if wait else None
