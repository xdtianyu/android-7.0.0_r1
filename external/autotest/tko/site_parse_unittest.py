#!/usr/bin/python -u
#
# Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
#

#pylint: disable-msg=C0111

import mox, os, shutil, tempfile, unittest

from django.conf import settings

import common
from autotest_lib.client.common_lib import global_config
from autotest_lib.frontend import database_settings_helper
from autotest_lib.frontend import setup_django_environment
from autotest_lib.frontend import setup_test_environment
from autotest_lib.frontend.afe import frontend_test_utils
from autotest_lib.frontend.afe import models as django_afe_models
from autotest_lib.frontend.tko import models as django_tko_models
from autotest_lib.tko import db as tko_db
from autotest_lib.tko.site_parse import StackTrace

# Have to import this after setup_django_environment and setup_test_environment.
# It creates a database connection, so the mocking has to be done first.
from django.db import connections

class stack_trace_test(unittest.TestCase):


    def setUp(self):
        setup_test_environment.set_up()
        self._fake_results = tempfile.mkdtemp()
        self._cros_src_dir = global_config.global_config.get_config_value(
            'CROS', 'source_tree', default=None)

        if not self._cros_src_dir:
            self.fail('No Chrome OS source tree defined in global_config.ini')

        self._stack_trace = StackTrace(
            self._fake_results, self._cros_src_dir)

        self._cache_dir = os.path.join(
            self._cros_src_dir, 'chroot', self._stack_trace._CACHE_DIR)

        # Ensure we don't obliterate a live cache directory by accident.
        if os.path.exists(self._cache_dir):
            self.fail(
                'Symbol cache directory already exists. Cowardly refusing to'
                ' run. Please remove this directory manually to continue.')


    def tearDown(self):
        setup_test_environment.tear_down()
        shutil.rmtree(self._fake_results)
        if os.path.exists(self._cache_dir):
            shutil.rmtree(self._cache_dir)


    def _setup_basic_cache(self,
                           job_name='x86-alex-r16-R16-1166.0.0-a1-b1118_bvt',
                           mkdir=True):
        # Ensure cache directory is present.
        self._stack_trace._get_cache_dir()
        board, rev, version = self._stack_trace._parse_job_name(job_name)

        symbols_dir = os.path.join(
            self._cache_dir, '-'.join([board, rev, version]))
        if mkdir:
            os.mkdir(symbols_dir)

        chroot_symbols_dir = os.sep + os.path.relpath(
            symbols_dir, self._stack_trace._chroot_dir)

        return job_name, symbols_dir, chroot_symbols_dir


    def test_get_job_name(self):
        job_name = 'x86-alex-r16-R16-1166.0.0-a1-b1118_regression'
        with open(os.path.join(self._fake_results, 'keyval'), 'w') as f:
            f.write('label=%s' % job_name)

        self.assertEqual(self._stack_trace._get_job_name(), job_name)


    def test_parse_3_tuple_job_name(self):
        job_name = 'x86-alex-r16-R16-1166.0.0-a1-b1118_regression'
        board, rev, version = self._stack_trace._parse_job_name(job_name)
        self.assertEqual(board, 'x86-alex')
        self.assertEqual(rev, 'r16')
        self.assertEqual(version, '1166.0.0')


    def test_parse_4_tuple_job_name(self):
        job_name = 'x86-mario-r15-0.15.1011.74-a1-b61_bvt'
        board, rev, version = self._stack_trace._parse_job_name(job_name)
        self.assertEqual(board, 'x86-mario')
        self.assertEqual(rev, 'r15')
        self.assertEqual(version, '0.15.1011.74')


    def test_parse_4_tuple_au_job_name(self):
        job_name = 'x86-alex-r15-0.15.1011.81_to_0.15.1011.82-a1-b69_mton_au'
        board, rev, version = self._stack_trace._parse_job_name(job_name)
        self.assertEqual(board, 'x86-alex')
        self.assertEqual(rev, 'r15')
        self.assertEqual(version, '0.15.1011.82')


    def test_parse_3_tuple_au_job_name(self):
        job_name = 'x86-alex-r16-1165.0.0_to_R16-1166.0.0-a1-b69_mton_au'
        board, rev, version = self._stack_trace._parse_job_name(job_name)
        self.assertEqual(board, 'x86-alex')
        self.assertEqual(rev, 'r16')
        self.assertEqual(version, '1166.0.0')


