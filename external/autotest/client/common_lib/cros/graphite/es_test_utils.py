# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Helper functions for testing stats module and elasticsearch
"""

import logging
import time

import common

import elasticsearch

from autotest_lib.client.common_lib.cros.graphite import es_utils
from autotest_lib.client.common_lib.cros.graphite import autotest_stats


# Defines methods in the stats class that can take in metadata.
TARGET_TO_STATS_CLASS = {
    'timer': autotest_stats.Timer,
    'gauge': autotest_stats.Gauge,
    'raw': autotest_stats.Raw,
    'average': autotest_stats.Average,
    'counter': autotest_stats.Counter,
}

# Maps target type to method to trigger sending of metadata.
# This differs based on what each object does.
# For example, in order for timer to send something, its stop
# method must be called. This differs for other stats objects.
TARGET_TO_METHOD = {
    'timer': 'stop',
    'gauge': 'send',
    'raw': 'send',
    'average': 'send',
    'counter': '_send',
}

# Default maximum number of entries to return from ES query
DEFAULT_NUM_ENTRIES = 100

class EsTestUtilException(Exception):
    """Exception raised when functions here fail. """
    pass


def sequential_random_insert_ints(keys, num_entries, target_type, index,
                                  host, port, use_http, udp_port,
                                  between_insert_secs=0,
                                  print_interval=10):
    """Inserts a bunch of random entries into the es database.
    Keys are given, values are randomly generated.

    @param keys: A list of keys
    @param num_entries: Number of entries to insert
    @param target_type: This must be in
            ['timer', 'gauge', 'raw', 'average', 'counter']
    @param between_insert_secs: Time to sleep after each insert.
                                defaults to no sleep time.
    @param print_interval: how often to print
                           defaults to every 10 entries.
    @param index: Index of es db to insert to
    @param host: host of es db
    @param port: port of es db
    """
    # We are going to start the value at 0 and increment it by one per val.
    for i in range(num_entries):
        if print_interval == 0 or i % print_interval == 0:
            print('    Inserting entry #%s with keys %s into index "%s."'
                   % (i, str(keys), index))
        metadata = {}
        for value, key in enumerate(keys):
            metadata[key] = value

        # Subname and value are not important from metadata pov.
        subname = 'metadata.test'
        value = 10
        stats_target = TARGET_TO_STATS_CLASS[target_type](subname,
                metadata=metadata,
                es=es_utils.ESMetadata(use_http=use_http, host=host,
                                       port=port, index=index,
                                       udp_port=udp_port))

        if target_type == 'timer':
            stats_target.start()
            stats_target.stop()
        else:
            getattr(stats_target, TARGET_TO_METHOD[target_type])(subname, value)
        time.sleep(between_insert_secs)


def clear_index(index, host, port, timeout, sleep_time=0.5, clear_timeout=5):
    """Clears index in es db located at host:port.

    Warning: Will delete all data in es for a given index

    @param index: Index of es db to clear
    @param host: elasticsearch host
    @param port: elasticsearch port
    @param timeout: how long to wait while connecting to es.
    @param sleep_time: time between tries of clear_index
                       defaults to 0.5 seconds
    @param clear_timeout: how long to wait for index to be cleared.
                       defualts to 5 seconds
      Will quit and throw error if not cleared. (Number of seconds)
    """
    es = elasticsearch.Elasticsearch(host=host,
                                     port=port,
                                     timeout=timeout)
    if es.indices.exists(index=index):
        print 'deleting index %s' % (index)
        es.indices.delete(index=index)
        time_start = time.time()
        while es.indices.exists(index=index):
            print 'waiting until index is deleted...'
            time.sleep(sleep_time)
            if time.time() - time_start > clear_timeout:
                raise EsTestUtilException('clear_index failed.')

    print 'successfully deleted index %s' % (index)
