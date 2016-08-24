# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license
# that can be found in the LICENSE file.

import argparse
import logging
import mmap
import os
import signal
import struct
import sys
import threading
import time

# some magic numbers: see http://goo.gl/ecAgke for Intel docs
PCI_IMC_BAR_OFFSET = 0x48
IMC_DRAM_GT_REQUESTS = 0x5040 # GPU
IMC_DRAM_IA_REQUESTS = 0x5044 # CPU
IMC_DRAM_IO_REQUESTS = 0x5048 # PCIe, Display Engine, USB, etc.
IMC_DRAM_DATA_READS = 0x5050  # read traffic
IMC_DRAM_DATA_WRITES = 0x5054 # write traffic
IMC_MMAP_SIZE = 0x6000

CACHE_LINE = 64.0
MEGABYTE = 1048576.0

RATE_FIELD_FORMAT = '%s: %5d MB/s'
RAW_FIELD_FORMAT = '%s: %d'

class IMCCounter:
    """Small struct-like class to keep track of the
    location and attributes for each counter.

    Parameters:
      name: short, unique identifying token for this
        counter type
      idx: offset into the IMC memory where we can find
        this counter
      total: True if we should count this in the number
        for total bandwidth
    """
    def __init__(self, name, idx, total):
        self.name = name
        self.idx = idx
        self.total = total


counters = [
#              name          idx           total
    IMCCounter("GT", IMC_DRAM_GT_REQUESTS, False),
    IMCCounter("IA", IMC_DRAM_IA_REQUESTS, False),
    IMCCounter("IO", IMC_DRAM_IO_REQUESTS, False),
    IMCCounter("RD", IMC_DRAM_DATA_READS,  True),
    IMCCounter("WR", IMC_DRAM_DATA_WRITES, True),
]


class MappedFile:
    """Helper class to wrap mmap calls in a context
    manager so they are always cleaned up, and to
    help extract values from the bytes.

    Parameters:
      filename: name of file to mmap
      offset: offset from beginning of file to mmap
        from
      size: amount of the file to mmap
    """
    def __init__(self, filename, offset, size):
        self._filename = filename
        self._offset = offset
        self._size = size


    def __enter__(self):
        self._f = open(self._filename, 'rb')
        try:
            self._mm = mmap.mmap(self._f.fileno(),
                                 self._size,
                                 mmap.MAP_SHARED,
                                 mmap.PROT_READ,
                                 offset=self._offset)
        except mmap.error:
            self._f.close()
            raise
        return self


    def __exit__(self, exc_type, exc_val, exc_tb):
        self._mm.close()
        self._f.close()


    def bytes_to_python(self, offset, fmt):
        """Grab a portion of an mmapped file and return the bytes
        as a python object.

        Parameters:
          offset: offset into the mmapped file to start at
          fmt: string containing the struct type to extract from the
            file
        Returns: a Struct containing the bytes starting at offset
          into the mmapped file, reified as python values
        """
        s = struct.Struct(fmt)
        return s.unpack(self._mm[offset:offset+s.size])


def file_bytes_to_python(f, offset, fmt):
    """Grab a portion of a regular file and return the bytes
    as a python object.

    Parameters:
      f: file-like object to extract from
      offset: offset into the mmapped file to start at
      fmt: string containing the struct type to extract from the
        file
    Returns: a Struct containing the bytes starting at offset into
      f, reified as python values
    """
    s = struct.Struct(fmt)
    f.seek(0)
    bs = f.read()
    if len(bs) >= offset + s.size:
        return s.unpack(bs[offset:offset+s.size])
    else:
        raise IOError('Invalid seek in file')


def uint32_diff(l, r):
    """Compute the difference of two 32-bit numbers as
    another 32-bit number.

    Since the counters are monotonically increasing, we
    always want the unsigned difference.
    """
    return l - r if l >= r else l - r + 0x100000000


class MemoryBandwidthLogger(threading.Thread):
    """Class for gathering memory usage in MB/s on x86 systems.
    raw: dump raw counter values
    seconds_period: time period between reads

    If you are using non-raw mode and your seconds_period is
    too high, your results might be nonsense because the counters
    might have wrapped around.

    Parameters:
      raw: True if you want to dump raw counters. These will simply
        tell you the number of cache-line-size transactions that
        have occurred so far.
      seconds_period: Duration to wait before dumping counters again.
        Defaults to 2 seconds.
      """
    def __init__(self, raw, seconds_period=2):
        super(MemoryBandwidthLogger, self).__init__()
        self._raw = raw
        self._seconds_period = seconds_period
        self._running = True


    def run(self):
        # get base address register and align to 4k
        try:
            bar_addr = self._get_pci_imc_bar()
        except IOError:
            logging.error('Cannot read base address register')
            return
        bar_addr = (bar_addr // 4096) * 4096

        # set up the output formatting. raw counters don't have any
        # particular meaning in MB/s since they count how many cache
        # lines have been read from or written to up to that point,
        # and so don't represent a rate.
        # TOTAL is always given as a rate, though.
        rate_factor = CACHE_LINE / (self._seconds_period * MEGABYTE)
        if self._raw:
            field_format = RAW_FIELD_FORMAT
        else:
            field_format = RATE_FIELD_FORMAT

        # get /dev/mem and mmap it
        with MappedFile('/dev/mem', bar_addr, IMC_MMAP_SIZE) as mm:
            # take initial samples, then take samples every seconds_period
            last_values = self._take_samples(mm)
            while self._running:
                time.sleep(self._seconds_period)
                values = self._take_samples(mm)
                # we need to calculate the MB differences no matter what
                # because the "total" field uses it even when we are in
                # raw mode
                mb_diff = { c.name:
                    uint32_diff(values[c.name], last_values[c.name])
                        * rate_factor for c in counters }
                output_dict = values if self._raw else mb_diff
                output = list((c.name, output_dict[c.name]) for c in counters)

                total_rate = sum(mb_diff[c.name] for c in counters if c.total)
                output_str = \
                    ' '.join(field_format % (k, v) for k, v in output) + \
                    ' ' + (RATE_FIELD_FORMAT % ('TOTAL', total_rate))

                logging.debug(output_str)
                last_values = values


    def stop(self):
        self._running = False


    def _get_pci_imc_bar(self):
        """Get the base address register for the IMC (integrated
        memory controller). This is later used to extract counter
        values.

        Returns: physical address for the IMC.
        """
        with open('/proc/bus/pci/00/00.0', 'rb') as pci:
            return file_bytes_to_python(pci, PCI_IMC_BAR_OFFSET, '=Q')[0]


    def _take_samples(self, mm):
        """Get samples for each type of memory transaction.

        Parameters:
          mm: MappedFile representing physical memory
        Returns: dictionary mapping counter type to counter value
        """
        return { c.name: mm.bytes_to_python(c.idx, '=I')[0]
            for c in counters }
