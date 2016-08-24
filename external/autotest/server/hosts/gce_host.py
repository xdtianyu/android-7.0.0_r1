# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import string
import common

from chromite.lib import gce

from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib import lsbrelease_utils
from autotest_lib.client.cros import constants as client_constants
from autotest_lib.server.hosts import abstract_ssh

SSH_KEYS_METADATA_KEY = "sshKeys"

def extract_arguments(args_dict):
    """Extract GCE-specific arguments from arguments dictionary.

    @param args_dict: dictionary of all arguments supplied to the test.
    """

    return {k: v for k, v in args_dict.items()
            if k in ('gce_project', 'gce_instance',
                     'gce_zone', 'gce_key_file')}


class GceHost(abstract_ssh.AbstractSSHHost):
    """GCE-specific subclass of Host."""

    def _initialize(self, hostname, gce_args=None,
                    *args, **dargs):
        """Initializes this instance of GceHost.

        @param hostname: the hostnname to be passed down to AbstractSSHHost.
        @param gce_args: GCE-specific arguments extracted using
               extract_arguments().
        """
        super(GceHost, self)._initialize(hostname=hostname,
                                         *args, **dargs)

        if gce_args:
            self._gce_project = gce_args['gce_project']
            self._gce_zone = gce_args['gce_zone']
            self._gce_instance = gce_args['gce_instance']
            self._gce_key_file = gce_args['gce_key_file']
        else:
            # TODO(andreyu): determine project, zone and instance names by
            # querying metadata from the DUT instance
            raise error.AutoservError('No GCE flags provided.')

        self.gce = gce.GceContext.ForServiceAccountThreadSafe(
                self._gce_project, self._gce_zone, self._gce_key_file)


    def _modify_ssh_keys(self, to_add, to_remove):
        """Modifies the list of ssh keys.

        @param username: user name to add.
        @param to_add: a list of new enties.
        @param to_remove: a list of enties to be removed.
        """
        keys = self.gce.GetCommonInstanceMetadata(
                SSH_KEYS_METADATA_KEY) or ''
        key_set = set(string.split(keys, '\n'))
        new_key_set = (key_set | set(to_add)) - set(to_remove)
        if key_set != new_key_set:
            self.gce.SetCommonInstanceMetadata(
                    SSH_KEYS_METADATA_KEY,
                    string.join(list(new_key_set), '\n'))

    def add_ssh_key(self, username, ssh_key):
        """Adds a new SSH key in GCE metadata.

        @param username: user name to add.
        @param ssh_key: the key to add.
        """
        self._modify_ssh_keys(['%s:%s' % (username, ssh_key)], [])


    def del_ssh_key(self, username, ssh_key):
        """Deletes the given SSH key from GCE metadata

        @param username: user name to delete.
        @param ssh_key: the key to delete.
        """
        self._modify_ssh_keys([], ['%s:%s' % (username, ssh_key)])


    def get_release_version(self):
        """Get the value of attribute CHROMEOS_RELEASE_VERSION from lsb-release.

        @returns The version string in lsb-release, under attribute
                CHROMEOS_RELEASE_VERSION.
        """
        lsb_release_content = self.run(
                    'cat "%s"' % client_constants.LSB_RELEASE).stdout.strip()
        return lsbrelease_utils.get_chromeos_release_version(
                    lsb_release_content=lsb_release_content)


    def set_instance_metadata(self, key, value):
        """Sets a single metadata value on the DUT instance.

        @param key: Metadata key to be set.
        @param value: New value, or None if the given key should be removed.
        """
        self.gce.SetInstanceMetadata(self._gce_instance, key, value)

    def stop(self):
        """Stops the DUT instance
        """
        self.gce.StopInstance(self._gce_instance)

    def start(self):
        """Starts the DUT instance
        """
        self.gce.StartInstance(self._gce_instance)
