"""Django settings for frontend project.

Two databases are configured for the use with django here. One for tko tables,
which will always be the same database for all instances (the global database),
and one for everything else, which will be the same as the global database for
the master, but a local database for shards.
Additionally there is a third database connection for read only access to the
global database.

This is implemented using a Django database router.
For more details on how the routing works, see db_router.py.
"""

import common
from autotest_lib.client.common_lib import global_config
from autotest_lib.frontend import database_settings_helper

c = global_config.global_config
_section = 'AUTOTEST_WEB'

DEBUG = c.get_config_value(_section, "sql_debug_mode", type=bool, default=False)
TEMPLATE_DEBUG = c.get_config_value(_section, "template_debug_mode", type=bool,
                                    default=False)

FULL_ADMIN = False

ADMINS = (
    # ('Your Name', 'your_email@domain.com'),
)

MANAGERS = ADMINS

AUTOTEST_DEFAULT = database_settings_helper.get_default_db_config()
AUTOTEST_GLOBAL = database_settings_helper.get_global_db_config()
AUTOTEST_READONLY = database_settings_helper.get_readonly_db_config()
AUTOTEST_SERVER = database_settings_helper.get_server_db_config()

ALLOWED_HOSTS = '*'

DATABASES = {'default': AUTOTEST_DEFAULT,
             'global': AUTOTEST_GLOBAL,
             'readonly': AUTOTEST_READONLY,
             'server': AUTOTEST_SERVER,}

# Have to set SECRET_KEY before importing connections because of this bug:
# https://code.djangoproject.com/ticket/20704
# TODO: Order this again after an upgrade to Django 1.6 or higher.
# Make this unique, and don't share it with anybody.
SECRET_KEY = 'pn-t15u(epetamdflb%dqaaxw+5u&2#0u-jah70w1l*_9*)=n7'

# Do not do this here or from the router, or most unit tests will fail.
# from django.db import connection

DATABASE_ROUTERS = ['autotest_lib.frontend.db_router.Router']

# prefix applied to all URLs - useful if requests are coming through apache,
# and you need this app to coexist with others
URL_PREFIX = 'afe/server/'
TKO_URL_PREFIX = 'new_tko/server/'

# Local time zone for this installation. Choices can be found here:
# http://www.postgresql.org/docs/8.1/static/datetime-keywords.html#DATETIME-TIMEZONE-SET-TABLE
# although not all variations may be possible on all operating systems.
# If running in a Windows environment this must be set to the same as your
# system time zone.
TIME_ZONE = 'America/Los_Angeles'

# Language code for this installation. All choices can be found here:
# http://www.w3.org/TR/REC-html40/struct/dirlang.html#langcodes
# http://blogs.law.harvard.edu/tech/stories/storyReader$15
LANGUAGE_CODE = 'en-us'

SITE_ID = 1

# If you set this to False, Django will make some optimizations so as not
# to load the internationalization machinery.
USE_I18N = True

# Absolute path to the directory that holds media.
# Example: "/home/media/media.lawrence.com/"
MEDIA_ROOT = ''

# URL that handles the media served from MEDIA_ROOT.
# Example: "http://media.lawrence.com"
MEDIA_URL = ''

# URL prefix of static file. Only used by the admin interface.
STATIC_URL = '/' + URL_PREFIX + 'admin/'

# URL prefix for admin media -- CSS, JavaScript and images. Make sure to use a
# trailing slash.
# Examples: "http://foo.com/media/", "/media/".
ADMIN_MEDIA_PREFIX = '/media/'

# List of callables that know how to import templates from various sources.
TEMPLATE_LOADERS = (
    'django.template.loaders.filesystem.Loader',
    'django.template.loaders.app_directories.Loader',
#     'django.template.loaders.eggs.Loader',
)

MIDDLEWARE_CLASSES = (
    'django.middleware.common.CommonMiddleware',
    'django.contrib.sessions.middleware.SessionMiddleware',
    'frontend.apache_auth.ApacheAuthMiddleware',
    'django.contrib.auth.middleware.AuthenticationMiddleware',
    'django.middleware.doc.XViewMiddleware',
    'django.contrib.messages.middleware.MessageMiddleware',
    'frontend.shared.json_html_formatter.JsonToHtmlMiddleware',
)

ROOT_URLCONF = 'frontend.urls'

INSTALLED_APPS = (
    'frontend.afe',
    'frontend.tko',
    'django.contrib.admin',
    'django.contrib.auth',
    'django.contrib.contenttypes',
    'django.contrib.sessions',
    'django.contrib.sites',
)

AUTHENTICATION_BACKENDS = (
    'frontend.apache_auth.SimpleAuthBackend',
)
# TODO(scottz): Temporary addition until time can be spent untangling middleware
# session crosbug.com/31608
SESSION_COOKIE_AGE = 1200

AUTOTEST_CREATE_ADMIN_GROUPS = True
