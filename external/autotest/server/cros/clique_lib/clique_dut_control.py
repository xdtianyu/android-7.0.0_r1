# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import collections
import logging
import multiprocessing
import sys
import time

from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros.network import xmlrpc_datatypes
from autotest_lib.server.cros.network import connection_worker

"""DUT Control module is used to control all the DUT's in a Clique set.
We need to execute a sequence of steps on each DUT in the pool parallely and
collect the results from all the executions.

Class Hierarchy:
----------------
                                CliqueDUTControl
                                        |
            -------------------------------------------------------
            |                                                      |
        CliqueDUTRole                                          CliqueDUTBatch
            |                                                      |
   -------------------------------------               ---------------------
   |                                   |               |                   |
 DUTRoleConnectDisconnect DUTRoleFileTransfer     CliqueDUTSet     CliqueDUTPool

CliqueDUTControl - Base control class. Stores and retrieves test params used
for all control operations. Should never be directly instantiated.

CliqueDUTRole - Used to control one single DUT in the test. This is a base class
which should be derived to define a role to be performed by the DUT. Should
never be directly instantiated.

CliqueDUTBatch - Used to control a batch of DUT in the test. It could
either be controlling a DUT set or an entire DUT pool. Implements the setup,
cleanup and execute functions which spawn off multiple threads to
control the execution of each step in the objects controlled. Should
never be directly instantiated.

CliqueDUTSet - Used to control a set within the DUT pool. It has a number of
CliqueDUTRole objects to control.

CliqueDUTPool - Used to control the entire DUT pool. It has a number of
CliqueDUTSet objects to control.
"""


# Dummy result error reason to be used when exception is encountered in a role.
ROLE_SETUP_EXCEPTION = "Role Setup Exception! "
ROLE_EXECUTE_EXCEPTION = "Role Execute Exception! "
ROLE_CLEANUP_EXCEPTION = "Role Teardown Exception! "

# Dummy result error reason to be used when exception is encountered in a role.
POOL_SETUP_EXCEPTION = "Pool Setup Exception! "
POOL_CLEANUP_EXCEPTION = "Pool Teardown Exception! "

# Result to returned after execution a sequence of steps.
ControlResult = collections.namedtuple(
        'ControlResult', [ 'uid', 'run_num', 'success',
                           'error_reason', 'start_time', 'end_time' ])

class CliqueDUTUnknownParamError(error.TestError):
    """Indicates an error in finding a required param from the |test_params|."""
    pass


class CliqueControl(object):
    """CliqueControl is a base class which is used to control the DUT's in the
    test. Not to be directly instantiated.
    """

    def __init__(self, dut_objs, assoc_params=None, conn_worker=None,
                 test_params=None, uid=""):
        """Initialize.

        @param dut_objs: A list of objects that is being controlled by this
                         control object.
        @param assoc_params: Association paramters to be used for this control
                             object.
        @param conn_worker: ConnectionWorkerAbstract object, to run extra
                            work after successful connection.
        @param test_params: A dictionary of params to be used for executing the
                            test.
        @param uid: UID of this instance of the object. Host name for DUTRole
                    objects, Instance name for DUTBatch objects.
        """
        self._dut_objs = dut_objs
        self._test_params = test_params
        self._assoc_params = assoc_params
        self._conn_worker = conn_worker
        self._uid = uid

    def find_param(self, param_key):
        """Find the relevant param value for a role from internal dictionary.

        @param param_key: Look for the value of param_key in the dict.

        @raises CliqueDUTUnknownParamError if there is an error in lookup.
        """
        if not self._test_params.has_key(param_key):
            raise CliqueDUTUnknownParamError("Param %s not found in %s" %
                                             (param_key, self._test_params))
        return self._test_params.get(param_key)

    @property
    def dut_objs(self):
        """Returns the dut_objs controlled by the object."""
        return self._dut_objs

    @property
    def dut_obj(self):
        """Returns the first dut_obj controlled by the object."""
        return self._dut_objs[0]

    @property
    def uid(self):
        """Returns a unique identifier associated with this object. It could
        be just the hostname of the DUT in DUTRole objects or
        set-number/pool-number in DUTSet DUTPool objects.
        """
        return self._uid

    @property
    def assoc_params(self):
        """Returns the association params corresponding to the object."""
        return self._assoc_params

    @property
    def conn_worker(self):
        """Returns the connection worker corresponding to the object."""
        return self._conn_worker


    def setup(self, run_num):
        """Setup the DUT/DUT-set in the correct state before the sequence of
        actions to be taken for the role is executed.

        @param run_num: Run number of this execution.

        @returns: An instance of ControlResult corresponding to all the errors
                  that were returned by the DUT/DUT's in the DUT-set which
                  is being controlled.
        """
        pass

    def cleanup(self, run_num):
        """Cleanup the DUT/DUT-set state after the sequence of actions to be
        taken for the role is executed.

        @param run_num: Run number of this execution.

        @returns: An instance of ControlResult corresponding to all the errors
                  that were returned by the DUT/DUT's in the DUT-set which
                  is being controlled.
        """
        pass

    def execute(self, run_num):
        """Execute the sequence of actions to be taken for the role on the DUT
        /DUT-set.

        @param run_num: Run number of this execution.

        @returns: An instance of ControlResult corresponding to all the errors
                  that were returned by the DUT/DUT's in the DUT-set which
                  is being controlled.

        """
        pass


