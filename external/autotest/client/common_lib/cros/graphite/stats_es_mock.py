#pylint: disable-msg=C0111

# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.


class mock_class_base(object):
    """Base class for a mock statsd/es class."""
    def __init__(self, *args, **kwargs):
        pass


    def __getattribute__(self, name):
        def any_call(*args, **kwargs):
            pass

        def decorate(f):
            return f

        # TODO (dshi) crbug.com/256111 - Find better solution for mocking
        # statsd.
        def get_client(*args, **kwargs):
            return self

        if name == 'decorate':
            return decorate
        elif name == 'get_client':
            return get_client
        elif name == 'indices':
            return mock_class_base()

        return any_call