# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""This files contains helper methods to interact with the readonly database."""

# Please note this file doesn't contain and should not contain any logic to
# establish connections outside of Django, as that might lead to effects where
# connections get leaked, which will lead to Django not cleaning them up
# properly. See http://crbug.com/422637 for more details on this failure.

from django import db as django_db

_DISABLED = False

def set_globally_disabled(disable):
    """Disable and enable the use of readonly connections globally.

    If disabled, connection() will return the global connection instead of the
    readonly connection.

    @param disable: When True, readonly connections will be disabled.
    """
    _DISABLED = disable


def connection():
    """Return a readonly database connection."""
    if _DISABLED:
        return django_db.connections['global']
    return django_db.connections['readonly']


def cursor():
    """Return a cursor on the readonly database connection."""
    return connection().cursor()
