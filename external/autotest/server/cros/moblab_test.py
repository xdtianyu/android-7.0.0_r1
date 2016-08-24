# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
import logging
import re

from autotest_lib.client.common_lib import error, global_config
from autotest_lib.server import test
from autotest_lib.server.hosts import moblab_host


DEFAULT_IMAGE_STORAGE_SERVER = global_config.global_config.get_config_value(
        'CROS', 'image_storage_server')
STORAGE_SERVER_REGEX = 'gs://.*/'


class MoblabTest(test.test):
    """Base class for Moblab tests.
    """

    def initialize(self, host, boto_path='',
                   image_storage_server=DEFAULT_IMAGE_STORAGE_SERVER):
        """Initialize the Moblab Host.

        * Installs a boto file.
        * Sets up the image storage server for this test.
        * Finds and adds DUTs on the testing subnet.

        @param boto_path: Path to the boto file we want to install.
        @param image_storage_server: image storage server to use for grabbing
                                     images from Google Storage.
        """
        super(MoblabTest, self).initialize()
        self._host = host
        self._host.install_boto_file(boto_path)
        self.set_image_storage_server(image_storage_server)
        self._host.wait_afe_up()
        self._host.find_and_add_duts()


    def set_image_storage_server(self, image_storage_server):
        """Set the image storage server.

        @param image_storage_server: Name of image storage server to use. Must
                                     follow format or gs://bucket-name/
                                     (Note trailing slash is required).

        @raises error.TestError if the image_storage_server is incorrectly
                                formatted.
        """
        if not re.match(STORAGE_SERVER_REGEX, image_storage_server):
            raise error.TestError(
                    'Image Storage Server supplied is not in the correct '
                    'format. Must start with gs:// and end with a trailing '
                    'slash: %s' % image_storage_server)
        logging.debug('Setting image_storage_server to %s',
                      image_storage_server)
        # If the image_storage_server is already set, delete it.
        self._host.run('sed -i /image_storage_server/d %s' %
                       moblab_host.SHADOW_CONFIG_PATH, ignore_status=True)
        self._host.run("sed -i '/\[CROS\]/ a\image_storage_server: "
                       "%s' %s" %(image_storage_server,
                                  moblab_host.SHADOW_CONFIG_PATH))