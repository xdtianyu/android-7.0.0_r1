#!/usr/bin/python
#
# Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Updates all unlocked hosts in Autotest lab in parallel at a given rate.

Used to update all hosts, or only those of a given platform, in the Autotest
lab to a given version. Allows a configurable number of updates to be started in
parallel. Updates can also be staggered to reduce load."""

import logging
import os
import subprocess
import sys
import threading
import time
import traceback

from collections import deque
from optparse import OptionParser


# Default number of hosts to update in parallel.
DEFAULT_CONCURRENCY = 10


# By default do not stagger any of the updates.
DEFAULT_STAGGER = 0


# Default location of ChromeOS checkout.
DEFAULT_GCLIENT_ROOT = '/usr/local/google/home/${USER}/chromeos/chromeos'


# Default path for individual host logs. Each host will have it's own file. E.g.
# <default_log_path>/<host>.log
DEFAULT_LOG_PATH = '/tmp/mass_update_logs/%s/' % time.strftime('%Y-%m-%d-%H-%M',
                                                               time.gmtime())


# Location of Autotest cli executable.
AUTOTEST_LOCATION = '/home/chromeos-test/autotest/cli'


# Default time in seconds to sleep while waiting for threads to complete.
DEFAULT_SLEEP = 10


# Amount of time in seconds to wait before declaring an update as failed.
DEFAULT_TIMEOUT = 2400


class MassUpdateStatus():
  """Used to track status for all updates."""
  ssh_failures = []
  update_failures = []
  successful_updates = 0


class UpdateThread(threading.Thread):
  """Responsible for ssh-test, locking, imaging, and unlocking a host.

  Uses the atest CLI for host control and the image_to_live script to actually
  update the host. Each thread will continue to process hosts until the queue
  is empty."""

  _SUCCESS = 0            # Update was successful.
  _SSH_FAILURE = 1        # Could not SSH to host or related SSH failure.
  _UPDATE_FAILURE = 2     # Update failed for any reason other than SSH.

  def __init__(self, options, hosts, status):
    self._options = options
    self._hosts = hosts
    self._status = status
    self._logger = logging.getLogger()
    threading.Thread.__init__(self)

  def run(self):
    while self._hosts:
      host = self._hosts.popleft()
      status = UpdateThread._UPDATE_FAILURE

      self._logger.info('Updating host %s' % host)
      try:
        try:
          if not CheckSSH(host=host, options=self._options):
            status = UpdateThread._SSH_FAILURE
          elif LockHost(host) and ImageHost(host=host, options=self._options):
            status = UpdateThread._SUCCESS
        finally:
          if status == UpdateThread._SUCCESS:
            self._logger.info(
                'Completed update for host %s successfully.' % host)
            self._status.successful_updates += 1
          elif status == UpdateThread._SSH_FAILURE:
            self._logger.info('Failed to SSH to host %s.' % host)
            self._status.ssh_failures.append(host)
          else:
            self._logger.info('Failed to update host %s.' % host)
            self._status.update_failures.append(host)

          UnlockHost(host)
      except:
        traceback.print_exc()
        self._logger.warning(
            'Exception encountered during update. Skipping host %s.' % host)


def CheckSSH(host, options):
  """Uses the ssh_test script to ensure SSH access to a host is available.

  Returns true if an SSH connection to the host was successful."""
  return subprocess.Popen(
      '%s/src/scripts/ssh_test.sh --remote=%s' % (options.gclient, host),
      shell=True,
      stdout=subprocess.PIPE,
      stderr=subprocess.PIPE).wait() == 0


def ImageHost(host, options):
  """Uses the image_to_live script to update a host.

  Returns true if the imaging process was successful."""
  log_file = open(os.path.join(options.log, host + '.log'), 'w')
  log_file_err = open(os.path.join(options.log, host + '.log.err'), 'w')

  exit_code = subprocess.Popen(
      ('/usr/local/scripts/alarm %d %s/src/scripts/image_to_live.sh '
       '--update_url %s --remote %s' % (DEFAULT_TIMEOUT, options.gclient,
                                        options.url, host)),
      shell=True,
      stdout=log_file,
      stderr=log_file_err).wait()

  log_file.close()
  log_file_err.close()

  return exit_code == 0


def LockHost(host):
  """Locks a host using the atest CLI.

  Locking a host tells Autotest that the host shouldn't be scheduled for
  any other tasks. Returns true if the locking process was successful."""
  success = subprocess.Popen(
      '%s/atest host mod -l %s' % (AUTOTEST_LOCATION, host),
      shell=True,
      stdout=subprocess.PIPE,
      stderr=subprocess.PIPE).wait() == 0

  if not success:
    logging.getLogger().info('Failed to lock host %s.' % host)

  return success


def UnlockHost(host):
  """Unlocks a host using the atest CLI.

  Unlocking a host tells Autotest that the host is okay to be scheduled
  for other tasks. Returns true if the unlocking process was successful."""
  success = subprocess.Popen(
      '%s/atest host mod -u %s' % (AUTOTEST_LOCATION, host),
      shell=True,
      stdout=subprocess.PIPE,
      stderr=subprocess.PIPE).wait() == 0

  if not success:
    logging.getLogger().info('Failed to unlock host %s.' % host)

  return success


def GetHostQueue(options):
  """Returns a queue containing unlocked hosts retrieved from the atest CLI.

  If options.label has been specified only unlocked hosts with the specified
  label will be returned."""
  cmd = ('%s/atest host list --unlocked -s Ready -a acl_cros_test'
         % AUTOTEST_LOCATION)

  if options.label:
    cmd += ' -b ' + options.label

  # atest host list will return a tabular data set. Use sed to remove the first
  # line which contains column labels we don't need. Then since the first column
  # contains the host name, use awk to extract it
  cmd += " | sed '1d' | awk '{print $1}'"

  stdout = subprocess.Popen(cmd,
                            shell=True,
                            stdout=subprocess.PIPE,
                            stderr=subprocess.PIPE).communicate()[0]

  return deque(item.strip() for item in stdout.split('\n') if item.strip())


def ParseOptions():
  usage = 'usage: %prog --url=<update url> [options]'
  parser = OptionParser(usage)
  parser.add_option('-b', '--label', dest='label',
                    help='Only update hosts with the specified label.')
  parser.add_option('-c', '--concurrent', dest='concurrent',
                    default=DEFAULT_CONCURRENCY,
                    help=('Number of hosts to be updated concurrently. '
                          'Defaults to %d hosts.') % DEFAULT_CONCURRENCY)
  parser.add_option('-g', '--gclient', dest='gclient',
                    default=DEFAULT_GCLIENT_ROOT,
                    help=('Location of ChromeOS checkout. defaults to %s'
                    % DEFAULT_GCLIENT_ROOT))
  parser.add_option('-l', '--log', dest='log',
                    default=DEFAULT_LOG_PATH,
                    help=('Where to put individual host log files. '
                          'Defaults to %s' % DEFAULT_LOG_PATH))
  parser.add_option('-s', '--stagger', dest='stagger',
                    default=DEFAULT_STAGGER,
                    help=('Attempt to stagger updates. Waits the given amount '
                          'of time in minutes before starting each updater. '
                          'Updates will still overlap if the value is set as a '
                          'multiple of the update period.'))
  parser.add_option('-u', '--url', dest='url',
                    help='Update URL. Points to build for updating hosts.')

  options = parser.parse_args()[0]

  if options.url is None:
    parser.error('An update URL must be provided.')

  return options


def InitializeLogging():
  """Configure the global logger for time/date stamping console output.

  Returns a logger object for convenience."""
  logger = logging.getLogger()
  logger.setLevel(logging.INFO)

  stream_handler = logging.StreamHandler()
  stream_handler.setLevel(logging.INFO)
  stream_handler.setFormatter(logging.Formatter('%(asctime)s - %(message)s'))
  logger.addHandler(stream_handler)
  return logger


def main():
  options = ParseOptions()
  hosts = GetHostQueue(options)
  logger = InitializeLogging()
  status = MassUpdateStatus()

  # Create log folder if it doesn't exist.
  if not os.path.exists(options.log):
    os.makedirs(options.log)

  logger.info('Starting update using URL %s' % options.url)
  logger.info('Individual host logs can be found under %s' % options.log)

  try:
    # Spawn processing threads which will handle lock, update, and unlock.
    for i in range(int(options.concurrent)):
      UpdateThread(hosts=hosts, options=options, status=status).start()

      # Stagger threads if the option has been enabled.
      if options.stagger > 0:
        time.sleep(int(options.stagger) * 60)

    # Wait for all hosts to be processed and threads to complete. NOTE: Not
    # using hosts.join() here because it does not behave properly with CTRL-C
    # and KeyboardInterrupt.
    while len(threading.enumerate()) > 1:
      time.sleep(DEFAULT_SLEEP)
  except:
    traceback.print_exc()
    logger.warning(
        'Update process aborted. Some machines may be left locked or updating.')
    sys.exit(1)
  finally:
    logger.info(
        ('Mass updating complete. %d hosts updated successfully, %d failed.' %
        (status.successful_updates, len(status.ssh_failures) +
            len(status.update_failures))))

    logger.info(('-' * 25) + '[ SUMMARY ]' + ('-' * 25))

    for host in status.ssh_failures:
      logger.info('Failed to SSH to host %s.' % host)

    for host in status.update_failures:
      logger.info('Failed to update host %s.' % host)

    if len(status.ssh_failures) == 0 and len(status.update_failures) == 0:
      logger.info('All hosts updated successfully.')

    logger.info('-' * 61)


if __name__ == '__main__':
  main()
