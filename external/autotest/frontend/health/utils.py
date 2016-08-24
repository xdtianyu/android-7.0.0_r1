# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import common
from autotest_lib.frontend import setup_django_readonly_environment

# Django and the models are only setup after
# the setup_django_readonly_environment module is imported.
from autotest_lib.frontend.tko import models as tko_models
from django.db import models as django_models

_TEST_ERROR_STATUS = 'ERROR'
_TEST_ABORT_STATUS = 'ABORT'
_TEST_FAIL_STATUS = 'FAIL'
_TEST_WARN_STATUS = 'WARN'
_TEST_PASS_STATUS = 'GOOD'
_TEST_ALERT_STATUS = 'ALERT'


def get_last_pass_times():
    """
    Get all the tests that have passed and the time they last passed.

    @return the dict of test_name:last_finish_time pairs for tests that have
            passed.

    """
    results = tko_models.Test.objects.values('test').filter(
        status__word=_TEST_PASS_STATUS).annotate(
        last_pass=django_models.Max('started_time'))
    return {result['test']: result['last_pass'] for result in results}


def get_last_fail_times():
    """
    Get all the tests that have failed and the time they last failed.

    @return the dict of test_name:last_finish_time pairs for tests that have
            failed.

    """

    failure_clauses = (django_models.Q(status__word=_TEST_FAIL_STATUS) |
                       django_models.Q(status__word=_TEST_ERROR_STATUS) |
                       django_models.Q(status__word=_TEST_ABORT_STATUS) |
                       django_models.Q(status__word=_TEST_WARN_STATUS) |
                       django_models.Q(status__word=_TEST_ALERT_STATUS))

    results = tko_models.Test.objects.values('test').filter(
        failure_clauses).annotate(
        last_pass=django_models.Max('started_time'))

    return {result['test']: result['last_pass'] for result in results}
