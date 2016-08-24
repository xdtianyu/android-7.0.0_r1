#!/usr/bin/python
#
# Copyright 2010 Google Inc. All Rights Reserved.
"""
This allows a site to customize the test creation attributes.

"""


import common, re
from autotest_lib.frontend.afe import models


def _set_attributes_custom(test, data):
    # We set the test name to the dirname of the control file.
    test_new_name = test.path.split('/')
    if test_new_name[-1] == 'control' or test_new_name[-1] == 'control.srv':
        test.name = test_new_name[-2]
    else:
        control_name = "%s:%s"
        control_name %= (test_new_name[-2],
                         test_new_name[-1])
        test.name = re.sub('control.*\.', '', control_name)

    # We set verify to always False (0).
    test.run_verify = 0
    
    if hasattr(data, 'test_parameters'):
        for para_name in data.test_parameters:
            test_parameter = models.TestParameter.objects.get_or_create(
                test=test, name=para_name)[0]
            test_parameter.save()
