# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from distutils import version
import cStringIO
import HTMLParser
import httplib
import json
import logging
import multiprocessing
import os
import re
import sys
import urllib2

from autotest_lib.client.bin import utils as site_utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib import global_config
from autotest_lib.client.common_lib import utils
from autotest_lib.client.common_lib.cros import retry
from autotest_lib.client.common_lib.cros.graphite import autotest_stats
# TODO(cmasone): redo this class using requests module; http://crosbug.com/30107


CONFIG = global_config.global_config
# This file is generated at build time and specifies, per suite and per test,
# the DEPENDENCIES list specified in each control file.  It's a dict of dicts:
# {'bvt':   {'/path/to/autotest/control/site_tests/test1/control': ['dep1']}
#  'suite': {'/path/to/autotest/control/site_tests/test2/control': ['dep2']}
#  'power': {'/path/to/autotest/control/site_tests/test1/control': ['dep1'],
#            '/path/to/autotest/control/site_tests/test3/control': ['dep3']}
# }
DEPENDENCIES_FILE = 'test_suites/dependency_info'
# Number of seconds for caller to poll devserver's is_staged call to check if
# artifacts are staged.
_ARTIFACT_STAGE_POLLING_INTERVAL = 5
# Artifacts that should be staged when client calls devserver RPC to stage an
# image.
_ARTIFACTS_TO_BE_STAGED_FOR_IMAGE = 'full_payload,test_suites,stateful'
# Artifacts that should be staged when client calls devserver RPC to stage an
# image with autotest artifact.
_ARTIFACTS_TO_BE_STAGED_FOR_IMAGE_WITH_AUTOTEST = ('full_payload,test_suites,'
                                                   'control_files,stateful,'
                                                   'autotest_packages')
# Artifacts that should be staged when client calls devserver RPC to stage an
# Android build.
_ANDROID_ARTIFACTS_TO_BE_STAGED_FOR_IMAGE = ('bootloader_image,radio_image,'
                                             'zip_images,test_zip')
# Artifacts that should be staged when client calls devserver RPC to stage an
# Android build.
_BRILLO_ARTIFACTS_TO_BE_STAGED_FOR_IMAGE = ('zip_images,vendor_partitions')
SKIP_DEVSERVER_HEALTH_CHECK = CONFIG.get_config_value(
        'CROS', 'skip_devserver_health_check', type=bool)
# Number of seconds for the call to get devserver load to time out.
TIMEOUT_GET_DEVSERVER_LOAD = 2.0

# Android artifact path in devserver
ANDROID_BUILD_NAME_PATTERN = CONFIG.get_config_value(
        'CROS', 'android_build_name_pattern', type=str).replace('\\', '')

# Return value from a devserver RPC indicating the call succeeded.
SUCCESS = 'Success'

PREFER_LOCAL_DEVSERVER = CONFIG.get_config_value(
        'CROS', 'prefer_local_devserver', type=bool, default=False)

ENABLE_DEVSERVER_IN_RESTRICTED_SUBNET = CONFIG.get_config_value(
        'CROS', 'enable_devserver_in_restricted_subnet', type=bool,
        default=False)

class MarkupStripper(HTMLParser.HTMLParser):
    """HTML parser that strips HTML tags, coded characters like &amp;

    Works by, basically, not doing anything for any tags, and only recording
    the content of text nodes in an internal data structure.
    """
    def __init__(self):
        self.reset()
        self.fed = []


    def handle_data(self, d):
        """Consume content of text nodes, store it away."""
        self.fed.append(d)


    def get_data(self):
        """Concatenate and return all stored data."""
        return ''.join(self.fed)


def _get_image_storage_server():
    return CONFIG.get_config_value('CROS', 'image_storage_server', type=str)


def _get_canary_channel_server():
    """
    Get the url of the canary-channel server,
    eg: gsutil://chromeos-releases/canary-channel/<board>/<release>

    @return: The url to the canary channel server.
    """
    return CONFIG.get_config_value('CROS', 'canary_channel_server', type=str)


def _get_storage_server_for_artifacts(artifacts=None):
    """Gets the appropriate storage server for the given artifacts.

    @param artifacts: A list of artifacts we need to stage.
    @return: The address of the storage server that has these artifacts.
             The default image storage server if no artifacts are specified.
    """
    factory_artifact = global_config.global_config.get_config_value(
            'CROS', 'factory_artifact', type=str, default='')
    if artifacts and factory_artifact and factory_artifact in artifacts:
        return _get_canary_channel_server()
    return _get_image_storage_server()


def _get_dev_server_list():
    return CONFIG.get_config_value('CROS', 'dev_server', type=list, default=[])


def _get_crash_server_list():
    return CONFIG.get_config_value('CROS', 'crash_server', type=list,
        default=[])


def remote_devserver_call(timeout_min=30):
    """A decorator to use with remote devserver calls.

    This decorator converts urllib2.HTTPErrors into DevServerExceptions with
    any embedded error info converted into plain text.
    The method retries on urllib2.URLError to avoid devserver flakiness.
    """
    #pylint: disable=C0111
    def inner_decorator(method):

        @retry.retry(urllib2.URLError, timeout_min=timeout_min)
        def wrapper(*args, **kwargs):
            """This wrapper actually catches the HTTPError."""
            try:
                return method(*args, **kwargs)
            except urllib2.HTTPError as e:
                error_markup = e.read()
                strip = MarkupStripper()
                try:
                    strip.feed(error_markup.decode('utf_32'))
                except UnicodeDecodeError:
                    strip.feed(error_markup)
                raise DevServerException(strip.get_data())

        return wrapper

    return inner_decorator


class DevServerException(Exception):
    """Raised when the dev server returns a non-200 HTTP response."""
    pass


