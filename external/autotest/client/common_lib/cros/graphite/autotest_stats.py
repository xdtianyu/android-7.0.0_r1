# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import socket

import common
from autotest_lib.client.common_lib import global_config
from autotest_lib.client.common_lib.cros.graphite import autotest_es
from autotest_lib.client.common_lib.cros.graphite import es_utils
from autotest_lib.client.common_lib.cros.graphite import stats


# Pylint locally complains about "No value passed for parameter 'key'" here
# pylint: disable=E1120
# If one has their hostname listed including a domain, ie. |milleral.mtv|,
# then this will show up on Graphite as milleral/mtv/<stats>.  This seems
# silly, so let's replace '.'s with '_'s to disambiguate Graphite folders
# from FQDN hostnames.
STATSD_SERVER = global_config.global_config.get_config_value('CROS',
        'STATSD_SERVER', default='')
STATSD_PORT = global_config.global_config.get_config_value('CROS',
        'STATSD_PORT', type=int, default=0)
hostname = global_config.global_config.get_config_value(
        'SERVER', 'hostname', default='localhost')

if hostname.lower() in ['localhost', '127.0.0.1']:
    hostname = socket.gethostname()
hostname = hostname.replace('.', '_')

_default_es = es_utils.ESMetadata(use_http=autotest_es.ES_USE_HTTP,
                                  host=autotest_es.METADATA_ES_SERVER,
                                  port=autotest_es.ES_PORT,
                                  index=autotest_es.INDEX_METADATA,
                                  udp_port=autotest_es.ES_UDP_PORT)
_statsd = stats.Statsd(es=_default_es, host=STATSD_SERVER, port=STATSD_PORT,
                       prefix=hostname)


def _es_init(original):
    class _Derived(original):
        def __init__(self, *args, **kwargs):
            es = kwargs.pop('es', None)
            super(_Derived, self).__init__(*args, **kwargs)
            if es:
                self.es = es
    return _Derived


@_es_init
class Average(_statsd.Average):
    """Wrapper around _statsd.Average"""

@_es_init
class Counter(_statsd.Counter):
    """Wrapper around _statsd.Counter"""

@_es_init
class Gauge(_statsd.Gauge):
    """Wrapper around _statsd.Gauge"""

@_es_init
class Timer(_statsd.Timer):
    """Wrapper around _statsd.Timer"""

@_es_init
class Raw(_statsd.Raw):
    """Wrapper around _statd.Raw"""
