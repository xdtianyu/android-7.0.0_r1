#! /usr/bin/python

"""A simple heartbeat server.

Executes *readonly* heartbeats against the given database.

Usage:
1. heartbeat_server.py
    --port 8080

    Start to serve heartbeats on port 8080 using the database credentials
    found in the shadow_config. One would perform heartbeats for board:lumpy
    against this server with:
        curl http://localhost:8080/lumpy.
    Or just visiting the url through the browser.

    Such a server is capable of handling the following urls:
        /lumpy: Return formatted heartbeat packets with timing information for
                each stage, to be viewed in the browser.
        /lumpy?raw: Return raw json heartbeat packets for lumpy
        /lumpy?raw&host_limit=1&job_limit=0: Return a 'raw' heartbeat with the
                first host and not jobs.

2. heartbeat_server.py
    --db_host <ip, eg: production db server>
    --db_user <user, eg: chromeosqa-admin>
    --db_password <password, eg: production db password>

    The same as 1. but use the remote db server specified via
    db_(host,user,password).
"""


import argparse
import sys
import time
import urlparse
from BaseHTTPServer import BaseHTTPRequestHandler
from BaseHTTPServer import HTTPServer

import common
from autotest_lib.client.common_lib.global_config import global_config as config
from autotest_lib.frontend import setup_django_environment


# Populated with command line database credentials.
DB_SETTINGS = {
    'ENGINE': 'autotest_lib.frontend.db.backends.afe',
}

# Indent level used when formatting json for the browser.
JSON_FORMATTING_INDENT = 4


def time_call(func):
    """A simple timer wrapper.

    @param func: The function to wrap.
    """
    def wrapper(*args, **kwargs):
        """Wrapper returned by time_call decorator."""
        start = time.time()
        res = func(*args, **kwargs)
        return time.time()-start, res
    return wrapper


class BoardHandler(BaseHTTPRequestHandler):
    """Handles heartbeat urls."""

    # Prefix for all board labels.
    board_prefix = 'board:'


    @staticmethod
    @time_call
    def _get_jobs(board, job_limit=None):
        jobs = models.Job.objects.filter(
                dependency_labels__name=board).exclude(
                        hostqueueentry__complete=True).exclude(
                        hostqueueentry__active=True)
        return jobs[:job_limit] if job_limit is not None else jobs


    @staticmethod
    @time_call
    def _get_hosts(board, host_limit=None):
        hosts = models.Host.objects.filter(
                labels__name__in=[board], leased=False)
        return hosts[:host_limit] if host_limit is not None else hosts


    @staticmethod
    @time_call
    def _create_packet(hosts, jobs):
        return {
            'hosts': [h.serialize() for h in hosts],
            'jobs': [j.serialize() for j in jobs]
        }


    def do_GET(self):
        """GET handler.

        Handles urls like: http://localhost:8080/lumpy?raw&host_limit=5
        and writes the appropriate http response containing the heartbeat.
        """
        parsed_path = urlparse.urlparse(self.path, allow_fragments=True)
        board = '%s%s' % (self.board_prefix, parsed_path.path.rsplit('/')[-1])

        raw = False
        job_limit = None
        host_limit = None
        for query in parsed_path.query.split('&'):
            split_query = query.split('=')
            if split_query[0] == 'job_limit':
                job_limit = int(split_query[1])
            elif split_query[0] == 'host_limit':
                host_limit = int(split_query[1])
            elif split_query[0] == 'raw':
                raw = True

        host_time, hosts = self._get_hosts(board, host_limit)
        job_time, jobs = self._get_jobs(board, job_limit)

        serialize_time, heartbeat_packet = self._create_packet(hosts, jobs)
        self.send_response(200)
        self.end_headers()

        # Format browser requests, the heartbeat client will request using ?raw
        # while the browser will perform a plain request like
        # http://localhost:8080/lumpy. The latter needs to be human readable and
        # include more details timing information.
        json_encoder = django_encoder.DjangoJSONEncoder()
        if not raw:
            json_encoder.indent = JSON_FORMATTING_INDENT
            self.wfile.write('Serialize: %s,\nJob query: %s\nHost query: %s\n'
                             'Hosts: %s\nJobs: %s\n' %
                             (serialize_time, job_time, host_time,
                              len(heartbeat_packet['hosts']),
                              len(heartbeat_packet['jobs'])))
        self.wfile.write(json_encoder.encode(heartbeat_packet))
        return


def _parse_args(args):
    parser = argparse.ArgumentParser(
            description='Start up a simple heartbeat server on localhost.')
    parser.add_argument(
            '--port', default=8080,
            help='The port to start the heartbeat server.')
    parser.add_argument(
            '--db_host',
            default=config.get_config_value('AUTOTEST_WEB', 'host'),
            help='Db server ip address.')
    parser.add_argument(
            '--db_name',
            default=config.get_config_value('AUTOTEST_WEB', 'database'),
            help='Name of the db table.')
    parser.add_argument(
            '--db_user',
            default=config.get_config_value('AUTOTEST_WEB', 'user'),
            help='User for the db server.')
    parser.add_argument(
            '--db_password',
            default=config.get_config_value('AUTOTEST_WEB', 'password'),
            help='Password for the db server.')
    parser.add_argument(
            '--db_port',
            default=config.get_config_value('AUTOTEST_WEB', 'port', default=''),
            help='Port of the db server.')

    return parser.parse_args(args)


if __name__ == '__main__':
    args = _parse_args(sys.argv[1:])
    server = HTTPServer(('localhost', args.port), BoardHandler)
    print ('Starting heartbeat server, query eg: http://localhost:%s/lumpy' %
           args.port)

    # We need these lazy imports to allow command line specification of
    # database credentials.
    from autotest_lib.frontend import settings
    DB_SETTINGS['HOST'] = args.db_host
    DB_SETTINGS['NAME'] = args.db_name
    DB_SETTINGS['USER'] = args.db_user
    DB_SETTINGS['PASSWORD'] = args.db_password
    DB_SETTINGS['PORT'] = args.db_port
    settings.DATABASES['default'] = DB_SETTINGS
    from autotest_lib.frontend.afe import models
    from django.core.serializers import json as django_encoder

    server.serve_forever()

