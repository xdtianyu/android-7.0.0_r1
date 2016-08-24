# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import math
import threading

import common
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros import retry
from autotest_lib.frontend.afe.json_rpc import proxy
from autotest_lib.server import frontend
from chromite.lib import retry_util
from chromite.lib import timeout_util


def convert_timeout_to_retry(backoff, timeout_min, delay_sec):
    """Compute the number of retry attempts for use with chromite.retry_util.

    @param backoff: The exponential backoff factor.
    @param timeout_min: The maximum amount of time (in minutes) to sleep.
    @param delay_sec: The amount to sleep (in seconds) between each attempt.

    @return: The number of retry attempts in the case of exponential backoff.
    """
    # Estimate the max_retry in the case of exponential backoff:
    # => total_sleep = sleep*sum(r=0..max_retry-1, backoff^r)
    # => total_sleep = sleep( (1-backoff^max_retry) / (1-backoff) )
    # => max_retry*ln(backoff) = ln(1-(total_sleep/sleep)*(1-backoff))
    # => max_retry = ln(1-(total_sleep/sleep)*(1-backoff))/ln(backoff)
    total_sleep = timeout_min * 60
    numerator = math.log10(1-(total_sleep/delay_sec)*(1-backoff))
    denominator = math.log10(backoff)
    return int(math.ceil(numerator/denominator))


class RetryingAFE(frontend.AFE):
    """Wrapper around frontend.AFE that retries all RPCs.

    Timeout for retries and delay between retries are configurable.
    """
    def __init__(self, timeout_min=30, delay_sec=10, **dargs):
        """Constructor

        @param timeout_min: timeout in minutes until giving up.
        @param delay_sec: pre-jittered delay between retries in seconds.
        """
        self.timeout_min = timeout_min
        self.delay_sec = delay_sec
        super(RetryingAFE, self).__init__(**dargs)


    def run(self, call, **dargs):
        # exc_retry: We retry if this exception is raised.
        # blacklist: Exceptions that we raise immediately if caught.
        exc_retry = Exception
        blacklist = (ImportError, error.RPCException, proxy.JSONRPCException,
                     timeout_util.TimeoutError)
        backoff = 2
        max_retry = convert_timeout_to_retry(backoff, self.timeout_min,
                                             self.delay_sec)

        def _run(self, call, **dargs):
            return super(RetryingAFE, self).run(call, **dargs)

        def handler(exc):
            """Check if exc is an exc_retry or if it's blacklisted.

            @param exc: An exception.

            @return: True if exc is an exc_retry and is not
                     blacklisted. False otherwise.
            """
            is_exc_to_check = isinstance(exc, exc_retry)
            is_blacklisted = isinstance(exc, blacklist)
            return is_exc_to_check and not is_blacklisted

        # If the call is not in main thread, signal can't be used to abort the
        # call. In that case, use a basic retry which does not enforce timeout
        # if the process hangs.
        @retry.retry(Exception, timeout_min=self.timeout_min,
                     delay_sec=self.delay_sec,
                     blacklist=[ImportError, error.RPCException,
                                proxy.ValidationError])
        def _run_in_child_thread(self, call, **dargs):
            return super(RetryingAFE, self).run(call, **dargs)

        if isinstance(threading.current_thread(), threading._MainThread):
            # Set the keyword argument for GenericRetry
            dargs['sleep'] = self.delay_sec
            dargs['backoff_factor'] = backoff
            with timeout_util.Timeout(self.timeout_min * 60):
                return retry_util.GenericRetry(handler, max_retry, _run,
                                               self, call, **dargs)
        else:
            return _run_in_child_thread(self, call, **dargs)


class RetryingTKO(frontend.TKO):
    """Wrapper around frontend.TKO that retries all RPCs.

    Timeout for retries and delay between retries are configurable.
    """
    def __init__(self, timeout_min=30, delay_sec=10, **dargs):
        """Constructor

        @param timeout_min: timeout in minutes until giving up.
        @param delay_sec: pre-jittered delay between retries in seconds.
        """
        self.timeout_min = timeout_min
        self.delay_sec = delay_sec
        super(RetryingTKO, self).__init__(**dargs)


    def run(self, call, **dargs):
        @retry.retry(Exception, timeout_min=self.timeout_min,
                     delay_sec=self.delay_sec,
                     blacklist=[ImportError, error.RPCException,
                                proxy.ValidationError])
        def _run(self, call, **dargs):
            return super(RetryingTKO, self).run(call, **dargs)
        return _run(self, call, **dargs)
