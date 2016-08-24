# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Interactive feedback layer abstraction."""

from autotest_lib.client.common_lib import error


# All known queries.
#
# Audio playback and recording testing.
QUERY_AUDIO_PLAYBACK_SILENT = 0
QUERY_AUDIO_PLAYBACK_AUDIBLE = 1
QUERY_AUDIO_RECORDING = 2
# Motion sensor testing.
QUERY_MOTION_RESTING = 10
QUERY_MOTION_MOVING = 11
# USB keyboard plugging and typing.
QUERY_KEYBOARD_PLUG = 20
QUERY_KEYBOARD_TYPE = 21
# GPIO write/read testing.
QUERY_GPIO_WRITE = 30
QUERY_GPIO_READ = 31
# On-board light testing.
QUERY_LIGHT_ON = 40
# TODO(garnold) Camera controls testing.
#QUERY_CAMERA_???
# Power management testing.
QUERY_POWER_WAKEUP = 60

INPUT_QUERIES = set((
        QUERY_AUDIO_RECORDING,
        QUERY_MOTION_RESTING,
        QUERY_MOTION_MOVING,
        QUERY_KEYBOARD_PLUG,
        QUERY_KEYBOARD_TYPE,
        QUERY_GPIO_READ,
        QUERY_POWER_WAKEUP,
))

OUTPUT_QUERIES = set((
        QUERY_AUDIO_PLAYBACK_SILENT,
        QUERY_AUDIO_PLAYBACK_AUDIBLE,
        QUERY_GPIO_WRITE,
        QUERY_LIGHT_ON,
))

ALL_QUERIES = INPUT_QUERIES.union(OUTPUT_QUERIES)


# Feedback client definition.
#
class Client(object):
    """Interface for an interactive feedback layer."""

    def __init__(self):
        self._initialized = False
        self._finalized = False


    def _check_active(self):
        """Ensure that the client was initialized and not finalized."""
        if not self._initialized:
            raise error.TestError('Client was not initialized')
        if self._finalized:
            raise error.TestError('Client was already finalized')


    def __enter__(self):
        self._check_active()
        return self


    def __exit__(self, ex_type, ex_val, ex_tb):
        self.finalize()


    def initialize(self, test, host=None):
        """Initializes the feedback object.

        This method should be called once prior to any other call.

        @param test: An object representing the test case.
        @param host: An object representing the DUT; required for server-side
                     tests.

        @raise TestError: There was an error during initialization.
        """
        if self._initialized:
            raise error.TestError('Client was already initialized')
        if self._finalized:
            raise error.TestError('Client was already finalized')
        self._initialize_impl(test, host)
        self._initialized = True
        return self


    def _initialize_impl(self, test, host):
        """Implementation of feedback client initialization.

        This should be implemented in concrete subclasses.
        """
        raise NotImplementedError


    def new_query(self, query_id):
        """Instantiates a new query.

        @param query_id: A query identifier (see QUERY_ constants above).

        @return A query object.

        @raise TestError: Query is invalid or not supported.
        """
        self._check_active()
        return self._new_query_impl(query_id)


    def _new_query_impl(self, query_id):
        """Implementation of new query instantiation.

        This should be implemented in concrete subclasses.
        """
        raise NotImplementedError


    def finalize(self):
        """Finalizes the feedback object.

        This method should be called once when done using the client.

        @raise TestError: There was an error while finalizing the client.
        """
        self._check_active()
        self._finalize_impl()
        self._finalized = True


    def _finalize_impl(self):
        """Implementation of feedback client finalization.

        This should be implemented in concrete subclasses.
        """
        raise NotImplementedError


# Feedback query definitions.
#
class _Query(object):
    """Interactive feedback query base class.

    This class is further derived and should not be inherited directly.
    """

    def __init__(self):
        self._prepare_called = False
        self._validate_called = False


    def prepare(self, **kwargs):
        """Prepares the tester for providing or capturing feedback.

        @raise TestError: Query preparation failed.
        """
        if self._prepare_called:
            raise error.TestError('Prepare was already called')
        self._prepare_impl(**kwargs)
        self._prepare_called = True


    def _prepare_impl(self, **kwargs):
        """Implementation of query preparation logic.

        This should be implemented in concrete subclasses.
        """
        raise NotImplementedError


    def validate(self, **kwargs):
        """Validates the interactive input/output result.

        This enforces that the method is called at most once, then delegates
        to an underlying implementation method.

        @raise TestError: An error occurred during validation.
        @raise TestFail: Query validation failed.
        """
        if self._validate_called:
            raise error.TestError('Validate was already called')
        self._validate_impl(**kwargs)
        self._validate_called = True


    def _validate_impl(self, **kwargs):
        """Implementation of query validation logic.

        This should be implemented in concrete subclasses.
        """
        raise NotImplementedError


class OutputQuery(_Query):
    """Interface for an output interactive feedback query.

    This class mandates that prepare() is called prior to validate().
    Subclasses should override implementations of _prepare_impl() and
    _validate_impl().
    """

    def __init__(self):
        super(OutputQuery, self).__init__()


    def validate(self, **kwargs):
        """Validates the interactive input/output result.

        This enforces the precondition and delegates to the base method.

        @raise TestError: An error occurred during validation.
        @raise TestFail: Query validation failed.
        """
        if not self._prepare_called:
            raise error.TestError('Prepare was not called')
        super(OutputQuery, self).validate(**kwargs)


class InputQuery(_Query):
    """Interface for an input interactive feedback query.

    This class mandates that prepare() is called first, then emit(), and
    finally validate(). Subclasses should override implementations of
    _prepare_impl(), _emit_impl() and _validate_impl().
    """

    def __init__(self):
        super(InputQuery, self).__init__()
        self._emit_called = False


    def validate(self, **kwargs):
        """Validates the interactive input/output result.

        This enforces the precondition and delegates to the base method.

        @raise TestError: An error occurred during validation.
        @raise TestFail: Query validation failed.
        """
        if not self._emit_called:
            raise error.TestError('Emit was not called')
        super(InputQuery, self).validate(**kwargs)


    def emit(self):
        """Instructs the tester to emit a feedback to be captured by the test.

        This enforces the precondition and ensures the method is called at most
        once, then delegates to an underlying implementation method.

        @raise TestError: An error occurred during emission.
        """
        if not self._prepare_called:
            raise error.TestError('Prepare was not called')
        if self._emit_called:
            raise error.TestError('Emit was already called')
        self._emit_impl()
        self._emit_called = True


    def _emit_impl(self):
        """Implementation of query emission logic.

        This should be implemented in concrete subclasses.
        """
        raise NotImplementedError
