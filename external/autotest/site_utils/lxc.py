# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""This module provides some tools to interact with LXC containers, for example:
  1. Download base container from given GS location, setup the base container.
  2. Create a snapshot as test container from base container.
  3. Mount a directory in drone to the test container.
  4. Run a command in the container and return the output.
  5. Cleanup, e.g., destroy the container.

This tool can also be used to set up a base container for test. For example,
  python lxc.py -s -p /tmp/container
This command will download and setup base container in directory /tmp/container.
After that command finishes, you can run lxc command to work with the base
container, e.g.,
  lxc-start -P /tmp/container -n base -d
  lxc-attach -P /tmp/container -n base
"""


import argparse
import logging
import os
import re
import socket
import sys
import time

import common
from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib import global_config
from autotest_lib.client.common_lib.cros import retry
from autotest_lib.client.common_lib.cros.graphite import autotest_es
from autotest_lib.client.common_lib.cros.graphite import autotest_stats
from autotest_lib.server import utils as server_utils
from autotest_lib.site_utils import lxc_config
from autotest_lib.site_utils import lxc_utils


config = global_config.global_config

# Name of the base container.
BASE = 'base'
# Naming convention of test container, e.g., test_300_1422862512_2424, where:
# 300:        The test job ID.
# 1422862512: The tick when container is created.
# 2424:       The PID of autoserv that starts the container.
TEST_CONTAINER_NAME_FMT = 'test_%s_%d_%d'
# Naming convention of the result directory in test container.
RESULT_DIR_FMT = os.path.join(lxc_config.CONTAINER_AUTOTEST_DIR, 'results',
                              '%s')
# Attributes to retrieve about containers.
ATTRIBUTES = ['name', 'state']

# Format for mount entry to share a directory in host with container.
# source is the directory in host, destination is the directory in container.
# readonly is a binding flag for readonly mount, its value should be `,ro`.
MOUNT_FMT = ('lxc.mount.entry = %(source)s %(destination)s none '
             'bind%(readonly)s 0 0')
SSP_ENABLED = config.get_config_value('AUTOSERV', 'enable_ssp_container',
                                      type=bool, default=True)
# url to the base container.
CONTAINER_BASE_URL = config.get_config_value('AUTOSERV', 'container_base')
# Default directory used to store LXC containers.
DEFAULT_CONTAINER_PATH = config.get_config_value('AUTOSERV', 'container_path')

# Path to drone_temp folder in the container, which stores the control file for
# test job to run.
CONTROL_TEMP_PATH = os.path.join(lxc_config.CONTAINER_AUTOTEST_DIR, 'drone_tmp')

# Bash command to return the file count in a directory. Test the existence first
# so the command can return an error code if the directory doesn't exist.
COUNT_FILE_CMD = '[ -d %(dir)s ] && ls %(dir)s | wc -l'

# Command line to append content to a file
APPEND_CMD_FMT = ('echo \'%(content)s\' | sudo tee --append %(file)s'
                  '> /dev/null')

# Path to site-packates in Moblab
MOBLAB_SITE_PACKAGES = '/usr/lib64/python2.7/site-packages'
MOBLAB_SITE_PACKAGES_CONTAINER = '/usr/local/lib/python2.7/dist-packages/'

# Flag to indicate it's running in a Moblab. Due to crbug.com/457496, lxc-ls has
# different behavior in Moblab.
IS_MOBLAB = utils.is_moblab()

# TODO(dshi): If we are adding more logic in how lxc should interact with
# different systems, we should consider code refactoring to use a setting-style
# object to store following flags mapping to different systems.
# TODO(crbug.com/464834): Snapshot clone is disabled until Moblab can
# support overlayfs or aufs, which requires a newer kernel.
SUPPORT_SNAPSHOT_CLONE = not IS_MOBLAB

# Number of seconds to wait for network to be up in a container.
NETWORK_INIT_TIMEOUT = 300
# Network bring up is slower in Moblab.
NETWORK_INIT_CHECK_INTERVAL = 2 if IS_MOBLAB else 0.1

# Type string for container related metadata.
CONTAINER_CREATE_METADB_TYPE = 'container_create'
CONTAINER_CREATE_RETRY_METADB_TYPE = 'container_create_retry'
CONTAINER_RUN_TEST_METADB_TYPE = 'container_run_test'

STATS_KEY = 'lxc.%s' % socket.gethostname().replace('.', '_')
timer = autotest_stats.Timer(STATS_KEY)
# Timer used inside container should not include the hostname, as that will
# create individual timer for each container.
container_timer = autotest_stats.Timer('lxc')


def _get_container_info_moblab(container_path, **filters):
    """Get a collection of container information in the given container path
    in a Moblab.

    TODO(crbug.com/457496): remove this method once python 3 can be installed
    in Moblab and lxc-ls command can use python 3 code.

    When running in Moblab, lxc-ls behaves differently from a server with python
    3 installed:
    1. lxc-ls returns a list of containers installed under /etc/lxc, the default
       lxc container directory.
    2. lxc-ls --active lists all active containers, regardless where the
       container is located.
    For such differences, we have to special case Moblab to make the behavior
    close to a server with python 3 installed. That is,
    1. List only containers in a given folder.
    2. Assume all active containers have state of RUNNING.

    @param container_path: Path to look for containers.
    @param filters: Key value to filter the containers, e.g., name='base'

    @return: A list of dictionaries that each dictionary has the information of
             a container. The keys are defined in ATTRIBUTES.
    """
    info_collection = []
    active_containers = utils.run('sudo lxc-ls --active').stdout.split()
    name_filter = filters.get('name', None)
    state_filter = filters.get('state', None)
    if filters and set(filters.keys()) - set(['name', 'state']):
        raise error.ContainerError('When running in Moblab, container list '
                                   'filter only supports name and state.')

    for name in os.listdir(container_path):
        # Skip all files and folders without rootfs subfolder.
        if (os.path.isfile(os.path.join(container_path, name)) or
            not lxc_utils.path_exists(os.path.join(container_path, name,
                                                   'rootfs'))):
            continue
        info = {'name': name,
                'state': 'RUNNING' if name in active_containers else 'STOPPED'
               }
        if ((name_filter and name_filter != info['name']) or
            (state_filter and state_filter != info['state'])):
            continue

        info_collection.append(info)
    return info_collection


def get_container_info(container_path, **filters):
    """Get a collection of container information in the given container path.

    This method parse the output of lxc-ls to get a list of container
    information. The lxc-ls command output looks like:
    NAME      STATE    IPV4       IPV6  AUTOSTART  PID   MEMORY  RAM     SWAP
    --------------------------------------------------------------------------
    base      STOPPED  -          -     NO         -     -       -       -
    test_123  RUNNING  10.0.3.27  -     NO         8359  6.28MB  6.28MB  0.0MB

    @param container_path: Path to look for containers.
    @param filters: Key value to filter the containers, e.g., name='base'

    @return: A list of dictionaries that each dictionary has the information of
             a container. The keys are defined in ATTRIBUTES.
    """
    if IS_MOBLAB:
        return _get_container_info_moblab(container_path, **filters)

    cmd = 'sudo lxc-ls -P %s -f -F %s' % (os.path.realpath(container_path),
                                          ','.join(ATTRIBUTES))
    output = utils.run(cmd).stdout
    info_collection = []

    for line in output.splitlines()[2:]:
        info_collection.append(dict(zip(ATTRIBUTES, line.split())))
    if filters:
        filtered_collection = []
        for key, value in filters.iteritems():
            for info in info_collection:
                if key in info and info[key] == value:
                    filtered_collection.append(info)
        info_collection = filtered_collection
    return info_collection


def cleanup_if_fail():
    """Decorator to do cleanup if container fails to be set up.
    """
    def deco_cleanup_if_fail(func):
        """Wrapper for the decorator.

        @param func: Function to be called.
        """
        def func_cleanup_if_fail(*args, **kwargs):
            """Decorator to do cleanup if container fails to be set up.

            The first argument must be a ContainerBucket object, which can be
            used to retrieve the container object by name.

            @param func: function to be called.
            @param args: arguments for function to be called.
            @param kwargs: keyword arguments for function to be called.
            """
            bucket = args[0]
            name = utils.get_function_arg_value(func, 'name', args, kwargs)
            try:
                skip_cleanup = utils.get_function_arg_value(
                        func, 'skip_cleanup', args, kwargs)
            except (KeyError, ValueError):
                skip_cleanup = False
            try:
                return func(*args, **kwargs)
            except:
                exc_info = sys.exc_info()
                try:
                    container = bucket.get(name)
                    if container and not skip_cleanup:
                        container.destroy()
                except error.CmdError as e:
                    logging.error(e)

                try:
                    job_id = utils.get_function_arg_value(
                            func, 'job_id', args, kwargs)
                except (KeyError, ValueError):
                    job_id = ''
                metadata={'drone': socket.gethostname(),
                          'job_id': job_id,
                          'success': False}
                # Record all args if job_id is not available.
                if not job_id:
                    metadata['args'] = str(args)
                    if kwargs:
                        metadata.update(kwargs)
                autotest_es.post(use_http=True,
                                 type_str=CONTAINER_CREATE_METADB_TYPE,
                                 metadata=metadata)

                # Raise the cached exception with original backtrace.
                raise exc_info[0], exc_info[1], exc_info[2]
        return func_cleanup_if_fail
    return deco_cleanup_if_fail


@retry.retry(error.CmdError, timeout_min=5)
def download_extract(url, target, extract_dir):
    """Download the file from given url and save it to the target, then extract.

    @param url: Url to download the file.
    @param target: Path of the file to save to.
    @param extract_dir: Directory to extract the content of the file to.
    """
    utils.run('sudo wget --timeout=300 -nv %s -O %s' % (url, target))
    utils.run('sudo tar -xvf %s -C %s' % (target, extract_dir))


def install_package_precheck(packages):
    """If SSP is not enabled or the test is running in chroot (using test_that),
    packages installation should be skipped.

    The check does not raise exception so tests started by test_that or running
    in an Autotest setup with SSP disabled can continue. That assume the running
    environment, chroot or a machine, has the desired packages installed
    already.

    @param packages: A list of names of the packages to install.

    @return: True if package installation can continue. False if it should be
             skipped.

    """
    if not SSP_ENABLED and not utils.is_in_container():
        logging.info('Server-side packaging is not enabled. Install package %s '
                     'is skipped.', packages)
        return False

    if server_utils.is_inside_chroot():
        logging.info('Test is running inside chroot. Install package %s is '
                     'skipped.', packages)
        return False

    return True


@container_timer.decorate
@retry.retry(error.CmdError, timeout_min=30)
def install_packages(packages=[], python_packages=[]):
    """Install the given package inside container.

    @param packages: A list of names of the packages to install.
    @param python_packages: A list of names of the python packages to install
                            using pip.

    @raise error.ContainerError: If package is attempted to be installed outside
                                 a container.
    @raise error.CmdError: If the package doesn't exist or failed to install.

    """
    if not install_package_precheck(packages or python_packages):
        return

    if not utils.is_in_container():
        raise error.ContainerError('Package installation is only supported '
                                   'when test is running inside container.')
    # Always run apt-get update before installing any container. The base
    # container may have outdated cache.
    utils.run('sudo apt-get update')
    # Make sure the lists are not None for iteration.
    packages = [] if not packages else packages
    if python_packages:
        packages.extend(['python-pip', 'python-dev'])
    if packages:
        utils.run('sudo apt-get install %s -y --force-yes' % ' '.join(packages))
        logging.debug('Packages are installed: %s.', packages)

    target_setting = ''
    # For containers running in Moblab, /usr/local/lib/python2.7/dist-packages/
    # is a readonly mount from the host. Therefore, new python modules have to
    # be installed in /usr/lib/python2.7/dist-packages/
    # Containers created in Moblab does not have autotest/site-packages folder.
    if not os.path.exists('/usr/local/autotest/site-packages'):
        target_setting = '--target="/usr/lib/python2.7/dist-packages/"'
    if python_packages:
        utils.run('sudo pip install %s %s' % (target_setting,
                                              ' '.join(python_packages)))
        logging.debug('Python packages are installed: %s.', python_packages)


@container_timer.decorate
@retry.retry(error.CmdError, timeout_min=20)
def install_package(package):
    """Install the given package inside container.

    This function is kept for backwards compatibility reason. New code should
    use function install_packages for better performance.

    @param package: Name of the package to install.

    @raise error.ContainerError: If package is attempted to be installed outside
                                 a container.
    @raise error.CmdError: If the package doesn't exist or failed to install.

    """
    logging.warn('This function is obsoleted, please use install_packages '
                 'instead.')
    install_packages(packages=[package])


@container_timer.decorate
@retry.retry(error.CmdError, timeout_min=20)
def install_python_package(package):
    """Install the given python package inside container using pip.

    This function is kept for backwards compatibility reason. New code should
    use function install_packages for better performance.

    @param package: Name of the python package to install.

    @raise error.CmdError: If the package doesn't exist or failed to install.
    """
    logging.warn('This function is obsoleted, please use install_packages '
                 'instead.')
    install_packages(python_packages=[package])


class Container(object):
    """A wrapper class of an LXC container.

    The wrapper class provides methods to interact with a container, e.g.,
    start, stop, destroy, run a command. It also has attributes of the
    container, including:
    name: Name of the container.
    state: State of the container, e.g., ABORTING, RUNNING, STARTING, STOPPED,
           or STOPPING.

    lxc-ls can also collect other attributes of a container including:
    ipv4: IP address for IPv4.
    ipv6: IP address for IPv6.
    autostart: If the container will autostart at system boot.
    pid: Process ID of the container.
    memory: Memory used by the container, as a string, e.g., "6.2MB"
    ram: Physical ram used by the container, as a string, e.g., "6.2MB"
    swap: swap used by the container, as a string, e.g., "1.0MB"

    For performance reason, such info is not collected for now.

    The attributes available are defined in ATTRIBUTES constant.
    """

    def __init__(self, container_path, attribute_values):
        """Initialize an object of LXC container with given attribute values.

        @param container_path: Directory that stores the container.
        @param attribute_values: A dictionary of attribute values for the
                                 container.
        """
        self.container_path = os.path.realpath(container_path)
        # Path to the rootfs of the container. This will be initialized when
        # property rootfs is retrieved.
        self._rootfs = None
        for attribute, value in attribute_values.iteritems():
            setattr(self, attribute, value)


    def refresh_status(self):
        """Refresh the status information of the container.
        """
        containers = get_container_info(self.container_path, name=self.name)
        if not containers:
            raise error.ContainerError(
                    'No container found in directory %s with name of %s.' %
                    self.container_path, self.name)
        attribute_values = containers[0]
        for attribute, value in attribute_values.iteritems():
            setattr(self, attribute, value)


    @property
    def rootfs(self):
        """Path to the rootfs of the container.

        This property returns the path to the rootfs of the container, that is,
        the folder where the container stores its local files. It reads the
        attribute lxc.rootfs from the config file of the container, e.g.,
            lxc.rootfs = /usr/local/autotest/containers/t4/rootfs
        If the container is created with snapshot, the rootfs is a chain of
        folders, separated by `:` and ordered by how the snapshot is created,
        e.g.,
            lxc.rootfs = overlayfs:/usr/local/autotest/containers/base/rootfs:
            /usr/local/autotest/containers/t4_s/delta0
        This function returns the last folder in the chain, in above example,
        that is `/usr/local/autotest/containers/t4_s/delta0`

        Files in the rootfs will be accessible directly within container. For
        example, a folder in host "[rootfs]/usr/local/file1", can be accessed
        inside container by path "/usr/local/file1". Note that symlink in the
        host can not across host/container boundary, instead, directory mount
        should be used, refer to function mount_dir.

        @return: Path to the rootfs of the container.
        """
        if not self._rootfs:
            cmd = ('sudo lxc-info -P %s -n %s -c lxc.rootfs' %
                   (self.container_path, self.name))
            lxc_rootfs_config = utils.run(cmd).stdout.strip()
            match = re.match('lxc.rootfs = (.*)', lxc_rootfs_config)
            if not match:
                raise error.ContainerError(
                        'Failed to locate rootfs for container %s. lxc.rootfs '
                        'in the container config file is %s' %
                        (self.name, lxc_rootfs_config))
            lxc_rootfs = match.group(1)
            self.clone_from_snapshot = ':' in lxc_rootfs
            if self.clone_from_snapshot:
                self._rootfs = lxc_rootfs.split(':')[-1]
            else:
                self._rootfs = lxc_rootfs
        return self._rootfs


    def attach_run(self, command, bash=True):
        """Attach to a given container and run the given command.

        @param command: Command to run in the container.
        @param bash: Run the command through bash -c "command". This allows
                     pipes to be used in command. Default is set to True.

        @return: The output of the command.

        @raise error.CmdError: If container does not exist, or not running.
        """
        cmd = 'sudo lxc-attach -P %s -n %s' % (self.container_path, self.name)
        if bash and not command.startswith('bash -c'):
            command = 'bash -c "%s"' % utils.sh_escape(command)
        cmd += ' -- %s' % command
        # TODO(dshi): crbug.com/459344 Set sudo to default to False when test
        # container can be unprivileged container.
        return utils.run(cmd)


    def is_network_up(self):
        """Check if network is up in the container by curl base container url.

        @return: True if the network is up, otherwise False.
        """
        try:
            self.attach_run('curl --head %s' % CONTAINER_BASE_URL)
            return True
        except error.CmdError as e:
            logging.debug(e)
            return False


    @timer.decorate
    def start(self, wait_for_network=True):
        """Start the container.

        @param wait_for_network: True to wait for network to be up. Default is
                                 set to True.

        @raise ContainerError: If container does not exist, or fails to start.
        """
        cmd = 'sudo lxc-start -P %s -n %s -d' % (self.container_path, self.name)
        output = utils.run(cmd).stdout
        self.refresh_status()
        if self.state != 'RUNNING':
            raise error.ContainerError(
                    'Container %s failed to start. lxc command output:\n%s' %
                    (os.path.join(self.container_path, self.name),
                     output))

        if wait_for_network:
            logging.debug('Wait for network to be up.')
            start_time = time.time()
            utils.poll_for_condition(condition=self.is_network_up,
                                     timeout=NETWORK_INIT_TIMEOUT,
                                     sleep_interval=NETWORK_INIT_CHECK_INTERVAL)
            logging.debug('Network is up after %.2f seconds.',
                          time.time() - start_time)


    @timer.decorate
    def stop(self):
        """Stop the container.

        @raise ContainerError: If container does not exist, or fails to start.
        """
        cmd = 'sudo lxc-stop -P %s -n %s' % (self.container_path, self.name)
        output = utils.run(cmd).stdout
        self.refresh_status()
        if self.state != 'STOPPED':
            raise error.ContainerError(
                    'Container %s failed to be stopped. lxc command output:\n'
                    '%s' % (os.path.join(self.container_path, self.name),
                            output))


    @timer.decorate
    def destroy(self, force=True):
        """Destroy the container.

        @param force: Set to True to force to destroy the container even if it's
                      running. This is faster than stop a container first then
                      try to destroy it. Default is set to True.

        @raise ContainerError: If container does not exist or failed to destroy
                               the container.
        """
        cmd = 'sudo lxc-destroy -P %s -n %s' % (self.container_path,
                                                self.name)
        if force:
            cmd += ' -f'
        utils.run(cmd)


    def mount_dir(self, source, destination, readonly=False):
        """Mount a directory in host to a directory in the container.

        @param source: Directory in host to be mounted.
        @param destination: Directory in container to mount the source directory
        @param readonly: Set to True to make a readonly mount, default is False.
        """
        # Destination path in container must be relative.
        destination = destination.lstrip('/')
        # Create directory in container for mount.
        utils.run('sudo mkdir -p %s' % os.path.join(self.rootfs, destination))
        config_file = os.path.join(self.container_path, self.name, 'config')
        mount = MOUNT_FMT % {'source': source,
                             'destination': destination,
                             'readonly': ',ro' if readonly else ''}
        utils.run(APPEND_CMD_FMT % {'content': mount, 'file': config_file})


    def verify_autotest_setup(self, job_id):
        """Verify autotest code is set up properly in the container.

        @param job_id: ID of the job, used to format job result folder.

        @raise ContainerError: If autotest code is not set up properly.
        """
        # Test autotest code is setup by verifying a list of
        # (directory, minimum file count)
        if IS_MOBLAB:
            site_packages_path = MOBLAB_SITE_PACKAGES_CONTAINER
        else:
            site_packages_path = os.path.join(lxc_config.CONTAINER_AUTOTEST_DIR,
                                              'site-packages')
        directories_to_check = [
                (lxc_config.CONTAINER_AUTOTEST_DIR, 3),
                (RESULT_DIR_FMT % job_id, 0),
                (site_packages_path, 3)]
        for directory, count in directories_to_check:
            result = self.attach_run(command=(COUNT_FILE_CMD %
                                              {'dir': directory})).stdout
            logging.debug('%s entries in %s.', int(result), directory)
            if int(result) < count:
                raise error.ContainerError('%s is not properly set up.' %
                                           directory)


    def modify_import_order(self):
        """Swap the python import order of lib and local/lib.

        In Moblab, the host's python modules located in
        /usr/lib64/python2.7/site-packages is mounted to following folder inside
        container: /usr/local/lib/python2.7/dist-packages/. The modules include
        an old version of requests module, which is used in autotest
        site-packages. For test, the module is only used in
        dev_server/symbolicate_dump for requests.call and requests.codes.OK.
        When pip is installed inside the container, it installs requests module
        with version of 2.2.1 in /usr/lib/python2.7/dist-packages/. The version
        is newer than the one used in autotest site-packages, but not the latest
        either.
        According to /usr/lib/python2.7/site.py, modules in /usr/local/lib are
        imported before the ones in /usr/lib. That leads to pip to use the older
        version of requests (0.11.2), and it will fail. On the other hand,
        requests module 2.2.1 can't be installed in CrOS (refer to CL:265759),
        and higher version of requests module can't work with pip.
        The only fix to resolve this is to switch the import order, so modules
        in /usr/lib can be imported before /usr/local/lib.
        """
        site_module = '/usr/lib/python2.7/site.py'
        self.attach_run("sed -i ':a;N;$!ba;s/\"local\/lib\",\\n/"
                        "\"lib_placeholder\",\\n/g' %s" % site_module)
        self.attach_run("sed -i ':a;N;$!ba;s/\"lib\",\\n/"
                        "\"local\/lib\",\\n/g' %s" % site_module)
        self.attach_run('sed -i "s/lib_placeholder/lib/g" %s' %
                        site_module)



class ContainerBucket(object):
    """A wrapper class to interact with containers in a specific container path.
    """

    def __init__(self, container_path=DEFAULT_CONTAINER_PATH):
        """Initialize a ContainerBucket.

        @param container_path: Path to the directory used to store containers.
                               Default is set to AUTOSERV/container_path in
                               global config.
        """
        self.container_path = os.path.realpath(container_path)


    def get_all(self):
        """Get details of all containers.

        @return: A dictionary of all containers with detailed attributes,
                 indexed by container name.
        """
        info_collection = get_container_info(self.container_path)
        containers = {}
        for info in info_collection:
            container = Container(self.container_path, info)
            containers[container.name] = container
        return containers


    def get(self, name):
        """Get a container with matching name.

        @param name: Name of the container.

        @return: A container object with matching name. Returns None if no
                 container matches the given name.
        """
        return self.get_all().get(name, None)


    def exist(self, name):
        """Check if a container exists with the given name.

        @param name: Name of the container.

        @return: True if the container with the given name exists, otherwise
                 returns False.
        """
        return self.get(name) != None


    def destroy_all(self):
        """Destroy all containers, base must be destroyed at the last.
        """
        containers = self.get_all().values()
        for container in sorted(containers,
                                key=lambda n: 1 if n.name == BASE else 0):
            logging.info('Destroy container %s.', container.name)
            container.destroy()


    @timer.decorate
    def create_from_base(self, name, disable_snapshot_clone=False,
                         force_cleanup=False):
        """Create a container from the base container.

        @param name: Name of the container.
        @param disable_snapshot_clone: Set to True to force to clone without
                using snapshot clone even if the host supports that.
        @param force_cleanup: Force to cleanup existing container.

        @return: A Container object for the created container.

        @raise ContainerError: If the container already exist.
        @raise error.CmdError: If lxc-clone call failed for any reason.
        """
        if self.exist(name) and not force_cleanup:
            raise error.ContainerError('Container %s already exists.' % name)

        # Cleanup existing container with the given name.
        container_folder = os.path.join(self.container_path, name)
        if lxc_utils.path_exists(container_folder) and force_cleanup:
            container = Container(self.container_path, {'name': name})
            try:
                container.destroy()
            except error.CmdError as e:
                # The container could be created in a incompleted state. Delete
                # the container folder instead.
                logging.warn('Failed to destroy container %s, error: %s',
                             name, e)
                utils.run('sudo rm -rf "%s"' % container_folder)

        use_snapshot = SUPPORT_SNAPSHOT_CLONE and not disable_snapshot_clone
        snapshot = '-s' if  use_snapshot else ''
        # overlayfs is the default clone backend storage. However it is not
        # supported in Ganeti yet. Use aufs as the alternative.
        aufs = '-B aufs' if utils.is_vm() and use_snapshot else ''
        cmd = ('sudo lxc-clone -p %s -P %s %s' %
               (self.container_path, self.container_path,
                ' '.join([BASE, name, snapshot, aufs])))
        try:
            utils.run(cmd)
            return self.get(name)
        except error.CmdError:
            if not use_snapshot:
                raise
            else:
                # Snapshot clone failed, retry clone without snapshot. The retry
                # won't hit the code here and cause an infinite loop as
                # disable_snapshot_clone is set to True.
                container = self.create_from_base(
                        name, disable_snapshot_clone=True, force_cleanup=True)
                # Report metadata about retry success.
                autotest_es.post(use_http=True,
                                 type_str=CONTAINER_CREATE_RETRY_METADB_TYPE,
                                 metadata={'drone': socket.gethostname(),
                                           'name': name,
                                           'success': True})
                return container


    @cleanup_if_fail()
    def setup_base(self, name=BASE, force_delete=False):
        """Setup base container.

        @param name: Name of the base container, default to base.
        @param force_delete: True to force to delete existing base container.
                             This action will destroy all running test
                             containers. Default is set to False.
        """
        if not self.container_path:
            raise error.ContainerError(
                    'You must set a valid directory to store containers in '
                    'global config "AUTOSERV/ container_path".')

        if not os.path.exists(self.container_path):
            os.makedirs(self.container_path)

        base_path = os.path.join(self.container_path, name)
        if self.exist(name) and not force_delete:
            logging.error(
                    'Base container already exists. Set force_delete to True '
                    'to force to re-stage base container. Note that this '
                    'action will destroy all running test containers')
            # Set proper file permission. base container in moblab may have
            # owner of not being root. Force to update the folder's owner.
            # TODO(dshi): Change root to current user when test container can be
            # unprivileged container.
            utils.run('sudo chown -R root "%s"' % base_path)
            utils.run('sudo chgrp -R root "%s"' % base_path)
            return

        # Destroy existing base container if exists.
        if self.exist(name):
            # TODO: We may need to destroy all snapshots created from this base
            # container, not all container.
            self.destroy_all()

        # Download and untar the base container.
        tar_path = os.path.join(self.container_path, '%s.tar.xz' % name)
        path_to_cleanup = [tar_path, base_path]
        for path in path_to_cleanup:
            if os.path.exists(path):
                utils.run('sudo rm -rf "%s"' % path)
        download_extract(CONTAINER_BASE_URL, tar_path, self.container_path)
        # Remove the downloaded container tar file.
        utils.run('sudo rm "%s"' % tar_path)
        # Set proper file permission.
        # TODO(dshi): Change root to current user when test container can be
        # unprivileged container.
        utils.run('sudo chown -R root "%s"' % base_path)
        utils.run('sudo chgrp -R root "%s"' % base_path)

        # Update container config with container_path from global config.
        config_path = os.path.join(base_path, 'config')
        utils.run('sudo sed -i "s|container_dir|%s|g" "%s"' %
                  (self.container_path, config_path))


    @timer.decorate
    @cleanup_if_fail()
    def setup_test(self, name, job_id, server_package_url, result_path,
                   control=None, skip_cleanup=False):
        """Setup test container for the test job to run.

        The setup includes:
        1. Install autotest_server package from given url.
        2. Copy over local shadow_config.ini.
        3. Mount local site-packages.
        4. Mount test result directory.

        TODO(dshi): Setup also needs to include test control file for autoserv
                    to run in container.

        @param name: Name of the container.
        @param job_id: Job id for the test job to run in the test container.
        @param server_package_url: Url to download autotest_server package.
        @param result_path: Directory to be mounted to container to store test
                            results.
        @param control: Path to the control file to run the test job. Default is
                        set to None.
        @param skip_cleanup: Set to True to skip cleanup, used to troubleshoot
                             container failures.

        @return: A Container object for the test container.

        @raise ContainerError: If container does not exist, or not running.
        """
        start_time = time.time()

        if not os.path.exists(result_path):
            raise error.ContainerError('Result directory does not exist: %s',
                                       result_path)
        result_path = os.path.abspath(result_path)

        # Create test container from the base container.
        container = self.create_from_base(name)

        # Deploy server side package
        usr_local_path = os.path.join(container.rootfs, 'usr', 'local')
        autotest_pkg_path = os.path.join(usr_local_path,
                                         'autotest_server_package.tar.bz2')
        autotest_path = os.path.join(usr_local_path, 'autotest')
        # sudo is required so os.makedirs may not work.
        utils.run('sudo mkdir -p %s'% usr_local_path)

        download_extract(server_package_url, autotest_pkg_path, usr_local_path)
        deploy_config_manager = lxc_config.DeployConfigManager(container)
        deploy_config_manager.deploy_pre_start()

        # Copy over control file to run the test job.
        if control:
            container_drone_temp = os.path.join(autotest_path, 'drone_tmp')
            utils.run('sudo mkdir -p %s'% container_drone_temp)
            container_control_file = os.path.join(
                    container_drone_temp, os.path.basename(control))
            utils.run('sudo cp %s %s' % (control, container_control_file))

        if IS_MOBLAB:
            site_packages_path = MOBLAB_SITE_PACKAGES
            site_packages_container_path = MOBLAB_SITE_PACKAGES_CONTAINER[1:]
        else:
            site_packages_path = os.path.join(common.autotest_dir,
                                              'site-packages')
            site_packages_container_path = os.path.join(
                    lxc_config.CONTAINER_AUTOTEST_DIR, 'site-packages')
        mount_entries = [(site_packages_path, site_packages_container_path,
                          True),
                         (os.path.join(common.autotest_dir, 'puppylab'),
                          os.path.join(lxc_config.CONTAINER_AUTOTEST_DIR,
                                       'puppylab'),
                          True),
                         (result_path,
                          os.path.join(RESULT_DIR_FMT % job_id),
                          False),
                        ]
        # Update container config to mount directories.
        for source, destination, readonly in mount_entries:
            container.mount_dir(source, destination, readonly)

        # Update file permissions.
        # TODO(dshi): crbug.com/459344 Skip following action when test container
        # can be unprivileged container.
        utils.run('sudo chown -R root "%s"' % autotest_path)
        utils.run('sudo chgrp -R root "%s"' % autotest_path)

        container.start(name)
        deploy_config_manager.deploy_post_start()

        container.modify_import_order()

        container.verify_autotest_setup(job_id)

        autotest_es.post(use_http=True,
                         type_str=CONTAINER_CREATE_METADB_TYPE,
                         metadata={'drone': socket.gethostname(),
                                   'job_id': job_id,
                                   'time_used': time.time() - start_time,
                                   'success': True})

        logging.debug('Test container %s is set up.', name)
        return container


def parse_options():
    """Parse command line inputs.

    @raise argparse.ArgumentError: If command line arguments are invalid.
    """
    parser = argparse.ArgumentParser()
    parser.add_argument('-s', '--setup', action='store_true',
                        default=False,
                        help='Set up base container.')
    parser.add_argument('-p', '--path', type=str,
                        help='Directory to store the container.',
                        default=DEFAULT_CONTAINER_PATH)
    parser.add_argument('-f', '--force_delete', action='store_true',
                        default=False,
                        help=('Force to delete existing containers and rebuild '
                              'base containers.'))
    options = parser.parse_args()
    if not options.setup and not options.force_delete:
        raise argparse.ArgumentError(
                'Use --setup to setup a base container, or --force_delete to '
                'delete all containers in given path.')
    return options


def main():
    """main script."""
    # Force to run the setup as superuser.
    # TODO(dshi): crbug.com/459344 Set remove this enforcement when test
    # container can be unprivileged container.
    if utils.sudo_require_password():
        logging.warn('SSP requires root privilege to run commands, please '
                     'grant root access to this process.')
        utils.run('sudo true')

    options = parse_options()
    bucket = ContainerBucket(container_path=options.path)
    if options.setup:
        bucket.setup_base(force_delete=options.force_delete)
    elif options.force_delete:
        bucket.destroy_all()


if __name__ == '__main__':
    main()
