# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from autotest_lib.client.common_lib.cros.tendo import buffet_config
from autotest_lib.client.common_lib.cros.tendo import privet_helper
from autotest_lib.server import test

class buffet_WebServerSanity(test.test):
    """Test that we can connect to buffet's web server and get a response
    from a simple GET request."""
    version = 1

    def warmup(self, host):
        config = buffet_config.BuffetConfig(log_verbosity=3)
        config.restart_with_config(host=host)


    def cleanup(self, host):
        buffet_config.naive_restart(host=host)


    def run_once(self, host):
        helper = privet_helper.PrivetHelper(host=host)
        helper.ping_server()
        helper.ping_server(use_https=True)
