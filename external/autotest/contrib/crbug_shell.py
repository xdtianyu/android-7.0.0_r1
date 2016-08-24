#!/usr/bin/python

# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""A shell for crbug_crawler.
"""

import crbug_crawler
import cmd
import logging
import os
import sys

import common

from autotest_lib.client.common_lib import global_config
from autotest_lib.server.cros.dynamic_suite import reporting

try:
    from oauth2client import file as oauth_file
    from oauth2client import client
    from oauth2client import tools
except ImportError:
    logging.error('You do not have the appropriate oauth2client libraries'
            'required for authorization. Run ./<autotest_checkout>/utils/\ '
            'build_externals.py or pip install the oauth2client.')
    sys.exit(1)


def check_auth():
    """Checks if valid oath credentials exist on the system.

    If valid credentials aren't found on the client they're generated,
    if possible, using the cliend_id and client_secret from the shadow_config.
    """
    shadow_config = os.path.join(common.autotest_dir, 'shadow_config.ini')
    if not os.path.exists(shadow_config):
        logging.error('Cannot autorize without a shadow_config that contains'
               'the appropriate client id for oauth. Contact '
               'chromeos-lab-infrastructure if you think this is a mistake.')
        sys.exit(1)

    auth_store = oauth_file.Storage(reporting.Reporter.get_creds_abspath())
    creds = auth_store.get()
    if creds is None or creds.invalid:
        client_id = global_config.global_config.get_config_value(
                reporting.BUG_CONFIG_SECTION, 'client_id', default='')
        client_secret = global_config.global_config.get_config_value(
                reporting.BUG_CONFIG_SECTION, 'client_secret', default='')
        scope = global_config.global_config.get_config_value(
                reporting.BUG_CONFIG_SECTION, 'scope', default='')
        if not client_secret and not client_id:
            logging.error('Unable to generate oauth credentials, client_id '
                    'is %s and client_secret %s. If you do not require oauth '
                    'run this script with --noauth. This may or may not be '
                    'implemented ATM ;).', client_id, client_secret)

        input_flow = client.OAuth2WebServerFlow(client_id=client_id,
                client_secret=client_secret, scope=scope)
        logging.warning('Running oauth flow, make sure you use your chromium '
                'account during autorization.')
        creds = tools.run(input_flow, auth_store)


class CrBugShell(cmd.Cmd):
    def __init__(self, *args, **kwargs):
        cmd.Cmd.__init__(self, *args, **kwargs)
        self.queries = []
        self.labels = []
        if not kwargs.get('noauth'):
            check_auth()
        self.crawler = crbug_crawler.Crawler()


    def do_reap(self, line):
        self.crawler.filter_issues(queries='',
                labels=self.crawler.all_autofiled_label)
        if line:
            try:
                limit = int(line)
            except ValueError:
                logging.warning('Reap can only take an integer argument.')
                return
        else:
            limit = None
        self.crawler.dump_issues(limit=limit)


    def do_query_filter(self, query):
        print 'Adding query %s' % query
        self.queries.append(query)


    def do_label_filter(self, label):
        print 'Adding label %s' % label
        self.labels.append(label)


    def do_show_filters(self, line=''):
        print ('queries: %s, labels %s' %
               (self.queries, self.labels))


    def do_reset(self, line):
        self.crawler.issues = None
        self.queries = []
        self.labels = []


    def do_run_filter(self, line):
        print 'running the following filter: %s' % self.do_show_filters()

        # Populate cached issues if this is a first time query. If we have
        # cached issues from an incremental search, filter those instead.
        if self.crawler.issues:
            self.crawler.issues = self.crawler.filter_labels(
                    self.crawler.issues, self.labels)
            self.crawler.issues = self.crawler.filter_queries(
                    self.crawler.issues, self.queries)
        else:
            self.crawler.filter_issues(queries=' '.join(self.queries),
                    labels=' '.join(self.labels))
        self.crawler.dump_issues()


if __name__ == '__main__':
    CrBugShell().cmdloop()
