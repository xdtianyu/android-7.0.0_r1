"""Provides a factory method to create a host object."""

import logging
from contextlib import closing

from autotest_lib.client.bin import local_host
from autotest_lib.client.common_lib import error, global_config
from autotest_lib.server import utils as server_utils
from autotest_lib.server.hosts import cros_host, ssh_host
from autotest_lib.server.hosts import moblab_host, sonic_host
from autotest_lib.server.hosts import adb_host, testbed


SSH_ENGINE = global_config.global_config.get_config_value('AUTOSERV',
                                                          'ssh_engine',
                                                          type=str)

# Default ssh options used in creating a host.
DEFAULT_SSH_USER = 'root'
DEFAULT_SSH_PASS = ''
DEFAULT_SSH_PORT = 22
DEFAULT_SSH_VERBOSITY = ''
DEFAULT_SSH_OPTIONS = ''

# for tracking which hostnames have already had job_start called
_started_hostnames = set()

# A list of all the possible host types, ordered according to frequency of
# host types in the lab, so the more common hosts don't incur a repeated ssh
# overhead in checking for less common host types.
host_types = [cros_host.CrosHost, moblab_host.MoblabHost, sonic_host.SonicHost,
              adb_host.ADBHost,]
OS_HOST_DICT = {'cros' : cros_host.CrosHost,
                'android': adb_host.ADBHost}


def _get_host_arguments():
    """Returns parameters needed to ssh into a host.

    There are currently 2 use cases for creating a host.
    1. Through the server_job, in which case the server_job injects
       the appropriate ssh parameters into our name space and they
       are available as the variables ssh_user, ssh_pass etc.
    2. Directly through factory.create_host, in which case we use
       the same defaults as used in the server job to create a host.

    @returns: A tuple of parameters needed to create an ssh connection, ordered
              as: ssh_user, ssh_pass, ssh_port, ssh_verbosity, ssh_options.
    """
    g = globals()
    return (g.get('ssh_user', DEFAULT_SSH_USER),
            g.get('ssh_pass', DEFAULT_SSH_PASS),
            g.get('ssh_port', DEFAULT_SSH_PORT),
            g.get('ssh_verbosity_flag', DEFAULT_SSH_VERBOSITY),
            g.get('ssh_options', DEFAULT_SSH_OPTIONS))


def _detect_host(connectivity_class, hostname, **args):
    """Detect host type.

    Goes through all the possible host classes, calling check_host with a
    basic host object. Currently this is an ssh host, but theoretically it
    can be any host object that the check_host method of appropriate host
    type knows to use.

    @param connectivity_class: connectivity class to use to talk to the host
                               (ParamikoHost or SSHHost)
    @param hostname: A string representing the host name of the device.
    @param args: Args that will be passed to the constructor of
                 the host class.

    @returns: Class type of the first host class that returns True to the
              check_host method.
    """
    # TODO crbug.com/302026 (sbasi) - adjust this pathway for ADBHost in
    # the future should a host require verify/repair.
    with closing(connectivity_class(hostname, **args)) as host:
        for host_module in host_types:
            if host_module.check_host(host, timeout=10):
                return host_module

    logging.warning('Unable to apply conventional host detection methods, '
                    'defaulting to chromeos host.')
    return cros_host.CrosHost


def _choose_connectivity_class(hostname, ssh_port):
    """Choose a connectivity class for this hostname.

    @param hostname: hostname that we need a connectivity class for.
    @param ssh_port: SSH port to connect to the host.

    @returns a connectivity host class.
    """
    if (hostname == 'localhost' and ssh_port == DEFAULT_SSH_PORT):
        return local_host.LocalHost
    # by default assume we're using SSH support
    elif SSH_ENGINE == 'paramiko':
        # Not all systems have paramiko installed so only import paramiko host
        # if the global_config settings call for it.
        from autotest_lib.server.hosts import paramiko_host
        return paramiko_host.ParamikoHost
    elif SSH_ENGINE == 'raw_ssh':
        return ssh_host.SSHHost
    else:
        raise error.AutoServError("Unknown SSH engine %s. Please verify the "
                                  "value of the configuration key 'ssh_engine' "
                                  "on autotest's global_config.ini file." %
                                  SSH_ENGINE)


