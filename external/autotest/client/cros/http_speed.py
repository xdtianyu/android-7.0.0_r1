# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""A tool to measure single-stream link bandwidth using HTTP connections."""

import logging, random, time, urllib2

import numpy.random

TIMEOUT = 90


class Error(Exception):
  pass


def TimeTransfer(url, data):
    """Transfers data to/from url.  Returns (time, url contents)."""
    start_time = time.time()
    result = urllib2.urlopen(url, data=data, timeout=TIMEOUT)
    got = result.read()
    transfer_time = time.time() - start_time
    if transfer_time <= 0:
        raise Error("Transfer of %s bytes took nonsensical time %s"
                    % (url, transfer_time))
    return (transfer_time, got)


def TimeTransferDown(url_pattern, size):
    url = url_pattern % {'size': size}
    (transfer_time, got) = TimeTransfer(url, data=None)
    if len(got) != size:
      raise Error('Got %d bytes, expected %d' % (len(got), size))
    return transfer_time


def TimeTransferUp(url, size):
    """If size > 0, POST size bytes to URL, else GET url.  Return time taken."""
    data = numpy.random.bytes(size)
    (transfer_time, _) = TimeTransfer(url, data)
    return transfer_time


def BenchmarkOneDirection(latency, label, url, benchmark_function):
    """Transfer a reasonable amount of data and record the speed.

    Args:
        latency:  Time for a 1-byte transfer
        label:  Label to add to perf keyvals
        url:  URL (or pattern) to transfer at
        benchmark_function:  Function to perform actual transfer
    Returns:
        Key-value dictionary, suitable for reporting to write_perf_keyval.
        """

    size = 1 << 15              # Start with a small download
    maximum_size = 1 << 24      # Go large, if necessary
    multiple = 1

    remaining = 2
    transfer_time = 0

    # Long enough that startup latency shouldn't dominate.
    target = max(20 * latency, 10)
    logging.info('Target time: %s' % target)

    while remaining > 0:
        size = min(int(size * multiple), maximum_size)
        transfer_time = benchmark_function(url, size)
        logging.info('Transfer of %s took %s (%s b/s)'
                     % (size, transfer_time, 8 * size / transfer_time))
        if transfer_time >= target:
            break
        remaining -= 1

        # Take the latency into account when guessing a size for a
        # larger transfer.  This is a pretty simple model, but it
        # appears to work.
        adjusted_transfer_time = max(transfer_time - latency, 0.01)
        multiple = target / adjusted_transfer_time

    if remaining == 0:
        logging.warning(
            'Max size transfer still took less than minimum desired time %s'
            % target)

    return {'seconds_%s_fetch_time' % label: transfer_time,
            'bytes_%s_bytes_transferred' % label: size,
            'bits_second_%s_speed' % label: 8 * size / transfer_time,
            }


def HttpSpeed(download_url_format_string,
              upload_url):
    """Measures upload and download performance to the supplied URLs.

    Args:
        download_url_format_string:  URL pattern with %(size) for payload bytes
        upload_url:  URL that accepts large POSTs
    Returns:
        A dict of perf_keyval
    """
    # We want the download to be substantially longer than the
    # one-byte fetch time that we can isolate bandwidth instead of
    # latency.
    latency = TimeTransferDown(download_url_format_string, 1)

    logging.info('Latency is %s'  % latency)

    down = BenchmarkOneDirection(
        latency,
        'downlink',
        download_url_format_string,
        TimeTransferDown)

    up = BenchmarkOneDirection(
        latency,
        'uplink',
        upload_url,
        TimeTransferUp)

    up.update(down)
    return up
