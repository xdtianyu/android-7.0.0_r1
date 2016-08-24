# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os

import config_json_iterator


class ConfigurationManager(object):
    """A helper class to read configuration file.

    This class will load a given configuration file and
    save all the settings into a dictionary.
    """

    def __init__(self, config):
        """Constructor

        @param config: String of config file path.
        """
        if os.path.isfile(config):
            config_parser = config_json_iterator.ConfigJsonIterator()
            config_parser.set_config_dir(config)
            self._settings = config_parser.aggregated_config(config)
        else:
            raise IOError('configuration file does not exist')


    def get_config_settings(self):
        """Returns all _settings."""
        return self._settings