# TODO(kevcheng): Update the creation method so it's not a research project
# determining the class inheritance model.
def create_host(machine, host_class=None, connectivity_class=None, **args):
    """Create a host object.

    This method mixes host classes that are needed into a new subclass
    and creates a instance of the new class.

    @param machine: A dict representing the device under test or a String
                    representing the DUT hostname (for legacy caller support).
                    If it is a machine dict, the 'hostname' key is required.
                    Optional 'host_attributes' key will pipe in host_attributes
                    from the autoserv runtime or the AFE.
    @param host_class: Host class to use, if None, will attempt to detect
                       the correct class.
    @param connectivity_class: Connectivity class to use, if None will decide
                               based off of hostname and config settings.
    @param args: Args that will be passed to the constructor of
                 the new host class.

    @returns: A host object which is an instance of the newly created
              host class.
    """
    hostname, host_attributes = server_utils.get_host_info_from_machine(
            machine)
    args['host_attributes'] = host_attributes
    ssh_user, ssh_pass, ssh_port, ssh_verbosity_flag, ssh_options = \
            _get_host_arguments()

    hostname, args['user'], args['password'], ssh_port = \
            server_utils.parse_machine(hostname, ssh_user, ssh_pass, ssh_port)
    args['ssh_verbosity_flag'] = ssh_verbosity_flag
    args['ssh_options'] = ssh_options
    args['port'] = ssh_port

    if not connectivity_class:
        connectivity_class = _choose_connectivity_class(hostname, ssh_port)
    host_attributes = args.get('host_attributes', {})
    host_class = host_class or OS_HOST_DICT.get(host_attributes.get('os_type'))
    if host_class:
        classes = [host_class, connectivity_class]
    else:
        classes = [_detect_host(connectivity_class, hostname, **args),
                   connectivity_class]

    # create a custom host class for this machine and return an instance of it
    host_class = type("%s_host" % hostname, tuple(classes), {})
    host_instance = host_class(hostname, **args)

    # call job_start if this is the first time this host is being used
    if hostname not in _started_hostnames:
        host_instance.job_start()
        _started_hostnames.add(hostname)

    return host_instance


def create_testbed(machine, **kwargs):
    """Create the testbed object.

    @param machine: A dict representing the test bed under test or a String
                    representing the testbed hostname (for legacy caller
                    support).
                    If it is a machine dict, the 'hostname' key is required.
                    Optional 'host_attributes' key will pipe in host_attributes
                    from the autoserv runtime or the AFE.
    @param kwargs: Keyword args to pass to the testbed initialization.

    @returns: The testbed object with all associated host objects instantiated.
    """
    hostname, host_attributes = server_utils.get_host_info_from_machine(
            machine)
    kwargs['host_attributes'] = host_attributes
    return testbed.TestBed(hostname, **kwargs)


def create_target_machine(machine, **kwargs):
    """Create the target machine which could be a testbed or a *Host.

    @param machine: A dict representing the test bed under test or a String
                    representing the testbed hostname (for legacy caller
                    support).
                    If it is a machine dict, the 'hostname' key is required.
                    Optional 'host_attributes' key will pipe in host_attributes
                    from the autoserv runtime or the AFE.
    @param kwargs: Keyword args to pass to the testbed initialization.

    @returns: The target machine to be used for verify/repair.
    """
    # TODO(kevcheng): We'll want to have a smarter way of figuring out which
    # host to create (checking host labels).
    if server_utils.machine_is_testbed(machine):
        return create_testbed(machine, **kwargs)
    return create_host(machine, **kwargs)
