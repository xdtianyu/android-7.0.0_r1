#! /usr/bin/python

"""A simple heartbeat client.

Executes heartbeats against a simple_heartbeat_server running on the give
--server address and deserializes records into an in memory sqlite database.

Usage:
1. heartbeat_client.py
    --server http://localhost:8080
    --board lumpy

    Perform a heartbeat against the given server for the given board,
    and deserialize records into a sqlite database.

2. heartbeat_client.py
    --server http://localhost:8080
    --board lumpy
    --host_limit 1 --job_limit 100

    Do the same as 1, but instruct the server to limit the hosts to 1
    and jobs to 100. This is useful for debugging issues with only jobs/hosts.
"""


from json import decoder
import argparse
import sys
import urllib2

import common

from autotest_lib.scheduler.shard import simple_heartbeat_server
from autotest_lib.frontend import setup_django_environment
from autotest_lib.frontend.afe import frontend_test_utils
from autotest_lib.frontend.afe import models

json_decoder = decoder.JSONDecoder()


class HeartbeatHandler(frontend_test_utils.FrontendTestMixin):
    """Performs heartbeats and deserializes into an in memory database."""

    _config_section = 'AUTOTEST_WEB'


    def __init__(self, server):
        """Initialize a heartbeat server.

        @param server: The address of a simple_heartbeat_server.
        """
        self.server = server
        self._frontend_common_setup(setup_tables=True, fill_data=False)


    @staticmethod
    @simple_heartbeat_server.time_call
    def get_heartbeat_packet(server, board, host_limit, job_limit):
        """Perform the heartbeat.

        Constructs a url like: http://localhost:8080/lumpy?raw&host_limit=3
        and does a urlopen.

        @param server: The address of a simple_heartbeat_server.
        @param host_limit: The number of hosts to include in the heartbeat.
        @param job_limit: The number of jobs to include in the heartbeat.

        @return: A string containing the heartbeat packet.
        """
        url = '%s/%s?raw' % (server, board)
        if job_limit:
            url = '%s&job_limit=%s' % (url, job_limit)
        if host_limit:
            url = '%s&host_limit=%s' % (url, host_limit)
        print 'Performing heartbeat against %s' % url
        return urllib2.urlopen(url).read()


    @staticmethod
    @simple_heartbeat_server.time_call
    def deserialize_heartbeat(packet):
        """Deserialize the given heartbeat packet into an in memory database.

        @param packet: A string representing the heartbeat packet containing
                jobs and hosts.

        @return: The json decoded heartbeat.
        """
        response = json_decoder.decode(packet)
        [models.Host.deserialize(h) for h in response['hosts']]
        [models.Job.deserialize(j) for j in response['jobs']]
        return response


    def perform_heartbeat(self, board, host_limit, job_limit):
        """Perform a heartbeat against the given server, for the given board.

        @param board: Boardname, eg: lumpy.
        @param host_limit: Limit number of hosts retrieved.
        @param job_limit: Limit number of jobs retrieved.
        """
        timing, packet = self.get_heartbeat_packet(
                self.server, board, host_limit, job_limit)
        print 'Time to perform heartbeat %s' % timing
        timing, response = self.deserialize_heartbeat(packet)
        print 'Time to deserialize hearbeat %s' % timing
        print ('Jobs: %s, Hosts: %s' %
                (len(response['jobs']), len(response['hosts'])))


def _parse_args(args):
    parser = argparse.ArgumentParser(
            description='Start up a simple heartbeat client.')
    parser.add_argument(
            '--server', default='http://localhost:8080',
            help='Address of a simple_heartbeat_server to heartbeat against.')
    parser.add_argument(
            '--board', default='lumpy',
            help='Heartbeats can only be performed '
                 'against a specific board, eg: lumpy.')
    parser.add_argument(
            '--host_limit', default='',
            help='Limit hosts in the heartbeat.')
    parser.add_argument(
            '--job_limit', default='',
            help='Limit jobs in the heartbeat.')
    args = parser.parse_args(args)
    args.board = args.board.lstrip(
            simple_heartbeat_server.BoardHandler.board_prefix)
    return args


if __name__ == '__main__':
    args = _parse_args(sys.argv[1:])
    HeartbeatHandler(args.server).perform_heartbeat(
            args.board, args.host_limit, args.job_limit)
