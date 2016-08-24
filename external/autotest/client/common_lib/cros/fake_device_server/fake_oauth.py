# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

class FakeOAuth(object):
    """A Fake for oauth.OAuth to be used in unit-tests."""


    def is_request_authorized(self):
        """Checks if the access token in an incoming request is correct."""
        return True
