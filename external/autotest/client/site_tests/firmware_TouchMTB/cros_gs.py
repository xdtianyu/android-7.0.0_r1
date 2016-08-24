# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""An interface for accessing Google Cloud Storage."""

import os
import shutil
import tempfile

from autotest_lib.client.common_lib import file_utils
from common_util import simple_system


PUBLIC_BOTO = 'public/.chromeos.gestures.untrusted.write.boto'
GS_BUCKET = 'gs://chromeos-touchpad'
GS_BUCKET_PUBLIC = GS_BUCKET + '-public'
GSUTIL = 'gsutil'
GSUTIL_URI_ROOT = 'http://storage.googleapis.com/pub'
GSUTIL_TAR_NAME = 'gsutil.tar.gz'
GSUTIL_URI = os.path.join(GSUTIL_URI_ROOT, GSUTIL_TAR_NAME)
GSUTIL_INSTALL_DIR = os.path.join('/', 'usr', 'local', 'share')
GSUTIL_PATH = os.path.join(GSUTIL_INSTALL_DIR, GSUTIL)


def download_and_install_gsutil():
    """Download and install gsutil package."""
    if not os.path.isdir(GSUTIL_PATH):
        print 'Installing %s ...' % GSUTIL

        # Download the gsutil tarball to a temporary directory
        temp_dir = tempfile.mkdtemp()
        gsutil_temp_file = os.path.join(temp_dir, GSUTIL_TAR_NAME)
        print '  Downloading gsutil tarball: "%s".' % GSUTIL_URI
        file_utils.download_file(GSUTIL_URI, gsutil_temp_file)

        # Untar the gsutil tarball
        untar_cmd_str = 'tar xf %s -C %s'
        untar_cmd = untar_cmd_str % (gsutil_temp_file, GSUTIL_INSTALL_DIR)
        print '  Untarring the gsutil tarball.'
        simple_system(untar_cmd)

        # Remove the tarball and the temp directory
        shutil.rmtree(temp_dir)

    # Set the PATH environment variable for gsutil
    PATH = os.environ['PATH']
    os.environ['PATH'] = ':'.join([GSUTIL_PATH, PATH])


class CrosGs(object):
    """A class handling google cloud storage access."""
    def __init__(self, board, boto=PUBLIC_BOTO):
        download_and_install_gsutil()

        # Set up gsutil commands
        self.bucket = GS_BUCKET_PUBLIC if boto == PUBLIC_BOTO else GS_BUCKET
        bucket = self.bucket
        self.default_bucket_dir = os.path.join(
                'firmware_test', board, 'data', '')
        _cmd_prefix = 'BOTO_CONFIG=%s gsutil ' % boto
        self.ls_cmd = '{0} {1} {2}/%s'.format(_cmd_prefix, 'ls', bucket)
        upload_cmd_str = '{0} {1} %s %s {2}/%s'
        self.upload_cmd = upload_cmd_str.format(_cmd_prefix, 'cp', bucket)
        download_cmd_str = '{0} {1} %s {2}/%s %s'
        self.download_cmd = download_cmd_str.format(_cmd_prefix, 'cp', bucket)
        self.rm_cmd = '{0} {1} {2}/%s'.format(_cmd_prefix, 'rm', bucket)

    def ls(self, files=''):
        """ls the files in the selected bucket."""
        simple_system(self.ls_cmd % files)

    def upload(self, data, bucket_dir=''):
        """Upload the data to the chosen bucket."""
        if not bucket_dir:
            bucket_dir = self.default_bucket_dir
        cp_flag = '-R' if os.path.isdir(data) else ''
        simple_system(self.upload_cmd % (cp_flag, data, bucket_dir))
        msg = '\nGesture event files have been uploaded to "%s"\n'
        data_dir = os.path.basename(data)
        print msg % os.path.join(self.bucket, bucket_dir, data_dir)

    def rm(self, single_file):
        """Remove single_file."""
        simple_system(self.rm_cmd % single_file)

    def rmdir(self, data_dir):
        """Remove all files in the data directory."""
        simple_system(self.rm_cmd % os.path.join(data_dir, '*'))