class DevServer(object):
    """Base class for all DevServer-like server stubs.

    This is the base class for interacting with all Dev Server-like servers.
    A caller should instantiate a sub-class of DevServer with:

    host = SubClassServer.resolve(build)
    server = SubClassServer(host)
    """
    _MIN_FREE_DISK_SPACE_GB = 20
    # Threshold for the CPU load percentage for a devserver to be selected.
    MAX_CPU_LOAD = 80.0
    # Threshold for the network IO, set to 80MB/s
    MAX_NETWORK_IO = 1024 * 1024 * 80
    DISK_IO = 'disk_total_bytes_per_second'
    NETWORK_IO = 'network_total_bytes_per_second'
    CPU_LOAD = 'cpu_percent'
    FREE_DISK = 'free_disk'
    STAGING_THREAD_COUNT = 'staging_thread_count'


    def __init__(self, devserver):
        self._devserver = devserver


    def url(self):
        """Returns the url for this devserver."""
        return self._devserver


    @staticmethod
    def get_server_name(url):
        """Strip the http:// prefix and port from a url.

        @param url: A url of a server.

        @return the server name without http:// prefix and port.

        """
        return re.sub(r':\d+$', '', url.lstrip('http://'))


    @staticmethod
    def get_devserver_load_wrapper(devserver, timeout_sec, output):
        """A wrapper function to call get_devserver_load in parallel.

        @param devserver: url of the devserver.
        @param timeout_sec: Number of seconds before time out the devserver
                            call.
        @param output: An output queue to save results to.
        """
        load = DevServer.get_devserver_load(devserver,
                                            timeout_min=timeout_sec/60.0)
        if load:
            load['devserver'] = devserver
        output.put(load)


    @staticmethod
    def get_devserver_load(devserver, timeout_min=0.1):
        """Returns True if the |devserver| is healthy to stage build.

        @param devserver: url of the devserver.
        @param timeout_min: How long to wait in minutes before deciding the
                            the devserver is not up (float).

        @return: A dictionary of the devserver's load.

        """
        server_name = DevServer.get_server_name(devserver)
        # statsd treats |.| as path separator.
        server_name = server_name.replace('.', '_')
        call = DevServer._build_call(devserver, 'check_health')

        @remote_devserver_call(timeout_min=timeout_min)
        def make_call():
            """Inner method that makes the call."""
            return utils.urlopen_socket_timeout(
                    call, timeout=timeout_min * 60).read()

        try:
            result_dict = json.load(cStringIO.StringIO(make_call()))
            for key, val in result_dict.iteritems():
                try:
                    autotest_stats.Gauge(server_name).send(key, float(val))
                except ValueError:
                    # Ignore all non-numerical health data.
                    pass

            return result_dict
        except Exception as e:
            logging.error('Devserver call failed: "%s", timeout: %s seconds,'
                          ' Error: %s', call, timeout_min * 60, e)


    @staticmethod
    def is_free_disk_ok(load):
        """Check if a devserver has enough free disk.

        @param load: A dict of the load of the devserver.

        @return: True if the devserver has enough free disk or disk check is
                 skipped in global config.

        """
        if SKIP_DEVSERVER_HEALTH_CHECK:
            logging.debug('devserver health check is skipped.')
        elif load[DevServer.FREE_DISK] < DevServer._MIN_FREE_DISK_SPACE_GB:
            return False

        return True


    @staticmethod
    def devserver_healthy(devserver, timeout_min=0.1):
        """Returns True if the |devserver| is healthy to stage build.

        @param devserver: url of the devserver.
        @param timeout_min: How long to wait in minutes before deciding the
                            the devserver is not up (float).

        @return: True if devserver is healthy. Return False otherwise.

        """
        server_name = DevServer.get_server_name(devserver)
        # statsd treats |.| as path separator.
        server_name = server_name.replace('.', '_')
        load = DevServer.get_devserver_load(devserver, timeout_min=timeout_min)
        if not load:
            # Failed to get the load of devserver.
            autotest_stats.Counter(server_name +
                                   '.devserver_not_healthy').increment()
            return False

        disk_ok = DevServer.is_free_disk_ok(load)
        if not disk_ok:
            logging.error('Devserver check_health failed. Free disk space is '
                          'low. Only %dGB is available.',
                          load[DevServer.FREE_DISK])
        counter = '.devserver_healthy' if disk_ok else '.devserver_not_healthy'
        # This counter indicates the load of a devserver. By comparing the
        # value of this counter for all devservers, we can evaluate the
        # load balancing across all devservers.
        autotest_stats.Counter(server_name + counter).increment()
        return disk_ok


    @staticmethod
    def _build_call(host, method, **kwargs):
        """Build a URL to |host| that calls |method|, passing |kwargs|.

        Builds a URL that calls |method| on the dev server defined by |host|,
        passing a set of key/value pairs built from the dict |kwargs|.

        @param host: a string that is the host basename e.g. http://server:90.
        @param method: the dev server method to call.
        @param kwargs: a dict mapping arg names to arg values.
        @return the URL string.
        """
        argstr = '&'.join(map(lambda x: "%s=%s" % x, kwargs.iteritems()))
        return "%(host)s/%(method)s?%(argstr)s" % dict(
                host=host, method=method, argstr=argstr)


    def build_call(self, method, **kwargs):
        """Builds a devserver RPC string that can be invoked using urllib.open.

        @param method: remote devserver method to call.
        """
        return self._build_call(self._devserver, method, **kwargs)


    @classmethod
    def build_all_calls(cls, method, **kwargs):
        """Builds a list of URLs that makes RPC calls on all devservers.

        Build a URL that calls |method| on the dev server, passing a set
        of key/value pairs built from the dict |kwargs|.

        @param method: the dev server method to call.
        @param kwargs: a dict mapping arg names to arg values
        @return the URL string
        """
        calls = []
        # Note we use cls.servers as servers is class specific.
        for server in cls.servers():
            if cls.devserver_healthy(server):
                calls.append(cls._build_call(server, method, **kwargs))

        return calls


    @staticmethod
    def servers():
        """Returns a list of servers that can serve as this type of server."""
        raise NotImplementedError()


    @classmethod
    def get_devservers_in_same_subnet(cls, ip, mask_bits=19):
        """Get the devservers in the same subnet of the given ip.

        @param ip: The IP address of a dut to look for devserver.
        @param mask_bits: Number of mask bits. Default is 19.

        @return: A list of devservers in the same subnet of the given ip.

        """
        # server from cls.servers() is a URL, e.g., http://10.1.1.10:8082, so
        # we need a dict to return the full devserver path once the IPs are
        # filtered in get_servers_in_same_subnet.
        server_names = {}
        all_devservers = []
        for server in cls.servers():
            server_name = ImageServer.get_server_name(server)
            server_names[server_name] = server
            all_devservers.append(server_name)
        devservers = utils.get_servers_in_same_subnet(ip, mask_bits,
                                                      all_devservers)
        return [server_names[s] for s in devservers]


    @classmethod
    def get_unrestricted_devservers(
                cls, restricted_subnet=utils.RESTRICTED_SUBNETS):
        """Get the devservers not in any restricted subnet specified in
        restricted_subnet.

        @param restricted_subnet: A list of restriected subnets.

        @return: A list of devservers not in any restricted subnet.

        """
        devservers = []
        for server in cls.servers():
            server_name = ImageServer.get_server_name(server)
            if not utils.get_restricted_subnet(server_name, restricted_subnet):
                devservers.append(server)
        return devservers


    @classmethod
    def get_healthy_devserver(cls, build, devservers):
        """"Get a healthy devserver instance from the list of devservers.

        @param build: The build (e.g. x86-mario-release/R18-1586.0.0-a1-b1514).

        @return: A DevServer object of a healthy devserver. Return None if no
                 healthy devserver is found.

        """
        while devservers:
            hash_index = hash(build) % len(devservers)
            devserver = devservers.pop(hash_index)
            if cls.devserver_healthy(devserver):
                return cls(devserver)


    @classmethod
    def resolve(cls, build, hostname=None):
        """"Resolves a build to a devserver instance.

        @param build: The build (e.g. x86-mario-release/R18-1586.0.0-a1-b1514).
        @param hostname: The hostname of dut that requests a devserver. It's
                         used to make sure a devserver in the same subnet is
                         preferred.

        @raise DevServerException: If no devserver is available.
        """
        host_ip = None
        if hostname:
            host_ip = site_utils.get_ip_address(hostname)
            if not host_ip:
                logging.error('Failed to get IP address of %s. Will pick a '
                              'devserver without subnet constraint.', hostname)

        devservers = cls.servers()

        # Go through all restricted subnet settings and check if the DUT is
        # inside a restricted subnet. If so, get the subnet setting.
        restricted_subnet = None
        if host_ip and ENABLE_DEVSERVER_IN_RESTRICTED_SUBNET:
            for subnet_ip, mask_bits in utils.RESTRICTED_SUBNETS:
                if utils.is_in_same_subnet(host_ip, subnet_ip, mask_bits):
                    restricted_subnet = subnet_ip
                    logging.debug('The host %s (%s) is in a restricted subnet. '
                                  'Try to locate a devserver inside subnet '
                                  '%s:%d.', hostname, host_ip, subnet_ip,
                                  mask_bits)
                    devservers = cls.get_devservers_in_same_subnet(
                            subnet_ip, mask_bits)
                    break
        # If devserver election is not restricted and
        # enable_devserver_in_restricted_subnet in global config is set to True,
        # select a devserver from unrestricted servers. Otherwise, drone will
        # not be able to access devserver in restricted subnet.
        can_retry = False
        if (not restricted_subnet and utils.RESTRICTED_SUBNETS and
            ENABLE_DEVSERVER_IN_RESTRICTED_SUBNET):
            devservers = cls.get_unrestricted_devservers()
            if PREFER_LOCAL_DEVSERVER and host_ip:
                can_retry = True
                devservers = cls.get_devserver_in_same_subnet(
                        host_ip, cls.get_unrestricted_devservers() )
        devserver = cls.get_healthy_devserver(build, devservers)

        if not devserver and can_retry:
            devserver = cls.get_healthy_devserver(
                    build, cls.get_unrestricted_devservers())
        if devserver:
            return devserver
        else:
            if restricted_subnet:
                subnet_error = ('in the same subnet as the host %s (%s)' %
                                (hostname, host_ip))
            else:
                subnet_error = ''
            error_msg = 'All devservers %s are currently down!!!' % subnet_error
            logging.error(error_msg)
            raise DevServerException(error_msg)


