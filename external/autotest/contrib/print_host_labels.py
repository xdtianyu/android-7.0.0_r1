#!/usr/bin/env python

"""
Usage: ./print_host_labels.py <IP.or.hostname>
"""

import sys
import common
from autotest_lib.server.hosts import factory

if len(sys.argv) < 2:
    print 'Usage: %s <IP.or.hostname>' % sys.argv[0]
    exit(1)

host = factory.create_host(sys.argv[1])
labels = host.get_labels()
print 'Labels:'
print labels
