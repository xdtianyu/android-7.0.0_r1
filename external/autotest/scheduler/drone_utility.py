#!/usr/bin/python
#pylint: disable-msg=C0111

"""Utility module that executes management commands on the drone.

1. This is the module responsible for orchestrating processes on a drone.
2. It receives instructions via stdin and replies via stdout.
3. Each invocation is responsible for the initiation of a set of batched calls.
4. The batched calls may be synchronous or asynchronous.
5. The caller is responsible for monitoring asynchronous calls through pidfiles.
"""


import argparse
import pickle, subprocess, os, shutil, sys, time, signal, getpass
import datetime, traceback, tempfile, itertools, logging
import common
from autotest_lib.client.common_lib import utils, global_config, error
from autotest_lib.client.common_lib import logging_manager
from autotest_lib.client.common_lib.cros import retry
from autotest_lib.client.common_lib.cros.graphite import autotest_stats
from autotest_lib.scheduler import drone_logging_config
from autotest_lib.scheduler import email_manager, scheduler_config
from autotest_lib.server import hosts, subcommand


# An environment variable we add to the environment to enable us to
# distinguish processes we started from those that were started by
# something else during recovery.  Name credit goes to showard. ;)
DARK_MARK_ENVIRONMENT_VAR = 'AUTOTEST_SCHEDULER_DARK_MARK'

_TEMPORARY_DIRECTORY = 'drone_tmp'
_TRANSFER_FAILED_FILE = '.transfer_failed'

# script and log file for cleaning up orphaned lxc containers.
LXC_CLEANUP_SCRIPT = os.path.join(common.autotest_dir, 'site_utils',
                                  'lxc_cleanup.py')
LXC_CLEANUP_LOG_FILE = os.path.join(common.autotest_dir, 'logs',
                                    'lxc_cleanup.log')


_STATS_KEY = 'drone_utility'
timer = autotest_stats.Timer(_STATS_KEY)

class _MethodCall(object):
    def __init__(self, method, args, kwargs):
        self._method = method
        self._args = args
        self._kwargs = kwargs


    def execute_on(self, drone_utility):
        method = getattr(drone_utility, self._method)
        return method(*self._args, **self._kwargs)


    def __str__(self):
        args = ', '.join(repr(arg) for arg in self._args)
        kwargs = ', '.join('%s=%r' % (key, value) for key, value in
                           self._kwargs.iteritems())
        full_args = ', '.join(item for item in (args, kwargs) if item)
        return '%s(%s)' % (self._method, full_args)


def call(method, *args, **kwargs):
    return _MethodCall(method, args, kwargs)