class CrashServer(DevServer):
    """Class of DevServer that symbolicates crash dumps."""
    @staticmethod
    def servers():
        return _get_crash_server_list()


    @remote_devserver_call()
    def symbolicate_dump(self, minidump_path, build):
        """Ask the devserver to symbolicate the dump at minidump_path.

        Stage the debug symbols for |build| and, if that works, ask the
        devserver to symbolicate the dump at |minidump_path|.

        @param minidump_path: the on-disk path of the minidump.
        @param build: The build (e.g. x86-mario-release/R18-1586.0.0-a1-b1514)
                      whose debug symbols are needed for symbolication.
        @return The contents of the stack trace
        @raise DevServerException upon any return code that's not HTTP OK.
        """
        try:
            import requests
        except ImportError:
            logging.warning("Can't 'import requests' to connect to dev server.")
            return ''
        server_name = self.get_server_name(self.url())
        server_name = server_name.replace('.', '_')
        stats_key = 'CrashServer.%s.symbolicate_dump' % server_name
        autotest_stats.Counter(stats_key).increment()
        timer = autotest_stats.Timer(stats_key)
        timer.start()
        # Symbolicate minidump.
        call = self.build_call('symbolicate_dump',
                               archive_url=_get_image_storage_server() + build)
        request = requests.post(
                call, files={'minidump': open(minidump_path, 'rb')})
        if request.status_code == requests.codes.OK:
            timer.stop()
            return request.text

        error_fd = cStringIO.StringIO(request.text)
        raise urllib2.HTTPError(
                call, request.status_code, request.text, request.headers,
                error_fd)


