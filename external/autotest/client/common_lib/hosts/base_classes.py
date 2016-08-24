# Copyright 2009 Google Inc. Released under the GPL v2

"""
This module defines the base classes for the Host hierarchy.

Implementation details:
You should import the "hosts" package instead of importing each type of host.

        Host: a machine on which you can run programs
"""

__author__ = """
mbligh@google.com (Martin J. Bligh),
poirier@google.com (Benjamin Poirier),
stutsman@google.com (Ryan Stutsman)
"""

import cPickle, cStringIO, logging, os, re, time

from autotest_lib.client.common_lib import global_config, error, utils
from autotest_lib.client.common_lib.cros import path_utils
from autotest_lib.client.common_lib.cros.graphite import autotest_stats
from autotest_lib.client.bin import partition


class Host(object):
    """
    This class represents a machine on which you can run programs.

    It may be a local machine, the one autoserv is running on, a remote
    machine or a virtual machine.

    Implementation details:
    This is an abstract class, leaf subclasses must implement the methods
    listed here. You must not instantiate this class but should
    instantiate one of those leaf subclasses.

    When overriding methods that raise NotImplementedError, the leaf class
    is fully responsible for the implementation and should not chain calls
    to super. When overriding methods that are a NOP in Host, the subclass
    should chain calls to super(). The criteria for fitting a new method into
    one category or the other should be:
        1. If two separate generic implementations could reasonably be
           concatenated, then the abstract implementation should pass and
           subclasses should chain calls to super.
        2. If only one class could reasonably perform the stated function
           (e.g. two separate run() implementations cannot both be executed)
           then the method should raise NotImplementedError in Host, and
           the implementor should NOT chain calls to super, to ensure that
           only one implementation ever gets executed.
    """

    job = None
    DEFAULT_REBOOT_TIMEOUT = global_config.global_config.get_config_value(
        "HOSTS", "default_reboot_timeout", type=int, default=1800)
    WAIT_DOWN_REBOOT_TIMEOUT = global_config.global_config.get_config_value(
        "HOSTS", "wait_down_reboot_timeout", type=int, default=840)
    WAIT_DOWN_REBOOT_WARNING = global_config.global_config.get_config_value(
        "HOSTS", "wait_down_reboot_warning", type=int, default=540)
    HOURS_TO_WAIT_FOR_RECOVERY = global_config.global_config.get_config_value(
        "HOSTS", "hours_to_wait_for_recovery", type=float, default=2.5)
    # the number of hardware repair requests that need to happen before we
    # actually send machines to hardware repair
    HARDWARE_REPAIR_REQUEST_THRESHOLD = 4
    OP_REBOOT = 'reboot'
    OP_SUSPEND = 'suspend'
    PWR_OPERATION = [OP_REBOOT, OP_SUSPEND]


    def __init__(self, *args, **dargs):
        self._initialize(*args, **dargs)


    def _initialize(self, *args, **dargs):
        pass


    def close(self):
        pass


    def setup(self):
        pass


    def run(self, command, timeout=3600, ignore_status=False,
            stdout_tee=utils.TEE_TO_LOGS, stderr_tee=utils.TEE_TO_LOGS,
            stdin=None, args=()):
        """
        Run a command on this host.

        @param command: the command line string
        @param timeout: time limit in seconds before attempting to
                kill the running process. The run() function
                will take a few seconds longer than 'timeout'
                to complete if it has to kill the process.
        @param ignore_status: do not raise an exception, no matter
                what the exit code of the command is.
        @param stdout_tee/stderr_tee: where to tee the stdout/stderr
        @param stdin: stdin to pass (a string) to the executed command
        @param args: sequence of strings to pass as arguments to command by
                quoting them in " and escaping their contents if necessary

        @return a utils.CmdResult object

        @raises AutotestHostRunError: the exit code of the command execution
                was not 0 and ignore_status was not enabled
        """
        raise NotImplementedError('Run not implemented!')


    def run_output(self, command, *args, **dargs):
        return self.run(command, *args, **dargs).stdout.rstrip()


    def reboot(self):
        raise NotImplementedError('Reboot not implemented!')


    def suspend(self):
        raise NotImplementedError('Suspend not implemented!')


    def sysrq_reboot(self):
        raise NotImplementedError('Sysrq reboot not implemented!')


    def reboot_setup(self, *args, **dargs):
        pass


    def reboot_followup(self, *args, **dargs):
        pass


    def get_file(self, source, dest, delete_dest=False):
        raise NotImplementedError('Get file not implemented!')


    def send_file(self, source, dest, delete_dest=False):
        raise NotImplementedError('Send file not implemented!')


    def get_tmp_dir(self):
        raise NotImplementedError('Get temp dir not implemented!')


    def is_up(self):
        raise NotImplementedError('Is up not implemented!')


    def is_shutting_down(self):
        """ Indicates is a machine is currently shutting down. """
        return False


    def get_wait_up_processes(self):
        """ Gets the list of local processes to wait for in wait_up. """
        get_config = global_config.global_config.get_config_value
        proc_list = get_config("HOSTS", "wait_up_processes",
                               default="").strip()
        processes = set(p.strip() for p in proc_list.split(","))
        processes.discard("")
        return processes


    def get_boot_id(self, timeout=60):
        """ Get a unique ID associated with the current boot.

        Should return a string with the semantics such that two separate
        calls to Host.get_boot_id() return the same string if the host did
        not reboot between the two calls, and two different strings if it
        has rebooted at least once between the two calls.

        @param timeout The number of seconds to wait before timing out.

        @return A string unique to this boot or None if not available."""
        BOOT_ID_FILE = '/proc/sys/kernel/random/boot_id'
        NO_ID_MSG = 'no boot_id available'
        cmd = 'if [ -f %r ]; then cat %r; else echo %r; fi' % (
                BOOT_ID_FILE, BOOT_ID_FILE, NO_ID_MSG)
        boot_id = self.run(cmd, timeout=timeout).stdout.strip()
        if boot_id == NO_ID_MSG:
            return None
        return boot_id


    def wait_up(self, timeout=None):
        raise NotImplementedError('Wait up not implemented!')


    def wait_down(self, timeout=None, warning_timer=None, old_boot_id=None):
        raise NotImplementedError('Wait down not implemented!')


    def _construct_host_metadata(self, type_str):
        """Returns dict of metadata with type_str, hostname, time_recorded.

        @param type_str: String representing _type field in es db.
            For example: type_str='reboot_total'.
        """
        metadata = {
            'hostname': self.hostname,
            'time_recorded': time.time(),
            '_type': type_str,
        }
        return metadata


    def wait_for_restart(self, timeout=DEFAULT_REBOOT_TIMEOUT,
                         down_timeout=WAIT_DOWN_REBOOT_TIMEOUT,
                         down_warning=WAIT_DOWN_REBOOT_WARNING,
                         log_failure=True, old_boot_id=None, **dargs):
        """ Wait for the host to come back from a reboot. This is a generic
        implementation based entirely on wait_up and wait_down. """
        key_string = 'Reboot.%s' % dargs.get('board')

        total_reboot_timer = autotest_stats.Timer('%s.total' % key_string,
                metadata=self._construct_host_metadata('reboot_total'))
        wait_down_timer = autotest_stats.Timer('%s.wait_down' % key_string,
                metadata=self._construct_host_metadata('reboot_down'))

        total_reboot_timer.start()
        wait_down_timer.start()
        if not self.wait_down(timeout=down_timeout,
                              warning_timer=down_warning,
                              old_boot_id=old_boot_id):
            if log_failure:
                self.record("ABORT", None, "reboot.verify", "shut down failed")
            raise error.AutoservShutdownError("Host did not shut down")
        wait_down_timer.stop()
        wait_up_timer = autotest_stats.Timer('%s.wait_up' % key_string,
                metadata=self._construct_host_metadata('reboot_up'))
        wait_up_timer.start()
        if self.wait_up(timeout):
            self.record("GOOD", None, "reboot.verify")
            self.reboot_followup(**dargs)
            wait_up_timer.stop()
            total_reboot_timer.stop()
        else:
            self.record("ABORT", None, "reboot.verify",
                        "Host did not return from reboot")
            raise error.AutoservRebootError("Host did not return from reboot")


    def verify(self):
        self.verify_hardware()
        self.verify_connectivity()
        self.verify_software()


    def verify_hardware(self):
        pass


    def verify_connectivity(self):
        pass


    def verify_software(self):
        pass


    def check_diskspace(self, path, gb):
        """Raises an error if path does not have at least gb GB free.

        @param path The path to check for free disk space.
        @param gb A floating point number to compare with a granularity
            of 1 MB.

        1000 based SI units are used.

        @raises AutoservDiskFullHostError if path has less than gb GB free.
        """
        one_mb = 10 ** 6  # Bytes (SI unit).
        mb_per_gb = 1000.0
        logging.info('Checking for >= %s GB of space under %s on machine %s',
                     gb, path, self.hostname)
        df = self.run('df -PB %d %s | tail -1' % (one_mb, path)).stdout.split()
        free_space_gb = int(df[3]) / mb_per_gb
        if free_space_gb < gb:
            raise error.AutoservDiskFullHostError(path, gb, free_space_gb)
        else:
            logging.info('Found %s GB >= %s GB of space under %s on machine %s',
                free_space_gb, gb, path, self.hostname)


    def check_inodes(self, path, min_kilo_inodes):
        """Raises an error if a file system is short on i-nodes.

        @param path The path to check for free i-nodes.
        @param min_kilo_inodes Minimum number of i-nodes required,
                               in units of 1000 i-nodes.

        @raises AutoservNoFreeInodesError If the minimum required
                                  i-node count isn't available.
        """
        min_inodes = 1000 * min_kilo_inodes
        logging.info('Checking for >= %d i-nodes under %s '
                     'on machine %s', min_inodes, path, self.hostname)
        df = self.run('df -Pi %s | tail -1' % path).stdout.split()
        free_inodes = int(df[3])
        if free_inodes < min_inodes:
            raise error.AutoservNoFreeInodesError(path, min_inodes,
                                                  free_inodes)
        else:
            logging.info('Found %d >= %d i-nodes under %s on '
                         'machine %s', free_inodes, min_inodes,
                         path, self.hostname)


    def erase_dir_contents(self, path, ignore_status=True, timeout=3600):
        """Empty a given directory path contents."""
        rm_cmd = 'find "%s" -mindepth 1 -maxdepth 1 -print0 | xargs -0 rm -rf'
        self.run(rm_cmd % path, ignore_status=ignore_status, timeout=timeout)


    def repair(self):
        """Try and get the host to pass `self.verify()`."""
        self.verify()


    def disable_ipfilters(self):
        """Allow all network packets in and out of the host."""
        self.run('iptables-save > /tmp/iptable-rules')
        self.run('iptables -P INPUT ACCEPT')
        self.run('iptables -P FORWARD ACCEPT')
        self.run('iptables -P OUTPUT ACCEPT')


    def enable_ipfilters(self):
        """Re-enable the IP filters disabled from disable_ipfilters()"""
        if self.path_exists('/tmp/iptable-rules'):
            self.run('iptables-restore < /tmp/iptable-rules')


    def cleanup(self):
        pass


    def machine_install(self):
        raise NotImplementedError('Machine install not implemented!')


    def install(self, installableObject):
        installableObject.install(self)


    def get_autodir(self):
        raise NotImplementedError('Get autodir not implemented!')


    def set_autodir(self):
        raise NotImplementedError('Set autodir not implemented!')


    def start_loggers(self):
        """ Called to start continuous host logging. """
        pass


    def stop_loggers(self):
        """ Called to stop continuous host logging. """
        pass


    # some extra methods simplify the retrieval of information about the
    # Host machine, with generic implementations based on run(). subclasses
    # should feel free to override these if they can provide better
    # implementations for their specific Host types

    def get_num_cpu(self):
        """ Get the number of CPUs in the host according to /proc/cpuinfo. """
        proc_cpuinfo = self.run('cat /proc/cpuinfo',
                                stdout_tee=open(os.devnull, 'w')).stdout
        cpus = 0
        for line in proc_cpuinfo.splitlines():
            if line.startswith('processor'):
                cpus += 1
        return cpus


    def get_arch(self):
        """ Get the hardware architecture of the remote machine. """
        cmd_uname = path_utils.must_be_installed('/bin/uname', host=self)
        arch = self.run('%s -m' % cmd_uname).stdout.rstrip()
        if re.match(r'i\d86$', arch):
            arch = 'i386'
        return arch


    def get_kernel_ver(self):
        """ Get the kernel version of the remote machine. """
        cmd_uname = path_utils.must_be_installed('/bin/uname', host=self)
        return self.run('%s -r' % cmd_uname).stdout.rstrip()


    def get_cmdline(self):
        """ Get the kernel command line of the remote machine. """
        return self.run('cat /proc/cmdline').stdout.rstrip()


    def get_meminfo(self):
        """ Get the kernel memory info (/proc/meminfo) of the remote machine
        and return a dictionary mapping the various statistics. """
        meminfo_dict = {}
        meminfo = self.run('cat /proc/meminfo').stdout.splitlines()
        for key, val in (line.split(':', 1) for line in meminfo):
            meminfo_dict[key.strip()] = val.strip()
        return meminfo_dict


    def path_exists(self, path):
        """ Determine if path exists on the remote machine. """
        result = self.run('ls "%s" > /dev/null' % utils.sh_escape(path),
                          ignore_status=True)
        return result.exit_status == 0


    # some extra helpers for doing job-related operations

    def record(self, *args, **dargs):
        """ Helper method for recording status logs against Host.job that
        silently becomes a NOP if Host.job is not available. The args and
        dargs are passed on to Host.job.record unchanged. """
        if self.job:
            self.job.record(*args, **dargs)


    def log_kernel(self):
        """ Helper method for logging kernel information into the status logs.
        Intended for cases where the "current" kernel is not really defined
        and we want to explicitly log it. Does nothing if this host isn't
        actually associated with a job. """
        if self.job:
            kernel = self.get_kernel_ver()
            self.job.record("INFO", None, None,
                            optional_fields={"kernel": kernel})


    def log_op(self, op, op_func):
        """ Decorator for wrapping a management operaiton in a group for status
        logging purposes.

        @param op: name of the operation.
        @param op_func: a function that carries out the operation
                        (reboot, suspend)
        """
        if self.job and not hasattr(self, "RUNNING_LOG_OP"):
            self.RUNNING_LOG_OP = True
            try:
                self.job.run_op(op, op_func, self.get_kernel_ver)
            finally:
                del self.RUNNING_LOG_OP
        else:
            op_func()


    def list_files_glob(self, glob):
        """
        Get a list of files on a remote host given a glob pattern path.
        """
        SCRIPT = ("python -c 'import cPickle, glob, sys;"
                  "cPickle.dump(glob.glob(sys.argv[1]), sys.stdout, 0)'")
        output = self.run(SCRIPT, args=(glob,), stdout_tee=None,
                          timeout=60).stdout
        return cPickle.loads(output)


    def symlink_closure(self, paths):
        """
        Given a sequence of path strings, return the set of all paths that
        can be reached from the initial set by following symlinks.

        @param paths: sequence of path strings.
        @return: a sequence of path strings that are all the unique paths that
                can be reached from the given ones after following symlinks.
        """
        SCRIPT = ("python -c 'import cPickle, os, sys\n"
                  "paths = cPickle.load(sys.stdin)\n"
                  "closure = {}\n"
                  "while paths:\n"
                  "    path = paths.keys()[0]\n"
                  "    del paths[path]\n"
                  "    if not os.path.exists(path):\n"
                  "        continue\n"
                  "    closure[path] = None\n"
                  "    if os.path.islink(path):\n"
                  "        link_to = os.path.join(os.path.dirname(path),\n"
                  "                               os.readlink(path))\n"
                  "        if link_to not in closure.keys():\n"
                  "            paths[link_to] = None\n"
                  "cPickle.dump(closure.keys(), sys.stdout, 0)'")
        input_data = cPickle.dumps(dict((path, None) for path in paths), 0)
        output = self.run(SCRIPT, stdout_tee=None, stdin=input_data,
                          timeout=60).stdout
        return cPickle.loads(output)


    def cleanup_kernels(self, boot_dir='/boot'):
        """
        Remove any kernel image and associated files (vmlinux, system.map,
        modules) for any image found in the boot directory that is not
        referenced by entries in the bootloader configuration.

        @param boot_dir: boot directory path string, default '/boot'
        """
        # find all the vmlinuz images referenced by the bootloader
        vmlinuz_prefix = os.path.join(boot_dir, 'vmlinuz-')
        boot_info = self.bootloader.get_entries()
        used_kernver = [boot['kernel'][len(vmlinuz_prefix):]
                        for boot in boot_info.itervalues()]

        # find all the unused vmlinuz images in /boot
        all_vmlinuz = self.list_files_glob(vmlinuz_prefix + '*')
        used_vmlinuz = self.symlink_closure(vmlinuz_prefix + kernver
                                            for kernver in used_kernver)
        unused_vmlinuz = set(all_vmlinuz) - set(used_vmlinuz)

        # find all the unused vmlinux images in /boot
        vmlinux_prefix = os.path.join(boot_dir, 'vmlinux-')
        all_vmlinux = self.list_files_glob(vmlinux_prefix + '*')
        used_vmlinux = self.symlink_closure(vmlinux_prefix + kernver
                                            for kernver in used_kernver)
        unused_vmlinux = set(all_vmlinux) - set(used_vmlinux)

        # find all the unused System.map files in /boot
        systemmap_prefix = os.path.join(boot_dir, 'System.map-')
        all_system_map = self.list_files_glob(systemmap_prefix + '*')
        used_system_map = self.symlink_closure(
            systemmap_prefix + kernver for kernver in used_kernver)
        unused_system_map = set(all_system_map) - set(used_system_map)

        # find all the module directories associated with unused kernels
        modules_prefix = '/lib/modules/'
        all_moddirs = [dir for dir in self.list_files_glob(modules_prefix + '*')
                       if re.match(modules_prefix + r'\d+\.\d+\.\d+.*', dir)]
        used_moddirs = self.symlink_closure(modules_prefix + kernver
                                            for kernver in used_kernver)
        unused_moddirs = set(all_moddirs) - set(used_moddirs)

        # remove all the vmlinuz files we don't use
        # TODO: if needed this should become package manager agnostic
        for vmlinuz in unused_vmlinuz:
            # try and get an rpm package name
            rpm = self.run('rpm -qf', args=(vmlinuz,),
                           ignore_status=True, timeout=120)
            if rpm.exit_status == 0:
                packages = set(line.strip() for line in
                               rpm.stdout.splitlines())
                # if we found some package names, try to remove them
                for package in packages:
                    self.run('rpm -e', args=(package,),
                             ignore_status=True, timeout=120)
            # remove the image files anyway, even if rpm didn't
            self.run('rm -f', args=(vmlinuz,),
                     ignore_status=True, timeout=120)

        # remove all the vmlinux and System.map files left over
        for f in (unused_vmlinux | unused_system_map):
            self.run('rm -f', args=(f,),
                     ignore_status=True, timeout=120)

        # remove all unused module directories
        # the regex match should keep us safe from removing the wrong files
        for moddir in unused_moddirs:
            self.run('rm -fr', args=(moddir,), ignore_status=True)
