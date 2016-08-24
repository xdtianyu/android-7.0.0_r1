# Copyright 2009 Google Inc. Released under the GPL v2

"""
This file contains the implementation of a host object for the local machine.
"""

import distutils.core, glob, os, platform, shutil
from autotest_lib.client.common_lib import hosts, error
from autotest_lib.client.bin import utils

class LocalHost(hosts.Host):
    """This class represents a host running locally on the host."""


    def _initialize(self, hostname=None, bootloader=None, *args, **dargs):
        super(LocalHost, self)._initialize(*args, **dargs)

        # hostname will be an actual hostname when this client was created
        # by an autoserv process
        if not hostname:
            hostname = platform.node()
        self.hostname = hostname
        self.bootloader = bootloader
        self.tmp_dirs = []


    def close(self):
        """Cleanup after we're done."""
        for tmp_dir in self.tmp_dirs:
            self.run('rm -rf "%s"' % (utils.sh_escape(tmp_dir)),
                     ignore_status=True)


    def wait_up(self, timeout=None):
        # a local host is always up
        return True


    def run(self, command, timeout=3600, ignore_status=False,
            stdout_tee=utils.TEE_TO_LOGS, stderr_tee=utils.TEE_TO_LOGS,
            stdin=None, args=(), **kwargs):
        """
        @see common_lib.hosts.Host.run()
        """
        try:
            result = utils.run(
                command, timeout=timeout, ignore_status=True,
                stdout_tee=stdout_tee, stderr_tee=stderr_tee, stdin=stdin,
                args=args)
        except error.CmdError, e:
            # this indicates a timeout exception
            raise error.AutotestHostRunError('command timed out', e.result_obj)

        if not ignore_status and result.exit_status > 0:
            raise error.AutotestHostRunError('command execution error', result)

        return result


    def list_files_glob(self, path_glob):
        """
        Get a list of files on a remote host given a glob pattern path.
        """
        return glob.glob(path_glob)


    def symlink_closure(self, paths):
        """
        Given a sequence of path strings, return the set of all paths that
        can be reached from the initial set by following symlinks.

        @param paths: sequence of path strings.
        @return: a sequence of path strings that are all the unique paths that
                can be reached from the given ones after following symlinks.
        """
        paths = set(paths)
        closure = set()

        while paths:
            path = paths.pop()
            if not os.path.exists(path):
                continue
            closure.add(path)
            if os.path.islink(path):
                link_to = os.path.join(os.path.dirname(path),
                                       os.readlink(path))
                if link_to not in closure:
                    paths.add(link_to)

        return closure


    def _copy_file(self, source, dest, delete_dest=False, preserve_perm=False,
                   preserve_symlinks=False):
        """Copy files from source to dest, will be the base for {get,send}_file.

        @param source: The file/directory on localhost to copy.
        @param dest: The destination path on localhost to copy to.
        @param delete_dest: A flag set to choose whether or not to delete
                            dest if it exists.
        @param preserve_perm: Tells get_file() to try to preserve the sources
                              permissions on files and dirs.
        @param preserve_symlinks: Try to preserve symlinks instead of
                                  transforming them into files/dirs on copy.
        """
        if delete_dest and os.path.exists(dest):
            # Check if it's a file or a dir and use proper remove method.
            if os.path.isdir(dest):
                shutil.rmtree(dest)
            else:
                os.remove(dest)

        if preserve_symlinks and os.path.islink(source):
            os.symlink(os.readlink(source), dest)
        # If source is a dir, use distutils.dir_util.copytree since
        # shutil.copy_tree has weird limitations.
        elif os.path.isdir(source):
            distutils.dir_util.copy_tree(source, dest,
                    preserve_symlinks=preserve_symlinks,
                    preserve_mode=preserve_perm,
                    update=1)
        else:
            shutil.copyfile(source, dest)

        if preserve_perm:
            shutil.copymode(source, dest)


    def get_file(self, source, dest, delete_dest=False, preserve_perm=True,
                 preserve_symlinks=False):
        """Copy files from source to dest.

        @param source: The file/directory on localhost to copy.
        @param dest: The destination path on localhost to copy to.
        @param delete_dest: A flag set to choose whether or not to delete
                            dest if it exists.
        @param preserve_perm: Tells get_file() to try to preserve the sources
                              permissions on files and dirs.
        @param preserve_symlinks: Try to preserve symlinks instead of
                                  transforming them into files/dirs on copy.
        """
        self._copy_file(source, dest, delete_dest=delete_dest,
                        preserve_perm=preserve_perm,
                        preserve_symlinks=preserve_symlinks)


    def send_file(self, source, dest, delete_dest=False,
                  preserve_symlinks=False):
        """Copy files from source to dest.

        @param source: The file/directory on the drone to send to the device.
        @param dest: The destination path on the device to copy to.
        @param delete_dest: A flag set to choose whether or not to delete
                            dest on the device if it exists.
        @param preserve_symlinks: Controls if symlinks on the source will be
                                  copied as such on the destination or
                                  transformed into the referenced
                                  file/directory.
        """
        self._copy_file(source, dest, delete_dest=delete_dest,
                        preserve_symlinks=preserve_symlinks)


    def get_tmp_dir(self, parent='/tmp'):
        """
        Return the pathname of a directory on the host suitable
        for temporary file storage.

        The directory and its content will be deleted automatically
        on the destruction of the Host object that was used to obtain
        it.

        @param parent: The leading path to make the tmp dir.
        """
        self.run('mkdir -p "%s"' % parent)
        tmp_dir = self.run('mktemp -d -p "%s"' % parent).stdout.rstrip()
        self.tmp_dirs.append(tmp_dir)
        return tmp_dir
