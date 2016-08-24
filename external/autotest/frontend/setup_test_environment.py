from django.core import management
from django.conf import settings
import common

# we need to set DATABASE_ENGINE now, at import time, before the Django database
# system gets initialized.
# django.conf.settings.LazySettings is buggy and requires us to get something
# from it before we set stuff on it.
getattr(settings, 'DATABASES')
for name in ['default', 'global', 'readonly', 'server']:
    if name not in settings.DATABASES:
        settings.DATABASES[name] = {}
    settings.DATABASES[name]['ENGINE'] = (
            'autotest_lib.frontend.db.backends.afe_sqlite')
    settings.DATABASES[name]['NAME'] = ':memory:'


from django.db import connections
from autotest_lib.frontend.afe import readonly_connection

connection = connections['default']
connection_readonly = connections['readonly']
connection_global = connections['global']
connection_server = connections['server']

def run_syncdb(verbosity=0):
    """Call syncdb command to make sure database schema is uptodate.

    @param verbosity: Level of verbosity of the command, default to 0.
    """
    management.call_command('syncdb', verbosity=verbosity, interactive=False)
    management.call_command('syncdb', verbosity=verbosity, interactive=False,
                             database='readonly')
    management.call_command('syncdb', verbosity=verbosity, interactive=False,
                             database='global')
    management.call_command('syncdb', verbosity=verbosity, interactive=False,
                             database='server')


def destroy_test_database():
    """Close all connection to the test database.
    """
    connection.close()
    connection_readonly.close()
    connection_global.close()
    connection_server.close()
    # Django brilliantly ignores close() requests on in-memory DBs to keep us
    # naive users from accidentally destroying data.  So reach in and close
    # the real connection ourselves.
    # Note this depends on Django internals and will likely need to be changed
    # when we upgrade Django.
    for con in [connection, connection_global, connection_readonly,
                connection_server]:
        real_connection = con.connection
        if real_connection is not None:
            real_connection.close()
            con.connection = None


def set_up():
    """Run setup before test starts.
    """
    run_syncdb()
    readonly_connection.set_globally_disabled(True)


def tear_down():
    """Run cleanup after test is completed.
    """
    readonly_connection.set_globally_disabled(False)
    destroy_test_database()


def print_queries():
    """
    Print all SQL queries executed so far.  Useful for debugging failing tests -
    you can call it from tearDown(), and then execute the single test case of
    interest from the command line.
    """
    for query in connection.queries:
        print query['sql'] + ';\n'
