"""Sets up the Django environment for readonly access to the database.

This sets the DJANGO_SETTINGS_MODULE to point to frontend/settings_readonly.py.
Django will then use this file for configuration.

"""

import os


os.environ.setdefault('DJANGO_SETTINGS_MODULE',
                      'autotest_lib.frontend.settings_readonly')
