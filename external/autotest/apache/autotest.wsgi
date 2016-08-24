# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os, sys

# The frontend directory needs to be added to our path for the WSGI
# application to work properly.
frontend_path = os.path.abspath(
    os.path.join(os.path.dirname(__file__, '..', 'frontend'))
sys.path.append(frontend_path)

os.environ['DJANGO_SETTINGS_MODULE'] = 'settings'

import django.core.handlers.wsgi

application = django.core.handlers.wsgi.WSGIHandler()
