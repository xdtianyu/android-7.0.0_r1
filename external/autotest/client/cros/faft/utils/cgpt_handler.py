# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""A module to provide interface to gpt information.

gpt stands for GUID partition table, it is a data structure describing
partitions present of a storage device. cgpt is a utility which allows to read
and modify gpt. This module parses cgpt output to create a dictionary
including information about all defined partitions including their properties.
It also allows to modify partition properties as required.
"""

class CgptError(Exception):
    pass

class CgptHandler(object):
    """Object representing one or more gpts present in the system.

    Attributes:
      os_if: an instance of OSInterface, initialized by the caller.
      devices: a dictionary keyed by the storage device names (as in
               /dev/sda), the contents are dictionaries of cgpt information,
               where keys are partiton names, and contents are in turn
               dictionaries of partition properties, something like the below
               (compressed for brevity):
      {'/dev/sda': {
        'OEM': {'partition': 8, 'Type': 'Linux data', 'UUID': 'xxx'},
        'ROOT-A': {'partition': 3, 'Type': 'ChromeOS rootfs', 'UUID': 'xyz'},
        'ROOT-C': {'partition': 7, 'Type': 'ChromeOS rootfs', 'UUID': 'xzz'},
        'ROOT-B': {'partition': 5, 'Type': 'ChromeOS rootfs', 'UUID': 'aaa'},
        ...
        }
     }

    """

    # This dictionary maps gpt attributes the user can modify into the cgpt
    # utility command line options.
    ATTR_TO_COMMAND = {
        'priority' : 'P',
        'tries' : 'T',
        'successful' : 'S'
        }

    def __init__(self, os_if):
        self.os_if = os_if
        self.devices = {}

    def read_device_info(self, dev_name):
        """Get device information from cgpt and parse it into a dictionary.

        Inputs:
          dev_name: a string the Linux storage device name, (i.e. '/dev/sda')
        """

        device_dump = self.os_if.run_shell_command_get_output(
            'cgpt show %s' % dev_name)
        label = None
        label_data = {}
        device_data = {}
        for line in [x.strip() for x in device_dump]:
            if 'Label:' in line:
                if label and label not in device_data:
                    device_data[label] = label_data
                _, _, partition, _, label = line.split()
                label = line.split('Label:')[1].strip('" ')
                label_data = {'partition': int(partition)}
                continue
            if ':' in line:
                name, value = line.strip().split(':')
                if name != 'Attr':
                    label_data[name] = value.strip()
                    continue
                # Attributes are split around '=', each attribute becomes a
                # separate partition property.
                attrs = value.strip().split()
                for attr in attrs:
                    name, value = attr.split('=')
                    label_data[name] = int(value)
        if label_data:
            device_data[label] = label_data

        self.devices[dev_name] = device_data

    def get_partition(self, device, partition_name):
        """Retrieve a dictionary representing a partition on a device.

        Inputs:
          device: a string, the Linux device name
          partition_name: a string, the partition name as reported by cgpt.

        Raises:
          CgptError in case the device or partiton on that device are not
          known.
        """

        try:
            result = self.devices[device][partition_name]
        except KeyError:
            raise CgptError('could not retrieve partiton %s of device %s' % (
                    partition_name, device))
        return result

    def set_partition(self, device, partition_name, partition_value):
        """Set partition properties.

        Inputs:
          device: a string, the Linux device name
          partition_name: a string, the partition name as reported by cgpt.
          partiton_value: a dictionary, where keys are strings, names of the
                  properties which need to be modified, and values are the
                  values to set the properties to. The only properties which
                  can be modified are those which are keys of ATTR_TO_COMMAND
                  defined above.
        Raises:
          CgptError in case a property name is not known or not supposed to
              be modified.
        """

        current = self.get_partition(device, partition_name)
        options = []
        for prop, value in partition_value.iteritems():
            try:
                if value == current[prop]:
                    continue
                options.append('-%s %d' % (
                        self.ATTR_TO_COMMAND[prop], value))
            except KeyError:
                raise CgptError("unknown or immutable property '%s'" % prop)

        if not options:
            return

        command = 'cgpt add -i %d %s %s' % (
            current['partition'], ' '.join(options), device)
        self.os_if.run_shell_command(command)
