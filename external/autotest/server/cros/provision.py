# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.


import abc

import common
from autotest_lib.server.cros import provision_actionables as actionables


### Constants for label prefixes
CROS_VERSION_PREFIX = 'cros-version'
ANDROID_BUILD_VERSION_PREFIX = 'ab-version'
FW_RW_VERSION_PREFIX = 'fwrw-version'
FW_RO_VERSION_PREFIX = 'fwro-version'

# Default number of provisions attempts to try if we believe the devserver is
# flaky.
FLAKY_DEVSERVER_ATTEMPTS = 2


### Helpers to convert value to label
def cros_version_to_label(image):
    """
    Returns the proper label name for a ChromeOS build of |image|.

    @param image: A string of the form 'lumpy-release/R28-3993.0.0'
    @returns: A string that is the appropriate label name.

    """
    return CROS_VERSION_PREFIX + ':' + image


def fwro_version_to_label(image):
    """
    Returns the proper label name for a RO firmware build of |image|.

    @param image: A string of the form 'lumpy-release/R28-3993.0.0'
    @returns: A string that is the appropriate label name.

    """
    return FW_RO_VERSION_PREFIX + ':' + image


def fwrw_version_to_label(image):
    """
    Returns the proper label name for a RW firmware build of |image|.

    @param image: A string of the form 'lumpy-release/R28-3993.0.0'
    @returns: A string that is the appropriate label name.

    """
    return FW_RW_VERSION_PREFIX + ':' + image


class _SpecialTaskAction(object):
    """
    Base class to give a template for mapping labels to tests.
    """

    __metaclass__ = abc.ABCMeta


    # One cannot do
    #     @abc.abstractproperty
    #     _actions = {}
    # so this is the next best thing
    @abc.abstractproperty
    def _actions(self):
        """A dictionary mapping labels to test names."""
        pass


    @abc.abstractproperty
    def name(self):
        """The name of this special task to be used in output."""
        pass


    @classmethod
    def acts_on(cls, label):
        """
        Returns True if the label is a label that we recognize as something we
        know how to act on, given our _actions.

        @param label: The label as a string.
        @returns: True if there exists a test to run for this label.

        """
        return label.split(':')[0] in cls._actions


    @classmethod
    def test_for(cls, label):
        """
        Returns the test associated with the given (string) label name.

        @param label: The label for which the action is being requested.
        @returns: The string name of the test that should be run.
        @raises KeyError: If the name was not recognized as one we care about.

        """
        return cls._actions[label]


    @classmethod
    def partition(cls, labels):
        """
        Filter a list of labels into two sets: those labels that we know how to
        act on and those that we don't know how to act on.

        @param labels: A list of strings of labels.
        @returns: A tuple where the first element is a set of unactionable
                  labels, and the second element is a set of the actionable
                  labels.
        """
        capabilities = set()
        configurations = set()

        for label in labels:
            if cls.acts_on(label):
                configurations.add(label)
            else:
                capabilities.add(label)

        return capabilities, configurations


class Verify(_SpecialTaskAction):
    """
    Tests to verify that the DUT is in a sane, known good state that we can run
    tests on.  Failure to verify leads to running Repair.
    """

    _actions = {
        'modem_repair': actionables.TestActionable('cellular_StaleModemReboot'),
        # TODO(crbug.com/404421): set rpm action to power_RPMTest after the RPM
        # is stable in lab (destiny). The power_RPMTest failure led to reset job
        # failure and that left dut in Repair Failed. Since the test will fail
        # anyway due to the destiny lab issue, and test retry will retry the
        # test in another DUT.
        # This change temporarily disable the RPM check in reset job.
        # Another way to do this is to remove rpm dependency from tests' control
        # file. That will involve changes on multiple control files. This one
        # line change here is a simple temporary fix.
        'rpm': actionables.TestActionable('dummy_PassServer'),
    }

    name = 'verify'


class Provision(_SpecialTaskAction):
    """
    Provisioning runs to change the configuration of the DUT from one state to
    another.  It will only be run on verified DUTs.
    """

    # TODO(milleral): http://crbug.com/249555
    # Create some way to discover and register provisioning tests so that we
    # don't need to hand-maintain a list of all of them.
    _actions = {
        CROS_VERSION_PREFIX: actionables.TestActionable(
                'provision_AutoUpdate',
                extra_kwargs={'disable_sysinfo': False,
                              'disable_before_test_sysinfo': False,
                              'disable_before_iteration_sysinfo': True,
                              'disable_after_test_sysinfo': True,
                              'disable_after_iteration_sysinfo': True}),
        FW_RO_VERSION_PREFIX: actionables.TestActionable(
                'provision_FirmwareUpdate'),
        FW_RW_VERSION_PREFIX: actionables.TestActionable(
                'provision_FirmwareUpdate',
                extra_kwargs={'rw_only': True}),
        ANDROID_BUILD_VERSION_PREFIX : actionables.TestActionable(
                'provision_AndroidUpdate'),
    }

    name = 'provision'


