#!/usr/bin/python

"""
Poll server-status on cautotest to watch for RPCs taking longer than 10s. Then
we go and ssh around to figure out what the command line of the process that
caused the RPC was so that one can track down what is generating the expensive
RPC load.
"""

try:
  from bs4 import BeautifulSoup
except ImportError:
  print 'Run `apt-get install python-bs4`'
  raise

import time
import subprocess
import multiprocessing

import common
import requests


def check_cautotest():
  page = requests.get('http://cautotest/server-status').text
  soup = BeautifulSoup(page)
  pids = []
  for row in soup.table.findAll('tr'):
    cols = [x.text.strip() for x in row.findAll('td')]
    if not cols:
      continue
    if cols[3] == 'W' and int(cols[5]) > 10 and cols[1] != '-':
      pids.append((cols[1], cols[3], cols[5]))
  return pids

def pull_cautotest_info(proc_id):
  try:
    conn = subprocess.check_output('become chromeos-test@cautotest -- '
           '"sudo lsof -i | grep -e %s | grep -e ESTABLISHED"' % proc_id,
           shell=True)
    remote_info = conn.split()[8].split('->')[1].split(':')
  except Exception:
    remote_info = None
  return remote_info

def strace_cautotest(proc_id):
  try:
    straced = subprocess.check_output('become chromeos-test@cautotest -- '
              '"sudo strace -s 500 -p %s 2>&1 | head -n 20"' % proc_id,
              shell=True)
  except subprocess.CalledProcessError:
    straced = ""
  return straced

def pull_drone_info(host, port):
  try:
    lsof = subprocess.check_output('become chromeos-test@%s -- '
           '"sudo lsof -i | grep -e :%s | grep -e ESTABLISHED"'
           % (host, port), shell=True)
    proc_id = lsof.split()[1]
    cmdline = subprocess.check_output('become chromeos-test@%s -- '
              '"cat /proc/%s/cmdline"' % (host,proc_id), shell=True)
  except Exception:
    cmdline = ''
  return cmdline

def pull_all_data(pid, queue):
  try:
    remote_info = pull_cautotest_info(pid[0])
    if remote_info:
      drone_info = pull_drone_info(*remote_info)
    else:
      drone_info = None
    straced = strace_cautotest(pid[0])
    queue.put((pid, remote_info, drone_info, straced))
  except Exception:
    queue.put(None)

def print_data(x):
    (pid, remote_info, drone_info, straced) = x
    print "*** %s stuck in %s for %s secs" % pid
    print remote_info
    print drone_info
    print straced
    print '\a'

while True:
  queue = multiprocessing.Queue()
  processes = []
  pids = check_cautotest()
  for pid in pids:
    proc = multiprocessing.Process(target=pull_all_data, args=(pid, queue))
    proc.start()
    processes.append(proc)
  for proc in processes:
    x = queue.get()
    if x:
      print_data(x)
  for proc in processes:
    proc.terminate()
  time.sleep(5)