class ImageServerBase(DevServer):
    """Base class for devservers used to stage builds.

    CrOS and Android builds are staged in different ways as they have different
    sets of artifacts. This base class abstracts the shared functions between
    the two types of ImageServer.
    """

    @classmethod
    def servers(cls):
        """Returns a list of servers that can serve as a desired type of
        devserver.
        """
        return _get_dev_server_list()


    def _get_image_url(self, image):
        """Returns the url of the directory for this image on the devserver.

        @param image: the image that was fetched.
        """
        image = self.translate(image)
        url_pattern = CONFIG.get_config_value('CROS', 'image_url_pattern',
                                              type=str)
        return (url_pattern % (self.url(), image)).replace('update', 'static')


    @staticmethod
    def create_stats_str(subname, server_name, artifacts):
        """Create a graphite name given the staged items.

        The resulting name will look like
            'dev_server.subname.DEVSERVER_URL.artifact1_artifact2'
        The name can be used to create a stats object like
        stats.Timer, stats.Counter, etc.

        @param subname: A name for the graphite sub path.
        @param server_name: name of the devserver, e.g 172.22.33.44.
        @param artifacts: A list of artifacts.

        @return A name described above.

        """
        staged_items = sorted(artifacts) if artifacts else []
        staged_items_str = '_'.join(staged_items).replace(
                '.', '_') if staged_items else None
        server_name = server_name.replace('.', '_')
        stats_str = 'dev_server.%s.%s' % (subname, server_name)
        if staged_items_str:
            stats_str += '.%s' % staged_items_str
        return stats_str


    @staticmethod
    def create_metadata(server_name, image, artifacts=None, files=None):
        """Create a metadata dictionary given the staged items.

        The metadata can be send to metadata db along with stats.

        @param server_name: name of the devserver, e.g 172.22.33.44.
        @param image: The name of the image.
        @param artifacts: A list of artifacts.
        @param files: A list of files.

        @return A metadata dictionary.

        """
        metadata = {'devserver': server_name,
                    'image': image,
                    '_type': 'devserver'}
        if artifacts:
            metadata['artifacts'] = ' '.join(artifacts)
        if files:
            metadata['files'] = ' '.join(files)
        return metadata


    def _poll_is_staged(self, **kwargs):
        """Polling devserver.is_staged until all artifacts are staged.

        @param kwargs: keyword arguments to make is_staged devserver call.

        @return: True if all artifacts are staged in devserver.
        """
        call = self.build_call('is_staged', **kwargs)

        def all_staged():
            """Call devserver.is_staged rpc to check if all files are staged.

            @return: True if all artifacts are staged in devserver. False
                     otherwise.
            @rasies DevServerException, the exception is a wrapper of all
                    exceptions that were raised when devserver tried to download
                    the artifacts. devserver raises an HTTPError when an
                    exception was raised in the code. Such exception should be
                    re-raised here to stop the caller from waiting. If the call
                    to devserver failed for connection issue, a URLError
                    exception is raised, and caller should retry the call to
                    avoid such network flakiness.

            """
            try:
                return urllib2.urlopen(call).read() == 'True'
            except urllib2.HTTPError as e:
                error_markup = e.read()
                strip = MarkupStripper()
                try:
                    strip.feed(error_markup.decode('utf_32'))
                except UnicodeDecodeError:
                    strip.feed(error_markup)
                raise DevServerException(strip.get_data())
            except urllib2.URLError as e:
                # Could be connection issue, retry it.
                # For example: <urlopen error [Errno 111] Connection refused>
                return False

        site_utils.poll_for_condition(
                all_staged,
                exception=site_utils.TimeoutError(),
                timeout=sys.maxint,
                sleep_interval=_ARTIFACT_STAGE_POLLING_INTERVAL)

        return True


    def _call_and_wait(self, call_name, error_message,
                       expected_response=SUCCESS, **kwargs):
        """Helper method to make a urlopen call, and wait for artifacts staged.

        @param call_name: name of devserver rpc call.
        @param error_message: Error message to be thrown if response does not
                              match expected_response.
        @param expected_response: Expected response from rpc, default to
                                  |Success|. If it's set to None, do not compare
                                  the actual response. Any response is consider
                                  to be good.
        @param kwargs: keyword arguments to make is_staged devserver call.

        @return: The response from rpc.
        @raise DevServerException upon any return code that's expected_response.

        """
        call = self.build_call(call_name, async=True, **kwargs)
        try:
            response = urllib2.urlopen(call).read()
        except httplib.BadStatusLine as e:
            logging.error(e)
            raise DevServerException('Received Bad Status line, Devserver %s '
                                     'might have gone down while handling '
                                     'the call: %s' % (self.url(), call))

        if expected_response and not response == expected_response:
                raise DevServerException(error_message)

        # `os_type` is needed in build a devserver call, but not needed for
        # wait_for_artifacts_staged, since that method is implemented by
        # each ImageServerBase child class.
        if 'os_type' in kwargs:
            del kwargs['os_type']
        self.wait_for_artifacts_staged(**kwargs)
        return response


    def _stage_artifacts(self, build, artifacts, files, archive_url, **kwargs):
        """Tell the devserver to download and stage |artifacts| from |image|
        specified by kwargs.

        This is the main call point for staging any specific artifacts for a
        given build. To see the list of artifacts one can stage see:

        ~src/platfrom/dev/artifact_info.py.

        This is maintained along with the actual devserver code.

        @param artifacts: A list of artifacts.
        @param files: A list of files to stage.
        @param archive_url: Optional parameter that has the archive_url to stage
                this artifact from. Default is specified in autotest config +
                image.
        @param kwargs: keyword arguments that specify the build information, to
                make stage devserver call.

        @raise DevServerException upon any return code that's not HTTP OK.
        """
        if not archive_url:
            archive_url = _get_storage_server_for_artifacts(artifacts) + build

        artifacts_arg = ','.join(artifacts) if artifacts else ''
        files_arg = ','.join(files) if files else ''
        error_message = ("staging %s for %s failed;"
                         "HTTP OK not accompanied by 'Success'." %
                         ('artifacts=%s files=%s ' % (artifacts_arg, files_arg),
                          build))

        staging_info = ('build=%s, artifacts=%s, files=%s, archive_url=%s' %
                        (build, artifacts, files, archive_url))
        logging.info('Staging artifacts on devserver %s: %s',
                     self.url(), staging_info)
        if artifacts:
            server_name = self.get_server_name(self.url())
            timer_key = self.create_stats_str(
                    'stage_artifacts', server_name, artifacts)
            counter_key = self.create_stats_str(
                    'stage_artifacts_count', server_name, artifacts)
            metadata = self.create_metadata(server_name, build, artifacts,
                                            files)
            autotest_stats.Counter(counter_key, metadata=metadata).increment()
            timer = autotest_stats.Timer(timer_key, metadata=metadata)
            timer.start()
        try:
            arguments = {'archive_url': archive_url,
                         'artifacts': artifacts_arg,
                         'files': files_arg}
            if kwargs:
                arguments.update(kwargs)
            self.call_and_wait(call_name='stage',error_message=error_message,
                               **arguments)
            if artifacts:
                timer.stop()
            logging.info('Finished staging artifacts: %s', staging_info)
        except error.TimeoutException:
            logging.error('stage_artifacts timed out: %s', staging_info)
            if artifacts:
                timeout_key = self.create_stats_str(
                        'stage_artifacts_timeout', server_name, artifacts)
                autotest_stats.Counter(timeout_key,
                                       metadata=metadata).increment()
            raise DevServerException(
                    'stage_artifacts timed out: %s' % staging_info)


    def call_and_wait(self, *args, **kwargs):
        """Helper method to make a urlopen call, and wait for artifacts staged.

        This method needs to be overridden in the subclass to implement the
        logic to call _call_and_wait.
        """
        raise NotImplementedError


    def _trigger_download(self, build, artifacts, files, synchronous=True,
                          **kwargs_build_info):
        """Tell the devserver to download and stage image specified in
        kwargs_build_info.

        Tells the devserver to fetch |image| from the image storage server
        named by _get_image_storage_server().

        If |synchronous| is True, waits for the entire download to finish
        staging before returning. Otherwise only the artifacts necessary
        to start installing images onto DUT's will be staged before returning.
        A caller can then call finish_download to guarantee the rest of the
        artifacts have finished staging.

        @param synchronous: if True, waits until all components of the image are
               staged before returning.
        @param kwargs_build_info: Dictionary of build information.
                For CrOS, it is None as build is the CrOS image name.
                For Android, it is {'target': target,
                                    'build_id': build_id,
                                    'branch': branch}

        @raise DevServerException upon any return code that's not HTTP OK.

        """
        if kwargs_build_info:
            archive_url = None
        else:
            archive_url = _get_image_storage_server() + build
        error_message = ("trigger_download for %s failed;"
                         "HTTP OK not accompanied by 'Success'." % build)
        kwargs = {'archive_url': archive_url,
                  'artifacts': artifacts,
                  'files': files,
                  'error_message': error_message}
        if kwargs_build_info:
            kwargs.update(kwargs_build_info)

        logging.info('trigger_download starts for %s', build)
        server_name = self.get_server_name(self.url())
        artifacts_list = artifacts.split(',')
        counter_key = self.create_stats_str(
                'trigger_download_count', server_name, artifacts_list)
        metadata = self.create_metadata(server_name, build, artifacts_list)
        autotest_stats.Counter(counter_key, metadata=metadata).increment()
        try:
            response = self.call_and_wait(call_name='stage', **kwargs)
            logging.info('trigger_download finishes for %s', build)
        except error.TimeoutException:
            logging.error('trigger_download timed out for %s.', build)
            timeout_key = self.create_stats_str(
                    'trigger_download_timeout', server_name, artifacts_list)
            autotest_stats.Counter(timeout_key, metadata=metadata).increment()
            raise DevServerException(
                    'trigger_download timed out for %s.' % build)
        was_successful = response == SUCCESS
        if was_successful and synchronous:
            self._finish_download(build, artifacts, files, **kwargs_build_info)


    def _finish_download(self, build, artifacts, files, **kwargs_build_info):
        """Tell the devserver to finish staging image specified in
        kwargs_build_info.

        If trigger_download is called with synchronous=False, it will return
        before all artifacts have been staged. This method contacts the
        devserver and blocks until all staging is completed and should be
        called after a call to trigger_download.

        @param kwargs_build_info: Dictionary of build information.
                For CrOS, it is None as build is the CrOS image name.
                For Android, it is {'target': target,
                                    'build_id': build_id,
                                    'branch': branch}

        @raise DevServerException upon any return code that's not HTTP OK.
        """
        archive_url = _get_image_storage_server() + build
        error_message = ("finish_download for %s failed;"
                         "HTTP OK not accompanied by 'Success'." % build)
        kwargs = {'archive_url': archive_url,
                  'artifacts': artifacts,
                  'files': files,
                  'error_message': error_message}
        if kwargs_build_info:
            kwargs.update(kwargs_build_info)
        try:
            self.call_and_wait(call_name='stage', **kwargs)
        except error.TimeoutException:
            logging.error('finish_download timed out for %s', build)
            server_name = self.get_server_name(self.url())
            artifacts_list = artifacts.split(',')
            timeout_key = self.create_stats_str(
                    'finish_download_timeout', server_name, artifacts_list)
            metadata = self.create_metadata(server_name, build, artifacts_list)
            autotest_stats.Counter(timeout_key, metadata=metadata).increment()
            raise DevServerException(
                    'finish_download timed out for %s.' % build)


