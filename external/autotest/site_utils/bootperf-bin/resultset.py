# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Classes and functions for managing platform_BootPerf results.

Results from the platform_BootPerf test in the ChromiumOS autotest
package are stored as performance 'keyvals', that is, a mapping
of names to numeric values.  For each iteration of the test, one
set of keyvals is recorded.

This module currently tracks four kinds of keyval results: the boot
time results, the disk read results, the firmware time results, and
reboot time results.  These results are stored with keyval names
such as 'seconds_kernel_to_login', 'rdbytes_kernel_to_login', and
'seconds_power_on_to_kernel'.  These keyvals record an accumulated
total measured from a fixed time in the past, e.g.
'seconds_kernel_to_login' records the total seconds from kernel
startup to login screen ready.

The boot time keyval names all start with the prefix
'seconds_kernel_to_', and record time in seconds since kernel
startup.

The disk read keyval names all start with the prefix
'rdbytes_kernel_to_', and record bytes read from the boot device
since kernel startup.

The firmware keyval names all start with the prefix
'seconds_power_on_to_', and record time in seconds since CPU
power on.

The reboot keyval names are selected from a hard-coded list of
keyvals that include both some boot time and some firmware time
keyvals, plus specific keyvals keyed to record shutdown and reboot
time.

"""

import math


def _ListStats(list_):
  """Return the mean and sample standard deviation of a list.

  The returned result is float, even if the input list is full of
  integers.

  @param list_ The list over which to calculate.

  """
  sum_ = 0.0
  sumsq = 0.0
  for v in list_:
    sum_ += v
    sumsq += v * v
  n = len(list_)
  avg = sum_ / n
  var = (sumsq - sum_ * avg) / (n - 1)
  if var < 0.0:
    var = 0.0
  dev = math.sqrt(var)
  return (avg, dev)


class TestResultSet(object):
  """A set of boot time and disk usage result statistics.

  Objects of this class consist of three sets of result statistics:
  the boot time statistics, the disk statistics, and the firmware
  time statistics.

  Class TestResultsSet does not interpret or store keyval mappings
  directly; iteration results are processed by attached _KeySet
  objects, one for each of the three types of result keyval. The
  _KeySet objects are kept in a dictionary; they can be obtained
  by calling the KeySet with the name of the keyset desired.
  Various methods on the KeySet objects will calculate statistics on
  the results, and provide the raw data.

  """

  # The names of the available KeySets, to be used as arguments to
  # KeySet().
  BOOTTIME_KEYSET = "boot"
  DISK_KEYSET = "disk"
  FIRMWARE_KEYSET = "firmware"
  REBOOT_KEYSET = "reboot"
  AVAILABLE_KEYSETS = [
    BOOTTIME_KEYSET, DISK_KEYSET, FIRMWARE_KEYSET, REBOOT_KEYSET
  ]

  def __init__(self, name):
    self.name = name
    self._keysets = {
      self.BOOTTIME_KEYSET : _TimeKeySet(),
      self.DISK_KEYSET : _DiskKeySet(),
      self.FIRMWARE_KEYSET : _FirmwareKeySet(),
      self.REBOOT_KEYSET : _RebootKeySet(),
    }

  def AddIterationResults(self, runkeys):
    """Add keyval results from a single iteration.

    A TestResultSet is constructed by repeatedly calling
    AddIterationResults(), iteration by iteration.  Iteration
    results are passed in as a dictionary mapping keyval attributes
    to values.  When all iteration results have been added,
    FinalizeResults() makes the results available for analysis.

    @param runkeys The dictionary of keyvals for the iteration.

    """

    for keyset in self._keysets.itervalues():
      keyset.AddIterationResults(runkeys)

  def FinalizeResults(self):
    """Make results available for analysis.

    A TestResultSet is constructed by repeatedly feeding it results,
    iteration by iteration.  Iteration results are passed in as a
    dictionary mapping keyval attributes to values.  When all iteration
    results have been added, FinalizeResults() makes the results
    available for analysis.

    """

    for keyset in self._keysets.itervalues():
      keyset.FinalizeResults()

  def KeySet(self, keytype):
    """Return a selected keyset from the test results.

    @param keytype Selector for the desired keyset.

    """
    return self._keysets[keytype]


class _KeySet(object):
  """Container for a set of related statistics.

  _KeySet is an abstract superclass for containing collections of
  a related set of performance statistics.  Statistics are stored
  as a dictionary (`_keyvals`) mapping keyval names to lists of
  values.  The lists are indexed by the iteration number.

  The mapped keyval names are shortened by stripping the prefix
  that identifies the type of keyval (keyvals that don't start with
  the proper prefix are ignored).  So, for example, with boot time
  keyvals, 'seconds_kernel_to_login' becomes 'login' (and
  'rdbytes_kernel_to_login' is ignored).

  A list of all valid keyval names is stored in the `markers`
  instance variable.  The list is sorted by the ordering of the
  average of the corresponding values.  Each iteration is required
  to contain the same set of keyvals.  This is enforced in
  FinalizeResults() (see below).

  """

  def __init__(self):
    self._keyvals = {}

  def _CheckCounts(self):
    """Check the validity of the keyvals results dictionary.

    Each keyval must have occurred the same number of times.  When
    this check succeeds, it returns the total number of occurrences;
    on failure return `None`.

    """
    check = map(len, self._keyvals.values())
    if not check:
      return None
    for i in range(1, len(check)):
      if check[i] != check[i-1]:
        return None
    return check[0]

  def AddIterationResults(self, runkeys):
    """Add results for one iteration.

    @param runkeys The dictionary of keyvals for the iteration.
    """
    for key, value in runkeys.iteritems():
      if not key.startswith(self.PREFIX):
        continue
      shortkey = key[len(self.PREFIX):]
      keylist = self._keyvals.setdefault(shortkey, [])
      keylist.append(self._ConvertVal(value))

  def FinalizeResults(self):
    """Finalize this object's results.

    This method makes available the `markers` and `num_iterations`
    instance variables.  It also ensures that every keyval occurred
    in every iteration by requiring that all keyvals have the same
    number of data points.

    """
    count = self._CheckCounts()
    if count is None:
      self.num_iterations = 0
      self.markers = []
      return False
    self.num_iterations = count
    keylist = map(lambda k: (sum(self._keyvals[k]), k),
                  self._keyvals.keys())
    keylist.sort(key=lambda tp: tp[0])
    self.markers = map(lambda tp: tp[1], keylist)
    return True

  def RawData(self, key):
    """Return the list of values for the given key.

    @param key Key of the list of values to return.

    """
    return self._keyvals[key]

  def DeltaData(self, key0, key1):
    """Return the vector difference between two keyvals lists.

    @param key0 Key of the subtrahend vector.
    @param key1 Key of the subtractor vector.

    """
    return map(lambda a, b: b - a,
               self._keyvals[key0],
               self._keyvals[key1])

  def Statistics(self, key):
    """Return the average and standard deviation for a key.

    @param key
    """
    return _ListStats(self._keyvals[key])

  def DeltaStatistics(self, key0, key1):
    """Return the average and standard deviation between two keys.

    Calculates the difference between each matching element in the
    two key's lists, and returns the average and sample standard
    deviation of the differences.

    @param key0 Key of the subtrahend.
    @param key1 Key of the subtractor.

    """
    return _ListStats(self.DeltaData(key0, key1))


class _TimeKeySet(_KeySet):
  """Concrete subclass of _KeySet for boot time statistics."""

  PREFIX = 'seconds_kernel_to_'

  # Time-based keyvals are reported in seconds and get converted to
  # milliseconds
  TIME_SCALE = 1000

  def _ConvertVal(self, value):
    """Return a keyval value in its 'canonical' form.

    For boot time values, the input is seconds as a float; the
    canonical form is milliseconds as an integer.

    @param value A time statistic in seconds.

    """
    # We want to return the nearest exact integer here.  round()
    # returns a float, and int() truncates its results, so we have
    # to combine them.
    return int(round(self.TIME_SCALE * float(value)))

  def PrintableStatistic(self, value):
    """Return a keyval in its preferred form for printing.

    The return value is a tuple of a string to be printed, and
    value rounded to the precision to which it was printed.

    Rationale: Some callers of this function total up intermediate
    results.  Returning the rounded value makes those totals more
    robust against visible rounding anomalies.

    @param value The value to be printed.

    """
    v = int(round(value))
    return ("%d" % v, v)


class _FirmwareKeySet(_TimeKeySet):
  """Concrete subclass of _KeySet for firmware time statistics."""

  PREFIX = 'seconds_power_on_to_'

  # Time-based keyvals are reported in seconds and get converted to
  # milliseconds
  TIME_SCALE = 1000


class _RebootKeySet(_TimeKeySet):
  """Concrete subclass of _KeySet for reboot time statistics."""

  PREFIX = ''

  # Time-based keyvals are reported in seconds and get converted to
  # milliseconds
  TIME_SCALE = 1000

  def AddIterationResults(self, runkeys):
    """Add results for one iteration.

    For _RebootKeySet, we cherry-pick and normalize a hard-coded
    list of keyvals.

    @param runkeys The dictionary of keyvals for the iteration.
    """
    # The time values we report are calculated as the time from when
    # shutdown was requested.  However, the actual keyvals from the
    # test are reported, variously, as "time from shutdown request",
    # "time from power-on", and "time from kernel start".  So,
    # the values have to be normalized to a common time line.
    #
    # The keyvals below capture the time from shutdown request of
    # the _end_ of a designated phase of reboot, as follows:
    #   shutdown - end of shutdown, start of firmware power-on
    #       sequence.
    #   firmware - end of firmware, transfer to kernel.
    #   startup - end of kernel initialization, Upstart's "startup"
    #       event.
    #   chrome_exec - session_manager initialization complete,
    #       Chrome starts running.
    #   login - Chrome completes initialization of the login screen.
    #
    shutdown = float(runkeys["seconds_shutdown_time"])
    firmware_time = float(runkeys["seconds_power_on_to_kernel"])
    startup = float(runkeys["seconds_kernel_to_startup"])
    chrome_exec = float(runkeys["seconds_kernel_to_chrome_exec"])
    reboot = float(runkeys["seconds_reboot_time"])
    newkeys = {}
    newkeys["shutdown"] = shutdown
    newkeys["firmware"] = shutdown + firmware_time
    newkeys["startup"] = newkeys["firmware"] + startup
    newkeys["chrome_exec"] = newkeys["firmware"] + chrome_exec
    newkeys["login"] = reboot
    super(_RebootKeySet, self).AddIterationResults(newkeys)


class _DiskKeySet(_KeySet):
  """Concrete subclass of _KeySet for disk read statistics."""

  PREFIX = 'rdbytes_kernel_to_'

  # Disk read keyvals are reported in bytes and get converted to
  # MBytes (1 MByte = 1 million bytes, not 2**20)
  DISK_SCALE = 1.0e-6

  def _ConvertVal(self, value):
    """Return a keyval value in its 'canonical' form.

    For disk statistics, the input is bytes as a float; the
    canonical form is megabytes as a float.

    @param value A disk data statistic in megabytes.

    """
    return self.DISK_SCALE * float(value)

  def PrintableStatistic(self, value):
    """Return a keyval in its preferred form for printing.

    The return value is a tuple of a string to be printed, and
    value rounded to the precision to which it was printed.

    Rationale: Some callers of this function total up intermediate
    results.  Returning the rounded value makes those totals more
    robust against visible rounding anomalies.

    @param value The value to be printed.

    """
    v = round(value, 1)
    return ("%.1fM" % v, v)