class CliqueDUTRole(CliqueControl):
    """CliqueDUTRole is a base class which defines the role entrusted to each
    DUT in the Clique Test. Not to be directly instantiated.
    """

    def __init__(self, dut, assoc_params=None, conn_worker=None,
                 test_params=None):
        """Initialize.

        @param dut: A DUTObject representing a DUT in the set.
        @param assoc_params: Association paramters to be used for this role.
        @param conn_worker: ConnectionWorkerAbstract object, to run extra
                            work after successful connection.
        @param test_params: A dictionary of params to be used for executing the
                            test.
        """
        super(CliqueDUTRole, self).__init__(
                dut_objs=[dut], assoc_params=assoc_params,
                conn_worker=conn_worker, test_params=test_params,
                uid=dut.host.hostname)

    def setup(self, run_num):
        try:
            assoc_params = self.assoc_params
            self.dut_obj.wifi_client.shill.disconnect(assoc_params.ssid)
            if not self.dut_obj.wifi_client.shill.init_test_network_state():
                result = ControlResult(uid=self.uid,
                                       run_num=run_num,
                                       success=False,
                                       error_reason="Failed to set up isolated "
                                                    "test context profile.",
                                       start_time="",
                                       end_time="")
                return result
            else:
                return None
        except Exception as e:
            result = ControlResult(uid=self.uid,
                                   run_num=run_num,
                                   success=False,
                                   error_reason=ROLE_SETUP_EXCEPTION + str(e),
                                   start_time="",
                                   end_time="")
            return result

    def cleanup(self, run_num):
        try:
            self.dut_obj.wifi_client.shill.clean_profiles()
            return None
        except Exception as e:
            result = ControlResult(uid=self.uid,
                                   run_num=run_num,
                                   success=False,
                                   error_reason=ROLE_CLEANUP_EXCEPTION + str(e),
                                   start_time="",
                                   end_time="")
            return result

    def _connect_wifi(self, run_num):
        """Helper function to make a connection to the associated AP."""
        assoc_params = self.assoc_params
        logging.info('Connection attempt %d', run_num)
        self.dut_obj.host.syslog('Connection attempt %d' % run_num)
        start_time = self.dut_obj.host.run("date '+%FT%T.%N%:z'").stdout
        start_time = start_time.strip()
        assoc_result = xmlrpc_datatypes.deserialize(
            self.dut_obj.wifi_client.shill.connect_wifi(assoc_params))
        end_time = self.dut_obj.host.run("date '+%FT%T.%N%:z'").stdout
        end_time = end_time.strip()
        success = assoc_result.success
        if not success:
            logging.error('Connection attempt %d failed; reason: %s',
                          run_num, assoc_result.failure_reason)
            result = ControlResult(uid=self.uid,
                                   run_num=run_num,
                                   success=success,
                                   error_reason=assoc_result.failure_reason,
                                   start_time=start_time,
                                   end_time=end_time)
            return result
        else:
            logging.info('Connection attempt %d passed', run_num)
            return None

    def _disconnect_wifi(self):
        """Helper function to disconnect from the associated AP."""
        assoc_params = self.assoc_params
        self.dut_obj.wifi_client.shill.disconnect(assoc_params.ssid)


