# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import os
import time
from autotest_lib.client.bin import test
from autotest_lib.client.common_lib import error, utils


def FindDriver(ifname):
  """Finds the driver associated with network interface.

  @param ifname Interface name
  @return String containing the kernel driver name for this interface
  """

  driver_file = '/sys/class/net/%s/device/driver/module' % ifname
  if os.path.exists(driver_file):
    return os.path.basename(os.readlink(driver_file))


def GetInterfaceList():
  """Gets the list of network interfaces on this host.

  @return List containing a string for each interface name
  """
  return os.listdir('/sys/class/net')


def FindInterface(typelist=('wlan','mlan','eth')):
  """Finds an interface that we can unload the driver for.

  Retrieves a dict containing the name of a network interface
  that can quite likely be removed using the "rmmod" command,
  and the name of the module used to load the driver.

  @param typelist An iterable of interface prefixes to filter from. Only
                  return an interface that matches one of these prefixes
  @return Dict containing a 'intf' key with the interface name
          and a 'wlan' key with the kernel module name for the driver.

  """
  interface_list = GetInterfaceList()
  # Slice through the interfaces on a per-prefix basis priority order.
  for prefix in typelist:
    for intf in interface_list:
      if intf.startswith(prefix):
        driver = FindDriver(intf)
        if driver is not None:
          return {'intf': intf, 'driver': driver}

  logging.debug('Could not find an interface')


def RestartInterface():
  """Find and restart a network interface using "rmmod" and "modprobe".

  This function simulates a device eject and re-insert.

  @return True if successful, or if nothing was done
  """
  interface = FindInterface()
  if interface is None:
    logging.debug('No interface available for test')
    # We return success although we haven't done anything!
    return True

  logging.debug('Using %s for restart', str(interface))

  try:
    utils.system('rmmod %s' % interface['driver'])
  except error.CmdError, e:
    logging.debug(e)

  try:
    utils.system('modprobe %s' % interface['driver'])
  except error.CmdError, e:
    logging.debug(e)
    raise error.TestFail('Failed to reload driver %s' % interface['driver'])

  return True


def Upstart(service, action='status'):
  """Front-end to the 'initctl' command.

  Accepts arguments to initctl and executes them, raising an exception
  if it fails.

  @param service Service name to call initctl with.
  @param action Action to perform on the service

  @return The returned service status from initctl
  """
  if action not in ('status', 'start', 'stop'):
    logging.debug('Bad action')
    return None

  try:
    status_str = utils.system_output('initctl %s %s' % (action, service))
    status_list = status_str.split(' ')
  except error.CmdError, e:
    logging.debug(e)
    raise error.TestFail('Failed to perform %s on service %s' %
                         (action, service))

  if status_list[0] != service:
    return None

  return status_list[1].rstrip(',\n')


def RestartUdev():
  """Restarts the udev service.

  Stops and then restarts udev

  @return True if successful
  """
  if Upstart('udev') != 'start/running':
    raise error.TestFail('udev not running')

  if Upstart('udev', 'stop') != 'stop/waiting':
    raise error.TestFail('could not stop udev')

  if Upstart('udev', 'start') != 'start/running':
    raise error.TestFail('could not restart udev')

  if Upstart('udev') != 'start/running':
    raise error.TestFail('udev failed to stay running')

  return True


def TestUdevDeviceList(restart_fn):
  """Test interface list.

  Performs an operation, then compares the network interface list between
  a time before the test and after.  Raises an exception if the list changes.

  @param restart_fn The function that performs the operation of interest
  """
  iflist_pre = GetInterfaceList()
  if not restart_fn():
    raise error.TestFail('Reset function failed')

  # Debugging for crbug.com/418983,423741,424605,425066 added the loop to see
  # if it takes more than 3 attempts for all of the interfaces to come back up.
  for i in range(3):
    # We need to wait for udev to rename (or not) the interface!
    time.sleep(10)

    iflist_post = GetInterfaceList()

    if iflist_post == iflist_pre:
      logging.debug('Interfaces remain the same after %s; number of tries: %d',
                    restart_fn.__name__, i)
      return

  raise error.TestFail('Interfaces changed after %s (%s != %s)' %
                       (restart_fn.__name__, str(iflist_pre),
                        str(iflist_post)))


class network_UdevRename(test.test):
  """Test that network devices are not renamed unexpectedly"""
  version = 1

  def run_once(self):
    TestUdevDeviceList(RestartUdev)
    TestUdevDeviceList(RestartInterface)
