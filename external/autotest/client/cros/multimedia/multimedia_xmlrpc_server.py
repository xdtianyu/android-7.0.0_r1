#!/usr/bin/env python
# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""XML RPC server for multimedia testing."""

import argparse
import code
import logging
import xmlrpclib
import traceback
import common   # pylint: disable=unused-import
from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib import logging_config
from autotest_lib.client.common_lib.cros import chrome
from autotest_lib.client.cros import constants
from autotest_lib.client.cros import xmlrpc_server
from autotest_lib.client.cros.multimedia import audio_facade_native
from autotest_lib.client.cros.multimedia import browser_facade_native
from autotest_lib.client.cros.multimedia import display_facade_native
from autotest_lib.client.cros.multimedia import system_facade_native
from autotest_lib.client.cros.multimedia import usb_facade_native
from autotest_lib.client.cros.multimedia import facade_resource


class MultimediaXmlRpcDelegate(xmlrpc_server.XmlRpcDelegate):
    """XML RPC delegate for multimedia testing."""

    def __init__(self, resource):
        """Initializes the facade objects."""
        self._facades = {
            'audio': audio_facade_native.AudioFacadeNative(resource),
            'display': display_facade_native.DisplayFacadeNative(resource),
            'system': system_facade_native.SystemFacadeNative(),
            'usb': usb_facade_native.USBFacadeNative(),
            'browser': browser_facade_native.BrowserFacadeNative(resource),
        }


    def __exit__(self, exception, value, traceback):
        """Clean up the resources."""
        self._facades['audio'].cleanup()


    def _dispatch(self, method, params):
        """Dispatches the method to the proper facade.

        We turn off allow_dotted_names option. The method handles the dot
        and dispatches the method to the proper native facade, like
        DisplayFacadeNative.

        """
        try:
            try:
                if '.' not in method:
                    func = getattr(self, method)
                else:
                    facade_name, method_name = method.split('.', 1)
                    if facade_name in self._facades:
                        func = getattr(self._facades[facade_name], method_name)
                    else:
                        raise Exception('unknown facade: %s' % facade_name)
            except AttributeError:
                raise Exception('method %s not supported' % method)

            logging.info('Dispatching method %s with args %s',
                         str(func), str(params))
            return func(*params)
        except:
            # TODO(ihf): Try to return meaningful stacktraces from the client.
            return traceback.format_exc()


def config_logging():
    """Configs logging to be verbose and use console handler."""
    config = logging_config.LoggingConfig()
    config.configure_logging(use_console=True, verbose=True)


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('-d', '--debug', action='store_true', required=False,
                        help=('create a debug console with a ServerProxy "s" '
                              'connecting to the XML RPC sever at localhost'))
    parser.add_argument('--restart', action='store_true', required=False,
                        help=('restart the XML RPC server without clearing '
                              'the previous state'))
    args = parser.parse_args()

    if args.debug:
        s = xmlrpclib.ServerProxy('http://localhost:%d' %
                                  constants.MULTIMEDIA_XMLRPC_SERVER_PORT,
                                  allow_none=True)
        code.interact(local=locals())
    else:
        config_logging()
        logging.debug('multimedia_xmlrpc_server main...')


        # Restart Cras to clean up any audio activities.
        utils.restart_job('cras')

        with facade_resource.FacadeResource(restart=args.restart) as res:
            server = xmlrpc_server.XmlRpcServer(
                    'localhost', constants.MULTIMEDIA_XMLRPC_SERVER_PORT)
            server.register_delegate(MultimediaXmlRpcDelegate(res))
            server.run()
