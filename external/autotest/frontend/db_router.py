# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Django database Router

Django gets configured with three database connections in frontend/settings.py.
- The default database
    - This database should be used for most things.
    - For the master, this is the global database.
    - For shards, this this is the shard-local database.
- The global database
    - For the master, this is the same database as default, which is the global
      database.
    - For the shards, this is the global database (the same as for the master).
- The readonly connection
    - This should be the same database as the global database, but it should
      use an account on the database that only has readonly permissions.
- The server database
    - This is the database stores information about all servers in the Autotest
      instance. Each instance, master or shard should have its own server
      database is use_server_db is enabled in global config.

The reason shards need two distinct databases for different objects is, that
the tko parser should always write to the global database. Otherwise test
results wouldn't be synced back to the master and would not be accessible in one
place.

Therefore this class will route all queries for tables starts with `server`
prefix to server database, route all queries for tables that involve
`tko_`-prefixed tables to the global database. For all others this router will
not give a hint, which means the default database will be used.
"""

class Router(object):
    """
    Decide if an object should be written to the default or to the global db.

    This is an implementaton of Django's multi-database router interface:
    https://docs.djangoproject.com/en/1.5/topics/db/multi-db/
    """

    def _should_be_in_server_db(self, model):
        """Return True if the model should be stored in the server db.

        @param model: Model to decide for.

        @return: True if querying the model requires server database.
        """
        return model._meta.db_table.startswith('server')


    def _should_be_in_global(self, model):
        """Returns True if the model should be stored in the global db.

        @param model: Model to decide for.

        @return: True if querying the model requires global database.
        """
        return model._meta.db_table.startswith('tko_')


    def db_for_read(self, model, **hints):
        """Return the database for a reading access.

        @param model: Model to decide for.
        @param hints: Optional arguments to determine which database for read.

        @returns: 'server' for all server models. 'global' for all tko models,
                  None otherwise. None means the router doesn't have an opinion.
        """
        if self._should_be_in_server_db(model):
            return 'server'
        if self._should_be_in_global(model):
            return 'global'
        return None


    def db_for_write(self, model, **hints):
        """Return the database for a writing access.

        @param model: Model to decide for.
        @param hints: Optional arguments to determine which database for write.

        @returns: 'server' for all server models. 'global' for all tko models,
                  None otherwise. None means the router doesn't have an opinion.
        """
        if self._should_be_in_server_db(model):
            return 'server'
        if self._should_be_in_global(model):
            return 'global'
        return None


    def allow_relation(self, obj1, obj2, **hints):
        """
        Allow relations only if either both are in tko_ tables or none is.

        @param obj1: First object involved in the relation.
        @param obj2: Second object involved in the relation.
        @param hints: Optional arguments to determine if relation is allowed.

        @returns False, if the relation should be prohibited,
                 None, if the router doesn't have an opinion.
        """
        if (not self._should_be_in_server_db(type(obj1)) ==
            self._should_be_in_server_db(type(obj2))):
            return False
        if (not self._should_be_in_global(type(obj1)) ==
            self._should_be_in_global(type(obj2))):
            return False
        return None
