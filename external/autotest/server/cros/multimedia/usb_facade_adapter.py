# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""An adapter to remotely access the usb facade on DUT."""


class USBFacadeRemoteAdapter(object):
    """USBFacadeRemoteAdapter is an adapter to remotely control DUT USB.

    The Autotest host object representing the remote DUT, passed to this
    class on initialization, can be accessed from its _client property.

    """
    def __init__(self, remote_facade_proxy):
        """Construct an AudioFacadeRemoteAdapter.

        @param remote_facade_proxy: RemoteFacadeProxy object.

        """
        self._proxy = remote_facade_proxy


    @property
    def _usb_proxy(self):
        """Gets the proxy to DUT USB facade.

        @return XML RPC proxy to DUT USB facade.

        """
        return self._proxy.usb


    def plug(self):
        """Plugs the USB device into the host."""
        self._usb_proxy.plug()


    def unplug(self):
        """Unplugs the USB device from the host."""
        self._usb_proxy.unplug()
