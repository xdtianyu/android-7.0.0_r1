# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""This class defines the TestBed class."""

import logging
import re
from multiprocessing import pool

import common

from autotest_lib.client.common_lib import error
from autotest_lib.server.cros.dynamic_suite import constants
from autotest_lib.server.cros.dynamic_suite import frontend_wrappers
from autotest_lib.server import autoserv_parser
from autotest_lib.server.hosts import adb_host
from autotest_lib.server.hosts import teststation_host


# Thread pool size to provision multiple devices in parallel.
_POOL_SIZE = 4

# Pattern for the image name when used to provision a dut connected to testbed.
# It should follow the naming convention of branch/target/build_id[:serial],
# where serial is optional.
_IMAGE_NAME_PATTERN = '(.*/.*/[^:]*)(?::(.*))?'

class TestBed(object):
    """This class represents a collection of connected teststations and duts."""

    _parser = autoserv_parser.autoserv_parser
    VERSION_PREFIX = 'testbed-version'

    def __init__(self, hostname='localhost', host_attributes={},
                 adb_serials=None, **dargs):
        """Initialize a TestBed.

        This will create the Test Station Host and connected hosts (ADBHost for
        now) and allow the user to retrieve them.

        @param hostname: Hostname of the test station connected to the duts.
        @param serials: List of adb device serials.
        """
        logging.info('Initializing TestBed centered on host: %s', hostname)
        self.hostname = hostname
        self.teststation = teststation_host.create_teststationhost(
                hostname=hostname)
        self.is_client_install_supported = False
        serials_from_attributes = host_attributes.get('serials')
        if serials_from_attributes:
            serials_from_attributes = serials_from_attributes.split(',')

        self.adb_device_serials = (adb_serials or
                                   serials_from_attributes or
                                   self.query_adb_device_serials())
        self.adb_devices = {}
        for adb_serial in self.adb_device_serials:
            self.adb_devices[adb_serial] = adb_host.ADBHost(
                hostname=hostname, teststation=self.teststation,
                adb_serial=adb_serial)


    def query_adb_device_serials(self):
        """Get a list of devices currently attached to the test station.

        @returns a list of adb devices.
        """
        serials = []
        # Let's see if we can get the serials via host attributes.
        afe = frontend_wrappers.RetryingAFE(timeout_min=5, delay_sec=10)
        serials_attr = afe.get_host_attribute('serials', hostname=self.hostname)
        for serial_attr in serials_attr:
            serials.extend(serial_attr.value.split(','))

        # Looks like we got nothing from afe, let's probe the test station.
        if not serials:
            # TODO(kevcheng): Refactor teststation to be a class and make the
            # ADBHost adb_devices a static method I can use here.  For now this
            # is pretty much a c/p of the _adb_devices() method from ADBHost.
            serials = adb_host.ADBHost.parse_device_serials(
                self.teststation.run('adb devices').stdout)

        return serials


    def get_all_hosts(self):
        """Return a list of all the hosts in this testbed.

        @return: List of the hosts which includes the test station and the adb
                 devices.
        """
        device_list = [self.teststation]
        device_list.extend(self.adb_devices.values())
        return device_list


    def get_test_station(self):
        """Return the test station host object.

        @return: The test station host object.
        """
        return self.teststation


    def get_adb_devices(self):
        """Return the adb host objects.

        @return: A dict of adb device serials to their host objects.
        """
        return self.adb_devices


    def get_labels(self):
        """Return a list of the labels gathered from the devices connected.

        @return: A list of strings that denote the labels from all the devices
                 connected.
        """
        labels = []
        for adb_device in self.get_adb_devices().values():
            labels.extend(adb_device.get_labels())
        # Currently the board label will need to be modified for each adb
        # device.  We'll get something like 'board:android-shamu' and
        # we'll need to update it to 'board:android-shamu-1'.  Let's store all
        # the labels in a dict and keep track of how many times we encounter
        # it, that way we know what number to append.
        board_label_dict = {}
        updated_labels = []
        for label in labels:
            # Update the board labels
            if label.startswith(constants.BOARD_PREFIX):
                # Now let's grab the board num and append it to the board_label.
                board_num = board_label_dict.setdefault(label, 0) + 1
                board_label_dict[label] = board_num
                updated_labels.append('%s-%d' % (label, board_num))
            else:
                # We don't need to mess with this.
                updated_labels.append(label)
        return updated_labels


    def get_platform(self):
        """Return the platform of the devices.

        @return: A string representing the testbed platform.
        """
        return 'testbed'


    def repair(self):
        """Run through repair on all the devices."""
        for adb_device in self.get_adb_devices().values():
            adb_device.repair()


    def verify(self):
        """Run through verify on all the devices."""
        for device in self.get_all_hosts():
            device.verify()


    def cleanup(self):
        """Run through cleanup on all the devices."""
        for adb_device in self.get_adb_devices().values():
            adb_device.cleanup()


    def _parse_image(self, image_string):
        """Parse the image string to a dictionary.

        Sample value of image_string:
        branch1/shamu-userdebug/LATEST:ZX1G2,branch2/shamu-userdebug/LATEST

        @param image_string: A comma separated string of images. The image name
                is in the format of branch/target/build_id[:serial]. Serial is
                optional once testbed machine_install supports allocating DUT
                based on board.

        @returns: A list of tuples of (build, serial). serial could be None if
                  it's not specified.
        """
        images = []
        for image in image_string.split(','):
            match = re.match(_IMAGE_NAME_PATTERN, image)
            if not match:
                raise error.InstallError(
                        'Image name of "%s" has invalid format. It should '
                        'follow naming convention of '
                        'branch/target/build_id[:serial]', image)
            images.append((match.group(1), match.group(2)))
        return images


    @staticmethod
    def _install_device(inputs):
        """Install build to a device with the given inputs.

        @param inputs: A dictionary of the arguments needed to install a device.
            Keys include:
            host: An ADBHost object of the device.
            build_url: Devserver URL to the build to install.
        """
        host = inputs['host']
        build_url = inputs['build_url']

        logging.info('Starting installing device %s:%s from build url %s',
                     host.hostname, host.adb_serial, build_url)
        host.machine_install(build_url=build_url)
        logging.info('Finished installing device %s:%s from build url %s',
                     host.hostname, host.adb_serial, build_url)


    def locate_devices(self, images):
        """Locate device for each image in the given images list.

        @param images: A list of tuples of (build, serial). serial could be None
                if it's not specified. Following are some examples:
                [('branch1/shamu-userdebug/100', None),
                 ('branch1/shamu-userdebug/100', None)]
                [('branch1/hammerhead-userdebug/100', 'XZ123'),
                 ('branch1/hammerhead-userdebug/200', None)]
                where XZ123 is serial of one of the hammerheads connected to the
                testbed.

        @return: A dictionary of (serial, build). Note that build here should
                 not have a serial specified in it.
        @raise InstallError: If not enough duts are available to install the
                given images. Or there are more duts with the same board than
                the images list specified.
        """
        # The map between serial and build to install in that dut.
        serial_build_pairs = {}
        builds_without_serial = [build for build, serial in images
                                 if not serial]
        for build, serial in images:
            if serial:
                serial_build_pairs[serial] = build
        # Return the mapping if all builds have serial specified.
        if not builds_without_serial:
            return serial_build_pairs

        # serials grouped by the board of duts.
        duts_by_board = {}
        for serial, host in self.get_adb_devices().iteritems():
            # Excluding duts already assigned to a build.
            if serial in serial_build_pairs:
                continue
            board = host.get_board_name()
            duts_by_board.setdefault(board, []).append(serial)

        # Builds grouped by the board name.
        builds_by_board = {}
        for build in builds_without_serial:
            match = re.match(adb_host.BUILD_REGEX, build)
            if not match:
                raise error.InstallError('Build %s is invalid. Failed to parse '
                                         'the board name.' % build)
            board = match.group('BOARD')
            builds_by_board.setdefault(board, []).append(build)

        # Pair build with dut with matching board.
        for board, builds in builds_by_board.iteritems():
            duts = duts_by_board.get(board, None)
            if not duts or len(duts) != len(builds):
                raise error.InstallError(
                        'Expected number of DUTs for board %s is %d, got %d' %
                        (board, len(builds), len(duts) if duts else 0))
            serial_build_pairs.update(dict(zip(duts, builds)))
        return serial_build_pairs


    def machine_install(self):
        """Install the DUT.

        @returns The name of the image installed.
        """
        if not self._parser.options.image:
            raise error.InstallError('No image string is provided to test bed.')
        images = self._parse_image(self._parser.options.image)

        arguments = []
        for serial, build in self.locate_devices(images).iteritems():
            logging.info('Installing build %s on DUT with serial %s.', build,
                         serial)
            host = self.get_adb_devices()[serial]
            build_url, _ = host.stage_build_for_install(build)
            arguments.append({'host': host,
                              'build_url': build_url})

        thread_pool = pool.ThreadPool(_POOL_SIZE)
        thread_pool.map(self._install_device, arguments)
        thread_pool.close()
        return self._parser.options.image
