# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from autotest_lib.client.cros.multimedia import display_facade_adapter


class LocalFacadeFactory(object):
    """A factory to generate local multimedia facades.

    The facade objects are wrapped by adapters to accept non-native-type,
    like DisplayFacadeLocalAdapter. These adapted facades are returned.
    """
    def __init__(self, chrome):
        """Initializes the local facade adapter objects."""
        self._facades = {
            'display': display_facade_adapter.DisplayFacadeLocalAdapter(chrome)
        }


    def create_display_facade(self):
        """Creates a display facade object."""
        return self._facades['display']