class ImageServer(ImageServerBase):
    """Class for DevServer that handles RPCs related to CrOS images.

    The calls to devserver to stage artifacts, including stage and download, are
    made in async mode. That is, when caller makes an RPC |stage| to request
    devserver to stage certain artifacts, devserver handles the call and starts
    staging artifacts in a new thread, and return |Success| without waiting for
    staging being completed. When caller receives message |Success|, it polls
    devserver's is_staged call until all artifacts are staged.
    Such mechanism is designed to prevent cherrypy threads in devserver being
    running out, as staging artifacts might take long time, and cherrypy starts
    with a fixed number of threads that handle devserver rpc.
    """

    class ArtifactUrls(object):
        """A container for URLs of staged artifacts.

        Attributes:
            full_payload: URL for downloading a staged full release update
            mton_payload: URL for downloading a staged M-to-N release update
            nton_payload: URL for downloading a staged N-to-N release update

        """
        def __init__(self, full_payload=None, mton_payload=None,
                     nton_payload=None):
            self.full_payload = full_payload
            self.mton_payload = mton_payload
            self.nton_payload = nton_payload


    def wait_for_artifacts_staged(self, archive_url, artifacts='', files=''):
        """Polling devserver.is_staged until all artifacts are staged.

        @param archive_url: Google Storage URL for the build.
        @param artifacts: Comma separated list of artifacts to download.
        @param files: Comma separated list of files to download.
        @return: True if all artifacts are staged in devserver.
        """
        kwargs = {'archive_url': archive_url,
                  'artifacts': artifacts,
                  'files': files}
        return self._poll_is_staged(**kwargs)


    @remote_devserver_call()
    def call_and_wait(self, call_name, archive_url, artifacts, files,
                      error_message, expected_response=SUCCESS):
        """Helper method to make a urlopen call, and wait for artifacts staged.

        @param call_name: name of devserver rpc call.
        @param archive_url: Google Storage URL for the build..
        @param artifacts: Comma separated list of artifacts to download.
        @param files: Comma separated list of files to download.
        @param expected_response: Expected response from rpc, default to
                                  |Success|. If it's set to None, do not compare
                                  the actual response. Any response is consider
                                  to be good.
        @param error_message: Error message to be thrown if response does not
                              match expected_response.

        @return: The response from rpc.
        @raise DevServerException upon any return code that's expected_response.

        """
        kwargs = {'archive_url': archive_url,
                  'artifacts': artifacts,
                  'files': files}
        return self._call_and_wait(call_name, error_message,
                                   expected_response, **kwargs)


    @remote_devserver_call()
    def stage_artifacts(self, image, artifacts=None, files='',
                        archive_url=None):
        """Tell the devserver to download and stage |artifacts| from |image|.

         This is the main call point for staging any specific artifacts for a
        given build. To see the list of artifacts one can stage see:

        ~src/platfrom/dev/artifact_info.py.

        This is maintained along with the actual devserver code.

        @param image: the image to fetch and stage.
        @param artifacts: A list of artifacts.
        @param files: A list of files to stage.
        @param archive_url: Optional parameter that has the archive_url to stage
                this artifact from. Default is specified in autotest config +
                image.

        @raise DevServerException upon any return code that's not HTTP OK.
        """
        if not artifacts and not files:
            raise DevServerException('Must specify something to stage.')
        image = self.translate(image)
        self._stage_artifacts(image, artifacts, files, archive_url)


    @remote_devserver_call(timeout_min=0.5)
    def list_image_dir(self, image):
        """List the contents of the image stage directory, on the devserver.

        @param image: The image name, eg: <board>-<branch>/<Milestone>-<build>.

        @raise DevServerException upon any return code that's not HTTP OK.
        """
        image = self.translate(image)
        logging.info('Requesting contents from devserver %s for image %s',
                     self.url(), image)
        archive_url = _get_storage_server_for_artifacts() + image
        call = self.build_call('list_image_dir', archive_url=archive_url)
        response = urllib2.urlopen(call)
        for line in [line.rstrip() for line in response]:
            logging.info(line)


    def trigger_download(self, image, synchronous=True):
        """Tell the devserver to download and stage |image|.

        Tells the devserver to fetch |image| from the image storage server
        named by _get_image_storage_server().

        If |synchronous| is True, waits for the entire download to finish
        staging before returning. Otherwise only the artifacts necessary
        to start installing images onto DUT's will be staged before returning.
        A caller can then call finish_download to guarantee the rest of the
        artifacts have finished staging.

        @param image: the image to fetch and stage.
        @param synchronous: if True, waits until all components of the image are
               staged before returning.

        @raise DevServerException upon any return code that's not HTTP OK.

        """
        image = self.translate(image)
        artifacts = _ARTIFACTS_TO_BE_STAGED_FOR_IMAGE
        self._trigger_download(image, artifacts, files='',
                               synchronous=synchronous)


    @remote_devserver_call()
    def setup_telemetry(self, build):
        """Tell the devserver to setup telemetry for this build.

        The devserver will stage autotest and then extract the required files
        for telemetry.

        @param build: the build to setup telemetry for.

        @returns path on the devserver that telemetry is installed to.
        """
        build = self.translate(build)
        archive_url = _get_image_storage_server() + build
        call = self.build_call('setup_telemetry', archive_url=archive_url)
        try:
            response = urllib2.urlopen(call).read()
        except httplib.BadStatusLine as e:
            logging.error(e)
            raise DevServerException('Received Bad Status line, Devserver %s '
                                     'might have gone down while handling '
                                     'the call: %s' % (self.url(), call))
        return response


    def finish_download(self, image):
        """Tell the devserver to finish staging |image|.

        If trigger_download is called with synchronous=False, it will return
        before all artifacts have been staged. This method contacts the
        devserver and blocks until all staging is completed and should be
        called after a call to trigger_download.

        @param image: the image to fetch and stage.
        @raise DevServerException upon any return code that's not HTTP OK.
        """
        image = self.translate(image)
        artifacts = _ARTIFACTS_TO_BE_STAGED_FOR_IMAGE_WITH_AUTOTEST
        self._finish_download(image, artifacts, files='')


    def get_update_url(self, image):
        """Returns the url that should be passed to the updater.

        @param image: the image that was fetched.
        """
        image = self.translate(image)
        url_pattern = CONFIG.get_config_value('CROS', 'image_url_pattern',
                                              type=str)
        return (url_pattern % (self.url(), image))


    def get_staged_file_url(self, filename, image):
        """Returns the url of a staged file for this image on the devserver."""
        return '/'.join([self._get_image_url(image), filename])


    def get_full_payload_url(self, image):
        """Returns a URL to a staged full payload.

        @param image: the image that was fetched.

        @return A fully qualified URL that can be used for downloading the
                payload.

        """
        return self._get_image_url(image) + '/update.gz'


    def get_test_image_url(self, image):
        """Returns a URL to a staged test image.

        @param image: the image that was fetched.

        @return A fully qualified URL that can be used for downloading the
                image.

        """
        return self._get_image_url(image) + '/chromiumos_test_image.bin'


    @remote_devserver_call()
    def list_control_files(self, build, suite_name=''):
        """Ask the devserver to list all control files for |build|.

        @param build: The build (e.g. x86-mario-release/R18-1586.0.0-a1-b1514)
                      whose control files the caller wants listed.
        @param suite_name: The name of the suite for which we require control
                           files.
        @return None on failure, or a list of control file paths
                (e.g. server/site_tests/autoupdate/control)
        @raise DevServerException upon any return code that's not HTTP OK.
        """
        build = self.translate(build)
        call = self.build_call('controlfiles', build=build,
                               suite_name=suite_name)
        response = urllib2.urlopen(call)
        return [line.rstrip() for line in response]


    @remote_devserver_call()
    def get_control_file(self, build, control_path):
        """Ask the devserver for the contents of a control file.

        @param build: The build (e.g. x86-mario-release/R18-1586.0.0-a1-b1514)
                      whose control file the caller wants to fetch.
        @param control_path: The file to fetch
                             (e.g. server/site_tests/autoupdate/control)
        @return The contents of the desired file.
        @raise DevServerException upon any return code that's not HTTP OK.
        """
        build = self.translate(build)
        call = self.build_call('controlfiles', build=build,
                               control_path=control_path)
        return urllib2.urlopen(call).read()


    @remote_devserver_call()
    def get_dependencies_file(self, build):
        """Ask the dev server for the contents of the suite dependencies file.

        Ask the dev server at |self._dev_server| for the contents of the
        pre-processed suite dependencies file (at DEPENDENCIES_FILE)
        for |build|.

        @param build: The build (e.g. x86-mario-release/R21-2333.0.0)
                      whose dependencies the caller is interested in.
        @return The contents of the dependencies file, which should eval to
                a dict of dicts, as per site_utils/suite_preprocessor.py.
        @raise DevServerException upon any return code that's not HTTP OK.
        """
        build = self.translate(build)
        call = self.build_call('controlfiles',
                               build=build, control_path=DEPENDENCIES_FILE)
        return urllib2.urlopen(call).read()


    @remote_devserver_call()
    def get_latest_build_in_gs(self, board):
        """Ask the devservers for the latest offical build in Google Storage.

        @param board: The board for who we want the latest official build.
        @return A string of the returned build rambi-release/R37-5868.0.0
        @raise DevServerException upon any return code that's not HTTP OK.
        """
        call = self.build_call(
                'xbuddy_translate/remote/%s/latest-official' % board,
                image_dir=_get_image_storage_server())
        image_name = urllib2.urlopen(call).read()
        return os.path.dirname(image_name)


    def translate(self, build_name):
        """Translate the build name if it's in LATEST format.

        If the build name is in the format [builder]/LATEST, return the latest
        build in Google Storage otherwise return the build name as is.

        @param build_name: build_name to check.

        @return The actual build name to use.
        """
        match = re.match(r'([\w-]+)-(\w+)/LATEST', build_name)
        if not match:
            return build_name
        translated_build = self.get_latest_build_in_gs(match.groups()[0])
        logging.debug('Translated relative build %s to %s', build_name,
                      translated_build)
        return translated_build


    @classmethod
    @remote_devserver_call()
    def get_latest_build(cls, target, milestone=''):
        """Ask all the devservers for the latest build for a given target.

        @param target: The build target, typically a combination of the board
                       and the type of build e.g. x86-mario-release.
        @param milestone:  For latest build set to '', for builds only in a
                           specific milestone set to a str of format Rxx
                           (e.g. R16). Default: ''. Since we are dealing with a
                           webserver sending an empty string, '', ensures that
                           the variable in the URL is ignored as if it was set
                           to None.
        @return A string of the returned build e.g. R20-2226.0.0.
        @raise DevServerException upon any return code that's not HTTP OK.
        """
        calls = cls.build_all_calls('latestbuild', target=target,
                                    milestone=milestone)
        latest_builds = []
        for call in calls:
            latest_builds.append(urllib2.urlopen(call).read())

        return max(latest_builds, key=version.LooseVersion)


