#!/usr/bin/python

"""
Finds hosts that are shared between both cautotest and cautotest-cq.
"""

import common
from autotest_lib.server import frontend

cautotest = frontend.AFE(server='cautotest')
cautotest_cq = frontend.AFE(server='cautotest-cq')

cautotest_hosts = [x['hostname'] for x in cautotest.run('get_hosts')
                   if not x['locked']]
cautotest_cq_hosts = [x['hostname'] for x in cautotest_cq.run('get_hosts')
                      if not x['locked']]

for host in cautotest_hosts:
    if host in cautotest_cq_hosts:
        print host
