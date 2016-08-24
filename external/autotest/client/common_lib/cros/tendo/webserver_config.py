# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import collections
import json

from autotest_lib.client.common_lib import utils


# Represents one instance of a protocol handler that the webserver can be
# configured with.
ProtocolHandler = collections.namedtuple('ProtocolHandler',
                                         ['name', 'port', 'use_tls'])

def get_n_protocol_handlers(n, port_base, use_tls=False,
                            handler_name_prefix='test_protocol_handler_'):
    """Construct ProtocolHandler objects for a number of handlers.

    @param n: integer number of handlers.
    @param port_base: integer port number.  Each handler will be given a port
            from the port_base, port_base + 1, port_base + 2, etc.
    @param use_tls: True iff the handler should use encryption.
    @param handler_name_prefix: string prefix to be used in the names of
            the N handlers.

    """
    protocol_handlers = []
    for i in range(n):
        protocol_handlers.append(
                ProtocolHandler(name='%s%d' % (handler_name_prefix, i),
                                port=port_base + i,
                                use_tls=use_tls))
    return protocol_handlers


class WebserverConfig(object):
    """Helper object that knows how to configure webservd."""

    def __init__(self,
                 verbosity_level=3,
                 webserv_debug=None,
                 extra_protocol_handlers=None,
                 host=None):
        """Construct an instance.

        @param verbosity_level: integer verbosity level.
        @param webserv_debug: True iff the webserver should log in debug mode.
        @param extra_protocol_handlers: list of protocol handler objects
                obtained from get_n_protocol_handlers.  These replace the
                default handlers.
        @param host: Host object if we want to control webservd on a remote
                host.

        """
        self._verbosity_level = verbosity_level
        self._webserv_debug = webserv_debug
        self._extra_protocol_handlers = extra_protocol_handlers
        self._run = utils.run if host is None else host.run


    def _write_out_config_file(self, path, protocol_handlers):
        """Write a config file at |path| for |protocol_handlers|.

        @param path: file system path to write config dict to.
        @param protocol_handlers: list of ProtocolHandler objects.

        """
        handler_configs  = []
        # Each handler gets a JSON dict.
        for handler in self._extra_protocol_handlers:
            handler_configs.append({'name': handler.name,
                                    'port': handler.port,
                                    'use_tls': handler.use_tls})
        config = {'protocol_handlers': handler_configs}
        # Write out the actual file and give webservd permissions.
        with open(path, 'w') as f:
            f.write(json.dumps(config, indent=True))
        self._run('chown webservd:webservd %s' % path)


    def restart_with_config(self):
        """Restart the webserver with this configuration."""
        self._run('stop webservd', ignore_status=True)
        config_path = None
        if self._extra_protocol_handlers:
            config_path = '/tmp/webservd.conf'
            self._write_out_config_file(config_path,
                                        self._extra_protocol_handlers)
        args = ['WEBSERVD_LOG_LEVEL=%d' % self._verbosity_level]
        if self._webserv_debug:
            args.append('WEBSERVD_DEBUG=true')
        if config_path:
            args.append('WEBSERVD_CONFIG_PATH=%s' % config_path)
        self._run('start webservd %s' % ' '.join(args))


    def close(self):
        """Restarts the webserver with the default configuration."""
        self._run('stop webservd', ignore_status=True)
        self._run('start webservd')
