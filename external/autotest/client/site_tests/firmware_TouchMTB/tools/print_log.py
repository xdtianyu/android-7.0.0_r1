# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""A simple script to print the log in a readable format.

Usage:   python tools/print_log.py <log_dir>
Example: python tools/print_log.py tests/log/lumpy
"""


import glob
import pickle
import os
import sys

# Need to have this line because pickle needs firmware_log module to load logs.
sys.path.append(os.getcwd())


def _print_log(log_dir):
    ext = '.log'
    filenames = glob.glob(os.path.join(log_dir, '*.log'))
    for filename in filenames:
        print 'Printing %s ...' % filename
        fw, date, glogs = pickle.load(open(filename))
        prefix_spaces = ' ' * 2
        print prefix_spaces + 'fw:   ', fw
        print prefix_spaces + 'date: ', date
        print prefix_spaces + 'glogs: '
        for glog in glogs:
            vlogs = glog.vlogs
            if not vlogs:
                continue

            print prefix_spaces * 2 + '(%s %s)' % (glog.name, glog.variation)
            for vlog in vlogs:
                print prefix_spaces * 4 + '%s: ' % vlog.name
                print prefix_spaces * 5 + 'score: %s' % str(vlog.score)
                for metric in vlog.metrics:
                    print (prefix_spaces * 5 + 'metric %s: %s' %
                           (metric.name, metric.value))
        print


if __name__ == '__main__':
    if len(sys.argv) != 2 or not os.path.exists(sys.argv[1]):
        print 'Usage: python tools/%s <log_dir>' % sys.argv[0]
        exit(1)
    log_dir = sys.argv[1]
    _print_log(log_dir)
