#! /usr/bin/python

import os
import argparse
import datetime as datetime_base
from datetime import datetime
import logging
import sys

import common
from autotest_lib.frontend import setup_django_environment
from autotest_lib.frontend.afe import models
from autotest_lib.server.cros.dynamic_suite import job_status


def find_hostnames(pool='bvt', board='x86-mario'):
    return os.popen("/usr/local/autotest/cli/atest host list | "
                    "grep pool:%s | grep %s | "
                    "awk '{ print $1}'" % (pool, board)).read().split('\n')


def _parse_args(args):
    if not args:
        print ('Try ./contrib/poolhistory.py pool -name bvt -board x86-mario -start '
               '"2014-04-25 02:57:16" -end "2014-04-25 04:32:06"')
        sys.exit(0)
    parser = argparse.ArgumentParser(
            description='A script to get the special tasks on a host or job.')
    subparsers = parser.add_subparsers(help='Get tasks based on a job or host.')
    parser_host = subparsers.add_parser('pool', help='Per host analysis mode.')
    parser_host.add_argument('-name',
                             help='Hostname for which you would like tasks.')
    parser_host.add_argument('-board',
                             help='Hostname for which you would like tasks.')
    parser_host.add_argument(
            '-start', help='Start time. Eg: 2014-03-25 16:26:31')
    parser_host.add_argument(
            '-end', help='End time Eg: 2014-03-25 18:26:31.')
    return parser.parse_args(args)


if __name__ == '__main__':
    args = _parse_args(sys.argv[1:])
    print 'Pool %s, board %s' % (args.name, args.board)
    for name in find_hostnames(args.name, args.board):
        print 'Host: %s' % name
        print os.popen('/usr/local/autotest/contrib/onerous_tasks.py host -name %s '
                '-start "%s" -end "%s"' % (name, args.start, args.end)).read()
