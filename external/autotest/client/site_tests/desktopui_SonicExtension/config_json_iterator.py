# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import collections
import json
import os


class ConfigJsonIteratorError(Exception):
    """"Exception for config json iterator"""
    pass


class ConfigJsonIterator(object):
    """Class to consolidate multiple config json files.

    This class reads and combines input JSON instances into one based on the
    following rules:
    1. "_deps" value in the root config file contains a list of common config
       file paths. Each path represents a RELATIVE path to
       the root config file.
       For example (common.config is in the same directory as root.config):
       root.config:
           { "a": "123",
             "_deps": ["../common.config"]}
       common.config:
           { "b": "xxx" }
       End output:
           { "a": "123",
             "b": "xxx" }
    2. common config files defined in "_deps" MUST NOT contain identical keys
       (otherwise an exception will be thrown), for example (invalid - common1
       and common2.config are in the same directory as root.config):
       root.config:
           { "a": "123",
             "_deps": ["../common1.config",
                       "../common2.config"]}
       common1.config:
           { "b": "xxx" }
       common2.config:
           { "b": "yyy" }
    3. values in the root config will override the ones in the common config
       files. This logic applies to any dependency config file (imagine that
       the common config also has "_deps"), thus is recursive.
       For example (common.config is in the same directory as root.config):
       root.config:
           { "a": "123",
             "_deps": ["../common.config"]}
       common.config:
           { "a": "456",
             "b": "xxx" }
       End output:
           { "a": "123",
             "b": "xxx" }
    """
    DEPS = '_deps'


    def __init__(self, config_path=None):
        """Constructor.

        @param config_path: String of root config file path.
        """
        if config_path:
            self.set_config_dir(config_path)


    def set_config_dir(self, config_path):
        """Sets config dictionary.

        @param config_path: String of config file path.
        @raises ConfigJsonIteratorError if config does not exist.
        """
        if not os.path.isfile(config_path):
            raise ConfigJsonIteratorError('config file does not exist %s'
                                          % config_path)
        self._config_dir = os.path.abspath(os.path.dirname(config_path))


    def _load_config(self, config_path):
        """Iterate the base config file.

        @param config_path: String of config file path.
        @return Dictionary of the config file.
        @raises ConfigJsonIteratorError: if config file is not found or invalid.
        """
        if not os.path.isfile(config_path):
            raise ConfigJsonIteratorError('config file does not exist %s'
                                          % config_path)
        with open(config_path, 'r') as config_file:
            try:
                return json.load(config_file)
            except ValueError:
                raise ConfigJsonIteratorError(
                        'invalid JSON file %s' % config_file)


    def aggregated_config(self, config_path):
        """Returns dictionary of aggregated config files.
        The dependency list contains the RELATIVE path to the root config.

        @param config_path: String of config file path.
        @return Dictionary containing the aggregated config files.
        @raises ConfigJsonIteratorError: if dependency config list
            does not exist.
        """
        ret_dict = self._load_config(config_path)
        if ConfigJsonIterator.DEPS not in ret_dict:
            return ret_dict
        else:
            deps_list = ret_dict[ConfigJsonIterator.DEPS]
            if not isinstance(deps_list, list):
                raise ConfigJsonIteratorError('dependency must be a list %s'
                                              % deps_list)
            del ret_dict[ConfigJsonIterator.DEPS]
            common_dict = {}
            for dep in deps_list:
                common_config_path = os.path.join(self._config_dir, dep)
                dep_dict = self.aggregated_config(common_config_path)
                common_dict = self._merge_dict(common_dict, dep_dict,
                                               allow_override=False)
            return self._merge_dict(common_dict, ret_dict, allow_override=True)


    def _merge_dict(self, dict_one, dict_two, allow_override=True):
        """Returns a merged dictionary.

        @param dict_one: Dictionary to merge (first).
        @param dict_two: Dictionary to merge (second).
        @param allow_override: Boolean to allow override or not.
        @return Dictionary containing merged result.
        @raises ConfigJsonIteratorError: if no dictionary given.
        """
        if not isinstance(dict_one, dict) or not isinstance(dict_two, dict):
            raise ConfigJsonIteratorError('Input is not a dictionary')
        if allow_override:
            return dict(dict_one.items() + dict_two.items())
        else:
            merge = collections.Counter(
                    dict_one.keys() + dict_two.keys()).most_common()[0]
            if merge[1] > 1:
                raise ConfigJsonIteratorError(
                    'Duplicate key %s found', merge[0])
            return dict_one
