"""A singleton class for accessing global config values

provides access to global configuration file
"""

# The config values can be stored in 3 config files:
#     global_config.ini
#     moblab_config.ini
#     shadow_config.ini
# When the code is running in Moblab, config values in moblab config override
# values in global config, and config values in shadow config override values
# in both moblab and global config.
# When the code is running in a non-Moblab host, moblab_config.ini is ignored.
# Config values in shadow config will override values in global config.

__author__ = 'raphtee@google.com (Travis Miller)'

import ConfigParser
import os
import re
import sys

from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib import lsbrelease_utils

class ConfigError(error.AutotestError):
    """Configuration error."""
    pass


class ConfigValueError(ConfigError):
    """Configuration value error, raised when value failed to be converted to
    expected type."""
    pass



common_lib_dir = os.path.dirname(sys.modules[__name__].__file__)
client_dir = os.path.dirname(common_lib_dir)
root_dir = os.path.dirname(client_dir)

# Check if the config files are at autotest's root dir
# This will happen if client is executing inside a full autotest tree, or if
# other entry points are being executed
global_config_path_root = os.path.join(root_dir, 'global_config.ini')
moblab_config_path_root = os.path.join(root_dir, 'moblab_config.ini')
shadow_config_path_root = os.path.join(root_dir, 'shadow_config.ini')
config_in_root = os.path.exists(global_config_path_root)

# Check if the config files are at autotest's client dir
# This will happen if a client stand alone execution is happening
global_config_path_client = os.path.join(client_dir, 'global_config.ini')
config_in_client = os.path.exists(global_config_path_client)

if config_in_root:
    DEFAULT_CONFIG_FILE = global_config_path_root
    if os.path.exists(moblab_config_path_root):
        DEFAULT_MOBLAB_FILE = moblab_config_path_root
    else:
        DEFAULT_MOBLAB_FILE = None
    if os.path.exists(shadow_config_path_root):
        DEFAULT_SHADOW_FILE = shadow_config_path_root
    else:
        DEFAULT_SHADOW_FILE = None
    RUNNING_STAND_ALONE_CLIENT = False
elif config_in_client:
    DEFAULT_CONFIG_FILE = global_config_path_client
    DEFAULT_MOBLAB_FILE = None
    DEFAULT_SHADOW_FILE = None
    RUNNING_STAND_ALONE_CLIENT = True
else:
    DEFAULT_CONFIG_FILE = None
    DEFAULT_MOBLAB_FILE = None
    DEFAULT_SHADOW_FILE = None
    RUNNING_STAND_ALONE_CLIENT = True

