#!/usr/bin/python

# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.


"""This script copies all data from one index into another, and updates the
alias to point to the new index.

usage: es_reindex.py [-h] [--host HOST] [--port PORT] [--old OLD]
                     [--new NEW] [--alias ALIAS]

optional arguments:
  -h, --help            show this help message and exit
  --host HOST           name of ES server.
  --port PORT
  --old OLD             Name of the old index.
  --new NEW             Name of the new index.
  --alias ALIAS         alias to be pointed to the new index.

"""

import argparse

import common
from elasticsearch import Elasticsearch
from elasticsearch import helpers
from autotest_lib.client.common_lib.cros.graphite import autotest_es


def main():
    """main script. """

    parser = argparse.ArgumentParser()
    parser.add_argument('--host', type=str, dest='host',
                        help='name of ES server.')
    parser.add_argument('--port', type=str, dest='port', default=9200)
    parser.add_argument('--old', type=str, dest='old',
                        help='Name of the old index.')
    parser.add_argument('--new', type=str, dest='new',
                        help='Name of the new index.')
    parser.add_argument('--alias', type=str, dest='alias',
                        help='alias to be pointed to the new index.')

    options = parser.parse_args()

    query = {'query' : {'match_all' : {}},
             'size': 1}

    result = autotest_es.execute_query(index=options.old, host=options.host,
                                       port=options.port, query)
    print 'Total number of records in index %s: %d' % (options.old,
                                                       result.total)

    print ('Re-index: %s to index: %s for server %s:%s' %
           (options.old, options.new, options.host, options.port))

    client = Elasticsearch(hosts=[{'host': options.host, 'port': options.port}])
    helpers.reindex(client, options.old, options.new)
    print 'reindex completed.'

    print 'Checking records in the new index...'
    result = es.execute_query(index=options.new, host=options.host,
                              port=options.port, query)
    print 'Total number of records in index %s: %d' % (options.new,
                                                       result.total)

    # count_new can be larger than count if new records are added during
    # reindexing. This check only tries to make sure no record was lost.
    if count > count_new:
        raise Exception('Error! There are %d records missing after reindexing. '
                        'Alias will not be updated to the new index. You might '
                        'want to try reindex again.' %
                        (count - count_new))

    body = {'actions': [{'remove': {'alias': options.alias,
                                    'index': options.old}},
                        {'add': {'alias': options.alias,
                                 'index': options.new}}
                        ]
            }
    client.indices.update_aliases(body=body)
    print 'alias is updated.'
    print ('Please verify the new index is working before deleting old index '
           'with command:\n.curl -XDELETE %s:%s/%s' %
           (options.host, options.port, options.old))


if __name__ == '__main__':
    main()
