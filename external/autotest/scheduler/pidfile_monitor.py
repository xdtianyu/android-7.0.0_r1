#pylint: disable-msg=C0111

"""
Pidfile monitor.
"""

import logging
import time, traceback
from autotest_lib.client.common_lib import global_config
from autotest_lib.client.common_lib.cros.graphite import autotest_stats
from autotest_lib.scheduler import drone_manager, email_manager
from autotest_lib.scheduler import scheduler_config



def _get_pidfile_timeout_secs():
    """@returns How long to wait for autoserv to write pidfile."""
    pidfile_timeout_mins = global_config.global_config.get_config_value(
            scheduler_config.CONFIG_SECTION, 'pidfile_timeout_mins', type=int)
    return pidfile_timeout_mins * 60


class PidfileRunMonitor(object):
    """
    Client must call either run() to start a new process or
    attach_to_existing_process().
    """

    class _PidfileException(Exception):
        """
        Raised when there's some unexpected behavior with the pid file, but only
        used internally (never allowed to escape this class).
        """


    def __init__(self):
        self._drone_manager = drone_manager.instance()
        self.lost_process = False
        self._start_time = None
        self.pidfile_id = None
        self._killed = False
        self._state = drone_manager.PidfileContents()


    def _add_nice_command(self, command, nice_level):
        if not nice_level:
            return command
        return ['nice', '-n', str(nice_level)] + command


    def _set_start_time(self):
        self._start_time = time.time()


    def run(self, command, working_directory, num_processes, nice_level=None,
            log_file=None, pidfile_name=None, paired_with_pidfile=None,
            username=None, drone_hostnames_allowed=None):
        assert command is not None
        if nice_level is not None:
            command = ['nice', '-n', str(nice_level)] + command
        self._set_start_time()
        self.pidfile_id = self._drone_manager.execute_command(
            command, working_directory, pidfile_name=pidfile_name,
            num_processes=num_processes, log_file=log_file,
            paired_with_pidfile=paired_with_pidfile, username=username,
            drone_hostnames_allowed=drone_hostnames_allowed)


    def attach_to_existing_process(self, execution_path,
                                   pidfile_name=drone_manager.AUTOSERV_PID_FILE,
                                   num_processes=None):
        self._set_start_time()
        self.pidfile_id = self._drone_manager.get_pidfile_id_from(
            execution_path, pidfile_name=pidfile_name)
        if num_processes is not None:
            self._drone_manager.declare_process_count(self.pidfile_id, num_processes)


    def kill(self):
        if self.has_process():
            self._drone_manager.kill_process(self.get_process())
            self._killed = True


    def has_process(self):
        self._get_pidfile_info()
        return self._state.process is not None


    def get_process(self):
        self._get_pidfile_info()
        assert self._state.process is not None
        return self._state.process


    def _read_pidfile(self, use_second_read=False):
        assert self.pidfile_id is not None, (
            'You must call run() or attach_to_existing_process()')
        contents = self._drone_manager.get_pidfile_contents(
            self.pidfile_id, use_second_read=use_second_read)
        if contents.is_invalid():
            self._state = drone_manager.PidfileContents()
            raise self._PidfileException(contents)
        self._state = contents


    def _handle_pidfile_error(self, error, message=''):
        metadata = {'_type': 'scheduler_error',
                    'error': 'autoserv died without writing exit code',
                    'process': str(self._state.process),
                    'pidfile_id': str(self.pidfile_id)}
        autotest_stats.Counter('autoserv_died_without_writing_exit_code',
                               metadata=metadata).increment()
        self.on_lost_process(self._state.process)


    def _get_pidfile_info_helper(self):
        if self.lost_process:
            return

        self._read_pidfile()

        if self._state.process is None:
            self._handle_no_process()
            return

        if self._state.exit_status is None:
            # double check whether or not autoserv is running
            if self._drone_manager.is_process_running(self._state.process):
                return

            # pid but no running process - maybe process *just* exited
            self._read_pidfile(use_second_read=True)
            if self._state.exit_status is None:
                # autoserv exited without writing an exit code
                # to the pidfile
                self._handle_pidfile_error(
                    'autoserv died without writing exit code')


    def _get_pidfile_info(self):
        """\
        After completion, self._state will contain:
         pid=None, exit_status=None if autoserv has not yet run
         pid!=None, exit_status=None if autoserv is running
         pid!=None, exit_status!=None if autoserv has completed
        """
        try:
            self._get_pidfile_info_helper()
        except self._PidfileException, exc:
            self._handle_pidfile_error('Pidfile error', traceback.format_exc())


    def _handle_no_process(self):
        """\
        Called when no pidfile is found or no pid is in the pidfile.
        """
        message = 'No pid found at %s' % self.pidfile_id
        if time.time() - self._start_time > _get_pidfile_timeout_secs():
            # If we aborted the process, and we find that it has exited without
            # writing a pidfile, then it's because we killed it, and thus this
            # isn't a surprising situation.
            if not self._killed:
                email_manager.manager.enqueue_notify_email(
                    'Process has failed to write pidfile', message)
            else:
                logging.warning("%s didn't exit after SIGTERM", self.pidfile_id)
            self.on_lost_process()


    def on_lost_process(self, process=None):
        """\
        Called when autoserv has exited without writing an exit status,
        or we've timed out waiting for autoserv to write a pid to the
        pidfile.  In either case, we just return failure and the caller
        should signal some kind of warning.

        process is unimportant here, as it shouldn't be used by anyone.
        """
        self.lost_process = True
        self._state.process = process
        self._state.exit_status = 1
        self._state.num_tests_failed = 0


    def exit_code(self):
        self._get_pidfile_info()
        return self._state.exit_status


    def num_tests_failed(self):
        """@returns The number of tests that failed or -1 if unknown."""
        self._get_pidfile_info()
        if self._state.num_tests_failed is None:
            return -1
        return self._state.num_tests_failed


    def try_copy_results_on_drone(self, **kwargs):
        if self.has_process():
            # copy results logs into the normal place for job results
            self._drone_manager.copy_results_on_drone(self.get_process(), **kwargs)


    def try_copy_to_results_repository(self, source, **kwargs):
        if self.has_process():
            self._drone_manager.copy_to_results_repository(self.get_process(),
                                                      source, **kwargs)

