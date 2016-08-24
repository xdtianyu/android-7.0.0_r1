# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import pprint

import common
from autotest_lib.server.cros.faft.config import DEFAULTS


class Config(object):
    """Configuration for FAFT tests.

    This object is meant to be the interface to all configuration required
    by FAFT tests, including device specific overrides.

    It gets the default values from DEFAULTS.py which is found within
    the config package and the overrides come from module files named
    the same (Link.py, ...) as the value passed in via
    'platform' (e.g. Link, Stumpy, ...). Note we consider the platform name
    case insensitive and by convention platform override files should be
    lowercase.

    The DEFAULTS module must exist and contain a class named 'Values'.

    The override module is optional. If it exists, it must contain a 'Values'
    class. That class can inherit any other override's Values class.

    Attribute requests will first be searched through the overrides (if it
    exists) and then through the defaults.

    @attribute platform: string containing the platform name being tested.
    """

    def __init__(self, platform=None):
        """Initialize an object with FAFT settings.

        Initialize a list of objects that will be searched, in order, for
        the requested config attribute.

        @param platform: string containing the platform name being tested.
        """
        self.platform = platform
        # Defaults must always exist.
        self._precedence_list = [DEFAULTS.Values()]
        # Overrides are optional, and not an error.
        try:
            config_name = platform.rsplit('_', 1)[-1].lower()
            overrides = __import__(config_name, globals(), locals())
            overrides = overrides.Values()
            # Add overrides to the first position in the list
            self._precedence_list.insert(0, overrides)
        except ImportError:
            logging.debug("No config overrides found for platform: %s.",
                          platform)
        logging.debug(str(self))

    def __getattr__(self, name):
        """Search through every object (first in overrides then in defaults)
        for the first occurrence of the requested attribute (name) and
        return that. Else raise AttributeError.

        @param name: string with attribute being searched for. Should not be
            an empty string or None.
        """
        for cfg_obj in self._precedence_list:
            if hasattr(cfg_obj, name):
                return getattr(cfg_obj, name)
        raise AttributeError("No value exists for attribute (%s)" % name)

    def _filtered_attribute_list(self):
        """Return a sorted(set) containing all attributes from all objects with
        attributes starting with __ being filtered out."""
        filtered_attributes = set()
        for cfg_obj in self._precedence_list:
            filtered_attributes.update([name for name in dir(cfg_obj) if
                                                not name.startswith('__')])
        return sorted(filtered_attributes)

    def __str__(self):
        str_list = []
        str_list.append("--[ Raw FAFT Config Dump ]--------------------------")
        for cfg_obj in self._precedence_list:
            str_list.append("%s -> %s\n" % (cfg_obj,
                                            pprint.pformat(dir(cfg_obj))))
        str_list.append("--[ Resolved FAFT Config Values ]-------------------")
        for attr in self._filtered_attribute_list():
            str_list.append("%s = %s" % (attr, getattr(self, attr)))
        str_list.append("---------------------------------------------------")
        return "\n".join(str_list)
