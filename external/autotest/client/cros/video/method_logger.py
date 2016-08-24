# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging


def log(method_to_log):
    """ A decorator method to log when 'decorated' methods have been executed.
     This greatly simplifies tracing of the method calls.

     To log execution of a method just decorate it with *log

     The decorator logs the method to be executed, its class, the arguments
     supplied to it and its return value.

     @param method_to_log: Method object that will be logged and invoked.

    """
    def log_wrapper(self, *args, **kwargs):
        """ Actual method doing the logging and also invokes method_to_log
        """

        log_str = '%s.%s' % (self.__class__.__name__, method_to_log.__name__)

        logging.debug('+ ' + log_str)

        have_args = len(args) > 0
        have_kwargs = len(kwargs) > 0

        if have_args:
            logging.debug('*** Begin arguments:')
            logging.debug(args)
            logging.debug('=== End arguments.')

        if have_kwargs:
            logging.debug('*** Begin keyword arguments:')
            logging.debug(kwargs)
            logging.debug('=== End keyword arguments.')

        result = method_to_log(self, *args, **kwargs)

        if result is not None:
            logging.debug('### Begin results :')
            logging.debug(result)
            logging.debug('--- End results.')

        logging.debug('- ' + log_str)

        return result

    return log_wrapper