class Cleanup(_SpecialTaskAction):
    """
    Cleanup runs after a test fails to try and remove artifacts of tests and
    ensure the DUT will be in a sane state for the next test run.
    """

    _actions = {
        'cleanup-reboot': actionables.RebootActionable(),
    }

    name = 'cleanup'


class Repair(_SpecialTaskAction):
    """
    Repair runs when one of the other special tasks fails.  It should be able
    to take a component of the DUT that's in an unknown state and restore it to
    a good state.
    """

    _actions = {
    }

    name = 'repair'



# TODO(milleral): crbug.com/364273
# Label doesn't really mean label in this context.  We're putting things into
# DEPENDENCIES that really aren't DEPENDENCIES, and we should probably stop
# doing that.
def is_for_special_action(label):
    """
    If any special task handles the label specially, then we're using the label
    to communicate that we want an action, and not as an actual dependency that
    the test has.

    @param label: A string label name.
    @return True if any special task handles this label specially,
            False if no special task handles this label.
    """
    return (Verify.acts_on(label) or
            Provision.acts_on(label) or
            Cleanup.acts_on(label) or
            Repair.acts_on(label))


def filter_labels(labels):
    """
    Filter a list of labels into two sets: those labels that we know how to
    change and those that we don't.  For the ones we know how to change, split
    them apart into the name of configuration type and its value.

    @param labels: A list of strings of labels.
    @returns: A tuple where the first element is a set of unprovisionable
              labels, and the second element is a set of the provisionable
              labels.

    >>> filter_labels(['bluetooth', 'cros-version:lumpy-release/R28-3993.0.0'])
    (set(['bluetooth']), set(['cros-version:lumpy-release/R28-3993.0.0']))

    """
    return Provision.partition(labels)


def split_labels(labels):
    """
    Split a list of labels into a dict mapping name to value.  All labels must
    be provisionable labels, or else a ValueError

    @param labels: list of strings of label names
    @returns: A dict of where the key is the configuration name, and the value
              is the configuration value.
    @raises: ValueError if a label is not a provisionable label.

    >>> split_labels(['cros-version:lumpy-release/R28-3993.0.0'])
    {'cros-version': 'lumpy-release/R28-3993.0.0'}
    >>> split_labels(['bluetooth'])
    Traceback (most recent call last):
    ...
    ValueError: Unprovisionable label bluetooth

    """
    configurations = dict()

    for label in labels:
        if Provision.acts_on(label):
            name, value = label.split(':', 1)
            configurations[name] = value
        else:
            raise ValueError('Unprovisionable label %s' % label)

    return configurations


def join(provision_type, provision_value):
    """
    Combine the provision type and value into the label name.

    @param provision_type: One of the constants that are the label prefixes.
    @param provision_value: A string of the value for this provision type.
    @returns: A string that is the label name for this (type, value) pair.

    >>> join(CROS_VERSION_PREFIX, 'lumpy-release/R27-3773.0.0')
    'cros-version:lumpy-release/R27-3773.0.0'

    """
    return '%s:%s' % (provision_type, provision_value)


class SpecialTaskActionException(Exception):
    """
    Exception raised when a special task fails to successfully run a test that
    is required.

    This is also a literally meaningless exception.  It's always just discarded.
    """


def run_special_task_actions(job, host, labels, task):
    """
    Iterate through all `label`s and run any tests on `host` that `task` has
    corresponding to the passed in labels.

    Emits status lines for each run test, and INFO lines for each skipped label.

    @param job: A job object from a control file.
    @param host: The host to run actions on.
    @param labels: The list of job labels to work on.
    @param task: An instance of _SpecialTaskAction.
    @returns: None
    @raises: SpecialTaskActionException if a test fails.

    """
    capabilities, configuration = filter_labels(labels)

    for label in capabilities:
        if task.acts_on(label):
            action_item = task.test_for(label)
            success = action_item.execute(job=job, host=host)
            if not success:
                raise SpecialTaskActionException()
        else:
            job.record('INFO', None, task.name,
                       "Can't %s label '%s'." % (task.name, label))

    for name, value in split_labels(configuration).items():
        if task.acts_on(name):
            action_item = task.test_for(name)
            success = action_item.execute(job=job, host=host, value=value)
            if not success:
                raise SpecialTaskActionException()
        else:
            job.record('INFO', None, task.name,
                       "Can't %s label '%s:%s'." % (task.name, name, value))
