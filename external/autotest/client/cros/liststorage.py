#!/usr/bin/python

# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""This is a module to scan /sys/block/ virtual FS, query udev

It provides a list of all removable or USB devices connected to the machine on
which the module is running.
It can be used from command line or from a python script.

To use it as python module it's enough to call the get_all() function.
@see |get_all| documentation for the output format
|get_all()| output is human readable (as oppposite to python's data structures)
"""

import os, re

# this script can be run at command line on DUT (ie /usr/local/autotest
# contains only the client/ subtree), on a normal autotest
# installation/repository or as a python module used on a client-side test.
import common
from autotest_lib.client.common_lib import base_utils as utils

INFO_PATH = "/sys/block"


def get_udev_info(blockdev, method='udev'):
    """Get information about |blockdev|

    @param blockdev: a block device, e.g., /dev/sda1 or /dev/sda
    @param method: either 'udev' (default) or 'blkid'

    @return a dictionary with two or more of the followig keys:
        "ID_BUS", "ID_MODEL": always present
        "ID_FS_UUID", "ID_FS_TYPE", "ID_FS_LABEL": present only if those info
         are meaningul and present for the queried device
    """
    ret = {}
    cmd = None
    ignore_status = False

    if method == "udev":
        cmd = "udevadm info --name %s --query=property" % blockdev
    elif method == "blkid":
        # this script is run as root in a normal autotest run,
        # so this works: It doesn't have access to the necessary info
        # when run as a non-privileged user
        cmd = "blkid -c /dev/null -o udev %s" % blockdev
        ignore_status = True

    if cmd:
        output = utils.system_output(cmd, ignore_status=ignore_status)

        udev_keys = ("ID_BUS", "ID_MODEL", "ID_FS_UUID", "ID_FS_TYPE",
                     "ID_FS_LABEL")
        for line in output.splitlines():
            udev_key, udev_val = line.split('=')

            if udev_key in udev_keys:
                ret[udev_key] = udev_val

    return ret


def get_partition_info(part_path, bus, model, partid=None, fstype=None,
                       label=None, block_size=0, is_removable=False):
    """Return information about a device as a list of dictionaries

    Normally a single device described by the passed parameters will match a
    single device on the system, and thus a single element list as return
    value; although it's possible that a single block device is associated with
    several mountpoints, this scenario will lead to a dictionary for each
    mountpoint.

    @param part_path: full partition path under |INFO_PATH|
                      e.g., /sys/block/sda or /sys/block/sda/sda1
    @param bus: bus, e.g., 'usb' or 'ata', according to udev
    @param model: device moduel, e.g., according to udev
    @param partid: partition id, if present
    @param fstype: filesystem type, if present
    @param label: filesystem label, if present
    @param block_size: filesystem block size
    @param is_removable: whether it is a removable device

    @return a list of dictionaries contaning each a partition info.
            An empty list can be returned if no matching device is found
    """
    ret = []
    # take the partitioned device name from the /sys/block/ path name
    part = part_path.split('/')[-1]
    device = "/dev/%s" % part

    if not partid:
        info = get_udev_info(device, "blkid")
        partid = info.get('ID_FS_UUID', None)
        if not fstype:
            fstype = info.get('ID_FS_TYPE', None)
        if not label:
            label = partid

    readonly = open("%s/ro" % part_path).read()
    if not int(readonly):
        partition_blocks = open("%s/size" % part_path).read()
        size = block_size * int(partition_blocks)

        stub = {}
        stub['device'] = device
        stub['bus'] = bus
        stub['model'] = model
        stub['size'] = size

        # look for it among the mounted devices first
        mounts = open("/proc/mounts").readlines()
        seen = False
        for line in mounts:
            dev, mount, proc_fstype, flags = line.split(' ', 3)

            if device == dev:
                if 'rw' in flags.split(','):
                    seen = True # at least one match occurred

                    # Sorround mountpoint with quotes, to make it parsable in
                    # case of spaces. Also information retrieved from
                    # /proc/mount override the udev passed ones (e.g.,
                    # proc_fstype instead of fstype)
                    dev = stub.copy()
                    dev['fs_uuid'] = partid
                    dev['fstype'] = proc_fstype
                    dev['is_mounted'] = True
                    dev['mountpoint'] = mount
                    ret.append(dev)

        # If not among mounted devices, it's just attached, print about the
        # same information but suggest a place where the user can mount the
        # device instead
        if not seen:
            # we consider it if it's removable and and a partition id
            # OR it's on the USB bus.
            # Some USB HD do not get announced as removable, but they should be
            # showed.
            # There are good changes that if it's on a USB bus it's removable
            # and thus interesting for us, independently whether it's declared
            # removable
            if (is_removable and partid) or bus == 'usb':
                if not label:
                    info = get_udev_info(device, 'blkid')
                    label = info.get('ID_FS_LABEL', partid)

                dev = stub.copy()
                dev['fs_uuid'] = partid
                dev['fstype'] = fstype
                dev['is_mounted'] = False
                dev['mountpoint'] = "/media/removable/%s" % label
                ret.append(dev)

        return ret


def get_device_info(blockdev):
    """Retrieve information about |blockdev|

    @see |get_partition_info()| doc for the dictionary format

    @param blockdev: a block device name, e.g., "sda".

    @return a list of dictionary, with each item representing a found device
    """
    ret = []

    spath = "%s/%s" % (INFO_PATH, blockdev)
    block_size = int(open("%s/queue/physical_block_size" % spath).read())
    is_removable = bool(int(open("%s/removable" % spath).read()))

    info = get_udev_info(blockdev, "udev")
    dev_bus = info['ID_BUS']
    dev_model = info['ID_MODEL']
    dev_fs = info.get('ID_FS_TYPE', None)
    dev_uuid = info.get('ID_FS_UUID', None)
    dev_label = info.get('ID_FS_LABEL', dev_uuid)

    has_partitions = False
    for basename in os.listdir(spath):
        partition_path = "%s/%s" % (spath, basename)
        # we want to check if within |spath| there are subdevices with
        # partitions
        # e.g., if within /sys/block/sda sda1 and other partition are present
        if not re.match("%s[0-9]+" % blockdev, basename):
            continue # ignore what is not a subdevice

        # |blockdev| has subdevices: get info for them
        has_partitions = True
        devs = get_partition_info(partition_path, dev_bus, dev_model,
                                  block_size=block_size,
                                  is_removable=is_removable)
        ret.extend(devs)

    if not has_partitions:
        devs = get_partition_info(spath, dev_bus, dev_model, dev_uuid, dev_fs,
                                  dev_label, block_size=block_size,
                                  is_removable=is_removable)
        ret.extend(devs)

    return ret


def get_all():
    """Return all removable or USB storage devices attached

    @return a list of dictionaries, each list element describing a device
    """
    ret = []
    for dev in os.listdir(INFO_PATH):
        # Among block devices we need to filter out what are virtual
        if re.match("s[a-z]+", dev):
            # for each of them try to obtain some info
            ret.extend(get_device_info(dev))
    return ret


def main():
    for device in get_all():
        print ("%(device)s %(bus)s %(model)s %(size)d %(fs_uuid)s %(fstype)s "
               "%(is_mounted)d %(mountpoint)s" % device)


if __name__ == "__main__":
    main()
