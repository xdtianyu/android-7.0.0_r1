# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Storage device utilities to be used in storage device based tests
"""

import logging, re, os, time, hashlib

from autotest_lib.client.bin import test, utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib import base_utils
from autotest_lib.client.cros import liststorage


class StorageException(error.TestError):
    """Indicates that a storage/volume operation failed.
    It is fatal to the test unless caught.
    """
    pass


class StorageScanner(object):
    """Scan device for storage points.

    It also performs basic operations on found storage devices as mount/umount,
    creating file with randomized content or checksum file content.

    Each storage device is defined by a dictionary containing the following
    keys:

    device: the device path (e.g. /dev/sdb1)
    bus: the bus name (e.g. usb, ata, etc)
    model: the kind of device (e.g. Multi-Card, USB_DISK_2.0, SanDisk)
    size: the size of the volume/partition ib bytes (int)
    fs_uuid: the UUID for the filesystem (str)
    fstype: filesystem type
    is_mounted: wether the FS is mounted (0=False,1=True)
    mountpoint: where the FS is mounted (if mounted=1) or a suggestion where to
                mount it (if mounted=0)

    Also |filter()| and |scan()| will use the same dictionary keys associated
    with regular expression in order to filter a result set.
    Multiple keys act in an AND-fashion way. The absence of a key in the filter
    make the filter matching all the values for said key in the storage
    dictionary.

    Example: {'device':'/dev/sd[ab]1', 'is_mounted':'0'} will match all the
    found devices which block device file is either /dev/sda1 or /dev/sdb1, AND
    are not mounted, excluding all other devices from the matched result.
    """
    storages = None


    def __init__(self):
        self.__mounted = {}


    def filter(self, storage_filter={}):
        """Filters a stored result returning a list of matching devices.

        The passed dictionary represent the filter and its values are regular
        expressions (str). If an element of self.storage matches the regex
        defined in all the keys for a filter, the item will be part of the
        returning value.

        Calling this method does not change self.storages, thus can be called
        several times against the same result set.

        @param storage_filter: a dictionary representing the filter.

        @return a list of dictionaries representing the found devices after the
                application of the filter. The list can be empty if no device
                has been found.
        """
        ret = []

        for storage in self.storages:
            matches = True
            for key in storage_filter:
                if not re.match(storage_filter[key], storage[key]):
                    matches = False
                    break
            if matches:
                ret.append(storage.copy())

        return ret


    def scan(self, storage_filter={}):
        """Scan the current storage devices.

        If no parameter is given, it will return all the storage devices found.
        Otherwise it will internally call self.filter() with the passed
        filter.
        The result (being it filtered or not) will be saved in self.storages.

        Such list can be (re)-filtered using self.filter().

        @param storage_filter: a dict representing the filter, default is
                matching anything.

        @return a list of found dictionaries representing the found devices.
                 The list can be empty if no device has been found.
        """
        self.storages = liststorage.get_all()

        if storage_filter:
            self.storages = self.filter(storage_filter)

        return self.storages


    def mount_volume(self, index=None, storage_dict=None, args=''):
        """Mount the passed volume.

        Either index or storage_dict can be set, but not both at the same time.
        If neither is passed, it will mount the first volume found in
        self.storage.

        @param index: (int) the index in self.storages for the storage
                device/volume to be mounted.
        @param storage_dict: (dict) the storage dictionary representing the
                storage device, the dictionary should be obtained from
                self.storage or using self.scan() or self.filter().
        @param args: (str) args to be passed to the mount command, if needed.
                     e.g., "-o foo,bar -t ext3".
        """
        if index is None and storage_dict is None:
            storage_dict = self.storages[0]
        elif isinstance(index, int):
            storage_dict = self.storages[index]
        elif not isinstance(storage_dict, dict):
            raise TypeError('Either index or storage_dict passed '
                            'with the wrong type')

        if storage_dict['is_mounted']:
            logging.debug('Volume "%s" is already mounted, skipping '
                          'mount_volume().')
            return

        logging.info('Mounting %(device)s in %(mountpoint)s.', storage_dict)

        try:
            # Create the dir in case it does not exist.
            os.mkdir(storage_dict['mountpoint'])
        except OSError, e:
            # If it's not "file exists", report the exception.
            if e.errno != 17:
                raise e
        cmd = 'mount %s' % args
        cmd += ' %(device)s %(mountpoint)s' % storage_dict
        utils.system(cmd)
        storage_dict['is_mounted'] = True
        self.__mounted[storage_dict['mountpoint']] = storage_dict


    def umount_volume(self, index=None, storage_dict=None, args=''):
        """Un-mount the passed volume, by index or storage dictionary.

        Either index or storage_dict can be set, but not both at the same time.
        If neither is passed, it will mount the first volume found in
        self.storage.

        @param index: (int) the index in self.storages for the storage
                device/volume to be mounted.
        @param storage_dict: (dict) the storage dictionary representing the
                storage device, the dictionary should be obtained from
                self.storage or using self.scan() or self.filter().
        @param args: (str) args to be passed to the umount command, if needed.
                     e.g., '-f -t' for force+lazy umount.
        """
        if index is None and storage_dict is None:
            storage_dict = self.storages[0]
        elif isinstance(index, int):
            storage_dict = self.storages[index]
        elif not isinstance(storage_dict, dict):
            raise TypeError('Either index or storage_dict passed '
                            'with the wrong type')


        if not storage_dict['is_mounted']:
            logging.debug('Volume "%s" is already unmounted: skipping '
                          'umount_volume().')
            return

        logging.info('Unmounting %(device)s from %(mountpoint)s.',
                     storage_dict)
        cmd = 'umount %s' % args
        cmd += ' %(device)s' % storage_dict
        utils.system(cmd)
        # We don't care if it fails, it might be busy for a /proc/mounts issue.
        # See BUG=chromium-os:32105
        try:
            os.rmdir(storage_dict['mountpoint'])
        except OSError, e:
            logging.debug('Removing %s failed: %s: ignoring.',
                          storage_dict['mountpoint'], e)
        storage_dict['is_mounted'] = False
        # If we previously mounted it, remove it from our internal list.
        if storage_dict['mountpoint'] in self.__mounted:
            del self.__mounted[storage_dict['mountpoint']]


    def unmount_all(self):
        """Unmount all volumes mounted by self.mount_volume().
        """
        # We need to copy it since we are iterating over a dict which will
        # change size.
        for volume in self.__mounted.copy():
            self.umount_volume(storage_dict=self.__mounted[volume])


class StorageTester(test.test):
    """This is a class all tests about Storage can use.

    It has methods to
    - create random files
    - compute a file's md5 checksum
    - look/wait for a specific device (specified using StorageScanner
      dictionary format)

    Subclasses can override the _prepare_volume() method in order to disable
    them or change their behaviours.

    Subclasses should take care of unmount all the mounted filesystems when
    needed (e.g. on cleanup phase), calling self.umount_volume() or
    self.unmount_all().
    """
    scanner = None


    def initialize(self, filter_dict={'bus':'usb'}, filesystem='ext2'):
        """Initialize the test.

        Instantiate a StorageScanner instance to be used by tests and prepare
        any volume matched by |filter_dict|.
        Volume preparation is done by the _prepare_volume() method, which can be
        overriden by subclasses.

        @param filter_dict: a dictionary to filter attached USB devices to be
                            initialized.
        @param filesystem: the filesystem name to format the attached device.
        """
        super(StorageTester, self).initialize()

        self.scanner = StorageScanner()

        self._prepare_volume(filter_dict, filesystem=filesystem)

        # Be sure that if any operation above uses self.scanner related
        # methods, its result is cleaned after use.
        self.storages = None


    def _prepare_volume(self, filter_dict, filesystem='ext2'):
        """Prepare matching volumes for test.

        Prepare all the volumes matching |filter_dict| for test by formatting
        the matching storages with |filesystem|.

        This method is called by StorageTester.initialize(), a subclass can
        override this method to change its behaviour.
        Setting it to None (or a not callable) will disable it.

        @param filter_dict: a filter for the storages to be prepared.
        @param filesystem: filesystem with which volumes will be formatted.
        """
        if not os.path.isfile('/sbin/mkfs.%s' % filesystem):
            raise error.TestError('filesystem not supported by mkfs installed '
                                  'on this device')

        try:
            storages = self.wait_for_devices(filter_dict, cycles=1,
                                             mount_volume=False)[0]

            for storage in storages:
                logging.debug('Preparing volume on %s.', storage['device'])
                cmd = 'mkfs.%s %s' % (filesystem, storage['device'])
                utils.system(cmd)
        except StorageException, e:
            logging.warning("%s._prepare_volume() didn't find any device "
                            "attached: skipping volume preparation: %s",
                            self.__class__.__name__, e)
        except error.CmdError, e:
            logging.warning("%s._prepare_volume() couldn't format volume: %s",
                            self.__class__.__name__, e)

        logging.debug('Volume preparation finished.')


    def wait_for_devices(self, storage_filter, time_to_sleep=1, cycles=10,
                         mount_volume=True):
        """Cycles |cycles| times waiting |time_to_sleep| seconds each cycle,
        looking for a device matching |storage_filter|

        @param storage_filter: a dictionary holding a set of  storage device's
                keys which are used as filter, to look for devices.
                @see StorageDevice class documentation.
        @param time_to_sleep: time (int) to wait after each |cycles|.
        @param cycles: number of tentatives. Use -1 for infinite.

        @raises StorageException if no device can be found.

        @return (storage_dict, waited_time) tuple. storage_dict is the found
                 device list and waited_time is the time spent waiting for the
                 device to be found.
        """
        msg = ('Scanning for %s for %d times, waiting each time '
               '%d secs' % (storage_filter, cycles, time_to_sleep))
        if mount_volume:
            logging.debug('%s and mounting each matched volume.', msg)
        else:
            logging.debug('%s, but not mounting each matched volume.', msg)

        if cycles == -1:
            logging.info('Waiting until device is inserted, '
                         'no timeout has been set.')

        cycle = 0
        while cycles == -1 or cycle < cycles:
            ret = self.scanner.scan(storage_filter)
            if ret:
                logging.debug('Found %s (mount_volume=%d).', ret, mount_volume)
                if mount_volume:
                    for storage in ret:
                        self.scanner.mount_volume(storage_dict=storage)

                return (ret, cycle*time_to_sleep)
            else:
                logging.debug('Storage %s not found, wait and rescan '
                              '(cycle %d).', storage_filter, cycle)
                # Wait a bit and rescan storage list.
                time.sleep(time_to_sleep)
                cycle += 1

        # Device still not found.
        msg = ('Could not find anything matching "%s" after %d seconds' %
                (storage_filter, time_to_sleep*cycles))
        raise StorageException(msg)


    def wait_for_device(self, storage_filter, time_to_sleep=1, cycles=10,
                        mount_volume=True):
        """Cycles |cycles| times waiting |time_to_sleep| seconds each cycle,
        looking for a device matching |storage_filter|.

        This method needs to match one and only one device.
        @raises StorageException if no device can be found or more than one is
                 found.

        @param storage_filter: a dictionary holding a set of  storage device's
                keys which are used as filter, to look for devices
                The filter has to be match a single device, a multiple matching
                filter will lead to StorageException to e risen. Use
                self.wait_for_devices() if more than one device is allowed to
                be found.
                @see StorageDevice class documentation.
        @param time_to_sleep: time (int) to wait after each |cycles|.
        @param cycles: number of tentatives. Use -1 for infinite.

        @return (storage_dict, waited_time) tuple. storage_dict is the found
                 device list and waited_time is the time spent waiting for the
                 device to be found.
        """
        storages, waited_time = self.wait_for_devices(storage_filter,
            time_to_sleep=time_to_sleep,
            cycles=cycles,
            mount_volume=mount_volume)
        if len(storages) > 1:
            msg = ('filter matched more than one storage volume, use '
                '%s.wait_for_devices() if you need more than one match' %
                self.__class__)
            raise StorageException(msg)

        # Return the first element if only this one has been matched.
        return (storages[0], waited_time)


# Some helpers not present in base_utils.py to abstract normal file operations.

def create_file(path, size):
    """Create a file using /dev/urandom.

    @param path: the path of the file.
    @param size: the file size in bytes.
    """
    logging.debug('Creating %s (size %d) from /dev/urandom.', path, size)
    with file('/dev/urandom', 'rb') as urandom:
        utils.open_write_close(path, urandom.read(size))


def checksum_file(path):
    """Compute the MD5 Checksum for a file.

    @param path: the path of the file.

    @return a string with the checksum.
    """
    chunk_size = 1024

    m = hashlib.md5()
    with file(path, 'rb') as f:
        for chunk in f.read(chunk_size):
            m.update(chunk)

    logging.debug("MD5 checksum for %s is %s.", path, m.hexdigest())

    return m.hexdigest()


def args_to_storage_dict(args):
    """Map args into storage dictionaries.

    This function is to be used (likely) in control files to obtain a storage
    dictionary from command line arguments.

    @param args: a list of arguments as passed to control file.

    @return a tuple (storage_dict, rest_of_args) where storage_dict is a
            dictionary for storage filtering and rest_of_args is a dictionary
            of keys which do not match storage dict keys.
    """
    args_dict = base_utils.args_to_dict(args)
    storage_dict = {}

    # A list of all allowed keys and their type.
    key_list = ('device', 'bus', 'model', 'size', 'fs_uuid', 'fstype',
                'is_mounted', 'mountpoint')

    def set_if_exists(src, dst, key):
        """If |src| has |key| copies its value to |dst|.

        @return True if |key| exists in |src|, False otherwise.
        """
        if key in src:
            dst[key] = src[key]
            return True
        else:
            return False

    for key in key_list:
        if set_if_exists(args_dict, storage_dict, key):
            del args_dict[key]

    # Return the storage dict and the leftovers of the args to be evaluated
    # later.
    return storage_dict, args_dict