class AndroidBuildServer(ImageServerBase):
    """Class for DevServer that handles RPCs related to Android builds.

    The calls to devserver to stage artifacts, including stage and download, are
    made in async mode. That is, when caller makes an RPC |stage| to request
    devserver to stage certain artifacts, devserver handles the call and starts
    staging artifacts in a new thread, and return |Success| without waiting for
    staging being completed. When caller receives message |Success|, it polls
    devserver's is_staged call until all artifacts are staged.
    Such mechanism is designed to prevent cherrypy threads in devserver being
    running out, as staging artifacts might take long time, and cherrypy starts
    with a fixed number of threads that handle devserver rpc.
    """

    def wait_for_artifacts_staged(self, target, build_id, branch,
                                  archive_url=None, artifacts='', files=''):
        """Polling devserver.is_staged until all artifacts are staged.

        @param target: Target of the android build to stage, e.g.,
                       shamu-userdebug.
        @param build_id: Build id of the android build to stage.
        @param branch: Branch of the android build to stage.
        @param archive_url: Google Storage URL for the build.
        @param artifacts: Comma separated list of artifacts to download.
        @param files: Comma separated list of files to download.

        @return: True if all artifacts are staged in devserver.
        """
        kwargs = {'target': target,
                  'build_id': build_id,
                  'branch': branch,
                  'artifacts': artifacts,
                  'files': files,
                  'os_type': 'android'}
        if archive_url:
            kwargs['archive_url'] = archive_url
        return self._poll_is_staged(**kwargs)


    @remote_devserver_call()
    def call_and_wait(self, call_name, target, build_id, branch, archive_url,
                      artifacts, files, error_message,
                      expected_response=SUCCESS):
        """Helper method to make a urlopen call, and wait for artifacts staged.

        @param call_name: name of devserver rpc call.
        @param target: Target of the android build to stage, e.g.,
                       shamu-userdebug.
        @param build_id: Build id of the android build to stage.
        @param branch: Branch of the android build to stage.
        @param archive_url: Google Storage URL for the CrOS build.
        @param artifacts: Comma separated list of artifacts to download.
        @param files: Comma separated list of files to download.
        @param expected_response: Expected response from rpc, default to
                                  |Success|. If it's set to None, do not compare
                                  the actual response. Any response is consider
                                  to be good.
        @param error_message: Error message to be thrown if response does not
                              match expected_response.

        @return: The response from rpc.
        @raise DevServerException upon any return code that's expected_response.

        """
        kwargs = {'target': target,
                  'build_id': build_id,
                  'branch': branch,
                  'artifacts': artifacts,
                  'files': files,
                  'os_type': 'android'}
        if archive_url:
            kwargs['archive_url'] = archive_url
        return self._call_and_wait(call_name, error_message, expected_response,
                                   **kwargs)


    @remote_devserver_call()
    def stage_artifacts(self, target, build_id, branch, artifacts=None,
                        files='', archive_url=None):
        """Tell the devserver to download and stage |artifacts| from |image|.

         This is the main call point for staging any specific artifacts for a
        given build. To see the list of artifacts one can stage see:

        ~src/platfrom/dev/artifact_info.py.

        This is maintained along with the actual devserver code.

        @param target: Target of the android build to stage, e.g.,
                               shamu-userdebug.
        @param build_id: Build id of the android build to stage.
        @param branch: Branch of the android build to stage.
        @param artifacts: A list of artifacts.
        @param files: A list of files to stage.
        @param archive_url: Optional parameter that has the archive_url to stage
                this artifact from. Default is specified in autotest config +
                image.

        @raise DevServerException upon any return code that's not HTTP OK.
        """
        android_build_info = {'target': target,
                              'build_id': build_id,
                              'branch': branch}
        if not artifacts and not files:
            raise DevServerException('Must specify something to stage.')
        if not all(android_build_info.values()):
            raise DevServerException(
                    'To stage an Android build, must specify target, build id '
                    'and branch.')
        build = ANDROID_BUILD_NAME_PATTERN % android_build_info
        self._stage_artifacts(build, artifacts, files, archive_url,
                              **android_build_info)


    def trigger_download(self, target, build_id, branch, is_brillo=False,
                         synchronous=True):
        """Tell the devserver to download and stage an Android build.

        Tells the devserver to fetch an Android build from the image storage
        server named by _get_image_storage_server().

        If |synchronous| is True, waits for the entire download to finish
        staging before returning. Otherwise only the artifacts necessary
        to start installing images onto DUT's will be staged before returning.
        A caller can then call finish_download to guarantee the rest of the
        artifacts have finished staging.

        @param target: Target of the android build to stage, e.g.,
                       shamu-userdebug.
        @param build_id: Build id of the android build to stage.
        @param branch: Branch of the android build to stage.
        @param is_brillo: Set to True if it's a Brillo build. Default is False.
        @param synchronous: if True, waits until all components of the image are
               staged before returning.

        @raise DevServerException upon any return code that's not HTTP OK.

        """
        android_build_info = {'target': target,
                              'build_id': build_id,
                              'branch': branch}
        build = ANDROID_BUILD_NAME_PATTERN % android_build_info
        artifacts = (_BRILLO_ARTIFACTS_TO_BE_STAGED_FOR_IMAGE if is_brillo else
                     _ANDROID_ARTIFACTS_TO_BE_STAGED_FOR_IMAGE)
        self._trigger_download(build, artifacts, files='',
                               synchronous=synchronous, **android_build_info)


    def finish_download(self, target, build_id, branch, is_brillo=False):
        """Tell the devserver to finish staging an Android build.

        If trigger_download is called with synchronous=False, it will return
        before all artifacts have been staged. This method contacts the
        devserver and blocks until all staging is completed and should be
        called after a call to trigger_download.

        @param target: Target of the android build to stage, e.g.,
                       shamu-userdebug.
        @param build_id: Build id of the android build to stage.
        @param branch: Branch of the android build to stage.
        @param is_brillo: Set to True if it's a Brillo build. Default is False.

        @raise DevServerException upon any return code that's not HTTP OK.
        """
        android_build_info = {'target': target,
                              'build_id': build_id,
                              'branch': branch}
        build = ANDROID_BUILD_NAME_PATTERN % android_build_info
        artifacts = (_BRILLO_ARTIFACTS_TO_BE_STAGED_FOR_IMAGE if is_brillo else
                     _ANDROID_ARTIFACTS_TO_BE_STAGED_FOR_IMAGE)
        self._finish_download(build, artifacts, files='', **android_build_info)


    def get_staged_file_url(self, filename, target, build_id, branch):
        """Returns the url of a staged file for this image on the devserver.

        @param filename: Name of the file.
        @param target: Target of the android build to stage, e.g.,
                       shamu-userdebug.
        @param build_id: Build id of the android build to stage.
        @param branch: Branch of the android build to stage.

        @return: The url of a staged file for this image on the devserver.
        """
        android_build_info = {'target': target,
                              'build_id': build_id,
                              'branch': branch,
                              'os_type': 'android'}
        build = ANDROID_BUILD_NAME_PATTERN % android_build_info
        return '/'.join([self._get_image_url(build), filename])


    def translate(self, build_name):
        """Translate the build name if it's in LATEST format.

        If the build name is in the format [branch]/[target]/LATEST, return the
        latest build in Launch Control otherwise return the build name as is.

        @param build_name: build_name to check.

        @return The actual build name to use.
        """
        branch, target, build_id = utils.parse_android_build(build_name)
        if build_id != 'LATEST':
            return build_name
        call = self.build_call('latestbuild', branch=branch, target=target,
                               os_type='android')
        translated_build_id = urllib2.urlopen(call).read()
        translated_build = (ANDROID_BUILD_NAME_PATTERN %
                            {'branch': branch,
                             'target': target,
                             'build_id': translated_build_id})
        logging.debug('Translated relative build %s to %s', build_name,
                      translated_build)
        return translated_build


