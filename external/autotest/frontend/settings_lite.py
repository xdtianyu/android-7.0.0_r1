"""Django settings for lightweight in-memory model database.
"""

import common

LIGHTWEIGHT_DEFAULT = {
    'ENGINE': 'django.db.backends.sqlite3',
    'NAME': ':memory:'
}

DATABASES = {'default': LIGHTWEIGHT_DEFAULT}

INSTALLED_APPS = (
    'frontend.afe',
#    'frontend.tko',
)

# Required for Django to start, even though not used.
SECRET_KEY = 'Three can keep a secret if two are dead.'

AUTOTEST_CREATE_ADMIN_GROUPS = False
