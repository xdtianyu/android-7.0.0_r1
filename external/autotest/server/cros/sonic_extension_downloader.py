#!/usr/bin/python
# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import httplib2
import json
import logging
import os
import re
import shutil
import urllib2

import common

from autotest_lib.client.common_lib import autotemp
from autotest_lib.client.common_lib import utils


TEST_EXTENSION_ID = 'hfaagokkkhdbgiakmmlclaapfelnkoah'
UPDATE_CHECK_URL = ('https://clients2.google.com/service/update2/')
UPDATE_CHECK_PARAMETER = ('crx?x=id%%3D%s%%26v%%3D0%%26uc')
MANIFEST_KEY = ('MIGfMA0GCSqGSIb3DQEBAQUAA4GNADCBiQKBgQC+hlN5FB+tjCsBszmBIvI'
                'cD/djLLQm2zZfFygP4U4/o++ZM91EWtgII10LisoS47qT2TIOg4Un4+G57e'
                'lZ9PjEIhcJfANqkYrD3t9dpEzMNr936TLB2u683B5qmbB68Nq1Eel7KVc+F'
                '0BqhBondDqhvDvGPEV0vBsbErJFlNH7SQIDAQAB')


class SonicDownloaderException(Exception):
    """Generic sonic dowloader exception."""
    pass


def get_download_url_from_omaha(extension_id):
    """Retrieves an update url from omaha for the specified extension id.

    @param extension_id: The extension id of the chromecast extension.

    @return: A url to download the extension from.

    @raises IOError: If the response returned by the omaha server is invalid.
    """
    update_check_link = '%s%s' % (UPDATE_CHECK_URL,
                                  UPDATE_CHECK_PARAMETER % extension_id)
    response_xml = httplib2.Http().request(update_check_link, 'GET')[1]
    codebase_match = re.compile(r'codebase="(.*crx)"').search(response_xml)
    if codebase_match is not None:
        logging.info('Omaha response while downloading extension: %s',
                     response_xml)
        return codebase_match.groups()[0]
    raise IOError('Omaha response is invalid %s.' % response_xml)


def download_extension(dest_file):
    """Retrieve the extension into a destination crx file.

    @param dest_file: Path to a destination file for the extension.
    """
    download_url = get_download_url_from_omaha(TEST_EXTENSION_ID)
    logging.info('Downloading extension from %s', download_url)
    response = urllib2.urlopen(download_url)
    with open(dest_file, 'w') as f:
        f.write(response.read())


def fix_public_key(extracted_extension_folder):
    """Modifies the manifest.json to include a public key.

    This function will erase the content in the original manifest
    and replace it with a new manifest that contains the key.

    @param extracted_extension_folder: The folder containing
        the extracted extension.
    """
    manifest_json_file = os.path.join(extracted_extension_folder,
                                      'manifest.json')
    with open(manifest_json_file, 'r') as f:
        manifest_json = json.loads(f.read())

    manifest_json['key'] = MANIFEST_KEY

    with open(manifest_json_file, 'w') as f:
        f.write(json.dumps(manifest_json))


def setup_extension(unzipped_crx_dir):
    """Setup for tests that need a chromecast extension.

    Download the extension from an omaha server, unzip it and modify its
    manifest.json to include a public key.

    @param unzipped_crx_dir: Destination directory for the unzipped extension.

    @raises CmdTimeoutError: If we timeout unzipping the extension.
    """
    output_crx_dir = autotemp.tempdir()
    output_crx = os.path.join(output_crx_dir.name, 'sonic_extension.crx')
    try:
        download_extension(output_crx)
        unzip_cmd = 'unzip -o "%s" -d "%s"' % (output_crx, unzipped_crx_dir)

        # The unzip command will return a non-zero exit status if there are
        # extra bytes at the start/end of the zipfile. This is not a critical
        # failure and the extension will still work.
        cmd_output = utils.run(unzip_cmd, ignore_status=True, timeout=1)
    except Exception as e:
        if os.path.exists(unzipped_crx_dir):
            shutil.rmtree()
        raise SonicDownloaderException(e)
    finally:
        if os.path.exists(output_crx):
            os.remove(output_crx)
        output_crx_dir.clean()

    if not os.path.exists(unzipped_crx_dir):
        raise SonicDownloaderException('Unable to download sonic extension.')
    logging.info('Sonic extension successfully downloaded into %s.',
                 unzipped_crx_dir)

    # TODO(beeps): crbug.com/325869, investigate the limits of component
    # extensions. For now this is ok because even sonic testing inlines a
    # public key for their test extension.
    try:
        fix_public_key(unzipped_crx_dir)
    except Exception as e:
        shutil.rmtree(unzipped_crx_dir)
        raise SonicDownloaderException(e)
