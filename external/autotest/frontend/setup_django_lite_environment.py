import os
import common

os.environ.setdefault('DJANGO_SETTINGS_MODULE',
                      'autotest_lib.frontend.settings_lite')

from autotest_lib.frontend.afe import readonly_connection
readonly_connection.set_globally_disabled(True)

import django.core.management

django.core.management.call_command('syncdb', interactive=False, verbosity=0)
