#!/usr/bin/python -u

"""
A script to help find the last few jobs that ran on a set of hosts that match
the specified query, and rank them according to frequence across these hosts.
Usage:
1. Get last 5 jobs from 1 day ago running on all lumpies in pool suites that are
    currently in repair fail:
    ./sheriff_host_utils --days_back=1
    --query 'labels=pool:suites,board:lumpy status="Repair Failed"'

2. Email someone about the last 5 jobs on all Repair Failed hosts.
    ./sheriff_host_utils --limit 5 --query 'status="Repair Failed"'
            --email someone@something.com
"""

import argparse
import collections
import datetime
import operator
import shlex
import sys

import common

from autotest_lib.client.common_lib import mail
from autotest_lib.frontend import setup_django_environment
from autotest_lib.frontend.afe import models
from autotest_lib.server import frontend
from autotest_lib.server.cros import repair_utils
from django.utils import timezone as django_timezone


def _parse_args(args):
    description=('./sheriff_host_utils.py --limit 5 --days_back 5 '
                 '--query \'status="Repair Failed" invalid=0 locked=0\'')
    if not args:
        print ('Too few arguments, execute %s, or try '
               './sheriff_host_utils.py --help' % description)
        sys.exit(1)

    parser = argparse.ArgumentParser(description=description)
    parser.add_argument('--limit', default=5,
                        help='The number of jobs per host.Eg: --limit 5')
    parser.add_argument('--days_back', default=5,
                        help='Number of days to search. Eg: --days_back 5')
    default_query = 'status="Repair Failed" labels=pool:bvt,board:lumpy'
    parser.add_argument('--query', default=default_query,
                        help='Search query.Eg: --query %s' % default_query)
    parser.add_argument('--email', default=None, help='send results to email.')
    return parser.parse_args(args)


def _parse_query(query):
    """Parses query string for a host.

    All queries follow the format: 'key=value key2=value..' where all keys are
    are columns of the host table with the exception of labels. When specifying
    labels, the format is the same even though a label is a foreign key:
    --query 'lable=<comma seperated list of label names>'.

    @return: A dictionary into which the query has been parsed.
    """
    l = shlex.split(query)
    keys = [elem[:elem.find('=')] for elem in l]
    values = [elem[elem.find('=')+1:] for elem in l]
    payload = dict(zip(keys, values))
    return payload


def _get_pool(host):
    """Returns the pool of a host.
    """
    labels = host.labels.all()
    for label_name in [label.name for label in labels]:
        if 'pool' in label_name:
            return label_name


def retrieve_hosts(payload):
    """Retrieve hosts matching the payload.

    @param payload: A dict with selection criteria for hosts.

    @return: A queryset of hosts matching the payload.
    """
    # Replace label names with a foreign key query.
    query_hosts = models.Host.objects.all()
    if 'labels' in payload:
        for label in payload['labels'].split(','):
            query_hosts = query_hosts.filter(labels__name=label)
        del payload['labels']
    return query_hosts.filter(**payload)


def analyze_jobs(hqes):
    """Perform some aggregation on the jobs that ran on matching hosts.

    @return: A string with the results of the analysis.
    """
    names = [hqe.job.name for hqe in hqes]
    ranking = collections.Counter([name[name.rfind('/')+1:] for name in names])
    sorted_rankings = sorted(ranking.iteritems(), key=operator.itemgetter(1))
    m = 'Ranking tests that ran on those hosts by frequency: \n\t'
    for job_stat in reversed(sorted_rankings):
        m += '%s test name: %s\n\t' % (job_stat[1], job_stat[0])
    return m


def last_jobs_on_hosts(payload, limit_jobs, days_back):
    """Find the last limit_jobs on hosts with given status within days_back.

    @param payload: A dictionary specifiying the selection criteria of the hosts.
        Eg {'stauts': "Ready", 'id': 40}
    @param limit_jobs: The number of jobs per host.
    @param days_back: The days back to search for jobs.

    @retrurn: A string with information about the last jobs that ran on all
        hosts matching the query mentioned in the payload.
    """
    host_map = {}
    pool_less, job_less, jobs_to_analyze  = [], [], []
    hqes = models.HostQueueEntry.objects.all()
    cutoff = django_timezone.now().date() - datetime.timedelta(days=days_back)
    message = ''

    for host in retrieve_hosts(payload):
        pool = _get_pool(host)
        if not pool:
            pool_less.append(host.hostname)
            continue
        relevent_hqes = list(hqes.filter(host_id=host.id,
                started_on__gte=cutoff).order_by('-started_on')[:limit_jobs])
        if relevent_hqes:
            jobs = ['name: %s, id: %s' %
                    (hqe.job.name, hqe.job_id) for hqe in relevent_hqes]
            message += '%s\n%s\n\t%s' % (pool, host, '\n\t'.join(jobs))
            jobs_to_analyze += relevent_hqes
        else:
            job_less.append(host.hostname)

    if job_less:
        message += ('\nNo jobs found for the following hosts within cutoff %s\n\t' %
                    cutoff)
        message += '\n\t'.join(job_less)
    if pool_less:
        message += '%s%s' % ('\nNo pools found on the following hosts:',
                            '\n\t'.join(pool_less))
    if jobs_to_analyze:
        message += '\n\n%s' % analyze_jobs(jobs_to_analyze)

    if message:
        return '%s\n%s' % ('Host information:', message)
    return 'No hosts matching query %s from %s days back' % (payload, days_back)


if __name__ == '__main__':
    args = _parse_args(sys.argv[1:])
    message = last_jobs_on_hosts(_parse_query(args.query),
                                 int(args.limit), int(args.days_back))
    if args.email:
        mail.send('', args.email, '',
                  'Results from your sheirff script.', message)
    print message
