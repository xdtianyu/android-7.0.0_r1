# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import multiprocessing
import sys

from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib import global_config
from autotest_lib.client.common_lib.cros import dev_server
from autotest_lib.server.cros.dynamic_suite import constants

#Update status
UPDATE_SUCCESS = 0
UPDATE_FAILURE = 1

def update_dut_worker(updater_obj, dut, image, force):
    """The method called by multiprocessing worker pool for updating DUT.
    This function is the function which is repeatedly scheduled for each
    DUT through the multiprocessing worker. This has to be defined outside
    the class because it needs to be pickleable.

    @param updater_obj: An CliqueDUTUpdater object.
    @param dut: DUTObject representing the DUT.
    @param image: The build type and version to install on the host.
    @param force: If False, will only updated the host if it is not
                  already running the build. If True, force the
                  update regardless, and force a full-reimage.

    """
    updater_obj.update_dut(dut_host=dut.host, image=image, force=force)


class CliqueDUTUpdater(object):
    """CliqueDUTUpdater is responsible for updating all the DUT's in the
    DUT pool to the same release.
    """

    def __init__(self):
        """Initializes the DUT updater for updating the DUT's in the pool."""


    @staticmethod
    def _get_board_name_from_host(dut_host):
        """Get the board name of the remote host.

        @param host: Host object representing the DUT.

        @return: A string representing the board of the remote host.
        """
        try:
            board = dut_host.get_board().replace(constants.BOARD_PREFIX, '')
        except error.AutoservRunError:
            raise error.TestFail(
                    'Cannot determine board for host %s' % dut_host.hostname)
        logging.debug('Detected board %s for host %s', board, dut_host.hostname)
        return board

    @staticmethod
    def _construct_image_label(dut_board, release_version):
        """Constructs a label combining the board name and release version.

        @param dut_board: A string representing the board of the remote host.
        @param release_version: A chromeOS release version.

        @return: A string representing the release version.
                 Ex: lumpy-release/R28-3993.0.0
        """
        # todo(rpius): We should probably make this more flexible to accept
        # images from trybot's, etc.
        return dut_board + '-release/' + release_version

    @staticmethod
    def _get_update_url(ds_url, image):
        """Returns the full update URL. """
        config = global_config.global_config
        image_url_pattern = config.get_config_value(
                'CROS', 'image_url_pattern', type=str)
        return image_url_pattern % (ds_url, image)

    @staticmethod
    def _get_release_version_from_dut(dut_host):
        """Get release version from the DUT located in lsb-release file.

        @param dut_host: Host object representing the DUT.

        @return: A string representing the release version.
        """
        return dut_host.get_release_version()

    @staticmethod
    def _get_release_version_from_image(image):
        """Get release version from the image label.

        @param image: The build type and version to install on the host.

        @return: A string representing the release version.
        """
        return image.split('-')[-1]

    @staticmethod
    def _get_latest_release_version_from_server(dut_board):
        """Gets the latest release version for a given board from a dev server.

        @param dut_board: A string representing the board of the remote host.

        @return: A string representing the release version.
        """
        build_target = dut_board + "-release"
        config = global_config.global_config
        server_url_list = config.get_config_value(
                'CROS', 'dev_server', type=list, default=[])
        ds = dev_server.ImageServer(server_url_list[0])
        return ds.get_latest_build_in_server(build_target)

    def update_dut(self, dut_host, image, force=True):
        """The method called by to start the upgrade of a single DUT.

        @param dut_host: Host object representing the DUT.
        @param image: The build type and version to install on the host.
        @param force: If False, will only updated the host if it is not
                      already running the build. If True, force the
                      update regardless, and force a full-reimage.

        """
        logging.debug('Host: %s. Start updating DUT to %s', dut_host, image)

        # If the host is already on the correct build, we have nothing to do.
        dut_release_version = self._get_release_version_from_dut(dut_host)
        image_release_version = self._get_release_version_from_image(image)
        if not force and dut_release_version == image_release_version:
            logging.info('Host: %s. Already running %s',
                         dut_host, image_release_version)
            sys.exit(UPDATE_SUCCESS)

        try:
            ds = dev_server.ImageServer.resolve(image)
            # We need the autotest packages to run the tests.
            ds.stage_artifacts(image, ['full_payload', 'stateful',
                                       'autotest_packages'])
        except dev_server.DevServerException as e:
            error_str = 'Host: ' + dut_host + '. ' + e
            logging.error(error_str)
            sys.exit(UPDATE_FAILURE)

        url = self._get_update_url(ds.url(), image)
        logging.debug('Host: %s. Installing image from %s', dut_host, url)
        try:
            dut_host.machine_install(force_update=True, update_url=url,
                                     force_full_update=force)
        except error.InstallError as e:
            error_str = 'Host: ' + dut_host + '. ' + e
            logging.error(error_str)
            sys.exit(UPDATE_FAILURE)

        dut_release_version = self._get_release_version_from_dut(dut_host)
        if dut_release_version != image_release_version:
            error_str = 'Host: ' + dut_host + '. Expected version of ' + \
                        image_release_version + ' in DUT, but found '  + \
                        dut_release_version + '.'
            logging.error(error_str)
            sys.exit(UPDATE_FAILURE)

        logging.info('Host: %s. Finished updating DUT to %s', dut_host, image)
        sys.exit(UPDATE_SUCCESS)

    def update_dut_pool(self, dut_objects, release_version=""):
        """Updates all the DUT's in the pool to a provided release version.

        @param dut_objects: An array of DUTObjects corresponding to all the
                            DUT's in the DUT pool.
        @param release_version: A chromeOS release version.

        @return: True if all the DUT's successfully upgraded, False otherwise.
        """
        tasks = []
        for dut in dut_objects:
            dut_board = self._get_board_name_from_host(dut.host)
            if release_version == "":
                release_version = self._get_latest_release_version_from_server(
                        dut_board)
            dut_image = self._construct_image_label(dut_board, release_version)
            # Schedule the update for this DUT to the update process pool.
            task = multiprocessing.Process(
                    target=update_dut_worker,
                    args=(self, dut, dut_image, False))
            tasks.append(task)
        # Run the updates in parallel.
        for task in tasks:
            task.start()
        for task in tasks:
            task.join()

        # Check the exit code to determine if the updates were all successful
        # or not.
        for task in tasks:
            if task.exitcode == UPDATE_FAILURE:
                return False
        return True