# todo(rpius): Move these role implementations to a separate file since we'll
# end up having a lot of roles defined.
class DUTRoleConnectDisconnect(CliqueDUTRole):
    """DUTRoleConnectDisconnect is used to make a DUT connect and disconnect
    to a given AP repeatedly.
    """

    def execute(self, run_num):
        try:
            result = self._connect_wifi(run_num)
            if result:
                return result

            # Now disconnect from the AP.
            self._disconnect_wifi()

            return None
        except Exception as e:
            result = ControlResult(uid=self.uid,
                                   run_num=run_num,
                                   success=False,
                                   error_reason=ROLE_EXECUTE_EXCEPTION + str(e),
                                   start_time="",
                                   end_time="")
            return result


class DUTRoleConnectDuration(CliqueDUTRole):
    """DUTRoleConnectDuration is used to make a DUT connect to a given AP and
    then check the liveness of the connection from another worker device.
    """

    def setup(self, run_num):
        result = super(DUTRoleConnectDuration, self).setup(run_num)
        if result:
            return result
        # Let's check for the worker client now.
        if not self.conn_worker:
            return ControlResult(uid=self.uid,
                                 run_num=run_num,
                                 success=False,
                                 error_reason="No connection worker found",
                                 start_time="",
                                 end_time="")

    def execute(self, run_num):
        try:
            result = self._connect_wifi(run_num)
            if result:
                return result

            # Let's start the ping from the worker client.
            worker = connection_worker.ConnectionDuration.create_from_parent(
                    self.conn_worker)
            worker.run(self.dut_obj.wifi_client)

            return None
        except Exception as e:
            result = ControlResult(uid=self.uid,
                                   run_num=run_num,
                                   success=False,
                                   error_reason=ROLE_EXECUTE_EXCEPTION + str(e),
                                   start_time="",
                                   end_time="")
            return result


def dut_batch_worker(dut_control_obj, method, error_results_queue, run_num):
    """The method called by multiprocessing worker pool for running the DUT
    control object's setup/execute/cleanup methods. This function is the
    function which is repeatedly scheduled for each DUT/DUT-set through the
    multiprocessing worker. This has to be defined outside the class because it
    needs to be pickleable.

    @param dut_control_obj: An object corresponding to DUT/DUT-set to control.
    @param method: Method name to be invoked on the dut_control_obj.
                   it has to be one of setup/execute/teardown.
    @param error_results_queue: Queue to put the error results after test.
    @param run_num: Run number of this execution.
    """
    logging.info("%s: Running %s", dut_control_obj.uid, method)
    run_method = getattr(dut_control_obj, method, None)
    if callable(run_method):
        result = run_method(run_num)
        if result:
            error_results_queue.put(result)


