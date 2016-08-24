# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

class WiFiInterfaceClaimContext(object):
    """Context that encapsulates claiming of a wifi interface.

    This context ensures that if the test fails while the interface is claimed
    we will attempt to release it before our test exits.

    """

    def __init__(self, client):
        self._client = client


    def __enter__(self):
        self._client.claim_wifi_if()


    def __exit__(self, exception, value, traceback):
        self._client.release_wifi_if()
