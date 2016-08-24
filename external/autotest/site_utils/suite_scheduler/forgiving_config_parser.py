# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import ConfigParser


def forgive_config_error(func):
    """A decorator to make ConfigParser get*() functions return None on fail."""
    def wrapper(*args, **kwargs):
        try:
            return func(*args, **kwargs)
        except ConfigParser.Error:
            return None
    return wrapper


class ForgivingConfigParser(ConfigParser.SafeConfigParser):
    """A SafeConfigParser that returns None on any error in get*().

    Also implements reread(), which allows any already-read-in configs to be
    reloaded from disk on-demand.

    Note that I can't use super() here, as ConfigParser.SafeConfigParser
    isn't a new-style class.

    @var _cached_config_file_names: the names of the config files last read().
    """


    def __init__(self):
        ConfigParser.SafeConfigParser.__init__(self)
        self._cached_config_file_names = ''


    def read(self, filenames):
        """Caches filenames, then performs normal read() functionality.

        @param filenames: string or iterable.  The files to read.
        @return list of files that could not be read, as per super class.
        """
        to_return = ConfigParser.SafeConfigParser.read(self, filenames)
        self._cached_config_file_names = filenames
        return to_return


    def reread(self):
        """Clear all sections, re-read configs from disk."""
        for section in self.sections():
            self.remove_section(section)
        return ConfigParser.SafeConfigParser.read(
            self, self._cached_config_file_names)


    @forgive_config_error
    def getstring(self, section, option):
        """Can't override get(), as it breaks the other getters to have get()
        return None sometimes."""
        return ConfigParser.SafeConfigParser.get(self, section, option)


    @forgive_config_error
    def getint(self, section, option):
        return ConfigParser.SafeConfigParser.getint(self, section, option)


    @forgive_config_error
    def getfloat(self, section, option):
        return ConfigParser.SafeConfigParser.getfloat(self, section, option)


    @forgive_config_error
    def getboolean(self, section, option):
        return ConfigParser.SafeConfigParser.getboolean(self, section, option)
