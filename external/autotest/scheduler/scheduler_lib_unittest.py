#!/usr/bin/python
#
# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import mock
import unittest

import common

from autotest_lib.database import database_connection
from autotest_lib.frontend import setup_django_environment
from autotest_lib.frontend.afe import readonly_connection
from autotest_lib.server import utils as server_utils
from autotest_lib.scheduler import scheduler_lib
from django.db import utils as django_utils


class ConnectionManagerTests(unittest.TestCase):
    """Connection manager unittests."""

    def setUp(self):
        self.connection_manager = None
        readonly_connection.set_globally_disabled = mock.MagicMock()
        setup_django_environment.enable_autocommit = mock.MagicMock()
        server_utils.Singleton._instances = {}


    def tearDown(self):
        readonly_connection.set_globally_disabled.reset_mock()
        setup_django_environment.enable_autocommit.reset_mock()


    def testConnectionDisconnect(self):
        """Test connection and disconnecting from the database."""
        # Test that the connection manager only opens a connection once.
        connection_manager = scheduler_lib.ConnectionManager()
        connection_manager.open_connection = mock.MagicMock()
        connection = connection_manager.get_connection()
        connection_manager.open_connection.assert_called_once_with()
        connection_manager.open_connection.reset_mock()
        connection = connection_manager.get_connection()
        self.assertTrue(
                connection_manager.open_connection.call_count == 0)
        connection_manager.open_connection.reset_mock()

        # Test that del on the connection manager closes the connection
        connection_manager.disconnect = mock.MagicMock()
        connection_manager.__del__()
        connection_manager.disconnect.assert_called_once_with()


    def testConnectionReconnect(self):
        """Test that retries don't destroy the connection."""
        database_connection._DjangoBackend.execute = mock.MagicMock()
        database_connection._DjangoBackend.execute.side_effect = (
                django_utils.DatabaseError('Database Error'))
        connection_manager = scheduler_lib.ConnectionManager()
        connection = connection_manager.get_connection()
        self.assertRaises(django_utils.DatabaseError,
                          connection.execute, *('', None, True))
        self.assertTrue(
                database_connection._DjangoBackend.execute.call_count == 2)
        database_connection._DjangoBackend.execute.reset_mock()
        self.assertTrue(connection_manager.db_connection ==
                        connection_manager.get_connection())


    def testConnectionManagerSingleton(self):
        """Test that the singleton works as expected."""
        # Confirm that instantiating the class applies global db settings.
        connection_manager = scheduler_lib.ConnectionManager()
        readonly_connection.set_globally_disabled.assert_called_once_with(True)
        setup_django_environment.enable_autocommit.assert_called_once_with()

        readonly_connection.set_globally_disabled.reset_mock()
        setup_django_environment.enable_autocommit.reset_mock()

        # Confirm that instantiating another connection manager doesn't change
        # the database settings, and in fact, returns the original manager.
        connection_manager_2 = scheduler_lib.ConnectionManager()
        self.assertTrue(connection_manager == connection_manager_2)
        self.assertTrue(
                readonly_connection.set_globally_disabled.call_count == 0)
        self.assertTrue(
                setup_django_environment.enable_autocommit.call_count == 0)

        # Confirm that we don't open the connection when the class is
        # instantiated.
        self.assertTrue(connection_manager.db_connection is None)


if __name__ == '__main__':
    unittest.main()
