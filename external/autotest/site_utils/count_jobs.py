#! /usr/bin/python

# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Counts the number of jobs created in the last 24 hours."""

import argparse
from datetime import datetime, timedelta
import sys

import common
from autotest_lib.frontend import setup_django_environment
from autotest_lib.frontend.afe import models
from autotest_lib.client.common_lib.cros.graphite import autotest_stats


def number_of_jobs_since(delta):
    """Returns the number of jobs kicked off in the last |duration| minutes.

    @param delta: A timedelta which indicates the maximum age of the jobs to count
    """
    cutoff = datetime.now() - delta
    return models.Job.objects.filter(created_on__gt=cutoff).count()


def main():
    """Counts the number of AFE jobs in the last day, then pushes the count to statsd"""
    parser = argparse.ArgumentParser(
        description=('A script which records the number of afe jobs run in a time interval.'))
    parser.parse_args(sys.argv[1:])
    count = number_of_jobs_since(timedelta(days=1))
    autotest_stats.Gauge('jobs_rate').send('afe_daily_count', count)


if __name__ == '__main__':
    main()
