# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# This file contains utility functions to get and set stable versions for given
# boards.

import common
import django.core.exceptions
from autotest_lib.client.common_lib import global_config
from autotest_lib.client.common_lib.cros.graphite import autotest_es
from autotest_lib.frontend import setup_django_environment
from autotest_lib.frontend.afe import models


# Name of the default board. For boards that don't have stable version
# explicitly set, version for the default board will be used.
DEFAULT = 'DEFAULT'

# Type of metadata to store stable_version changes.
_STABLE_VERSION_TYPE = 'stable_version'

def get_all():
    """Get stable versions of all boards.

    @return: A dictionary of boards and stable versions.
    """
    versions = dict([(v.board, v.version)
                     for v in models.StableVersion.objects.all()])
    # Set default to the global config value of CROS.stable_cros_version if
    # there is no entry in afe_stable_versions table.
    if not versions:
        versions = {DEFAULT: global_config.global_config.get_config_value(
                            'CROS', 'stable_cros_version')}
    return versions


def get(board=DEFAULT, android=False):
    """Get stable version for the given board.

    @param board: Name of the board, default to value `DEFAULT`.
    @param android: If True, indicates we are looking up a Android/Brillo-based
                    board. There is no default version that works for all
                    Android/Brillo boards. If False, we are looking up a Chrome
                    OS based board.

    @return: Stable version of the given board. If the given board is not listed
             in afe_stable_versions table, DEFAULT will be used.
             Return global_config value of CROS.stable_cros_version if
             afe_stable_versions table does not have entry of board DEFAULT.
    """
    if board == DEFAULT and android:
        return None
    try:
        return models.StableVersion.objects.get(board=board).version
    except django.core.exceptions.ObjectDoesNotExist:
        if board == DEFAULT:
            return global_config.global_config.get_config_value(
                    'CROS', 'stable_cros_version')
        elif android:
            return global_config.global_config.get_config_value(
                    'ANDROID', 'stable_version_%s' % board, default=None)
        else:
            return get(board=DEFAULT)


def set(version, board=DEFAULT):
    """Set stable version for the given board.

    @param version: The new value of stable version for given board.
    @param board: Name of the board, default to value `DEFAULT`.
    """
    try:
        stable_version = models.StableVersion.objects.get(board=board)
        stable_version.version = version
        stable_version.save()
    except django.core.exceptions.ObjectDoesNotExist:
        models.StableVersion.objects.create(board=board, version=version)
    autotest_es.post(type_str=_STABLE_VERSION_TYPE,
                     metadata={'board': board, 'version': version})


def delete(board):
    """Delete stable version record for the given board.

    @param board: Name of the board.
    """
    stable_version = models.StableVersion.objects.get(board=board)
    stable_version.delete()
    autotest_es.post(type_str=_STABLE_VERSION_TYPE,
                     metadata={'board': board, 'version': get()})