class BaseDroneUtility(object):
    """
    This class executes actual OS calls on the drone machine.

    All paths going into and out of this class are absolute.
    """
    _WARNING_DURATION = 400

    def __init__(self):
        # Tattoo ourselves so that all of our spawn bears our mark.
        os.putenv(DARK_MARK_ENVIRONMENT_VAR, str(os.getpid()))

        self.warnings = []
        self._subcommands = []


    def initialize(self, results_dir):
        temporary_directory = os.path.join(results_dir, _TEMPORARY_DIRECTORY)
        if os.path.exists(temporary_directory):
            # TODO crbug.com/391111: before we have a better solution to
            # periodically cleanup tmp files, we have to use rm -rf to delete
            # the whole folder. shutil.rmtree has performance issue when a
            # folder has large amount of files, e.g., drone_tmp.
            os.system('rm -rf %s' % temporary_directory)
        self._ensure_directory_exists(temporary_directory)
        # TODO (sbasi) crbug.com/345011 - Remove this configuration variable
        # and clean up build_externals so it NO-OP's.
        build_externals = global_config.global_config.get_config_value(
                scheduler_config.CONFIG_SECTION, 'drone_build_externals',
                default=True, type=bool)
        if build_externals:
            build_extern_cmd = os.path.join(common.autotest_dir,
                                            'utils', 'build_externals.py')
            utils.run(build_extern_cmd)


    def _warn(self, warning):
        self.warnings.append(warning)


    @staticmethod
    def _check_pid_for_dark_mark(pid, open=open):
        try:
            env_file = open('/proc/%s/environ' % pid, 'rb')
        except EnvironmentError:
            return False
        try:
            env_data = env_file.read()
        finally:
            env_file.close()
        return DARK_MARK_ENVIRONMENT_VAR in env_data


    _PS_ARGS = ('pid', 'pgid', 'ppid', 'comm', 'args')


    @classmethod
    @timer.decorate
    def _get_process_info(cls):
        """Parse ps output for all process information.

        @returns A generator of dicts with cls._PS_ARGS as keys and
            string values each representing a running process. eg:
            {
                'comm': command_name,
                'pgid': process group id,
                'ppid': parent process id,
                'pid': process id,
                'args': args the command was invoked with,
            }
        """
        @retry.retry(subprocess.CalledProcessError,
                     timeout_min=0.5, delay_sec=0.25)
        def run_ps():
            return subprocess.check_output(
                    ['/bin/ps', 'x', '-o', ','.join(cls._PS_ARGS)])

        ps_output = run_ps()
        # split each line into the columns output by ps
        split_lines = [line.split(None, 4) for line in ps_output.splitlines()]
        return (dict(itertools.izip(cls._PS_ARGS, line_components))
                for line_components in split_lines)


    def _refresh_processes(self, command_name, open=open,
                           site_check_parse=None):
        """Refreshes process info for the given command_name.

        Examines ps output as returned by get_process_info and returns
        the process dicts for processes matching the given command name.

        @param command_name: The name of the command, eg 'autoserv'.

        @return: A list of process info dictionaries as returned by
            _get_process_info.
        """
        # The open argument is used for test injection.
        check_mark = global_config.global_config.get_config_value(
            'SCHEDULER', 'check_processes_for_dark_mark', bool, False)
        processes = []
        for info in self._get_process_info():
            is_parse = (site_check_parse and site_check_parse(info))
            if info['comm'] == command_name or is_parse:
                if (check_mark and not
                        self._check_pid_for_dark_mark(info['pid'], open=open)):
                    self._warn('%(comm)s process pid %(pid)s has no '
                               'dark mark; ignoring.' % info)
                    continue
                processes.append(info)

        return processes


    @timer.decorate
    def _read_pidfiles(self, pidfile_paths):
        pidfiles = {}
        for pidfile_path in pidfile_paths:
            if not os.path.exists(pidfile_path):
                continue
            try:
                file_object = open(pidfile_path, 'r')
                pidfiles[pidfile_path] = file_object.read()
                file_object.close()
            except IOError:
                continue
        return pidfiles


    @timer.decorate
    def refresh(self, pidfile_paths):
        """
        pidfile_paths should be a list of paths to check for pidfiles.

        Returns a dict containing:
        * pidfiles: dict mapping pidfile paths to file contents, for pidfiles
        that exist.
        * autoserv_processes: list of dicts corresponding to running autoserv
        processes.  each dict contain pid, pgid, ppid, comm, and args (see
        "man ps" for details).
        * parse_processes: likewise, for parse processes.
        * pidfiles_second_read: same info as pidfiles, but gathered after the
        processes are scanned.
        """
        site_check_parse = utils.import_site_function(
                __file__, 'autotest_lib.scheduler.site_drone_utility',
                'check_parse', lambda x: False)
        results = {
            'pidfiles' : self._read_pidfiles(pidfile_paths),
            # element 0 of _get_process_info() is the headers from `ps`
            'all_processes' : list(self._get_process_info())[1:],
            'autoserv_processes' : self._refresh_processes('autoserv'),
            'parse_processes' : self._refresh_processes(
                    'parse', site_check_parse=site_check_parse),
            'pidfiles_second_read' : self._read_pidfiles(pidfile_paths),
        }
        return results


    def get_signal_queue_to_kill(self, process):
        """Get the signal queue needed to kill a process.

        autoserv process has a handle on SIGTERM, in which it can do some
        cleanup work. However, abort a process with SIGTERM then SIGKILL has
        its overhead, detailed in following CL:
        https://chromium-review.googlesource.com/230323
        This method checks the process's argument and determine if SIGTERM is
        required, and returns signal queue accordingly.

        @param process: A drone_manager.Process object to be killed.

        @return: The signal queue needed to kill a process.

        """
        signal_queue_with_sigterm = (signal.SIGTERM, signal.SIGKILL)
        try:
            ps_output = subprocess.check_output(
                    ['/bin/ps', '-p', str(process.pid), '-o', 'args'])
            # For test running with server-side packaging, SIGTERM needs to be
            # sent for autoserv process to destroy container used by the test.
            if '--require-ssp' in ps_output:
                logging.debug('PID %d requires SIGTERM to abort to cleanup '
                              'container.', process.pid)
                return signal_queue_with_sigterm
        except subprocess.CalledProcessError:
            # Ignore errors, return the signal queue with SIGTERM to be safe.
            return signal_queue_with_sigterm
        # Default to kill the process with SIGKILL directly.
        return (signal.SIGKILL,)


    @timer.decorate
    def kill_processes(self, process_list):
        """Send signals escalating in severity to the processes in process_list.

        @param process_list: A list of drone_manager.Process objects
                             representing the processes to kill.
        """
        kill_proc_key = 'kill_processes'
        autotest_stats.Gauge(_STATS_KEY).send('%s.%s' % (kill_proc_key, 'net'),
                                              len(process_list))
        try:
            logging.info('List of process to be killed: %s', process_list)
            processes_to_kill = {}
            for p in process_list:
                signal_queue = self.get_signal_queue_to_kill(p)
                processes_to_kill[signal_queue] = (
                        processes_to_kill.get(signal_queue, []) + [p])
            sig_counts = {}
            for signal_queue, processes in processes_to_kill.iteritems():
                sig_counts.update(utils.nuke_pids(
                        [-process.pid for process in processes],
                        signal_queue=signal_queue))
            for name, count in sig_counts.iteritems():
                autotest_stats.Gauge(_STATS_KEY).send(
                        '%s.%s' % (kill_proc_key, name), count)
        except error.AutoservRunError as e:
            self._warn('Error occured when killing processes. Error: %s' % e)


    def _convert_old_host_log(self, log_path):
        """
        For backwards compatibility only.  This can safely be removed in the
        future.

        The scheduler used to create files at results/hosts/<hostname>, and
        append all host logs to that file.  Now, it creates directories at
        results/hosts/<hostname>, and places individual timestamped log files
        into that directory.

        This can be a problem the first time the scheduler runs after upgrading.
        To work around that, we'll look for a file at the path where the
        directory should be, and if we find one, we'll automatically convert it
        to a directory containing the old logfile.
        """
        # move the file out of the way
        temp_dir = tempfile.mkdtemp(suffix='.convert_host_log')
        base_name = os.path.basename(log_path)
        temp_path = os.path.join(temp_dir, base_name)
        os.rename(log_path, temp_path)

        os.mkdir(log_path)

        # and move it into the new directory
        os.rename(temp_path, os.path.join(log_path, 'old_log'))
        os.rmdir(temp_dir)


    def _ensure_directory_exists(self, path):
        if os.path.isdir(path):
            return

        if os.path.exists(path):
            # path exists already, but as a file, not a directory
            if '/hosts/' in path:
                self._convert_old_host_log(path)
                return
            else:
                raise IOError('Path %s exists as a file, not a directory')

        os.makedirs(path)


    def execute_command(self, command, working_directory, log_file,
                        pidfile_name):
        out_file = None
        if log_file:
            self._ensure_directory_exists(os.path.dirname(log_file))
            try:
                out_file = open(log_file, 'a')
                separator = ('*' * 80) + '\n'
                out_file.write('\n' + separator)
                out_file.write("%s> %s\n" % (time.strftime("%X %x"), command))
                out_file.write(separator)
            except (OSError, IOError):
                email_manager.manager.log_stacktrace(
                    'Error opening log file %s' % log_file)

        if not out_file:
            out_file = open('/dev/null', 'w')

        in_devnull = open('/dev/null', 'r')

        self._ensure_directory_exists(working_directory)
        pidfile_path = os.path.join(working_directory, pidfile_name)
        if os.path.exists(pidfile_path):
            self._warn('Pidfile %s already exists' % pidfile_path)
            os.remove(pidfile_path)

        subprocess.Popen(command, stdout=out_file, stderr=subprocess.STDOUT,
                         stdin=in_devnull)
        out_file.close()
        in_devnull.close()


    def write_to_file(self, file_path, contents, is_retry=False):
        """Write the specified contents to the end of the given file.

        @param file_path: Path to the file.
        @param contents: Content to be written to the file.
        @param is_retry: True if this is a retry after file permission be
                         corrected.
        """
        self._ensure_directory_exists(os.path.dirname(file_path))
        try:
            file_object = open(file_path, 'a')
            file_object.write(contents)
            file_object.close()
        except IOError as e:
            # TODO(dshi): crbug.com/459344 Remove following retry when test
            # container can be unprivileged container.
            # If write failed with error 'Permission denied', one possible cause
            # is that the file was created in a container and thus owned by
            # root. If so, fix the file permission, and try again.
            if e.errno == 13 and not is_retry:
                logging.error('Error write to file %s: %s. Will be retried.',
                              file_path, e)
                utils.run('sudo chown %s "%s"' % (os.getuid(), file_path))
                utils.run('sudo chgrp %s "%s"' % (os.getgid(), file_path))
                self.write_to_file(file_path, contents, is_retry=True)
            else:
                self._warn('Error write to file %s: %s' % (file_path, e))


    def copy_file_or_directory(self, source_path, destination_path):
        """
        This interface is designed to match server.hosts.abstract_ssh.get_file
        (and send_file).  That is, if the source_path ends with a slash, the
        contents of the directory are copied; otherwise, the directory iself is
        copied.
        """
        if self._same_file(source_path, destination_path):
            return
        self._ensure_directory_exists(os.path.dirname(destination_path))
        if source_path.endswith('/'):
            # copying a directory's contents to another directory
            assert os.path.isdir(source_path)
            assert os.path.isdir(destination_path)
            for filename in os.listdir(source_path):
                self.copy_file_or_directory(
                    os.path.join(source_path, filename),
                    os.path.join(destination_path, filename))
        elif os.path.isdir(source_path):
            shutil.copytree(source_path, destination_path, symlinks=True)
        elif os.path.islink(source_path):
            # copied from shutil.copytree()
            link_to = os.readlink(source_path)
            os.symlink(link_to, destination_path)
        else:
            shutil.copy(source_path, destination_path)


    def _same_file(self, source_path, destination_path):
        """Checks if the source and destination are the same

        Returns True if the destination is the same as the source, False
        otherwise. Also returns False if the destination does not exist.
        """
        if not os.path.exists(destination_path):
            return False
        return os.path.samefile(source_path, destination_path)


    def cleanup_orphaned_containers(self):
        """Run lxc_cleanup script to clean up orphaned container.
        """
        # The script needs to run with sudo as the containers are privileged.
        # TODO(dshi): crbug.com/459344 Call lxc_cleanup.main when test
        # container can be unprivileged container.
        command = ['sudo', LXC_CLEANUP_SCRIPT, '-x', '-v', '-l',
                   LXC_CLEANUP_LOG_FILE]
        logging.info('Running %s', command)
        # stdout and stderr needs to be direct to /dev/null, otherwise existing
        # of drone_utils process will kill lxc_cleanup script.
        subprocess.Popen(
                command, shell=False, stdin=None, stdout=open('/dev/null', 'w'),
                stderr=open('/dev/null', 'a'), preexec_fn=os.setpgrp)


    def wait_for_all_async_commands(self):
        for subproc in self._subcommands:
            subproc.fork_waitfor()
        self._subcommands = []


    def _poll_async_commands(self):
        still_running = []
        for subproc in self._subcommands:
            if subproc.poll() is None:
                still_running.append(subproc)
        self._subcommands = still_running


    def _wait_for_some_async_commands(self):
        self._poll_async_commands()
        max_processes = scheduler_config.config.max_transfer_processes
        while len(self._subcommands) >= max_processes:
            time.sleep(1)
            self._poll_async_commands()


    def run_async_command(self, function, args):
        subproc = subcommand.subcommand(function, args)
        self._subcommands.append(subproc)
        subproc.fork_start()


    def _sync_get_file_from(self, hostname, source_path, destination_path):
        logging.debug('_sync_get_file_from hostname: %s, source_path: %s,'
                      'destination_path: %s', hostname, source_path,
                      destination_path)
        self._ensure_directory_exists(os.path.dirname(destination_path))
        host = create_host(hostname)
        host.get_file(source_path, destination_path, delete_dest=True)


    def get_file_from(self, hostname, source_path, destination_path):
        self.run_async_command(self._sync_get_file_from,
                               (hostname, source_path, destination_path))


    def sync_send_file_to(self, hostname, source_path, destination_path,
                           can_fail):
        logging.debug('sync_send_file_to. hostname: %s, source_path: %s, '
                      'destination_path: %s, can_fail:%s', hostname,
                      source_path, destination_path, can_fail)
        host = create_host(hostname)
        try:
            host.run('mkdir -p ' + os.path.dirname(destination_path))
            host.send_file(source_path, destination_path, delete_dest=True)
        except error.AutoservError:
            if not can_fail:
                raise

            if os.path.isdir(source_path):
                failed_file = os.path.join(source_path, _TRANSFER_FAILED_FILE)
                file_object = open(failed_file, 'w')
                try:
                    file_object.write('%s:%s\n%s\n%s' %
                                      (hostname, destination_path,
                                       datetime.datetime.now(),
                                       traceback.format_exc()))
                finally:
                    file_object.close()
            else:
                copy_to = destination_path + _TRANSFER_FAILED_FILE
                self._ensure_directory_exists(os.path.dirname(copy_to))
                self.copy_file_or_directory(source_path, copy_to)


    def send_file_to(self, hostname, source_path, destination_path,
                     can_fail=False):
        self.run_async_command(self.sync_send_file_to,
                               (hostname, source_path, destination_path,
                                can_fail))


    def _report_long_execution(self, calls, duration):
        call_count = {}
        for call in calls:
            call_count.setdefault(call._method, 0)
            call_count[call._method] += 1
        call_summary = '\n'.join('%d %s' % (count, method)
                                 for method, count in call_count.iteritems())
        self._warn('Execution took %f sec\n%s' % (duration, call_summary))


    def execute_calls(self, calls):
        results = []
        start_time = time.time()
        max_processes = scheduler_config.config.max_transfer_processes
        for method_call in calls:
            results.append(method_call.execute_on(self))
            if len(self._subcommands) >= max_processes:
                self._wait_for_some_async_commands()
        self.wait_for_all_async_commands()

        duration = time.time() - start_time
        if duration > self._WARNING_DURATION:
            self._report_long_execution(calls, duration)

        warnings = self.warnings
        self.warnings = []
        return dict(results=results, warnings=warnings)


