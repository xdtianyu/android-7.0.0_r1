#!/usr/bin/env python

"""
This script prints out a csv file of `suite,test,path/to/control.file` where
each row is a test that has failed every time that it ran for the past N days,
where N is that one constant lower in this file.

You run it like this

  ./always_failing_tests.py | tee output

But please note that since we're using the models to do queries, you'll probably
need to move your local shadow config out of the way before you run this script
so that you point at prod.
"""

import time
import hashlib
import re
import datetime
import sys

import common
from autotest_lib.frontend import setup_django_readonly_environment

# Django and the models are only setup after
# the setup_django_readonly_environment module is imported.
from autotest_lib.frontend.tko import models as tko_models
from autotest_lib.frontend.afe import models as afe_models
from autotest_lib.server.cros.dynamic_suite import suite


_DAYS_NOT_RUNNING_CUTOFF = 30


def md5(s):
  m = hashlib.md5()
  m.update(s)
  return m.hexdigest()


def main():
    cutoff_delta = datetime.timedelta(_DAYS_NOT_RUNNING_CUTOFF)
    cutoff_date = datetime.datetime.today() - cutoff_delta
    statuses = {s.status_idx: s.word for s in tko_models.Status.objects.all()}
    now = time.time()

    tests = tko_models.Test.objects.select_related('job'
            ).filter(started_time__gte=cutoff_date
            ).exclude(test__contains='/'
            ).exclude(test__contains='_JOB'
            ).exclude(test='provision'
            ).exclude(test__contains='try_new_image')
    tests = list(tests)
    # These prints are vague profiling work.  We're handling a lot of data, so I
    # had to dump some decent work into making sure things chug along at a
    # decent speed.
    print "DB: %d -- len=%d" % (time.time()-now, len(tests))

    def only_failures(d, t):
      word = statuses[t.status_id]
      if word == 'TEST_NA':
        return d
      if word == 'GOOD' or word == 'WARN':
        passed = True
      else:
        passed = False
      d[t.test] = d.get(t.test, False) or passed
      return d
    dct = reduce(only_failures, tests, {})
    print "OF: %d -- len=%d" % (time.time()-now, len(dct))

    all_fail = filter(lambda x: x.test in dct and not dct[x.test], tests)
    print "AF: %d -- len=%d" % (time.time()-now, len(all_fail))

    hash_to_file = {}
    fs_getter = suite.Suite.create_fs_getter(common.autotest_dir)
    for control_file in fs_getter.get_control_file_list():
      with open(control_file, 'rb') as f:
        h = md5(f.read())
        hash_to_file[h] = control_file.replace(common.autotest_dir, '')\
                                      .lstrip('/')
    print "HF: %d -- len=%d" % (time.time()-now, len(hash_to_file))

    afe_job_ids = set(map(lambda t: t.job.afe_job_id, all_fail))
    afe_jobs = afe_models.Job.objects.select_related('parent_job')\
                                     .filter(id__in=afe_job_ids)
    print "AJ: %d -- len=%d" % (time.time()-now, len(afe_jobs))

    job_to_hash = {}
    for job in afe_jobs:
      job_to_hash[job.id] = md5(job.control_file)
    print "JH: %d -- len=%d" % (time.time()-now, len(job_to_hash))

    job_to_suite = {}
    rgx = re.compile("test_suites/control.(\w+)")
    for job in afe_jobs:
      job_id = job.parent_job
      if not job_id:
        job_id = job
      x = rgx.search(job_id.name)
      if not x:
        print job_id.name
        continue
      job_to_suite[job.id] = x.groups(1)[0]

    def collect_by_suite_name(d, t):
      s = job_to_suite.get(t.job.afe_job_id, None)
      d.setdefault((s, t.test), []).append(t)
      return d
    by_name = reduce(collect_by_suite_name, all_fail, {})
    print "BN: %d -- len=%d" % (time.time()-now, len(by_name))

    for (s, testname), tests in by_name.iteritems():
      for test in tests:
        h = job_to_hash[test.job.afe_job_id]
        if h in hash_to_file:
          print "%s,%s,%s" % (s, testname, hash_to_file[h])
          break
      else:
        print "%s,%s,?" % (s, testname)


if __name__ == '__main__':
    sys.exit(main())
