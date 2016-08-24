# Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Host attributes define properties on individual hosts.

Host attributes are specified a strings with the format:
    <key>{,<value>}?

A machine may have a list of strings for attributes like:

    ['has_80211n,True',
     'has_ssd,False',
     'drive_kind,string,ssd,1']

A legal attribute has the pattern:
    <name>,<kind>(,<extra>)?

Name can be any legal python identifier.  Kind may be any of 'string', 'True',
or 'False'.  Only if kind is string can there be extra data.

Strings which are not legal attributes are ignored.

Given the above list of attributes, you can use the syntax:
    host_attributes.drive_kind => 'ssd,1'
    host_attributes.has_80211n => True
    host_attributes.has_ssd => False
    host_attributes.unknown_attribute => raise KeyError

Machine attributes can be specified in two ways.

If you create private_host_attributes_config.py and private_host_attributes
there, we will use it when possible instead of using the server front-end.

Example configuration:
    private_host_attributes = {
        "mydevice": ["has_80211n,True",
                     "has_resume_bug,False"]
    }

We also consult the AFE database for its labels which are all treated as host
attribute strings defined above.  Illegal strings are ignored.
"""


import hashlib, logging, os, utils


private_host_attributes = utils.import_site_symbol(
    __file__,
    'autotest_lib.server.private_host_attributes_config',
    'private_host_attributes', dummy={})

try:
    settings = 'autotest_lib.frontend.settings'
    os.environ['DJANGO_SETTINGS_MODULE'] = settings
    from autotest_lib.frontend.afe import models
    has_models = True
except Exception:
    has_models = False


_DEFAULT_ATTRIBUTES = [
    'has_80211n,True',
    'has_bluetooth,False',
    'has_chromeos_firmware,True',
    'has_resume_bug,False',
    'has_ssd,True'
    ]


class HostAttributes(object):
    """Host attribute class for site specific attributes."""

    def __init__(self, host):
        """Create an instance of HostAttribute for the given hostname.

        We look up the host in both the hardcoded configuration and the AFE
        models if they can be found.

        Args:
            host: Host name to find attributes for.
        """
        self._add_attributes(_DEFAULT_ATTRIBUTES)
        if host in private_host_attributes:
            logging.info('Including private_host_attributes file for %s', host)
            self._add_attributes(private_host_attributes[host])
        if has_models:
            logging.info("Including labels for %s from database", host)
            host_obj = models.Host.valid_objects.get(hostname=host)
            self._add_attributes([label.name for label in
                                  host_obj.labels.all()])
        for key, value in self.__dict__.items():
            logging.info('Host attribute: %s => %s', key, value)

    def _add_attributes(self, attributes):
        for attribute in attributes:
            splitnames = attribute.split(',')
            if len(splitnames) == 1:
                if 'netbook_' in attribute:
                    # Hash board names to prevent any accidental leaks.
                    splitnames = ['netbook_' + hashlib.sha256(
                        attribute.split('netbook_')[1]).hexdigest()[:8], 'True']
                else:
                    splitnames = attribute.split(':')
                    if len(splitnames) == 2:
                        setattr(self, splitnames[0], splitnames[1])
                    continue
            value = ','.join(splitnames[1:])
            if value == 'True':
                value = True
            elif value == 'False':
                value = False
            elif splitnames[1] == 'string' and len(splitnames) > 2:
                value = ','.join(splitnames[2:])
            else:
                logging.info('Non-attribute string "%s" is ignored', attribute)
                continue
            setattr(self, splitnames[0], value)

    def get_attributes(self):
        """Return a list of non-False attributes for this host."""
        return [key for key, value in self.__dict__.items() if value]
