# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import random
import signal
import sys
import threading
import time

from autotest_lib.client.common_lib import error


def handler(signum, frame):
    """
    Register a handler for the timeout.
    """
    raise error.TimeoutException('Call is timed out.')


def install_sigalarm_handler(new_handler):
    """
    Try installing a sigalarm handler.

    In order to protect apache, wsgi intercepts any attempt to install a
    sigalarm handler, so our function will feel the full force of a sigalarm
    even if we try to install a pacifying signal handler. To avoid this we
    need to confirm that the handler we tried to install really was installed.

    @param new_handler: The new handler to install. This must be a callable
                        object, or signal.SIG_IGN/SIG_DFL which correspond to
                        the numbers 1,0 respectively.
    @return: True if the installation of new_handler succeeded, False otherwise.
    """
    if (new_handler is None or
        (not callable(new_handler) and
         new_handler != signal.SIG_IGN and
         new_handler != signal.SIG_DFL)):
        logging.warning('Trying to install an invalid sigalarm handler.')
        return False

    signal.signal(signal.SIGALRM, new_handler)
    installed_handler = signal.getsignal(signal.SIGALRM)
    return installed_handler == new_handler


def set_sigalarm_timeout(timeout_secs, default_timeout=60):
    """
    Set the sigalarm timeout.

    This methods treats any timeout <= 0 as a possible error and falls back to
    using it's default timeout, since negative timeouts can have 'alarming'
    effects. Though 0 is a valid timeout, it is often used to cancel signals; in
    order to set a sigalarm of 0 please call signal.alarm directly as there are
    many situations where a 0 timeout is considered invalid.

    @param timeout_secs: The new timeout, in seconds.
    @param default_timeout: The default timeout to use, if timeout <= 0.
    @return: The old sigalarm timeout
    """
    timeout_sec_n = int(timeout_secs)
    if timeout_sec_n <= 0:
        timeout_sec_n = int(default_timeout)
    return signal.alarm(timeout_sec_n)


def timeout(func, args=(), kwargs={}, timeout_sec=60.0, default_result=None):
    """
    This function run the given function using the args, kwargs and
    return the given default value if the timeout_sec is exceeded.

    @param func: function to be called.
    @param args: arguments for function to be called.
    @param kwargs: keyword arguments for function to be called.
    @param timeout_sec: timeout setting for call to exit, in seconds.
    @param default_result: default return value for the function call.

    @return 1: is_timeout 2: result of the function call. If
            is_timeout is True, the call is timed out. If the
            value is False, the call is finished on time.
    """
    old_alarm_sec = 0
    old_handler = signal.getsignal(signal.SIGALRM)
    installed_handler = install_sigalarm_handler(handler)
    if installed_handler:
        old_alarm_sec = set_sigalarm_timeout(timeout_sec, default_timeout=60)

    # If old_timeout_time = 0 we either didn't install a handler, or sigalrm
    # had a signal.SIG_DFL handler with 0 timeout. In the latter case we still
    # need to restore the handler/timeout.
    old_timeout_time = (time.time() + old_alarm_sec) if old_alarm_sec > 0 else 0

    try:
        default_result = func(*args, **kwargs)
        return False, default_result
    except error.TimeoutException:
        return True, default_result
    finally:
        # If we installed a sigalarm handler, cancel it since our function
        # returned on time. If we can successfully restore the old handler,
        # reset the old timeout, or, if the old timeout's deadline has passed,
        # set the sigalarm to fire in one second. If the old_timeout_time is 0
        # we don't need to set the sigalarm timeout since we have already set it
        # as a byproduct of cancelling the current signal.
        if installed_handler:
            signal.alarm(0)
            if install_sigalarm_handler(old_handler) and old_timeout_time:
                set_sigalarm_timeout(int(old_timeout_time - time.time()),
                                     default_timeout=1)



def retry(ExceptionToCheck, timeout_min=1.0, delay_sec=3, blacklist=None):
    """Retry calling the decorated function using a delay with jitter.

    Will raise RPC ValidationError exceptions from the decorated
    function without retrying; a malformed RPC isn't going to
    magically become good. Will raise exceptions in blacklist as well.

    If the retry is done in a child thread, timeout may not be enforced as
    signal only works in main thread. Therefore, the retry inside a child
    thread may run longer than timeout or even hang.

    original from:
      http://www.saltycrane.com/blog/2009/11/trying-out-retry-decorator-python/

    @param ExceptionToCheck: the exception to check.  May be a tuple of
                             exceptions to check.
    @param timeout_min: timeout in minutes until giving up.
    @param delay_sec: pre-jittered delay between retries in seconds.  Actual
                      delays will be centered around this value, ranging up to
                      50% off this midpoint.
    @param blacklist: a list of exceptions that will be raised without retrying
    """
    def deco_retry(func):
        random.seed()


        def delay():
            """
            'Jitter' the delay, up to 50% in either direction.
            """
            random_delay = random.uniform(.5 * delay_sec, 1.5 * delay_sec)
            logging.warning('Retrying in %f seconds...', random_delay)
            time.sleep(random_delay)


        def func_retry(*args, **kwargs):
            # Used to cache exception to be raised later.
            exc_info = None
            delayed_enabled = False
            exception_tuple = () if blacklist is None else tuple(blacklist)
            start_time = time.time()
            remaining_time = timeout_min * 60
            is_main_thread = isinstance(threading.current_thread(),
                                        threading._MainThread)
            while remaining_time > 0:
                if delayed_enabled:
                    delay()
                else:
                    delayed_enabled = True
                try:
                    # Clear the cache
                    exc_info = None
                    if is_main_thread:
                        is_timeout, result = timeout(func, args, kwargs,
                                                     remaining_time)
                        if not is_timeout:
                            return result
                    else:
                        return func(*args, **kwargs)
                except exception_tuple:
                    raise
                except error.CrosDynamicSuiteException:
                    raise
                except ExceptionToCheck as e:
                    logging.warning('%s(%s)', e.__class__, e)
                    # Cache the exception to be raised later.
                    exc_info = sys.exc_info()

                remaining_time = int(timeout_min*60 -
                                     (time.time() - start_time))

            # The call must have timed out or raised ExceptionToCheck.
            if not exc_info:
                raise error.TimeoutException('Call is timed out.')
            # Raise the cached exception with original backtrace.
            raise exc_info[0], exc_info[1], exc_info[2]


        return func_retry  # true decorator
    return deco_retry