class global_config_class(object):
    """Object to access config values."""
    _NO_DEFAULT_SPECIFIED = object()

    config = None
    config_file = DEFAULT_CONFIG_FILE
    moblab_file=DEFAULT_MOBLAB_FILE
    shadow_file = DEFAULT_SHADOW_FILE
    running_stand_alone_client = RUNNING_STAND_ALONE_CLIENT


    def check_stand_alone_client_run(self):
        """Check if this is a stand alone client that does not need config."""
        return self.running_stand_alone_client


    def set_config_files(self, config_file=DEFAULT_CONFIG_FILE,
                         shadow_file=DEFAULT_SHADOW_FILE,
                         moblab_file=DEFAULT_MOBLAB_FILE):
        self.config_file = config_file
        self.moblab_file = moblab_file
        self.shadow_file = shadow_file
        self.config = None


    def _handle_no_value(self, section, key, default):
        if default is self._NO_DEFAULT_SPECIFIED:
            msg = ("Value '%s' not found in section '%s'" %
                   (key, section))
            raise ConfigError(msg)
        else:
            return default


    def get_section_values(self, section):
        """
        Return a config parser object containing a single section of the
        global configuration, that can be later written to a file object.

        @param section: Section we want to turn into a config parser object.
        @return: ConfigParser() object containing all the contents of section.
        """
        cfgparser = ConfigParser.ConfigParser()
        cfgparser.add_section(section)
        for option, value in self.config.items(section):
            cfgparser.set(section, option, value)
        return cfgparser


    def get_config_value(self, section, key, type=str,
                         default=_NO_DEFAULT_SPECIFIED, allow_blank=False):
        """Get a configuration value

        @param section: Section the key is in.
        @param key: The key to look up.
        @param type: The expected type of the returned value.
        @param default: A value to return in case the key couldn't be found.
        @param allow_blank: If False, an empty string as a value is treated like
                            there was no value at all. If True, empty strings
                            will be returned like they were normal values.

        @raises ConfigError: If the key could not be found and no default was
                             specified.

        @return: The obtained value or default.
        """
        self._ensure_config_parsed()

        try:
            val = self.config.get(section, key)
        except ConfigParser.Error:
            return self._handle_no_value(section, key, default)

        if not val.strip() and not allow_blank:
            return self._handle_no_value(section, key, default)

        return self._convert_value(key, section, val, type)


    def get_config_value_regex(self, section, key_regex, type=str):
        """Get a dict of configs in given section with key matched to key-regex.

        @param section: Section the key is in.
        @param key_regex: The regex that key should match.
        @param type: data type the value should have.

        @return: A dictionary of key:value with key matching `key_regex`. Return
                 an empty dictionary if no matching key is found.
        """
        configs = {}
        self._ensure_config_parsed()
        for option, value in self.config.items(section):
            if re.match(key_regex, option):
                configs[option] = self._convert_value(option, section, value,
                                                      type)
        return configs


    # This order of parameters ensures this can be called similar to the normal
    # get_config_value which is mostly called with (section, key, type).
    def get_config_value_with_fallback(self, section, key, fallback_key,
                                       type=str, fallback_section=None,
                                       default=_NO_DEFAULT_SPECIFIED, **kwargs):
        """Get a configuration value if it exists, otherwise use fallback.

        Tries to obtain a configuration value for a given key. If this value
        does not exist, the value looked up under a different key will be
        returned.

        @param section: Section the key is in.
        @param key: The key to look up.
        @param fallback_key: The key to use in case the original key wasn't
                             found.
        @param type: data type the value should have.
        @param fallback_section: The section the fallback key resides in. In
                                 case none is specified, the the same section as
                                 for the primary key is used.
        @param default: Value to return if values could neither be obtained for
                        the key nor the fallback key.
        @param **kwargs: Additional arguments that should be passed to
                         get_config_value.

        @raises ConfigError: If the fallback key doesn't exist and no default
                             was provided.

        @return: The value that was looked up for the key. If that didn't
                 exist, the value looked up for the fallback key will be
                 returned. If that also didn't exist, default will be returned.
        """
        if fallback_section is None:
            fallback_section = section

        try:
            return self.get_config_value(section, key, type, **kwargs)
        except ConfigError:
            return self.get_config_value(fallback_section, fallback_key,
                                         type, default=default, **kwargs)


    def override_config_value(self, section, key, new_value):
        """Override a value from the config file with a new value.

        @param section: Name of the section.
        @param key: Name of the key.
        @param new_value: new value.
        """
        self._ensure_config_parsed()
        self.config.set(section, key, new_value)


    def reset_config_values(self):
        """
        Reset all values to those found in the config files (undoes all
        overrides).
        """
        self.parse_config_file()


    def _ensure_config_parsed(self):
        """Make sure config files are parsed.
        """
        if self.config is None:
            self.parse_config_file()


    def merge_configs(self, override_config):
        """Merge existing config values with the ones in given override_config.

        @param override_config: Configs to override existing config values.
        """
        # overwrite whats in config with whats in override_config
        sections = override_config.sections()
        for section in sections:
            # add the section if need be
            if not self.config.has_section(section):
                self.config.add_section(section)
            # now run through all options and set them
            options = override_config.options(section)
            for option in options:
                val = override_config.get(section, option)
                self.config.set(section, option, val)


    def parse_config_file(self):
        """Parse config files."""
        self.config = ConfigParser.ConfigParser()
        if self.config_file and os.path.exists(self.config_file):
            self.config.read(self.config_file)
        else:
            raise ConfigError('%s not found' % (self.config_file))

        # If it's running in Moblab, read moblab config file if exists,
        # overwrite the value in global config.
        if (lsbrelease_utils.is_moblab() and self.moblab_file and
            os.path.exists(self.moblab_file)):
            moblab_config = ConfigParser.ConfigParser()
            moblab_config.read(self.moblab_file)
            # now we merge moblab into global
            self.merge_configs(moblab_config)

        # now also read the shadow file if there is one
        # this will overwrite anything that is found in the
        # other config
        if self.shadow_file and os.path.exists(self.shadow_file):
            shadow_config = ConfigParser.ConfigParser()
            shadow_config.read(self.shadow_file)
            # now we merge shadow into global
            self.merge_configs(shadow_config)


    # the values that are pulled from ini
    # are strings.  But we should attempt to
    # convert them to other types if needed.
    def _convert_value(self, key, section, value, value_type):
        # strip off leading and trailing white space
        sval = value.strip()

        # if length of string is zero then return None
        if len(sval) == 0:
            if value_type == str:
                return ""
            elif value_type == bool:
                return False
            elif value_type == int:
                return 0
            elif value_type == float:
                return 0.0
            elif value_type == list:
                return []
            else:
                return None

        if value_type == bool:
            if sval.lower() == "false":
                return False
            else:
                return True

        if value_type == list:
            # Split the string using ',' and return a list
            return [val.strip() for val in sval.split(',')]

        try:
            conv_val = value_type(sval)
            return conv_val
        except:
            msg = ("Could not convert %s value %r in section %s to type %s" %
                    (key, sval, section, value_type))
            raise ConfigValueError(msg)


    def get_sections(self):
        """Return a list of sections available."""
        self._ensure_config_parsed()
        return self.config.sections()


# insure the class is a singleton.  Now the symbol global_config
# will point to the one and only one instace of the class
global_config = global_config_class()