def _is_load_healthy(load):
    """Check if devserver's load meets the minimum threshold.

    @param load: The devserver's load stats to check.

    @return: True if the load meets the minimum threshold. Return False
             otherwise.

    """
    # Threshold checks, including CPU load.
    if load[DevServer.CPU_LOAD] > DevServer.MAX_CPU_LOAD:
        logging.debug('CPU load of devserver %s is at %s%%, which is higher '
                      'than the threshold of %s%%', load['devserver'],
                      load[DevServer.CPU_LOAD], DevServer.MAX_CPU_LOAD)
        return False
    if load[DevServer.NETWORK_IO] > DevServer.MAX_NETWORK_IO:
        logging.debug('Network IO of devserver %s is at %i Bps, which is '
                      'higher than the threshold of %i bytes per second.',
                      load['devserver'], load[DevServer.NETWORK_IO],
                      DevServer.MAX_NETWORK_IO)
        return False
    return True


def _compare_load(devserver1, devserver2):
    """Comparator function to compare load between two devservers.

    @param devserver1: A dictionary of devserver load stats to be compared.
    @param devserver2: A dictionary of devserver load stats to be compared.

    @return: Negative value if the load of `devserver1` is less than the load
             of `devserver2`. Return positive value otherwise.

    """
    return int(devserver1[DevServer.DISK_IO] - devserver2[DevServer.DISK_IO])


