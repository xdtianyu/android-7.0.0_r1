import os

import common

os.environ.setdefault('DJANGO_SETTINGS_MODULE', 'autotest_lib.frontend.settings')

def _enable_autocommit_by_name(name):
    """Enable autocommit for the connection with matching name.

    @param name: Name of the connection.
    """
    from django.db import connections
    # ensure a connection is open
    connections[name].cursor()
    connections[name].connection.autocommit(True)


def enable_autocommit():
    """Enable autocommit for default and global connection.
    """
    _enable_autocommit_by_name('default')
    _enable_autocommit_by_name('global')
