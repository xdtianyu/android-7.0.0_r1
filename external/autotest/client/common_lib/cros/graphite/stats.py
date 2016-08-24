# pylint: disable-msg=C0111
# TODO: get rid of above, fix docstrings. crbug.com/273903
# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging


try:
    import statsd
except ImportError:
    import statsd_mock as statsd


# This is _type for all metadata logged to elasticsearch from here.
STATS_ES_TYPE = 'stats_metadata'


# statsd logs details about what its sending at the DEBUG level, which I really
# don't want to see tons of stats in logs, so all of these are silenced by
# setting the logging level for all of statsdto WARNING.
logging.getLogger('statsd').setLevel(logging.WARNING)


def _prepend_init(_es, _conn, _prefix):
    def wrapper(original):
        """Decorator to override __init__."""

        class _Derived(original):
            def __init__(self, name, connection=None, bare=False,
                         metadata=None):
                name = self._add_prefix(name, _prefix, bare)
                conn = connection if connection else _conn
                super(_Derived, self).__init__(name, conn)
                self.metadata = metadata
                self.es = _es

            def _add_prefix(self, name, prefix, bare=False):
                """
                Since many people run their own local AFE, stats from a local
                setup shouldn't get mixed into stats from prod.  Therefore,
                this function exists to add a prefix, nominally the name of
                the local server, if |name| doesn't already start with the
                server name, so that each person has their own "folder" of
                stats that they can look at.

                However, this functionality might not always be wanted, so we
                allow one to pass in |bare=True| to force us to not prepend
                the local server name. (I'm not sure when one would use this,
                but I don't see why I should disallow it...)

                >>> prefix = 'potato_nyc'
                >>> _add_prefix('rpc.create_job', bare=False)
                'potato_nyc.rpc.create_job'
                >>> _add_prefix('rpc.create_job', bare=True)
                'rpc.create_job'

                @param name The name to append to the server name if it
                            doesn't start with the server name.
                @param bare If True, |name| will be returned un-altered.
                @return A string to use as the stat name.

                """
                if not bare and not name.startswith(prefix):
                    name = '%s.%s' % (prefix, name)
                return name

        return _Derived
    return wrapper


class Statsd(object):
    def __init__(self, es, host, port, prefix):
        # This is the connection that we're going to reuse for every client
        # that gets created. This should maximally reduce overhead of stats
        # logging.
        self.conn = statsd.Connection(host=host, port=port)

        @_prepend_init(es, self.conn, prefix)
        class Average(statsd.Average):
            """Wrapper around statsd.Average."""

            def send(self, subname, value):
                """Sends time-series data to graphite and metadata (if any)
                to es.

                @param subname: The subname to report the data to (i.e.
                    'daisy.reboot')
                @param value: Value to be sent.
                """
                statsd.Average.send(self, subname, value)
                self.es.post(type_str=STATS_ES_TYPE, metadata=self.metadata,
                             subname=subname, value=value)

        self.Average = Average

        @_prepend_init(es, self.conn, prefix)
        class Counter(statsd.Counter):
            """Wrapper around statsd.Counter."""

            def _send(self, subname, value):
                """Sends time-series data to graphite and metadata (if any)
                to es.

                @param subname: The subname to report the data to (i.e.
                    'daisy.reboot')
                @param value: Value to be sent.
                """
                statsd.Counter._send(self, subname, value)
                self.es.post(type_str=STATS_ES_TYPE, metadata=self.metadata,
                             subname=subname, value=value)

        self.Counter = Counter

        @_prepend_init(es, self.conn, prefix)
        class Gauge(statsd.Gauge):
            """Wrapper around statsd.Gauge."""

            def send(self, subname, value):
                """Sends time-series data to graphite and metadata (if any)
                to es.

                @param subname: The subname to report the data to (i.e.
                    'daisy.reboot')
                @param value: Value to be sent.
                """
                statsd.Gauge.send(self, subname, value)
                self.es.post(type_str=STATS_ES_TYPE, metadata=self.metadata,
                             subname=subname, value=value)

        self.Gauge = Gauge

        @_prepend_init(es, self.conn, prefix)
        class Timer(statsd.Timer):
            """Wrapper around statsd.Timer."""

            # To override subname to not implicitly append 'total'.
            def stop(self, subname=''):
                statsd.Timer.stop(self, subname)


            def send(self, subname, value):
                """Sends time-series data to graphite and metadata (if any)
                to es.

                @param subname: The subname to report the data to (i.e.
                    'daisy.reboot')
                @param value: Value to be sent.
                """
                statsd.Timer.send(self, subname, value)
                self.es.post(type_str=STATS_ES_TYPE, metadata=self.metadata,
                             subname=self.name, value=value)


            def __enter__(self):
                self.start()
                return self


            def __exit__(self, exn_type, exn_value, traceback):
                if exn_type is None:
                    self.stop()

        self.Timer = Timer

        @_prepend_init(es, self.conn, prefix)
        class Raw(statsd.Raw):
            """Wrapper around statsd.Raw."""

            def send(self, subname, value, timestamp=None):
                """Sends time-series data to graphite and metadata (if any)
                to es.

                The datapoint we send is pretty much unchanged (will not be
                aggregated)

                @param subname: The subname to report the data to (i.e.
                    'daisy.reboot')
                @param value: Value to be sent.
                @param timestamp: Time associated with when this stat was sent.
                """
                statsd.Raw.send(self, subname, value, timestamp)
                self.es.post(type_str=STATS_ES_TYPE, metadata=self.metadata,
                             subname=subname, value=value, timestamp=timestamp)

        self.Raw = Raw