class CliqueDUTBatch(CliqueControl):
    """CliqueDUTBatch is a base class which is used to control a batch of DUTs.
    This could either be a DUT set or the entire DUT pool. Not to be directly
    instantiated.
    """
    # Used to store the instance number of derived classes.
    BATCH_UID_NUM = {}

    def __init__(self, dut_objs, test_params=None):
        """Initialize.

        @param dut_objs: A list of DUTRole objects representing the DUTs in set.
        @param test_params: A dictionary of params to be used for executing the
                            test.
        """
        uid_num = self.BATCH_UID_NUM.get(self.__class__.__name__, 1)
        uid = self.__class__.__name__ + str(uid_num)
        self.BATCH_UID_NUM[self.__class__.__name__] = uid_num + 1
        super(CliqueDUTBatch, self).__init__(
                dut_objs=dut_objs, test_params=test_params, uid=uid)

    def _spawn_worker_threads(self, method, run_num):
        """Spawns multiple threads to run the the |method(run_num)| on all the
        control objects in parallel.

        @param method: Method to be invoked on the dut_objs.
        @param run_num: Run number of this execution.

        @returns: An instance of ControlResult corresponding to all the errors
                  that were returned by the DUT/DUT's in the DUT-set which
                  is being controlled.
        """
        tasks = []
        error_results_queue = multiprocessing.Queue()
        for dut_obj in self.dut_objs:
            task = multiprocessing.Process(
                    target=dut_batch_worker,
                    args=(dut_obj, method, error_results_queue, run_num))
            tasks.append(task)
        # Run the tasks in parallel.
        for task in tasks:
            task.start()
        for task in tasks:
            task.join()
        error_results = []
        while not error_results_queue.empty():
            result = error_results_queue.get()
            # error_results returned at the DUT set level will be a list of
            # ControlResult objects from each of the DUTs in the set.
            # error_results returned at the DUT pool level will be a list of
            # lists from each DUT set. Let's flatten out the list in that case
            # since there could be ControlResult objects that are generated at
            # the pool or set level which will make the final error result list
            # assymetric where some elements are lists of ControlResult objects
            # and some are just ControlResult objects.
            if isinstance(result, list):
                error_results.extend(result)
            else:
                error_results.append(result)
        return error_results

    def setup(self, run_num):
        """Setup the DUT-set/pool in the correct state before the sequence of
        actions to be taken for the role is executed.

        @param run_num: Run number of this execution.

        @returns: An instance of ControlResult corresponding to all the errors
                  that were returned by the DUT/DUT's in the DUT-set which
                  is being controlled.
        """
        return self._spawn_worker_threads("setup", run_num)

    def cleanup(self, run_num):
        """Cleanup the DUT-set/pool state after the sequence of actions to be
        taken for the role is executed.

        @param run_num: Run number of this execution.

        @returns: An instance of ControlResult corresponding to all the errors
                  that were returned by the DUT/DUT's in the DUT-set which
                  is being controlled.
        """
        return self._spawn_worker_threads("cleanup", run_num)

    def execute(self, run_num):
        """Execute the sequence of actions to be taken for the role on the
        DUT-set/pool.

        @param run_num: Run number of this execution.

        @returns: An instance of ControlResult corresponding to all the errors
                  that were returned by the DUT/DUT's in the DUT-set which
                  is being controlled.

        """
        return self._spawn_worker_threads("execute", run_num)


class CliqueDUTSet(CliqueDUTBatch):
    """CliqueDUTSet is an object which is used to control all the DUT's in a DUT
    set.
    """
    def setup(self, run_num):
        # Placeholder to add any set specific actions.
        return super(CliqueDUTSet, self).setup(run_num)

    def cleanup(self, run_num):
        # Placeholder to add any set specific actions.
        return super(CliqueDUTSet, self).cleanup(run_num)

    def execute(self, run_num):
        # Placeholder to add any set specific actions.
        return super(CliqueDUTSet, self).execute(run_num)


class CliqueDUTPool(CliqueDUTBatch):
    """CliqueDUTSet is an object which is used to control all the DUT-sets in a
    DUT pool.
    """

    def setup(self, run_num):
        # Let's start the packet capture before we kick off the entire pool
        # execution.
        try:
            capturer = self.find_param('capturer')
            capturer_frequency = self.find_param('capturer_frequency')
            capturer_ht_type = self.find_param('capturer_ht_type')
            capturer.start_capture(capturer_frequency, ht_type=capturer_ht_type)
        except Exception as e:
            result = ControlResult(uid=self.uid,
                                   run_num=run_num,
                                   success=False,
                                   error_reason=POOL_SETUP_EXCEPTION + str(e),
                                   start_time="",
                                   end_time="")
            # We cannot proceed with the test if this failed.
            return result
        # Now execute the setup on all the DUT-sets.
        return super(CliqueDUTPool, self).setup(run_num)

    def cleanup(self, run_num):
        # First execute the cleanup on all the DUT-sets.
        results = super(CliqueDUTPool, self).cleanup(run_num)
        # Now stop the packet capture.
        try:
            capturer = self.find_param('capturer')
            filename = str('connect_try_%d.trc' % (run_num)),
            capturer.stop_capture(save_dir=self.outputdir,
                                  save_filename=filename)
        except Exception as e:
            result = ControlResult(uid=self.uid,
                                   run_num=run_num,
                                   success=False,
                                   error_reason=POOL_CLEANUP_EXCEPTION + str(e),
                                   start_time="",
                                   end_time="")
            if results:
                results.append(result)
            else:
                results = result
        return results

    def execute(self, run_num):
        # Placeholder to add any pool specific actions.
        return super(CliqueDUTPool, self).execute(run_num)


