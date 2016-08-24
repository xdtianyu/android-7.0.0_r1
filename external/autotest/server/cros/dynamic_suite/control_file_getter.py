# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import common
import logging, os, re
from autotest_lib.client.common_lib import error, utils
from autotest_lib.client.common_lib.cros import dev_server


# Relevant CrosDynamicSuiteExceptions are defined in client/common_lib/error.py.


class ControlFileGetter(object):
    """
    Interface for classes that can list and fetch known control files.
    """

    def __init__(self):
        pass


    def get_control_file_list(self, suite_name=''):
        """
        Gather a list of paths to control files.

        @param suite_name: The name of a suite we would like control files for.
        @return A list of file paths.
        @throws NoControlFileList if there is an error while listing.
        """
        pass


    def get_control_file_contents(self, test_path):
        """
        Given a path to a control file, return its contents.

        @param test_path: the path to the control file.
        @return the contents of the control file specified by the path.
        @throws ControlFileNotFound if the file cannot be retrieved.
        """
        pass


    def get_control_file_contents_by_name(self, test_name):
        """
        Given the name of a control file, return its contents.

        @param test_name: the name of the test whose control file is desired.
        @return the contents of the control file specified by the name.
        @throws ControlFileNotFound if the file cannot be retrieved.
        """
        pass


class CacheingAndFilteringControlFileGetter(ControlFileGetter):
    """Wraps ControlFileGetter to cache the retrieved control file list and
    filter out unwanted control files."""

    CONTROL_FILE_FILTERS = ['src/debian/control']

    def __init__(self):
        super(CacheingAndFilteringControlFileGetter, self).__init__()
        self._files = []


    def get_control_file_list(self, suite_name=''):
        """
        Gather a list of paths to control files.

        Gets a list of control files; populates |self._files| with that list
        and then returns the paths to all useful and wanted files in the list.

        @param suite_name: The name of a suite we would like control files for.
        @return A list of file paths.
        @throws NoControlFileList if there is an error while listing.
        """
        files = self._get_control_file_list(suite_name=suite_name)
        for cf_filter in self.CONTROL_FILE_FILTERS:
          files = filter(lambda path: not path.endswith(cf_filter), files)
        self._files = files
        return self._files


    def get_control_file_contents_by_name(self, test_name):
        """
        Given the name of a control file, return its contents.

        Searches through previously-compiled list in |self._files| for a
        test named |test_name| and returns the contents of the control file
        for that test if it is found.

        @param test_name: the name of the test whose control file is desired.
        @return the contents of the control file specified by the name.
        @throws ControlFileNotFound if the file cannot be retrieved.
        """
        if not self._files and not self.get_control_file_list():
            raise error.ControlFileNotFound('No control files found.')

        if 'control' not in test_name:
            regexp = re.compile(os.path.join(test_name, 'control$'))
        else:
            regexp = re.compile(test_name + '$')
        candidates = filter(regexp.search, self._files)
        if not candidates:
            raise error.ControlFileNotFound('No control file for ' + test_name)
        if len(candidates) > 1:
            raise error.ControlFileNotFound(test_name + ' is not unique.')
        return self.get_control_file_contents(candidates[0])


class FileSystemGetter(CacheingAndFilteringControlFileGetter):
    """
    Class that can list and fetch known control files from disk.

    @var _CONTROL_PATTERN: control file name format to match.
    """

    _CONTROL_PATTERN = '^control(?:\..+)?$'

    def __init__(self, paths):
        """
        @param paths: base directories to start search.
        """
        super(FileSystemGetter, self).__init__()
        self._paths = paths


    def _is_useful_file(self, name):
        return '__init__.py' not in name and '.svn' not in name


    def _get_control_file_list(self, suite_name=''):
        """
        Gather a list of paths to control files under |self._paths|.

        Searches under |self._paths| for files that match
        |self._CONTROL_PATTERN|.  Populates |self._files| with that list
        and then returns the paths to all useful files in the list.

        @param suite_name: The name of a suite we would like control files for.
        @return A list of files that match |self._CONTROL_PATTERN|.
        @throws NoControlFileList if we find no files.
        """
        if suite_name:
            logging.debug('Getting control files for a specific suite has '
                          'not been implemented for FileSystemGetter. '
                          'Getting all control files instead.')


        regexp = re.compile(self._CONTROL_PATTERN)
        directories = self._paths
        while len(directories) > 0:
            directory = directories.pop()
            if not os.path.exists(directory):
                continue
            try:
                for name in os.listdir(directory):
                    fullpath = os.path.join(directory, name)
                    if os.path.isfile(fullpath):
                        if regexp.search(name):
                            # if we are a control file
                            self._files.append(fullpath)
                    elif os.path.isdir(fullpath):
                        directories.append(fullpath)
            except OSError:
                # Some directories under results/ like the Chrome Crash
                # Reports will cause issues when attempted to be searched.
                logging.error('Unable to search directory %d for control '
                              'files.', directory)
                pass
        if not self._files:
            msg = 'No control files under ' + ','.join(self._paths)
            raise error.NoControlFileList(msg)
        return [f for f in self._files if self._is_useful_file(f)]


    def get_control_file_contents(self, test_path):
        """
        Get the contents of the control file at |test_path|.

        @return The contents of the aforementioned file.
        @throws ControlFileNotFound if the file cannot be retrieved.
        """
        try:
            return utils.read_file(test_path)
        except EnvironmentError as (errno, strerror):
            msg = "Can't retrieve {0}: {1} ({2})".format(test_path,
                                                         strerror,
                                                         errno)
            raise error.ControlFileNotFound(msg)


class DevServerGetter(CacheingAndFilteringControlFileGetter):
    """Class that can list and fetch known control files from DevServer.

    @var _CONTROL_PATTERN: control file name format to match.
    """
    def __init__(self, build, ds):
        """
        @param build: The build from which to get control files.
        @param ds: An existing dev_server.DevServer object to use.
        """
        super(DevServerGetter, self).__init__()
        self._dev_server = ds
        self._build = build


    @staticmethod
    def create(build, ds=None):
        """Wraps constructor.  Can be mocked for testing purposes.
        @param build: The build from which to get control files.
        @param ds: An existing dev_server.DevServer object to use
                  (default=None)
        @returns: New DevServerGetter.
        """
        return DevServerGetter(build, ds)


    def _get_control_file_list(self, suite_name=''):
        """
        Gather a list of paths to control files from |self._dev_server|.

        Get a listing of all the control files for |self._build| on
        |self._dev_server|.  Populates |self._files| with that list
        and then returns paths (under the autotest dir) to them. If suite_name
        is specified, this method populates |self._files| with the control
        files from just the specified suite.

        @param suite_name: The name of a suite we would like control files for.
        @return A list of control file paths.
        @throws NoControlFileList if there is an error while listing.
        """
        try:
            return self._dev_server.list_control_files(self._build,
                                                       suite_name=suite_name)
        except dev_server.DevServerException as e:
            raise error.NoControlFileList(e)


    def get_control_file_contents(self, test_path):
        """
        Return the contents of |test_path| from |self._dev_server|.

        Get the contents of the control file at |test_path| for |self._build| on
        |self._dev_server|.

        @return The contents of |test_path|.  None on failure.
        @throws ControlFileNotFound if the file cannot be retrieved.
        """
        try:
            return self._dev_server.get_control_file(self._build, test_path)
        except dev_server.DevServerException as e:
            raise error.ControlFileNotFound(e)
