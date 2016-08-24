#!/usr/bin/env python

import codecs, httplib, json, optparse, os, urllib, shutil, subprocess, sys

upstream_git = 'https://github.com/catapult-project/catapult.git'

script_dir = os.path.dirname(os.path.abspath(sys.argv[0]))
catapult_dir = os.path.join(script_dir, 'catapult')

parser = optparse.OptionParser()
parser.add_option('--local', dest='local_dir', metavar='DIR',
                  help='use a local catapult')
parser.add_option('--no-min', dest='no_min', default=False, action='store_true',
                  help='skip minification')
options, args = parser.parse_args()

# Update the source if needed.
if options.local_dir is None:
  # Remove the old source tree.
  shutil.rmtree(catapult_dir, True)

  # Pull the latest source from the upstream git.
  git_args = ['git', 'clone', upstream_git, catapult_dir]
  p = subprocess.Popen(git_args, stdout=subprocess.PIPE, cwd=script_dir)
  p.communicate()
  if p.wait() != 0:
    print 'Failed to checkout source from upstream git.'
    sys.exit(1)

  catapult_git_dir = os.path.join(catapult_dir, '.git')
  # Update the UPSTREAM_REVISION file
  git_args = ['git', 'rev-parse', 'HEAD']
  p = subprocess.Popen(git_args,
                       stdout=subprocess.PIPE,
                       cwd=catapult_dir,
                       env={"GIT_DIR":catapult_git_dir})
  out, err = p.communicate()
  if p.wait() != 0:
    print 'Failed to get revision.'
    sys.exit(1)

  shutil.rmtree(catapult_git_dir, True)

  rev = out.strip()
  with open('UPSTREAM_REVISION', 'wt') as f:
    f.write(rev + '\n')
else:
  catapult_dir = options.local_dir


# Update systrace_trace_viewer.html
systrace_dir = os.path.join(catapult_dir, 'systrace', 'systrace')
sys.path.append(systrace_dir)
import update_systrace_trace_viewer
update_systrace_trace_viewer.update(no_auto_update=True, no_min=options.no_min)