def get_least_loaded_devserver(devserver_type=ImageServer):
    """Get the devserver with the least load.

    Iterate through all devservers and get the one with least load.

    TODO(crbug.com/486278): Devserver with required build already staged should
    take higher priority. This will need check_health call to be able to verify
    existence of a given build/artifact. Also, in case all devservers are
    overloaded, the logic here should fall back to the old behavior that randomly
    selects a devserver based on the hash of the image name/url.

    @param devserver_type: Type of devserver to select from. Default is set to
                           ImageServer.

    @return: Name of the devserver with the least load.

    """
    # get_devserver_load call needs to be made in a new process to allow force
    # timeout using signal.
    output = multiprocessing.Queue()
    processes = []
    for devserver in devserver_type.servers():
        processes.append(multiprocessing.Process(
                target=DevServer.get_devserver_load_wrapper,
                args=(devserver, TIMEOUT_GET_DEVSERVER_LOAD, output)))

    for p in processes:
        p.start()
    for p in processes:
        p.join()
    loads = [output.get() for p in processes]
    # Filter out any load failed to be retrieved or does not support load check.
    loads = [load for load in loads if load and DevServer.CPU_LOAD in load and
             DevServer.is_free_disk_ok(load)]
    if not loads:
        logging.debug('Failed to retrieve load stats from any devserver. No '
                      'load balancing can be applied.')
        return None
    loads = [load for load in loads if _is_load_healthy(load)]
    if not loads:
        logging.error('No devserver has the capacity to be selected.')
        return None
    loads = sorted(loads, cmp=_compare_load)
    return loads[0]['devserver']
