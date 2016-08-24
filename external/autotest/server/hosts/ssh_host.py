#
# Copyright 2007 Google Inc. Released under the GPL v2

"""
This module defines the SSHHost class.

Implementation details:
You should import the "hosts" package instead of importing each type of host.

        SSHHost: a remote machine with a ssh access
"""

import re, logging
from autotest_lib.client.common_lib import error, pxssh
from autotest_lib.client.common_lib.cros.graphite import autotest_stats
from autotest_lib.server import utils
from autotest_lib.server.hosts import abstract_ssh


class SSHHost(abstract_ssh.AbstractSSHHost):
    """
    This class represents a remote machine controlled through an ssh
    session on which you can run programs.

    It is not the machine autoserv is running on. The machine must be
    configured for password-less login, for example through public key
    authentication.

    It includes support for controlling the machine through a serial
    console on which you can run programs. If such a serial console is
    set up on the machine then capabilities such as hard reset and
    boot strap monitoring are available. If the machine does not have a
    serial console available then ordinary SSH-based commands will
    still be available, but attempts to use extensions such as
    console logging or hard reset will fail silently.

    Implementation details:
    This is a leaf class in an abstract class hierarchy, it must
    implement the unimplemented methods in parent classes.
    """

    def _initialize(self, hostname, *args, **dargs):
        """
        Construct a SSHHost object

        Args:
                hostname: network hostname or address of remote machine
        """
        super(SSHHost, self)._initialize(hostname=hostname, *args, **dargs)
        self.setup_ssh()


    def ssh_command(self, connect_timeout=30, options='', alive_interval=300):
        """
        Construct an ssh command with proper args for this host.

        @param connect_timeout: connection timeout (in seconds)
        @param options: SSH options
        @param alive_interval: SSH Alive interval.

        """
        options = "%s %s" % (options, self.master_ssh_option)
        base_cmd = self.make_ssh_command(user=self.user, port=self.port,
                                         opts=options,
                                         hosts_file=self.known_hosts_file,
                                         connect_timeout=connect_timeout,
                                         alive_interval=alive_interval)
        return "%s %s" % (base_cmd, self.hostname)


    def _run(self, command, timeout, ignore_status,
             stdout, stderr, connect_timeout, env, options, stdin, args,
             ignore_timeout):
        """Helper function for run()."""
        ssh_cmd = self.ssh_command(connect_timeout, options)
        if not env.strip():
            env = ""
        else:
            env = "export %s;" % env
        for arg in args:
            command += ' "%s"' % utils.sh_escape(arg)
        full_cmd = '%s "%s %s"' % (ssh_cmd, env, utils.sh_escape(command))

        # TODO(jrbarnette):  crbug.com/484726 - When we're in an SSP
        # container, sometimes shortly after reboot we will see DNS
        # resolution errors on ssh commands; the problem never
        # occurs more than once in a row.  This especially affects
        # the autoupdate_Rollback test, but other cases have been
        # affected, too.
        #
        # We work around it by detecting the first DNS resolution error
        # and retrying exactly one time.
        dns_retry_count = 2
        while True:
            result = utils.run(full_cmd, timeout, True, stdout, stderr,
                               verbose=False, stdin=stdin,
                               stderr_is_expected=ignore_status,
                               ignore_timeout=ignore_timeout)
            dns_retry_count -= 1
            if (result and result.exit_status == 255 and
                    re.search(r'^ssh: .*: Name or service not known',
                              result.stderr)):
                if dns_retry_count:
                    logging.debug('Retrying because of DNS failure')
                    continue
                logging.debug('Retry failed.')
                autotest_stats.Counter('dns_retry_hack.fail').increment()
            elif not dns_retry_count:
                logging.debug('Retry succeeded.')
                autotest_stats.Counter('dns_retry_hack.pass').increment()
            break

        if ignore_timeout and not result:
            return None

        # The error messages will show up in band (indistinguishable
        # from stuff sent through the SSH connection), so we have the
        # remote computer echo the message "Connected." before running
        # any command.  Since the following 2 errors have to do with
        # connecting, it's safe to do these checks.
        if result.exit_status == 255:
            if re.search(r'^ssh: connect to host .* port .*: '
                         r'Connection timed out\r$', result.stderr):
                raise error.AutoservSSHTimeout("ssh timed out", result)
            if "Permission denied." in result.stderr:
                msg = "ssh permission denied"
                raise error.AutoservSshPermissionDeniedError(msg, result)

        if not ignore_status and result.exit_status > 0:
            raise error.AutoservRunError("command execution error", result)

        return result


    def run(self, command, timeout=3600, ignore_status=False,
            stdout_tee=utils.TEE_TO_LOGS, stderr_tee=utils.TEE_TO_LOGS,
            connect_timeout=30, options='', stdin=None, verbose=True, args=(),
            ignore_timeout=False):
        """
        Run a command on the remote host.
        @see common_lib.hosts.host.run()

        @param connect_timeout: connection timeout (in seconds)
        @param options: string with additional ssh command options
        @param verbose: log the commands
        @param ignore_timeout: bool True if SSH command timeouts should be
                ignored.  Will return None on command timeout.

        @raises AutoservRunError: if the command failed
        @raises AutoservSSHTimeout: ssh connection has timed out
        """
        if verbose:
            logging.debug("Running (ssh) '%s'", command)

        # Start a master SSH connection if necessary.
        self.start_master_ssh()

        env = " ".join("=".join(pair) for pair in self.env.iteritems())
        try:
            return self._run(command, timeout, ignore_status,
                             stdout_tee, stderr_tee, connect_timeout, env,
                             options, stdin, args, ignore_timeout)
        except error.CmdError, cmderr:
            # We get a CmdError here only if there is timeout of that command.
            # Catch that and stuff it into AutoservRunError and raise it.
            timeout_message = str('Timeout encountered: %s' % cmderr.args[0])
            raise error.AutoservRunError(timeout_message, cmderr.args[1])


    def run_background(self, command, verbose=True):
        """Start a command on the host in the background.

        The command is started on the host in the background, and
        this method call returns immediately without waiting for the
        command's completion.  The PID of the process on the host is
        returned as a string.

        The command may redirect its stdin, stdout, or stderr as
        necessary.  Without redirection, all input and output will
        use /dev/null.

        @param command The command to run in the background
        @param verbose As for `self.run()`

        @return Returns the PID of the remote background process
                as a string.
        """
        # Redirection here isn't merely hygienic; it's a functional
        # requirement.  sshd won't terminate until stdin, stdout,
        # and stderr are all closed.
        #
        # The subshell is needed to do the right thing in case the
        # passed in command has its own I/O redirections.
        cmd_fmt = '( %s ) </dev/null >/dev/null 2>&1 & echo -n $!'
        return self.run(cmd_fmt % command, verbose=verbose).stdout


    def run_short(self, command, **kwargs):
        """
        Calls the run() command with a short default timeout.

        Takes the same arguments as does run(),
        with the exception of the timeout argument which
        here is fixed at 60 seconds.
        It returns the result of run.

        @param command: the command line string

        """
        return self.run(command, timeout=60, **kwargs)


    def run_grep(self, command, timeout=30, ignore_status=False,
                 stdout_ok_regexp=None, stdout_err_regexp=None,
                 stderr_ok_regexp=None, stderr_err_regexp=None,
                 connect_timeout=30):
        """
        Run a command on the remote host and look for regexp
        in stdout or stderr to determine if the command was
        successul or not.


        @param command: the command line string
        @param timeout: time limit in seconds before attempting to
                        kill the running process. The run() function
                        will take a few seconds longer than 'timeout'
                        to complete if it has to kill the process.
        @param ignore_status: do not raise an exception, no matter
                              what the exit code of the command is.
        @param stdout_ok_regexp: regexp that should be in stdout
                                 if the command was successul.
        @param stdout_err_regexp: regexp that should be in stdout
                                  if the command failed.
        @param stderr_ok_regexp: regexp that should be in stderr
                                 if the command was successul.
        @param stderr_err_regexp: regexp that should be in stderr
                                 if the command failed.
        @param connect_timeout: connection timeout (in seconds)

        Returns:
                if the command was successul, raises an exception
                otherwise.

        Raises:
                AutoservRunError:
                - the exit code of the command execution was not 0.
                - If stderr_err_regexp is found in stderr,
                - If stdout_err_regexp is found in stdout,
                - If stderr_ok_regexp is not found in stderr.
                - If stdout_ok_regexp is not found in stdout,
        """

        # We ignore the status, because we will handle it at the end.
        result = self.run(command, timeout, ignore_status=True,
                          connect_timeout=connect_timeout)

        # Look for the patterns, in order
        for (regexp, stream) in ((stderr_err_regexp, result.stderr),
                                 (stdout_err_regexp, result.stdout)):
            if regexp and stream:
                err_re = re.compile (regexp)
                if err_re.search(stream):
                    raise error.AutoservRunError(
                        '%s failed, found error pattern: "%s"' % (command,
                                                                regexp), result)

        for (regexp, stream) in ((stderr_ok_regexp, result.stderr),
                                 (stdout_ok_regexp, result.stdout)):
            if regexp and stream:
                ok_re = re.compile (regexp)
                if ok_re.search(stream):
                    if ok_re.search(stream):
                        return

        if not ignore_status and result.exit_status > 0:
            raise error.AutoservRunError("command execution error", result)


    def setup_ssh_key(self):
        """Setup SSH Key"""
        logging.debug('Performing SSH key setup on %s:%d as %s.',
                      self.hostname, self.port, self.user)

        try:
            host = pxssh.pxssh()
            host.login(self.hostname, self.user, self.password,
                        port=self.port)
            public_key = utils.get_public_key()

            host.sendline('mkdir -p ~/.ssh')
            host.prompt()
            host.sendline('chmod 700 ~/.ssh')
            host.prompt()
            host.sendline("echo '%s' >> ~/.ssh/authorized_keys; " %
                            public_key)
            host.prompt()
            host.sendline('chmod 600 ~/.ssh/authorized_keys')
            host.prompt()
            host.logout()

            logging.debug('SSH key setup complete.')

        except:
            logging.debug('SSH key setup has failed.')
            try:
                host.logout()
            except:
                pass


    def setup_ssh(self):
        """Setup SSH"""
        if self.password:
            try:
                self.ssh_ping()
            except error.AutoservSshPingHostError:
                self.setup_ssh_key()
