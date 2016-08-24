import os, time, socket, shutil, glob, logging, traceback, tempfile, re
import subprocess

from multiprocessing import Lock
from autotest_lib.client.common_lib import autotemp, error
from autotest_lib.server import utils, autotest
from autotest_lib.server.hosts import remote
from autotest_lib.server.hosts import rpc_server_tracker
from autotest_lib.client.common_lib.global_config import global_config

# pylint: disable-msg=C0111

get_value = global_config.get_config_value
enable_master_ssh = get_value('AUTOSERV', 'enable_master_ssh', type=bool,
                              default=False)


class AbstractSSHHost(remote.RemoteHost):
    """
    This class represents a generic implementation of most of the
    framework necessary for controlling a host via ssh. It implements
    almost all of the abstract Host methods, except for the core
    Host.run method.
    """
    VERSION_PREFIX = ''

    def _initialize(self, hostname, user="root", port=22, password="",
                    is_client_install_supported=True, host_attributes={},
                    *args, **dargs):
        super(AbstractSSHHost, self)._initialize(hostname=hostname,
                                                 *args, **dargs)
        # IP address is retrieved only on demand. Otherwise the host
        # initialization will fail for host is not online.
        self._ip = None
        self.user = user
        self.port = port
        self.password = password
        self._is_client_install_supported = is_client_install_supported
        self._use_rsync = None
        self.known_hosts_file = tempfile.mkstemp()[1]
        self._rpc_server_tracker = rpc_server_tracker.RpcServerTracker(self);

        """
        Master SSH connection background job, socket temp directory and socket
        control path option. If master-SSH is enabled, these fields will be
        initialized by start_master_ssh when a new SSH connection is initiated.
        """
        self.master_ssh_job = None
        self.master_ssh_tempdir = None
        self.master_ssh_option = ''

        # Create a Lock to protect against race conditions.
        self._lock = Lock()

        self.host_attributes = host_attributes


    @property
    def ip(self):
        """@return IP address of the host.
        """
        if not self._ip:
            self._ip = socket.getaddrinfo(self.hostname, None)[0][4][0]
        return self._ip


    @property
    def is_client_install_supported(self):
        """"
        Returns True if the host supports autotest client installs, False
        otherwise.
        """
        return self._is_client_install_supported


    @property
    def rpc_server_tracker(self):
        """"
        @return The RPC server tracker associated with this host.
        """
        return self._rpc_server_tracker


    def make_ssh_command(self, user="root", port=22, opts='',
                         hosts_file='/dev/null',
                         connect_timeout=30, alive_interval=300):
        base_command = ("/usr/bin/ssh -a -x %s -o StrictHostKeyChecking=no "
                        "-o UserKnownHostsFile=%s -o BatchMode=yes "
                        "-o ConnectTimeout=%d -o ServerAliveInterval=%d "
                        "-l %s -p %d")
        assert isinstance(connect_timeout, (int, long))
        assert connect_timeout > 0 # can't disable the timeout
        return base_command % (opts, hosts_file, connect_timeout,
                               alive_interval, user, port)


    def use_rsync(self):
        if self._use_rsync is not None:
            return self._use_rsync

        # Check if rsync is available on the remote host. If it's not,
        # don't try to use it for any future file transfers.
        self._use_rsync = self._check_rsync()
        if not self._use_rsync:
            logging.warning("rsync not available on remote host %s -- disabled",
                         self.hostname)
        return self._use_rsync


    def _check_rsync(self):
        """
        Check if rsync is available on the remote host.
        """
        try:
            self.run("rsync --version", stdout_tee=None, stderr_tee=None)
        except error.AutoservRunError:
            return False
        return True


    def _encode_remote_paths(self, paths, escape=True):
        """
        Given a list of file paths, encodes it as a single remote path, in
        the style used by rsync and scp.
        """
        if escape:
            paths = [utils.scp_remote_escape(path) for path in paths]

        remote = self.hostname

        # rsync and scp require IPv6 brackets, even when there isn't any
        # trailing port number (ssh doesn't support IPv6 brackets).
        # In the Python >= 3.3 future, 'import ipaddress' will parse addresses.
        if re.search(r':.*:', remote):
            remote = '[%s]' % remote

        return '%s@%s:"%s"' % (self.user, remote, " ".join(paths))


    def _make_rsync_cmd(self, sources, dest, delete_dest, preserve_symlinks):
        """
        Given a list of source paths and a destination path, produces the
        appropriate rsync command for copying them. Remote paths must be
        pre-encoded.
        """
        ssh_cmd = self.make_ssh_command(user=self.user, port=self.port,
                                        opts=self.master_ssh_option,
                                        hosts_file=self.known_hosts_file)
        if delete_dest:
            delete_flag = "--delete"
        else:
            delete_flag = ""
        if preserve_symlinks:
            symlink_flag = ""
        else:
            symlink_flag = "-L"
        command = ("rsync %s %s --timeout=1800 --rsh='%s' -az --no-o --no-g "
                   "%s \"%s\"")
        return command % (symlink_flag, delete_flag, ssh_cmd,
                          " ".join(['"%s"' % p for p in sources]), dest)


    def _make_ssh_cmd(self, cmd):
        """
        Create a base ssh command string for the host which can be used
        to run commands directly on the machine
        """
        base_cmd = self.make_ssh_command(user=self.user, port=self.port,
                                         opts=self.master_ssh_option,
                                         hosts_file=self.known_hosts_file)

        return '%s %s "%s"' % (base_cmd, self.hostname, utils.sh_escape(cmd))

    def _make_scp_cmd(self, sources, dest):
        """
        Given a list of source paths and a destination path, produces the
        appropriate scp command for encoding it. Remote paths must be
        pre-encoded.
        """
        command = ("scp -rq %s -o StrictHostKeyChecking=no "
                   "-o UserKnownHostsFile=%s -P %d %s '%s'")
        return command % (self.master_ssh_option, self.known_hosts_file,
                          self.port, " ".join(sources), dest)


    def _make_rsync_compatible_globs(self, path, is_local):
        """
        Given an rsync-style path, returns a list of globbed paths
        that will hopefully provide equivalent behaviour for scp. Does not
        support the full range of rsync pattern matching behaviour, only that
        exposed in the get/send_file interface (trailing slashes).

        The is_local param is flag indicating if the paths should be
        interpreted as local or remote paths.
        """

        # non-trailing slash paths should just work
        if len(path) == 0 or path[-1] != "/":
            return [path]

        # make a function to test if a pattern matches any files
        if is_local:
            def glob_matches_files(path, pattern):
                return len(glob.glob(path + pattern)) > 0
        else:
            def glob_matches_files(path, pattern):
                result = self.run("ls \"%s\"%s" % (utils.sh_escape(path),
                                                   pattern),
                                  stdout_tee=None, ignore_status=True)
                return result.exit_status == 0

        # take a set of globs that cover all files, and see which are needed
        patterns = ["*", ".[!.]*"]
        patterns = [p for p in patterns if glob_matches_files(path, p)]

        # convert them into a set of paths suitable for the commandline
        if is_local:
            return ["\"%s\"%s" % (utils.sh_escape(path), pattern)
                    for pattern in patterns]
        else:
            return [utils.scp_remote_escape(path) + pattern
                    for pattern in patterns]


    def _make_rsync_compatible_source(self, source, is_local):
        """
        Applies the same logic as _make_rsync_compatible_globs, but
        applies it to an entire list of sources, producing a new list of
        sources, properly quoted.
        """
        return sum((self._make_rsync_compatible_globs(path, is_local)
                    for path in source), [])


    def _set_umask_perms(self, dest):
        """
        Given a destination file/dir (recursively) set the permissions on
        all the files and directories to the max allowed by running umask.
        """

        # now this looks strange but I haven't found a way in Python to _just_
        # get the umask, apparently the only option is to try to set it
        umask = os.umask(0)
        os.umask(umask)

        max_privs = 0777 & ~umask

        def set_file_privs(filename):
            """Sets mode of |filename|.  Assumes |filename| exists."""
            file_stat = os.stat(filename)

            file_privs = max_privs
            # if the original file permissions do not have at least one
            # executable bit then do not set it anywhere
            if not file_stat.st_mode & 0111:
                file_privs &= ~0111

            os.chmod(filename, file_privs)

        # try a bottom-up walk so changes on directory permissions won't cut
        # our access to the files/directories inside it
        for root, dirs, files in os.walk(dest, topdown=False):
            # when setting the privileges we emulate the chmod "X" behaviour
            # that sets to execute only if it is a directory or any of the
            # owner/group/other already has execute right
            for dirname in dirs:
                os.chmod(os.path.join(root, dirname), max_privs)

            # Filter out broken symlinks as we go.
            for filename in filter(os.path.exists, files):
                set_file_privs(os.path.join(root, filename))


        # now set privs for the dest itself
        if os.path.isdir(dest):
            os.chmod(dest, max_privs)
        else:
            set_file_privs(dest)


    def get_file(self, source, dest, delete_dest=False, preserve_perm=True,
                 preserve_symlinks=False):
        """
        Copy files from the remote host to a local path.

        Directories will be copied recursively.
        If a source component is a directory with a trailing slash,
        the content of the directory will be copied, otherwise, the
        directory itself and its content will be copied. This
        behavior is similar to that of the program 'rsync'.

        Args:
                source: either
                        1) a single file or directory, as a string
                        2) a list of one or more (possibly mixed)
                                files or directories
                dest: a file or a directory (if source contains a
                        directory or more than one element, you must
                        supply a directory dest)
                delete_dest: if this is true, the command will also clear
                             out any old files at dest that are not in the
                             source
                preserve_perm: tells get_file() to try to preserve the sources
                               permissions on files and dirs
                preserve_symlinks: try to preserve symlinks instead of
                                   transforming them into files/dirs on copy

        Raises:
                AutoservRunError: the scp command failed
        """
        logging.debug('get_file. source: %s, dest: %s, delete_dest: %s,'
                      'preserve_perm: %s, preserve_symlinks:%s', source, dest,
                      delete_dest, preserve_perm, preserve_symlinks)
        # Start a master SSH connection if necessary.
        self.start_master_ssh()

        if isinstance(source, basestring):
            source = [source]
        dest = os.path.abspath(dest)

        # If rsync is disabled or fails, try scp.
        try_scp = True
        if self.use_rsync():
            logging.debug('Using Rsync.')
            try:
                remote_source = self._encode_remote_paths(source)
                local_dest = utils.sh_escape(dest)
                rsync = self._make_rsync_cmd([remote_source], local_dest,
                                             delete_dest, preserve_symlinks)
                utils.run(rsync)
                try_scp = False
            except error.CmdError, e:
                logging.warning("trying scp, rsync failed: %s", e)

        if try_scp:
            logging.debug('Trying scp.')
            # scp has no equivalent to --delete, just drop the entire dest dir
            if delete_dest and os.path.isdir(dest):
                shutil.rmtree(dest)
                os.mkdir(dest)

            remote_source = self._make_rsync_compatible_source(source, False)
            if remote_source:
                # _make_rsync_compatible_source() already did the escaping
                remote_source = self._encode_remote_paths(remote_source,
                                                          escape=False)
                local_dest = utils.sh_escape(dest)
                scp = self._make_scp_cmd([remote_source], local_dest)
                try:
                    utils.run(scp)
                except error.CmdError, e:
                    logging.debug('scp failed: %s', e)
                    raise error.AutoservRunError(e.args[0], e.args[1])

        if not preserve_perm:
            # we have no way to tell scp to not try to preserve the
            # permissions so set them after copy instead.
            # for rsync we could use "--no-p --chmod=ugo=rwX" but those
            # options are only in very recent rsync versions
            self._set_umask_perms(dest)


    def send_file(self, source, dest, delete_dest=False,
                  preserve_symlinks=False):
        """
        Copy files from a local path to the remote host.

        Directories will be copied recursively.
        If a source component is a directory with a trailing slash,
        the content of the directory will be copied, otherwise, the
        directory itself and its content will be copied. This
        behavior is similar to that of the program 'rsync'.

        Args:
                source: either
                        1) a single file or directory, as a string
                        2) a list of one or more (possibly mixed)
                                files or directories
                dest: a file or a directory (if source contains a
                        directory or more than one element, you must
                        supply a directory dest)
                delete_dest: if this is true, the command will also clear
                             out any old files at dest that are not in the
                             source
                preserve_symlinks: controls if symlinks on the source will be
                    copied as such on the destination or transformed into the
                    referenced file/directory

        Raises:
                AutoservRunError: the scp command failed
        """
        logging.debug('send_file. source: %s, dest: %s, delete_dest: %s,'
                      'preserve_symlinks:%s', source, dest,
                      delete_dest, preserve_symlinks)
        # Start a master SSH connection if necessary.
        self.start_master_ssh()

        if isinstance(source, basestring):
            source = [source]
        remote_dest = self._encode_remote_paths([dest])

        local_sources = [utils.sh_escape(path) for path in source]
        if not local_sources:
            raise error.TestError('source |%s| yielded an empty list' % (
                source))
        if any([local_source.find('\x00') != -1 for
                local_source in local_sources]):
            raise error.TestError('one or more sources include NUL char')

        # If rsync is disabled or fails, try scp.
        try_scp = True
        if self.use_rsync():
            logging.debug('Using Rsync.')
            try:
                rsync = self._make_rsync_cmd(local_sources, remote_dest,
                                             delete_dest, preserve_symlinks)
                utils.run(rsync)
                try_scp = False
            except error.CmdError, e:
                logging.warning("trying scp, rsync failed: %s", e)

        if try_scp:
            logging.debug('Trying scp.')
            # scp has no equivalent to --delete, just drop the entire dest dir
            if delete_dest:
                is_dir = self.run("ls -d %s/" % dest,
                                  ignore_status=True).exit_status == 0
                if is_dir:
                    cmd = "rm -rf %s && mkdir %s"
                    cmd %= (dest, dest)
                    self.run(cmd)

            local_sources = self._make_rsync_compatible_source(source, True)
            if local_sources:
                scp = self._make_scp_cmd(local_sources, remote_dest)
                try:
                    utils.run(scp)
                except error.CmdError, e:
                    logging.debug('scp failed: %s', e)
                    raise error.AutoservRunError(e.args[0], e.args[1])
            else:
                logging.debug('skipping scp for empty source list')


    def verify_ssh_user_access(self):
        """Verify ssh access to this host.

        @returns False if ssh_ping fails due to Permissions error, True
                 otherwise.
        """
        try:
            self.ssh_ping()
        except (error.AutoservSshPermissionDeniedError,
                error.AutoservSshPingHostError):
            return False
        return True


    def ssh_ping(self, timeout=60, base_cmd='true'):
        """
        Pings remote host via ssh.

        @param timeout: Time in seconds before giving up.
                        Defaults to 60 seconds.
        @param base_cmd: The base command to run with the ssh ping.
                         Defaults to true.
        @raise AutoservSSHTimeout: If the ssh ping times out.
        @raise AutoservSshPermissionDeniedError: If ssh ping fails due to
                                                 permissions.
        @raise AutoservSshPingHostError: For other AutoservRunErrors.
        """
        try:
            self.run(base_cmd, timeout=timeout, connect_timeout=timeout)
        except error.AutoservSSHTimeout:
            msg = "Host (ssh) verify timed out (timeout = %d)" % timeout
            raise error.AutoservSSHTimeout(msg)
        except error.AutoservSshPermissionDeniedError:
            #let AutoservSshPermissionDeniedError be visible to the callers
            raise
        except error.AutoservRunError, e:
            # convert the generic AutoservRunError into something more
            # specific for this context
            raise error.AutoservSshPingHostError(e.description + '\n' +
                                                 repr(e.result_obj))


    def is_up(self, timeout=60, base_cmd='true'):
        """
        Check if the remote host is up by ssh-ing and running a base command.

        @param timeout: timeout in seconds.
        @param base_cmd: a base command to run with ssh. The default is 'true'.
        @returns True if the remote host is up before the timeout expires,
                 False otherwise.
        """
        try:
            self.ssh_ping(timeout=timeout, base_cmd=base_cmd)
        except error.AutoservError:
            return False
        else:
            return True


    def wait_up(self, timeout=None):
        """
        Wait until the remote host is up or the timeout expires.

        In fact, it will wait until an ssh connection to the remote
        host can be established, and getty is running.

        @param timeout time limit in seconds before returning even
            if the host is not up.

        @returns True if the host was found to be up before the timeout expires,
                 False otherwise
        """
        if timeout:
            current_time = int(time.time())
            end_time = current_time + timeout

        while not timeout or current_time < end_time:
            if self.is_up(timeout=end_time - current_time):
                try:
                    if self.are_wait_up_processes_up():
                        logging.debug('Host %s is now up', self.hostname)
                        return True
                except error.AutoservError:
                    pass
            time.sleep(1)
            current_time = int(time.time())

        logging.debug('Host %s is still down after waiting %d seconds',
                      self.hostname, int(timeout + time.time() - end_time))
        return False


    def wait_down(self, timeout=None, warning_timer=None, old_boot_id=None):
        """
        Wait until the remote host is down or the timeout expires.

        If old_boot_id is provided, this will wait until either the machine
        is unpingable or self.get_boot_id() returns a value different from
        old_boot_id. If the boot_id value has changed then the function
        returns true under the assumption that the machine has shut down
        and has now already come back up.

        If old_boot_id is None then until the machine becomes unreachable the
        method assumes the machine has not yet shut down.

        Based on this definition, the 4 possible permutations of timeout
        and old_boot_id are:
        1. timeout and old_boot_id: wait timeout seconds for either the
                                    host to become unpingable, or the boot id
                                    to change. In the latter case we've rebooted
                                    and in the former case we've only shutdown,
                                    but both cases return True.
        2. only timeout: wait timeout seconds for the host to become unpingable.
                         If the host remains pingable throughout timeout seconds
                         we return False.
        3. only old_boot_id: wait forever until either the host becomes
                             unpingable or the boot_id changes. Return true
                             when either of those conditions are met.
        4. not timeout, not old_boot_id: wait forever till the host becomes
                                         unpingable.

        @param timeout Time limit in seconds before returning even
            if the host is still up.
        @param warning_timer Time limit in seconds that will generate
            a warning if the host is not down yet.
        @param old_boot_id A string containing the result of self.get_boot_id()
            prior to the host being told to shut down. Can be None if this is
            not available.

        @returns True if the host was found to be down, False otherwise
        """
        #TODO: there is currently no way to distinguish between knowing
        #TODO: boot_id was unsupported and not knowing the boot_id.
        current_time = int(time.time())
        if timeout:
            end_time = current_time + timeout

        if warning_timer:
            warn_time = current_time + warning_timer

        if old_boot_id is not None:
            logging.debug('Host %s pre-shutdown boot_id is %s',
                          self.hostname, old_boot_id)

        # Impose semi real-time deadline constraints, since some clients
        # (eg: watchdog timer tests) expect strict checking of time elapsed.
        # Each iteration of this loop is treated as though it atomically
        # completes within current_time, this is needed because if we used
        # inline time.time() calls instead then the following could happen:
        #
        # while not timeout or time.time() < end_time:      [23 < 30]
        #    some code.                                     [takes 10 secs]
        #    try:
        #        new_boot_id = self.get_boot_id(timeout=end_time - time.time())
        #                                                   [30 - 33]
        # The last step will lead to a return True, when in fact the machine
        # went down at 32 seconds (>30). Hence we need to pass get_boot_id
        # the same time that allowed us into that iteration of the loop.
        while not timeout or current_time < end_time:
            try:
                new_boot_id = self.get_boot_id(timeout=end_time-current_time)
            except error.AutoservError:
                logging.debug('Host %s is now unreachable over ssh, is down',
                              self.hostname)
                return True
            else:
                # if the machine is up but the boot_id value has changed from
                # old boot id, then we can assume the machine has gone down
                # and then already come back up
                if old_boot_id is not None and old_boot_id != new_boot_id:
                    logging.debug('Host %s now has boot_id %s and so must '
                                  'have rebooted', self.hostname, new_boot_id)
                    return True

            if warning_timer and current_time > warn_time:
                self.record("INFO", None, "shutdown",
                            "Shutdown took longer than %ds" % warning_timer)
                # Print the warning only once.
                warning_timer = None
                # If a machine is stuck switching runlevels
                # This may cause the machine to reboot.
                self.run('kill -HUP 1', ignore_status=True)

            time.sleep(1)
            current_time = int(time.time())

        return False


    # tunable constants for the verify & repair code
    AUTOTEST_GB_DISKSPACE_REQUIRED = get_value("SERVER",
                                               "gb_diskspace_required",
                                               type=float,
                                               default=20.0)


    def verify_connectivity(self):
        super(AbstractSSHHost, self).verify_connectivity()

        logging.info('Pinging host ' + self.hostname)
        self.ssh_ping()
        logging.info("Host (ssh) %s is alive", self.hostname)

        if self.is_shutting_down():
            raise error.AutoservHostIsShuttingDownError("Host is shutting down")


    def verify_software(self):
        super(AbstractSSHHost, self).verify_software()
        try:
            self.check_diskspace(autotest.Autotest.get_install_dir(self),
                                 self.AUTOTEST_GB_DISKSPACE_REQUIRED)
        except error.AutoservHostError:
            raise           # only want to raise if it's a space issue
        except autotest.AutodirNotFoundError:
            # autotest dir may not exist, etc. ignore
            logging.debug('autodir space check exception, this is probably '
                          'safe to ignore\n' + traceback.format_exc())


    def close(self):
        super(AbstractSSHHost, self).close()
        self._cleanup_master_ssh()
        os.remove(self.known_hosts_file)
        self.rpc_server_tracker.disconnect_all()


    def _cleanup_master_ssh(self):
        """
        Release all resources (process, temporary directory) used by an active
        master SSH connection.
        """
        # If a master SSH connection is running, kill it.
        if self.master_ssh_job is not None:
            logging.debug('Nuking master_ssh_job.')
            utils.nuke_subprocess(self.master_ssh_job.sp)
            self.master_ssh_job = None

        # Remove the temporary directory for the master SSH socket.
        if self.master_ssh_tempdir is not None:
            logging.debug('Cleaning master_ssh_tempdir.')
            self.master_ssh_tempdir.clean()
            self.master_ssh_tempdir = None
            self.master_ssh_option = ''


    def start_master_ssh(self, timeout=5):
        """
        Called whenever a slave SSH connection needs to be initiated (e.g., by
        run, rsync, scp). If master SSH support is enabled and a master SSH
        connection is not active already, start a new one in the background.
        Also, cleanup any zombie master SSH connections (e.g., dead due to
        reboot).

        timeout: timeout in seconds (default 5) to wait for master ssh
                 connection to be established. If timeout is reached, a
                 warning message is logged, but no other action is taken.
        """
        if not enable_master_ssh:
            return

        # Multiple processes might try in parallel to clean up the old master
        # ssh connection and create a new one, therefore use a lock to protect
        # against race conditions.
        with self._lock:
            # If a previously started master SSH connection is not running
            # anymore, it needs to be cleaned up and then restarted.
            if self.master_ssh_job is not None:
                socket_path = os.path.join(self.master_ssh_tempdir.name,
                                           'socket')
                if (not os.path.exists(socket_path) or
                        self.master_ssh_job.sp.poll() is not None):
                    logging.info("Master ssh connection to %s is down.",
                                 self.hostname)
                    self._cleanup_master_ssh()

            # Start a new master SSH connection.
            if self.master_ssh_job is None:
                # Create a shared socket in a temp location.
                self.master_ssh_tempdir = autotemp.tempdir(
                        unique_id='ssh-master')
                self.master_ssh_option = ("-o ControlPath=%s/socket" %
                                          self.master_ssh_tempdir.name)

                # Start the master SSH connection in the background.
                master_cmd = self.ssh_command(
                        options="-N -o ControlMaster=yes")
                logging.info("Starting master ssh connection '%s'", master_cmd)
                self.master_ssh_job = utils.BgJob(master_cmd,
                                                  nickname='master-ssh',
                                                  no_pipes=True)
                # To prevent a race between the the master ssh connection
                # startup and its first attempted use, wait for socket file to
                # exist before returning.
                end_time = time.time() + timeout
                socket_file_path = os.path.join(self.master_ssh_tempdir.name,
                                                'socket')
                while time.time() < end_time:
                    if os.path.exists(socket_file_path):
                        break
                    time.sleep(.2)
                else:
                    logging.info('Timed out waiting for master-ssh connection '
                                 'to be established.')


    def clear_known_hosts(self):
        """Clears out the temporary ssh known_hosts file.

        This is useful if the test SSHes to the machine, then reinstalls it,
        then SSHes to it again.  It can be called after the reinstall to
        reduce the spam in the logs.
        """
        logging.info("Clearing known hosts for host '%s', file '%s'.",
                     self.hostname, self.known_hosts_file)
        # Clear out the file by opening it for writing and then closing.
        fh = open(self.known_hosts_file, "w")
        fh.close()


    def collect_logs(self, remote_src_dir, local_dest_dir, ignore_errors=True):
        """Copy log directories from a host to a local directory.

        @param remote_src_dir: A destination directory on the host.
        @param local_dest_dir: A path to a local destination directory.
            If it doesn't exist it will be created.
        @param ignore_errors: If True, ignore exceptions.

        @raises OSError: If there were problems creating the local_dest_dir and
            ignore_errors is False.
        @raises AutoservRunError, AutotestRunError: If something goes wrong
            while copying the directories and ignore_errors is False.
        """
        locally_created_dest = False
        if (not os.path.exists(local_dest_dir)
                or not os.path.isdir(local_dest_dir)):
            try:
                os.makedirs(local_dest_dir)
                locally_created_dest = True
            except OSError as e:
                logging.warning('Unable to collect logs from host '
                                '%s: %s', self.hostname, e)
                if not ignore_errors:
                    raise
                return
        try:
            self.get_file(
                    remote_src_dir, local_dest_dir, preserve_symlinks=True)
        except (error.AutotestRunError, error.AutoservRunError,
                error.AutoservSSHTimeout) as e:
            logging.warning('Collection of %s to local dir %s from host %s '
                            'failed: %s', remote_src_dir, local_dest_dir,
                            self.hostname, e)
            if locally_created_dest:
                shutil.rmtree(local_dest_dir, ignore_errors=ignore_errors)
            if not ignore_errors:
                raise


    def _create_ssh_tunnel(self, port, local_port):
        """Create an ssh tunnel from local_port to port.

        @param port: remote port on the host.
        @param local_port: local forwarding port.

        @return: the tunnel process.
        """
        tunnel_options = '-n -N -q -L %d:localhost:%d' % (local_port, port)
        ssh_cmd = self.make_ssh_command(opts=tunnel_options)
        tunnel_cmd = '%s %s' % (ssh_cmd, self.hostname)
        logging.debug('Full tunnel command: %s', tunnel_cmd)
        tunnel_proc = subprocess.Popen(tunnel_cmd, shell=True, close_fds=True)
        logging.debug('Started ssh tunnel, local = %d'
                      ' remote = %d, pid = %d',
                      local_port, port, tunnel_proc.pid)
        return tunnel_proc


    def rpc_port_forward(self, port, local_port):
        """
        Forwards a port securely through a tunnel process from the server
        to the DUT for RPC server connection.

        @param port: remote port on the DUT.
        @param local_port: local forwarding port.

        @return: the tunnel process.
        """
        return self._create_ssh_tunnel(port, local_port)


    def rpc_port_disconnect(self, tunnel_proc, port):
        """
        Disconnects a previously forwarded port from the server to the DUT for
        RPC server connection.

        @param tunnel_proc: the original tunnel process returned from
                            |rpc_port_forward|.
        @param port: remote port on the DUT.

        """
        if tunnel_proc.poll() is None:
            tunnel_proc.terminate()
            logging.debug('Terminated tunnel, pid %d', tunnel_proc.pid)
        else:
            logging.debug('Tunnel pid %d terminated early, status %d',
                          tunnel_proc.pid, tunnel_proc.returncode)


    def get_os_type(self):
        """Returns the host OS descriptor (to be implemented in subclasses).

        @return A string describing the OS type.
        """
        raise NotImplementedError