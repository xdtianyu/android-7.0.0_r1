# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# This file defines helper functions for putting entries into elasticsearch.

"""Utils for sending metadata to elasticsearch

Elasticsearch is a key-value store NOSQL database.
Source is here: https://github.com/elasticsearch/elasticsearch
We will be using es to store our metadata.

For example, if we wanted to store the following metadata:

metadata = {
    'host_id': 1
    'job_id': 20
    'time_start': 100000
    'time_recorded': 100006
}

The following call will send metadata to the default es server.
    autotest_es.post(index, metadata)
We can also specify which port and host to use.

Using for testing: Sometimes, when we choose a single index
to put entries into, we want to clear that index of all
entries before running our tests. Use clear_index function.
(see es_utils_functionaltest.py for an example)

This file also contains methods for sending queries to es. Currently,
the query (json dict) we send to es is quite complicated (but flexible).

For example, the below query returns job_id, host_id, and job_start
for all job_ids in [0, 99999] and host_id matching 10.

range_eq_query = {
    'fields': ['job_id', 'host_id', 'job_start'],
    'query': {
        'filtered': {
            'query': {
                'match': {
                    'host_id': 10,
                }
            }
            'filter': {
                'range': {
                    'job_id': {
                        'gte': 0,
                        'lte': 99999,
                    }
                }
            }
        }
    }
}

To send a query once it is created, call execute_query() to send it to the
intended elasticsearch server. The query() function can be used to construct a
query with certain parameters and execute it all in one call.

"""

import es_utils

import common
from autotest_lib.client.common_lib import global_config


# Server and ports for elasticsearch (for metadata use only)
METADATA_ES_SERVER = global_config.global_config.get_config_value(
        'CROS', 'ES_HOST', default='localhost')
ES_PORT = global_config.global_config.get_config_value(
        'CROS', 'ES_PORT', type=int, default=9200)
ES_UDP_PORT = global_config.global_config.get_config_value(
        'CROS', 'ES_UDP_PORT', type=int, default=9700)
# Whether to use http. udp is very little overhead (around 3 ms) compared to
# using http (tcp) takes ~ 500 ms for the first connection and 50-100ms for
# subsequent connections.
ES_USE_HTTP = global_config.global_config.get_config_value(
        'CROS', 'ES_USE_HTTP', type=bool, default=False)

# If CLIENT/metadata_index is not set, INDEX_METADATA falls back to
# autotest instance name (SERVER/hostname).
INDEX_METADATA = global_config.global_config.get_config_value(
        'CLIENT', 'metadata_index', type=str, default=None)
if not INDEX_METADATA:
    INDEX_METADATA = global_config.global_config.get_config_value(
            'SERVER', 'hostname', type=str, default='localhost')

# 3 Seconds before connection to esdb timeout.
DEFAULT_TIMEOUT = 3

DEFAULT_BULK_POST_RETRIES = 5

def post(use_http=ES_USE_HTTP, host=METADATA_ES_SERVER, port=ES_PORT,
         timeout=DEFAULT_TIMEOUT, index=INDEX_METADATA, udp_port=ES_UDP_PORT,
         *args, **kwargs):
    """This function takes a series of arguments which are passed to the
    es_utils.ESMetadata constructor, and any other arguments are passed to
    its post() function. For an explanation of each, see those functions in
    es_utils.
    """
    esmd = es_utils.ESMetadata(use_http=use_http, host=host, port=port,
                               timeout=timeout, index=index, udp_port=udp_port)
    return esmd.post(*args, **kwargs)


def bulk_post(data_list, host=METADATA_ES_SERVER, port=ES_PORT,
              timeout=DEFAULT_TIMEOUT, index=INDEX_METADATA,
              retries=DEFAULT_BULK_POST_RETRIES, *args, **kwargs):
    """This function takes a series of arguments which are passed to the
    es_utils.ESMetadata constructor, and a list of metadata, then upload to
    Elasticsearch server using Elasticsearch bulk API. This can greatly nhance
    the performance of uploading data using HTTP.
    For an explanation of each argument, see those functions in es_utils.
    """
    esmd = es_utils.ESMetadata(use_http=True, host=host, port=port,
                               timeout=timeout, index=index,
                               udp_port=ES_UDP_PORT)
    # bulk post may fail due to the amount of data, retry several times.
    for _ in range(retries):
        if esmd.bulk_post(data_list, *args, **kwargs):
            return True
    return False


def execute_query(host=METADATA_ES_SERVER, port=ES_PORT,
                  timeout=DEFAULT_TIMEOUT, index=INDEX_METADATA,
                  *args, **kwargs):
    """This function takes a series of arguments which are passed to the
    es_utils.ESMetadata constructor, and any other arguments are passed to
    its execute_query() function. For an explanation of each, see those
    functions in es_utils.
    """
    esmd = es_utils.ESMetadata(use_http=True, host=host, port=port,
                               timeout=timeout, index=index, udp_port=0)
    return esmd.execute_query(*args, **kwargs)


def query(host=METADATA_ES_SERVER, port=ES_PORT, timeout=DEFAULT_TIMEOUT,
          index=INDEX_METADATA, *args, **kwargs):
    """This function takes a series of arguments which are passed to the
    es_utils.ESMetadata constructor, and any other arguments are passed to
    its query() function. For an explanation of each, see those functions in
    es_utils.
    """
    esmd = es_utils.ESMetadata(use_http=True, host=host, port=port,
                               timeout=timeout, index=index, udp_port=0)
    return esmd.query(*args, **kwargs)