def execute_dut_pool(dut_pool, dut_role_classes, assoc_params_list,
                     conn_workers, test_params, num_runs=1):

    """Controls the DUT's in a given test scenario. The DUT's are assigned a
    role according to the dut_role_classes provided for each DUT-set and all of
    the sequence of steps are executed parallely on all the DUT's in the pool.

    @param dut_pool: 2D list of DUT objects corresponding to the DUT's in the
                    DUT pool.
    @param dut_role_classes: List of roles to be assigned to each set in the DUT
                             pool. Each element has to be a derived class of
                             CliqueDUTRole.
    @param assoc_params_list: List of association parameters corrresponding
                              to the AP to test against for each set in the
                              DUT.
    @param conn_workers: List of ConnectionWorkerAbstract objects, to
                         run extra work after successful connection.
    @param test_params: List of params to be used for the test.
    @num_runs: Number of iterations of the test to be run.
    """
    # Every DUT set in the pool needs to have a corresponding DUT role,
    # association parameters and connection worker assigned from the test.
    # It is the responsibilty of the test scenario to make sure that there is a
    # one to one mapping of all these elements since DUT control is going to
    # be generic.
    # This might mean that the test needs to duplicate the association
    # parameters in the list if there is only 1 AP and 2 DUT sets.
    # Or if there is no connection worker required, then the test should create
    # a list of 'None' objects with length of 2.
    # DUT control does not care if the same AP is used for 2 DUT sets or if the
    # same connection worker is shared across 2 DUT sets as long as the
    # length of the lists are equal.

    if ((len(dut_pool) != len(dut_role_classes)) or
        (len(dut_pool) != len(assoc_params_list)) or
        (len(dut_pool) != len(conn_workers))):
        raise error.TestError("Incorrect test configuration. Num DUT sets: %d, "
                              "Num DUT roles: %d, Num association params: %d, "
                              "Num connection workers: %d" %
                              (len(dut_pool), len(dut_role_classes),
                               len(assoc_params_list), len(conn_workers)))

    dut_set_control_objs = []
    for dut_set, dut_role_class, assoc_params, conn_worker in \
        zip(dut_pool, dut_role_classes, assoc_params_list, conn_workers):
        dut_control_objs = []
        for dut in dut_set:
            dut_control_obj = dut_role_class(
                    dut, assoc_params, conn_worker, test_params)
            dut_control_objs.append(dut_control_obj)
        dut_set_control_obj = CliqueDUTSet(dut_control_objs, test_params)
        dut_set_control_objs.append(dut_set_control_obj)
    dut_pool_control_obj = CliqueDUTPool(dut_set_control_objs, test_params)

    for run_num in range(0, num_runs):
        # This setup, execute, cleanup calls on pool object, results in parallel
        # invocation of call on all the DUT-sets which in turn results in
        # parallel invocation of call on all the DUTs.
        error_results = dut_pool_control_obj.setup(run_num)
        if error_results:
            return error_results

        error_results = dut_pool_control_obj.execute(run_num)
        if error_results:
            # Try to cleanup before we leave.
            dut_pool_control_obj.cleanup(run_num)
            return error_results

        error_results = dut_pool_control_obj.cleanup(run_num)
        if error_results:
            return error_results
    return None