def create_host(hostname):
    username = global_config.global_config.get_config_value(
        'SCHEDULER', hostname + '_username', default=getpass.getuser())
    return hosts.SSHHost(hostname, user=username)


def parse_input():
    input_chunks = []
    chunk_of_input = sys.stdin.read()
    while chunk_of_input:
        input_chunks.append(chunk_of_input)
        chunk_of_input = sys.stdin.read()
    pickled_input = ''.join(input_chunks)

    try:
        return pickle.loads(pickled_input)
    except Exception:
        separator = '*' * 50
        raise ValueError('Unpickling input failed\n'
                         'Input: %r\n'
                         'Exception from pickle:\n'
                         '%s\n%s\n%s' %
                         (pickled_input, separator, traceback.format_exc(),
                          separator))


def _parse_args(args):
    parser = argparse.ArgumentParser(description='Local drone process manager.')
    parser.add_argument('--call_time',
                        help='Time this process was invoked from the master',
                        default=None, type=float)
    return parser.parse_args(args)


SiteDroneUtility = utils.import_site_class(
   __file__, 'autotest_lib.scheduler.site_drone_utility',
   'SiteDroneUtility', BaseDroneUtility)


class DroneUtility(SiteDroneUtility):
    pass


def return_data(data):
    print pickle.dumps(data)


def main():
    logging_manager.configure_logging(
            drone_logging_config.DroneLoggingConfig())
    with timer.get_client('decode'):
        calls = parse_input()
    args = _parse_args(sys.argv[1:])
    if args.call_time is not None:
        autotest_stats.Gauge(_STATS_KEY).send('invocation_overhead',
                                              time.time() - args.call_time)

    drone_utility = DroneUtility()
    return_value = drone_utility.execute_calls(calls)
    with timer.get_client('encode'):
        return_data(return_value)


if __name__ == '__main__':
    main()
