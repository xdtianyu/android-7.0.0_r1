#!/usr/bin/python
#
# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import unittest

import common
from autotest_lib.client.common_lib.test_utils import unittest
from autotest_lib.frontend import setup_django_environment
from autotest_lib.frontend.afe import frontend_test_utils
from autotest_lib.frontend.afe import rdb_model_extensions as rdb_models
from autotest_lib.scheduler import rdb_hosts
from autotest_lib.scheduler import rdb_testing_utils
from autotest_lib.scheduler import rdb_utils


class RDBHostTests(unittest.TestCase, frontend_test_utils.FrontendTestMixin):
    """Unittests for RDBHost objects."""

    def setUp(self):
        self.db_helper = rdb_testing_utils.DBHelper()
        self._database = self.db_helper.database
        # Runs syncdb setting up initial database conditions
        self._frontend_common_setup()


    def tearDown(self):
        self._database.disconnect()
        self._frontend_common_teardown()


    def testWireFormat(self):
        """Test that we can create a client host with the server host's fields.

        Get the wire_format fields of an RDBServerHostWrapper and use them to
        create an RDBClientHostWrapper.

        @raises AssertionError: If the labels and acls don't match up after
            going through the complete wire_format conversion, of the bare
            wire_format conversion also converts labels and acls.
        @raises RDBException: If some critical fields were lost during
            wire_format conversion, as we won't be able to construct the
            RDBClientHostWrapper.
        """
        labels = set(['a', 'b', 'c'])
        acls = set(['d', 'e'])
        server_host = rdb_hosts.RDBServerHostWrapper(
                self.db_helper.create_host('h1', deps=labels, acls=acls))
        acl_ids = set([aclgroup.id for aclgroup in
                   self.db_helper.get_acls(name__in=acls)])
        label_ids = set([label.id for label in
                         self.db_helper.get_labels(name__in=labels)])

        # The RDBServerHostWrapper keeps ids of labels/acls to perform
        # comparison operations within the rdb, but converts labels to
        # strings because this is the format the scheduler expects them in.
        self.assertTrue(set(server_host.labels) == label_ids and
                        set(server_host.acls) == acl_ids)

        formatted_server_host = server_host.wire_format()
        client_host = rdb_hosts.RDBClientHostWrapper(**formatted_server_host)
        self.assertTrue(set(client_host.labels) == labels and
                        set(client_host.acls) == acl_ids)
        bare_formatted_server_host = server_host.wire_format(
                unwrap_foreign_keys=False)
        self.assertTrue(bare_formatted_server_host.get('labels') is None and
                        bare_formatted_server_host.get('acls') is None)


    def testLeasing(self):
        """Test that leasing a leased host raises an exception.

        @raises AssertionError: If double leasing a host doesn't raise
            an RDBException, or the leased bits are not set after the
            first attempt at leasing it.
        @raises RDBException: If the host is created with the leased bit set.
        """
        hostname = 'h1'
        server_host = rdb_hosts.RDBServerHostWrapper(
                self.db_helper.create_host(hostname))
        server_host.lease()
        host = self.db_helper.get_host(hostname=hostname)[0]
        self.assertTrue(host.leased and server_host.leased)
        self.assertRaises(rdb_utils.RDBException, server_host.lease)


    def testPlatformAndLabels(self):
        """Test that a client host returns the right platform and labels.

        @raises AssertionError: If client host cannot return the right platform
            and labels.
        """
        platform_name = 'x86'
        label_names = ['a', 'b']
        self.db_helper.create_label(name=platform_name, platform=True)
        server_host = rdb_hosts.RDBServerHostWrapper(
                self.db_helper.create_host(
                        'h1', deps=set(label_names + [platform_name])))
        client_host = rdb_hosts.RDBClientHostWrapper(
                **server_host.wire_format())
        platform, labels = client_host.platform_and_labels()
        self.assertTrue(platform == platform_name)
        self.assertTrue(set(labels) == set(label_names))


    def testClientUpdateSave(self):
        """Test that a client host is capable of saving its attributes.

        Create a client host, set its attributes and verify that the attributes
        are saved properly by recreating a server host and checking them.

        @raises AssertionError: If the server host has the wrong attributes.
        """
        hostname = 'h1'
        db_host = self.db_helper.create_host(hostname, leased=True)
        server_host_dict = rdb_hosts.RDBServerHostWrapper(db_host).wire_format()
        client_host = rdb_hosts.RDBClientHostWrapper(**server_host_dict)

        host_data = {'hostname': hostname, 'id': db_host.id}
        default_values = rdb_models.AbstractHostModel.provide_default_values(
                host_data)
        for k, v in default_values.iteritems():
            self.assertTrue(server_host_dict[k] == v)

        updated_client_fields = {
                    'locked': True,
                    'leased': False,
                    'status': 'FakeStatus',
                    'invalid': True,
                    'protection': 1,
                    'dirty': True,
                }
        client_host.__dict__.update(updated_client_fields)
        client_host.save()

        updated_server_host = rdb_hosts.RDBServerHostWrapper(
                self.db_helper.get_host(hostname=hostname)[0]).wire_format()
        for k, v in updated_client_fields.iteritems():
            self.assertTrue(updated_server_host[k] == v)


    def testUpdateField(self):
        """Test that update field on the client host works as expected.

        @raises AssertionError: If a bad update is processed without an
            exception, of a good update isn't processed as expected.
        """
        hostname = 'h1'
        db_host = self.db_helper.create_host(hostname, dirty=False)
        server_host_dict = rdb_hosts.RDBServerHostWrapper(db_host).wire_format()
        client_host = rdb_hosts.RDBClientHostWrapper(**server_host_dict)
        self.assertRaises(rdb_utils.RDBException, client_host.update_field,
                          *('id', 'fakeid'))
        self.assertRaises(rdb_utils.RDBException, client_host.update_field,
                          *('Nonexist', 'Nonexist'))
        client_host.update_field('dirty', True)
        self.assertTrue(
                self.db_helper.get_host(hostname=hostname)[0].dirty == True and
                client_host.dirty == True)
        new_status = 'newstatus'
        client_host.set_status(new_status)
        self.assertTrue(
                self.db_helper.get_host(hostname=hostname)[0].status ==
                new_status and client_host.status == new_status)


if __name__ == '__main__':
    unittest.main()
