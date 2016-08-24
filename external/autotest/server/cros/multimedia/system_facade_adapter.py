# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""An adapter to remotely access the system facade on DUT."""


class SystemFacadeRemoteAdapter(object):
    """SystemFacadeRemoteAdapter is an adapter to remotely control DUT system.

    The Autotest host object representing the remote DUT, passed to this
    class on initialization, can be accessed from its _client property.

    """
    def __init__(self, host, remote_facade_proxy):
        """Construct an SystemFacadeRemoteAdapter.

        @param host: Host object representing a remote host.
        @param remote_facade_proxy: RemoteFacadeProxy object.

        """
        self._client = host
        self._proxy = remote_facade_proxy


    @property
    def _system_proxy(self):
        """Gets the proxy to DUT system facade.

        @return XML RPC proxy to DUT system facade.

        """
        return self._proxy.system


    def set_scaling_governor_mode(self, index, mode):
        """Set mode of CPU scaling governor on one CPU of DUT.

        @param index: CPU index starting from 0.

        @param mode: Mode of scaling governor, accept 'interactive' or
                     'performance'.

        @returns: The original mode.

        """
        return self._system_proxy.set_scaling_governor_mode(index, mode)
