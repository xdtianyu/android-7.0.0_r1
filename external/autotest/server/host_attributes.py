# Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import utils


class base_host_attributes(object):
    def __init__(self, host):
        pass

    def get_attributes(self):
        return []


site_host_attributes = utils.import_site_class(
    __file__, "autotest_lib.server.site_host_attributes",
    "HostAttributes", base_host_attributes)


class host_attributes(site_host_attributes):
    pass
