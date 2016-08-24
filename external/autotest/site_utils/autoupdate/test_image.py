# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Module for discovering Chrome OS test images and payloads."""

import logging
import re

import common
from autotest_lib.client.common_lib import global_config
from autotest_lib.utils import external_packages

from autotest_lib.site_utils.autoupdate import import_common
devserver = import_common.download_and_import('devserver',
                                              external_packages.DevServerRepo())
from devserver import gsutil_util


# A string indicating a zip-file boundary within a URI path. This string must
# end with a '/', in order for standard basename code to work correctly for
# zip-encapsulated paths.
ZIPFILE_BOUNDARY = '//'
ARCHIVE_URL_FORMAT = '%(archive_prefix)s/%(version)s'


class TestImageError(BaseException):
    """Raised on any error in this module."""
    pass


class NotSingleItem(Exception):
    """Raised when we want a single item but got multiple."""


def get_default_archive_url(board, build_version):
    """Returns the default archive_url for the given board and build_version .

    @param board: the platform/board name
    @param build_version: the full build version i.e. R27-3823.0.0-a2.
    """
    archive_base = global_config.global_config.get_config_value(
            'CROS', 'image_storage_server')
    archive_base = archive_base.rstrip('/') # Remove any trailing /'s.

    # TODO(garnold) adjustment to -he variant board names; should be removed
    # once we switch to using artifacts from gs://chromeos-images/
    # (see chromium-os:38222)
    board = re.sub('-he$', '_he', board)
    archive_prefix = archive_base + '/%s-release' % board
    return ARCHIVE_URL_FORMAT % dict(
            archive_prefix=archive_prefix, version=build_version)


def get_archive_url_from_prefix(archive_prefix, build_version):
    """Returns the gs archive_url given a particular archive_prefix.

    @param archive_prefix: Use the archive_prefix as the base of your URL

                           construction (instead of config + board-release) e.g.
                           gs://my_location/my_super_awesome_directory.
    @param build_version: the full build version i.e. R27-3823.0.0-a2.
    """
    return ARCHIVE_URL_FORMAT % dict(
            archive_prefix=archive_prefix, version=build_version)


def gs_ls(pattern, archive_url, single):
    """Returns a list of URIs that match a given pattern.

    @param pattern: a regexp pattern to match (feeds into re.match).
    @param archive_url: the gs uri where to search (see ARCHIVE_URL_FORMAT).
    @param single: if true, expect a single match and return it.

    @return A list of URIs (possibly an empty list).

    """
    try:
        logging.debug('Searching for pattern %s from url %s', pattern,
                      archive_url)
        uri_list = gsutil_util.GetGSNamesWithWait(
                pattern, archive_url, err_str=__name__, timeout=1)
        # Convert to the format our clients expect (full archive path).
        if uri_list:
            if not single or (single and len(uri_list) == 1):
                return ['/'.join([archive_url, u]) for u in uri_list]
            else:
                raise NotSingleItem()

        return []
    except gsutil_util.PatternNotSpecific as e:
        raise TestImageError(str(e))
    except gsutil_util.GSUtilError:
        return []


def find_payload_uri(archive_url, delta=False, single=False):
    """Finds test payloads corresponding to a given board/release.

    @param archive_url: Archive_url directory to find the payload.
    @param delta: if true, seek delta payloads to the given release
    @param single: if true, expect a single match and return it, otherwise
           None

    @return A (possibly empty) list of URIs, or a single (possibly None) URI if
            |single| is True.

    @raise TestImageError if an error has occurred.

    """
    if delta:
        pattern = '*_delta_*'
    else:
        pattern = '*_full_*'

    payload_uri_list = gs_ls(pattern, archive_url, single)
    if not payload_uri_list:
        return None if single else []

    return payload_uri_list[0] if single else payload_uri_list


def find_image_uri(archive_url):
    """Returns a URI to a test image.

    @param archive_url: archive_url directory to find the payload.

    @return A URI to the desired image if found, None otherwise. It will most
            likely be a file inside an image archive (image.zip), in which case
            we'll be using ZIPFILE_BOUNDARY ('//') to denote a zip-encapsulated
            file, for example:
            gs://chromeos-image-archive/.../image.zip//chromiumos_test_image.bin

    @raise TestImageError if an error has occurred.

    """
    image_archive = gs_ls('image.zip', archive_url, single=True)
    if not image_archive:
        return None

    return (image_archive[0] + ZIPFILE_BOUNDARY + 'chromiumos_test_image.bin')
