# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""This is a script that inserts a bunch of entries into
   elasticdb by reporting stats with metadata in the stats module.

Usage:
    # runs tests on all stats objects on prod instance of es
    python stats_es_functionaltest.py --all --es_host=prod
    # runs tests on all stats objects on test instance of es (localhost)
    python stats_es_functionaltest.py --all --es_host=test

    python stats_es_functionaltest.py --test=timer # runs tests on timer obj.
"""

import optparse
import time

import common
from autotest_lib.client.common_lib.cros.graphite import autotest_es
from autotest_lib.client.common_lib.cros.graphite import es_test_utils


TESTS_ALL = ['timer', 'gauge', 'raw', 'average', 'counter']


class StatsFunctionalTest(object):
    """Test stats module with metadata"""

    def __init__(self, es_host, es_port, index):
        self.host = es_host
        self.port = es_port
        self.index = index
        self.wait_time = 6 # Bulk flush is 5 seconds
        if autotest_es.ES_USE_HTTP:
            # No flush time for http requests.
            self.wait_time = 2

    def run_tests(self, tests=TESTS_ALL,
                  num_entries=10,
                  keys=['job_id', 'host_id', 'job_start']):
        """Runs test listed in the param tests.

        @param tests: list of tests to run
        @param num_entries: number of metadata entries to insert
        @param keys: keys each metadata dictionary will have

        """
        for test_type in tests:
            if test_type not in TESTS_ALL:
                print 'Skipping test %s, it is not supported. ' % (test_type)
            es_test_utils.clear_index(index=self.index,
                                      host=self.host,
                                      port=self.port,
                                      timeout=10)
            print 'running %s test.' % (test_type)
            self._run_one_test_metadata(test_type, num_entries, keys)


    def _run_one_test_metadata(self, test_type, num_entries, keys):
        """Puts many entries into elasticdb, then query it. """

        print ('preparing to insert %s entries with keys %s into elasticdb...'
               % (num_entries, keys))
        es_test_utils.sequential_random_insert_ints(
                keys=keys,
                target_type=test_type,
                index=self.index,
                host=self.host,
                port=self.port,
                use_http = autotest_es.ES_USE_HTTP,
                udp_port = autotest_es.ES_UDP_PORT,
                num_entries=num_entries,
                print_interval=num_entries/5)
        # Wait a bit for es to be populated with the metadata entry.
        # I set this to 6 seconds because bulk.udp.flush_interval (es server)
        # is configured to be 5 seconds.
        print 'waiting %s seconds...' % (self.wait_time)
        time.sleep(self.wait_time)
        result = autotest_es.query(host=self.host, port=self.port,
                                   index=self.index, fields_returned=keys,
                                   range_constraints=[('host_id', 0, None)])
        if not result:
            print ('%s test error: Index %s not found.'
                   %(test_type, self.index))
            return

        # TODO(michaelliang): Check hits and total are valid keys at each layer.
        num_entries_found = result.total
        print('  Inserted %s entries, found %s entries.'
              %(num_entries, num_entries_found))
        if num_entries_found != num_entries:
            print '\n\n%s test failed! \n\n' % (test_type)
        else:
            print '\n\n%s test passed! \n\n' % (test_type)


def main():
    """main script. """

    parser = optparse.OptionParser()
    parser.add_option('--all', action='store_true', dest='run_all',
                      default=False,
                      help='set --all flag to run all tests.')
    parser.add_option('--test', type=str,
            help=('Enter subset of [\'timer\', \'gauge\', \'raw\','
                  '\'average\', \'counter\']'),
            dest='test_to_run',
            default=None)
    parser.add_option('--es_host', type=str,
            help=('Enter "prod" or "test" or an ip'),
            dest='es_host',
            default='localhost')
    parser.add_option('--es_port', type=int,
            help=('Enter port of es instance, usually 9200'),
            dest='es_port',
            default=9200)
    options, _ = parser.parse_args()


    if not options.run_all and not options.test_to_run:
        print ('No tests specified.'
               'For help: python stats_es_functionaltest.py -h')
    if options.es_host == 'prod':
        es_host = autotest_es.METADATA_ES_SERVER
        es_port = autotest_es.ES_PORT
    elif options.es_host == 'test':
        es_host = 'http://localhost'
        es_port = autotest_es.ES_PORT
    else:
        es_host = options.es_host
        es_port = options.es_port
    test_obj = StatsFunctionalTest(es_host,
                                   es_port,
                                   'stats_es_functionaltest')
    if options.run_all:
        test_obj.run_tests()
    elif options.test_to_run:
        test_obj.run_tests([options.test_to_run])


if __name__ == '__main__':
    main()