class database_selection_test(mox.MoxTestBase,
                              frontend_test_utils.FrontendTestMixin):

    def setUp(self):
        super(database_selection_test, self).setUp()
        self._frontend_common_setup(fill_data=False)


    def tearDown(self):
        super(database_selection_test, self).tearDown()
        self._frontend_common_teardown()
        global_config.global_config.reset_config_values()


    def assertQueries(self, database, assert_in, assert_not_in):
        assert_in_found = False
        for query in connections[database].queries:
            sql = query['sql']
            # Ignore CREATE TABLE statements as they are always executed
            if 'INSERT INTO' in sql or 'SELECT' in sql:
                self.assertNotIn(assert_not_in, sql)
                if assert_in in sql:
                    assert_in_found = True
        self.assertTrue(assert_in_found)


    def testDjangoModels(self):
        # If DEBUG=False connection.query will be empty
        settings.DEBUG = True

        afe_job = django_afe_models.Job.objects.create(created_on='2014-08-12')
        # Machine has less dependencies than tko Job so it's easier to create
        tko_job = django_tko_models.Machine.objects.create()

        django_afe_models.Job.objects.get(pk=afe_job.id)
        django_tko_models.Machine.objects.get(pk=tko_job.pk)

        self.assertQueries('global', 'tko_machines', 'afe_jobs')
        self.assertQueries('default', 'afe_jobs', 'tko_machines')

        # Avoid unnecessary debug output from other tests
        settings.DEBUG = True


    def testRunOnShardWithoutGlobalConfigsFails(self):
        global_config.global_config.override_config_value(
                'SHARD', 'shard_hostname', 'host1')
        from autotest_lib.frontend import settings
        # settings module was already loaded during the imports of this file,
        # so before the configuration setting was made, therefore reload it:
        reload(database_settings_helper)
        self.assertRaises(global_config.ConfigError,
                          reload, settings)


    def testRunOnMasterWithoutGlobalConfigsWorks(self):
        global_config.global_config.override_config_value(
                'SHARD', 'shard_hostname', '')
        from autotest_lib.frontend import settings
        # settings module was already loaded during the imports of this file,
        # so before the configuration setting was made, therefore reload it:
        reload(database_settings_helper)
        reload(settings)


    def testTkoDatabase(self):
        global_host = 'GLOBAL_HOST'
        global_user = 'GLOBAL_USER'
        global_db = 'GLOBAL_DB'
        global_pw = 'GLOBAL_PW'
        global_port = ''
        local_host = 'LOCAL_HOST'

        global_config.global_config.override_config_value(
                'AUTOTEST_WEB', 'global_db_type', '')

        global_config.global_config.override_config_value(
                'AUTOTEST_WEB', 'global_db_host', global_host)
        global_config.global_config.override_config_value(
                'AUTOTEST_WEB', 'global_db_database', global_db)
        global_config.global_config.override_config_value(
                'AUTOTEST_WEB', 'global_db_user', global_user)
        global_config.global_config.override_config_value(
                'AUTOTEST_WEB', 'global_db_password', global_pw)
        global_config.global_config.override_config_value(
                'AUTOTEST_WEB', 'host', local_host)

        class ConnectCalledException(Exception):
            pass

        # We're only interested in the parameters connect is called with here.
        # Take the fast path out so we don't have to mock all the other calls
        # that will later be made on the connection
        def fake_connect(*args, **kwargs):
            raise ConnectCalledException

        tko_db.db_sql.connect = None
        self.mox.StubOutWithMock(tko_db.db_sql, 'connect')
        tko_db.db_sql.connect(
                global_host, global_db, global_user, global_pw,
                global_port).WithSideEffects(fake_connect)

        self.mox.ReplayAll()

        self.assertRaises(ConnectCalledException, tko_db.db_sql)


if __name__ == "__main__":
    unittest.main()
