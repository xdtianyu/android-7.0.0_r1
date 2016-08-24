#!/usr/bin/env python

# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import functools
import inspect
import logging
import traceback

def log_dbus_method(return_logger=logging.debug, raise_logger=logging.warning,
                    return_cb_arg=None, raise_cb_arg=None):
    """
    Factory method for decorator to log dbus responses / errors.

    This method should be used to decorate the most concerete implementation of
    a dbus method.

    @param return_logger: A function that accepts a string argument to log the
            response from the decorated function.
    @param raise_logger: A function accepts a string argument to log the
            exception raised by the decorated function.
    @param return_cb_arg: str name of the async callback argument for the return
            value, if the function takes one.
    @param raise_cb_arg: str name of the async callback argument for the error
            return value, if the function takes one.

    """
    def wrapper(func):
        """
        The decorator returned by this factory.

        @param func: The function to be decorated.

        """
        @functools.wraps(func)
        def wrapped_func(*args, **kwargs):
            """The modified function for the decorated function."""
            modified_args = list(args)
            modified_kwargs = kwargs
            return_cb_index = getattr(wrapped_func, '_logging_return_cb_index')
            if return_cb_index > -1:
                if len(args) > return_cb_index:
                    modified_args[return_cb_index] = _wrap_async_return(
                            args[return_cb_index],
                            func.__name__,
                            return_logger)
                elif return_cb_arg in kwargs:
                    modified_kwargs[return_cb_arg] = _wrap_async_return(
                            kwargs[return_cb_arg],
                            func.__name__,
                            return_logger)
                else:
                    logging.debug('Not logging default return_cb')

            raise_cb_index = getattr(wrapped_func, '_logging_raise_cb_index')
            if raise_cb_index > -1:
                if len(args) > raise_cb_index:
                    modified_args[raise_cb_index] = _wrap_async_raise(
                            args[raise_cb_index],
                            func.__name__,
                            raise_logger)
                elif raise_cb_arg in kwargs:
                    modified_kwargs[raise_cb_arg] = _wrap_async_raise(
                            kwargs[raise_cb_arg],
                            func.__name__,
                            raise_logger)
                else:
                    logging.debug('Not logging default raise_cb')

            try:
                retval = func(*modified_args, **modified_kwargs)
                # No |return_cb_arg| ==> return value is the DBus response, so
                # it needs to be logged.
                if return_cb_index == -1:
                    return_logger('Response[%s] OK: |%s|' % (func.__name__,
                                                             repr(retval)))
            except Exception as e:
                raise_logger('Response[%s] ERROR: |%s|' % (func.__name__,
                                                           repr(e)))
                raise_logger(traceback.format_exc())
                raise
            return retval


        args, _, _, defaults = inspect.getargspec(func)
        wrapped_func._logging_return_cb_index = -1
        wrapped_func._logging_raise_cb_index = -1
        if return_cb_arg:
            if return_cb_arg not in args:
                logging.warning(
                        'Did not find expected argument %s in argument list '
                        'of %s', return_cb_arg, func.__name__)
            wrapped_func._logging_return_cb_index = args.index(return_cb_arg)
        if raise_cb_arg:
            if raise_cb_arg not in args:
                logging.warning(
                        'Did not find expected argument %s in argument list '
                        'of %s', raise_cb_arg, func.__name__)
            wrapped_func._logging_raise_cb_index = args.index(raise_cb_arg)
        return wrapped_func
    return wrapper


def _wrap_async_return(return_cb, fname, logger):
    """
    Wrap return_cb to log the return value.

    @param return_cb: The function to be wrapped.
    @param fname: Name of the DBus function called.
    @param logger: The logger to use for logging.
    @returns: Wrapped |return_cb| that additionally logs its arguments.

    """
    @functools.wraps(return_cb)
    def wrapped_return_cb(*args, **kwargs):
        """ Log arguments before calling return_cb. """
        logger('AsyncResponse[%s] OK: |%s|' % (fname, str((args, kwargs))))
        return_cb(*args, **kwargs)

    return wrapped_return_cb


def _wrap_async_raise(raise_cb, fname, logger):
    """
    Wrap raise_cb to log the raised error.

    @param raise_cb: The function to be wrapped.
    @param fname: Name of the DBus function called.
    @param logger: The logger to use for logging.
    @returns: Wrapped |raise_cb| that additionally logs its arguments.

    """
    @functools.wraps(raise_cb)
    def wrapped_raise_cb(*args, **kwargs):
        """ Log arguments before calling raise_cb. """
        logger('AsyncResponse[%s] ERROR: |%s|' % (fname, str((args, kwargs))))
        logger(traceback.format_exc())
        raise_cb(*args, **kwargs)

    return wrapped_raise_cb
