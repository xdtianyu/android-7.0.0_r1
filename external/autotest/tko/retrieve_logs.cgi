#!/usr/bin/python

import cgi, os, sys, urllib2
import common
from multiprocessing import pool
from autotest_lib.frontend import setup_django_environment

from autotest_lib.client.common_lib import global_config
from autotest_lib.client.bin import utils
from autotest_lib.frontend.afe.json_rpc import serviceHandler
from autotest_lib.server import system_utils
from autotest_lib.server import utils as server_utils


_PAGE = """\
Status: 302 Found
Content-Type: text/plain
Location: %s\r\n\r
"""

GOOGLE_STORAGE_PATTERN = 'storage.cloud.google.com/'

# Define function for retrieving logs
def _retrieve_logs_dummy(job_path):
    pass

site_retrieve_logs = utils.import_site_function(__file__,
    "autotest_lib.tko.site_retrieve_logs", "site_retrieve_logs",
    _retrieve_logs_dummy)

site_find_repository_host = utils.import_site_function(__file__,
    "autotest_lib.tko.site_retrieve_logs", "site_find_repository_host",
    _retrieve_logs_dummy)

form = cgi.FieldStorage(keep_blank_values=True)
# determine if this is a JSON-RPC request. we support both so that the new TKO
# client can use its RPC client code, but the old TKO can still use simple GET
# params.
_is_json_request = form.has_key('callback')

# if this key exists, we check if requested log exists in local machine,
# and do not return Google Storage URL when the log doesn't exist.
_local_only = form.has_key('localonly')


def _get_requested_path():
    if _is_json_request:
        request_data = form['request'].value
        request = serviceHandler.ServiceHandler.translateRequest(request_data)
        parameters = request['params'][0]
        return parameters['path']

    return form['job'].value


def _check_result(args):
    host = args['host']
    job_path = args['job_path']
    shard = args['shard']
    if shard:
        http_path = 'http://%s/tko/retrieve_logs.cgi?localonly&job=%s' % (
                host, job_path)
    else:
        http_path = 'http://%s%s' % (host, job_path)

    try:
        utils.urlopen(http_path)

        # On Vms the shard name is set to the default gateway but the
        # browser used to navigate frontends (that runs on the host of
        # the vms) is immune to the same NAT routing the vms have, so we
        # need to replace the gateway with 'localhost'.
        if utils.DEFAULT_VM_GATEWAY in host:
            normalized_host = host.replace(utils.DEFAULT_VM_GATEWAY, 'localhost')
        else:
            normalized_host = utils.normalize_hostname(host)
        return 'http', normalized_host, job_path
    except urllib2.URLError:
        return None


def _get_tpool_args(hosts, job_path, is_shard, host_set):
    """Get a list of arguments to be passed to multiprocessing.pool.ThreadPool.

    @param hosts: a list of host names.
    @param job_path: a requested job path.
    @param is_shard: True if hosts are shards, False otherwise.
    @param host_set: a Set to filter out duplicated hosts.

    @return: a list of dictionaries to be used as input of _check_result().
    """
    args = []
    for host in hosts:
        host = host.strip()
        if host and host != 'localhost' and host not in host_set:
            host_set.add(host)
            arg = {'host': host, 'job_path': job_path, 'shard': is_shard}
            args.append(arg)
    return args


def find_repository_host(job_path):
    """Find the machine holding the given logs and return a URL to the logs"""
    site_repo_info = site_find_repository_host(job_path)
    if site_repo_info is not None:
        return site_repo_info

    # This cgi script is run only in master (cautotest) and shards.
    # Drones do not run this script when receiving '/results/...' request.
    # Only master should check drones and shards for the requested log.
    # Also restricted users do not have access to drones or shards,
    # always point them to localhost or google storage.
    if (not server_utils.is_shard() and
        not server_utils.is_restricted_user(os.environ.get('REMOTE_USER'))):
        drones = system_utils.get_drones()
        shards = system_utils.get_shards()

        host_set = set()
        tpool_args = _get_tpool_args(drones, job_path, False, host_set)
        tpool_args += _get_tpool_args(shards, job_path, True, host_set)

        tpool = pool.ThreadPool()
        for result_path in tpool.imap_unordered(_check_result, tpool_args):
            if result_path:
                return result_path

    # If the URL requested is a test result, it is now either on the local
    # host or in Google Storage.
    if job_path.startswith('/results/'):
        # We only care about the path after '/results/'.
        job_relative_path = job_path[9:]
        if not _local_only and not os.path.exists(
                    os.path.join('/usr/local/autotest/results',
                                 job_relative_path)):
            gsuri = utils.get_offload_gsuri().split('gs://')[1]
            return ['https', GOOGLE_STORAGE_PATTERN, gsuri + job_relative_path]


def get_full_url(info, log_path):
    if info is not None:
        protocol, host, path = info
        prefix = '%s://%s' % (protocol, host)
    else:
        prefix = ''
        path = log_path

    if _is_json_request:
        return '%s/tko/jsonp_fetcher.cgi?%s' % (prefix,
                                                os.environ['QUERY_STRING'])
    else:
        return prefix + path


log_path = _get_requested_path()
info = find_repository_host(log_path)
site_retrieve_logs(log_path)
print _PAGE % get_full_url(info, log_path)
