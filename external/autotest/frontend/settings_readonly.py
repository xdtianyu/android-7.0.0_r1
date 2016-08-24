"""Django settings for code that only needs readonly access to the database."""

import common

# This copies all the non-readonly settings into the readonly settings.
# We then overwrite the relevant settings to set readonly access.
# This allows us to update the generic settings without having duplicated
# entries between the readonly and non-readonly settings.
from autotest_lib.frontend.settings import *


AUTOTEST_DEFAULT['USER'] = AUTOTEST_DEFAULT['READONLY_USER']
AUTOTEST_DEFAULT['HOST'] = AUTOTEST_DEFAULT['READONLY_HOST']
AUTOTEST_DEFAULT['PASSWORD'] = AUTOTEST_DEFAULT['READONLY_PASSWORD']
