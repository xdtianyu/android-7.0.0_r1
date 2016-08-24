# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import os

from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.cros import constants
from autotest_lib.server import site_utils
from autotest_lib.server import test
from autotest_lib.server.cros.multimedia import remote_facade_factory
from autotest_lib.site_utils import lxc


class AudioTest(test.test):
    """Base class for audio tests.

    AudioTest provides a common warmup() function for the collection
    of audio tests.
    It is not mandatory to use this base class for audio tests, it is for
    convenience only.

    """

    def warmup(self):
        """Warmup for the test before executing main logic of the test."""
        # test.test is an old-style class.
        test.test.warmup(self)
        audio_test_requirement()


    def create_remote_facade_factory(self, host):
        """Creates a remote facade factory to access multimedia server.

        @param host: A CrosHost object to access Cros device.

        @returns: A RemoteFacadeFactory object to create different facade for
                  different functionalities provided by multimedia server.

        """
        return create_remote_facade_factory(host, self.resultsdir)


def create_remote_facade_factory(host, result_dir):
    """Creates a remote facade factory to access multimedia server.

    @param host: A CrosHost object to access Cros device.
    @param result_dir: A directory to store multimedia server init log.

    @returns: A RemoteFacadeFactory object to create different facade for
              different functionalities provided by multimedia server.

    """
    try:
        factory = remote_facade_factory.RemoteFacadeFactory(host)
    finally:
        host.get_file(
                constants.MULTIMEDIA_XMLRPC_SERVER_LOG_FILE,
                os.path.join(
                        result_dir,
                        'multimedia_xmlrpc_server.log.init'))
    return factory


def audio_test_requirement():
    """Installs sox and checks it is installed correctly."""
    install_sox()
    check_sox_installed()


def install_sox():
    """Install sox command on autotest drone."""
    try:
        lxc.install_package('sox')
    except error.ContainerError:
        logging.info('Can not install sox outside of container.')


def check_sox_installed():
    """Checks if sox is installed.

    @raises: error.TestError if sox is not installed.

    """
    try:
        utils.run('sox --help')
        logging.info('Found sox executable.')
    except error.CmdError:
        error_message = 'sox command is not installed.'
        if site_utils.is_inside_chroot():
            error_message += ' sudo emerge sox to install sox in chroot'
        raise error.TestError(error_message)
