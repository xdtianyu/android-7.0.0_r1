#!/usr/bin/python -t

"""
Nice little script to quickly stage a build onto a devserver.
"""

import argparse
import sys

import common
from autotest_lib.client.common_lib.cros import dev_server
from autotest_lib.server.cros.dynamic_suite import tools
from autotest_lib.server import frontend

def parse_args():
  """Parse command line arguments."""
  parser = argparse.ArgumentParser()
  parser.add_argument('--build', help='e.g. lumpy-release/R26-4321.0.0')
  parser.add_argument('--server', help='OPTIONAL: e.g. devserver.cros')
  parser.add_argument('--host',
                      help='OPTIONAL: e.g. chromeos2-row3-rack4-host5')

  args = parser.parse_args()
  if not args.build:
    parser.print_help()
    sys.exit(1)

  return args

def main():
  """Stage a build on the devserver."""
  options = parse_args()
  if options.server:
    server = 'http://%s/' % options.server
    ds = dev_server.ImageServer(server)
  else:
    ds = dev_server.ImageServer.resolve(options.build)

  print "Downloading %s..." % options.build
  ds.stage_artifacts(options.build, ['full_payload', 'stateful',
                                     'control_files', 'autotest_packages'])
  if options.host:
    print "Poking job_repo_url on %s..." % options.host
    repo_url = tools.get_package_url(ds.url(), options.build)
    AFE = frontend.AFE()
    AFE.set_host_attribute('job_repo_url', repo_url, hostname=options.host)

if __name__ == '__main__':
  main()
