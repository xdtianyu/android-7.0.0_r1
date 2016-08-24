#!/usr/bin/python

# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""
This script crawls crbug. Sort-of.
Invocation:
    Get all bugs with labels, strings (in summary and/or comments):
        crbug_crawler.py --labels 'one two three'
                         --queries '"first query" "second query"'

    Get baddest open bugs of all time:
        crbug_crawler.py --reap

Tips:
    - Label based queries will return faster than text queries.
    - contrib/crbug_shell.py is a wrapper that allows you to incrementally
        filter search results using this script.
"""

import argparse
import cmd
import logging
import sys
import shlex

import common
from autotest_lib.client.common_lib import global_config
from autotest_lib.server.cros.dynamic_suite import reporting


def _parse_args(args):
    if not args:
        import crbug_crawler
        logging.error('Improper usage of crbug_crawler: %s',
                crbug_crawler.__doc__)
        sys.exit(1)

    description = ('Usage: crbug_crawler.py --reap')
    parser = argparse.ArgumentParser(description=description)
    parser.add_argument('--quiet', help=('Turn off logging noise.'),
            action='store_true', default=False)
    parser.add_argument('--num', help='Number of issues to output.', default=10,
            type=int)
    parser.add_argument('--queries',
                        help=('Search query. Eg: --queries "%s %s"' %
                              ('build_Root', 'login')),
                        default='')
    parser.add_argument('--labels',
                        help=('Search labels. Eg: --labels "%s %s"' %
                              ('autofiled', 'Pri-1')), default=None)
    parser.add_argument('--reap', help=('Top autofiled bugs ordered by count.'),
            action='store_true', default=False)
    return parser.parse_args(args)


class Update(object):
    """Class encapsulating fields of an update to a bug.
    """
    open_statuses = ['Unconfirmed', 'Untriaged', 'Available', 'Assigned',
                     'Started', 'ExternalDependency']
    closed_statuses = ['Fixed', 'Verified', 'Duplicate', 'WontFix', 'Archived']

    def __init__(self, comment='', labels='', status=''):
        self.comment = comment
        self.labels = labels if labels else []
        self.status = status


    def __str__(self):
        msg = 'status: %s' % self.status
        if self.labels:
            msg = '%s labels: %s' % (msg, self.labels)
        if self.comment:
            msg = '%s comment: %s' % (msg, self.comment)
        return msg


class UpdateManager(object):
    """Update manager that allows you to revert status updates.

    This class keeps track of the last update applied and is capable
    of reverting it.
    """

    def __init__(self, autocommit=False):
        """Initialize update manager.

        @param autocommit: If False just print out the update instead
            of committing it.
        """
        self.history = {}
        self.present = {}
        self.reporter = reporting.Reporter()
        self.phapi_lib = self.reporter.get_bug_tracker_client()
        self.autocommit = autocommit


    def revert(self):
        """Only manages status reverts as of now.
        """
        for issue_id, update in self.history.iteritems():
            logging.warning('You will have to manually update %s and %s on %s',
                    self.present[issue_id].labels,
                    self.present[issue_id].comment, issue_id)
            # Create a new update with just the status.
            self.update(issue_id, Update(status=update.status))


    def update(self, old_issue, update):
        """Record the state of an issue before updating it.

        @param old_issue: The issue to update. If an id is specified an
            issue is constructed. If an issue object (as defined in phapi_lib
            Issue)is passed in, it is used directly.
        @param update: The Update object to apply to the issue.
        """
        if type(old_issue) == int:
            old_issue = self.phapi_lib.get_tracker_issue_by_id(old_issue)
        old_update = Update(
                labels=old_issue.labels, status=old_issue.status)

        if not update.status:
            update.status = old_update.status
        elif (update.status not in Update.open_statuses and
              update.status not in Update.closed_statuses):
            raise ValueError('Unknown status %s' % update.status)

        if not self.autocommit:
            logging.warning('Would have applied the following update: '
                    '%s -> %s', old_update, update)
            return

        self.history[old_issue.id] = old_update
        self.reporter.modify_bug_report(
                issue_id=old_issue.id, comment=update.comment,
                label_update=update.labels,
                status=update.status)
        self.present[old_issue.id] = update


class Crawler(object):
    """Class capable of crawling crbug.

    This class applies filters to issues it crawls and caches them locally.
    """

    # The limit at which we ask for confirmation to proceed with the crawl.
    PROMPT_LIMIT = 2000

    def __init__(self):
        self.reporter = reporting.Reporter()
        self.phapi_client = self.reporter.get_bug_tracker_client()
        self.issues = None
        self.all_autofiled_query = 'ANCHOR  TestFailure'
        self.all_autofiled_label = 'autofiled'
        self.prompted = False


    def fuzzy_search(self, query='', label='', fast=True):
        """Returns all issues using one query and/or one label.

        @param query: A string representing the query.
        @param label: A string representing the label.
        @param fast: If true, don't bother fetching comments.

        @return: A list of issues matching the query. If fast is
            specified the issues won't have comments.
        """
        if not query and not label:
            raise ValueError('Require query or labels to make a tracker query, '
                    'try query = "%s" or one of the predefined labels %s' %
                    (self.fuzzy_search_anchor(),
                     self.reporter._PREDEFINED_LABELS))
        if type(label) != str:
            raise ValueError('The crawler only supports one label per query, '
                    'and it must be a string. you supplied %s' % label)
        return self.phapi_client.get_tracker_issues_by_text(
                query, label=label, full_text=not fast)


    @staticmethod
    def _get_autofiled_count(issue):
        """Return the autofiled count.

        @param issue: An issue object that has labels.

        @return: An integer representing the autofiled count.
        """
        for label in issue.labels:
            if 'autofiled-count-' in label:
                return int(label.replace('autofiled-count-', ''))

        # Force bugs without autofiled-count to sink
        return 0


    def _prompt_crawl(self, new_issues, start_index):
        """Warn the user that a crawl is getting large.

        This method prompts for a y/n answer in case the user wants to abort the
        crawl and specify another set of labels/queries.

        @param new_issues: A list of issues used with the start_index to
            determine the number of issues already processed.
        @param start_index: The start index of the next crawl iteration.
        """
        logging.warning('Found %s issues, Crawling issues starting from %s',
                len(new_issues), start_index)
        if start_index > self.PROMPT_LIMIT and not self.prompted:
            logging.warning('Already crawled %s issues, it is possible that'
                    'you\'ve specified a very general label. If this is the '
                    'case consider re-rodering the labels so they start with '
                    'the rarest. Continue crawling [y/n]?',
                    start_index + len(new_issues))
            self.prompted = raw_input() == 'y'
            if not self.prompted:
                sys.exit(0)


    def exhaustive_crawl(self, query='', label='', fast=True):
        """Perform an exhaustive crawl using one label and query string.

        @param query: A string representing one query.
        @param lable: A string representing one label.

        @return A list of issues sorted by descending autofiled count.
        """
        start_index = 0
        self.phapi_client.set_max_results(200)
        logging.warning('Performing an exhaustive crawl with label %s query %s',
                label, query)
        vague_issues = []
        new_issues = self.fuzzy_search(query=query, label=label, fast=fast)
        while new_issues:
            vague_issues += new_issues
            start_index += len(new_issues) + 1
            self.phapi_client.set_start_index(start_index)
            new_issues = self.fuzzy_search(query=query, label=label,
                    fast=fast)
            self._prompt_crawl(new_issues, start_index)

        # Subsequent calls will clear the issues cache with new results.
        self.phapi_client.set_start_index(1)
        return sorted(vague_issues, reverse=True,
                      key=lambda issue: self._get_autofiled_count(issue))


    @staticmethod
    def filter_labels(issues, labels):
        """Takes a list of labels and returns matching issues.

        @param issues: A list of issues to parse for labels.
        @param labels: A list of labels to match.

        @return: A list of matching issues. The issues must contain
            all the labels specified.
        """
        if not labels:
            return issues
        matching_issues = set([])
        labels = set(labels)
        for issue in issues:
            issue_labels = set(issue.labels)
            if issue_labels.issuperset(labels):
                matching_issues.add(issue)
        return matching_issues


    @classmethod
    def does_query_match(cls, issue, query):
        """Check if a query matches the given issue.

        @param issue: The issue to check.
        @param query: The query to check against.

        @return: True if the query matches, false otherwise.
        """
        if query in issue.title or query in issue.summary:
            return True
        # We can only search comments if the issue is a complete issue
        # i.e as defined in phapi_lib.Issue.
        try:
            if any(query in comment for comment in issue.comments):
                return True
        except (AttributeError, TypeError):
            pass
        return False


    @classmethod
    def filter_queries(cls, issues, queries):
        """Take a list of queries and returns matching issues.

        @param issues: A list of issues to parse. If the issues contain
            comments and a query is not in the issues title or summmary,
            the comments are parsed for a substring match.
        @param queries: A list of queries to parse the issues for.
            This method looks for an exact substring match within each issue.

        @return: A list of matching issues.
        """
        if not queries:
            return issues
        matching_issues = set([])
        for issue in issues:
            # For each query, check if it's in the title, description or
            # comments. If a query isn't in any of these, discard the issue.
            for query in queries:
                if cls.does_query_match(issue, query):
                    matching_issues.add(issue)
                else:
                    if issue in matching_issues:
                        logging.warning('%s: %s\n \tPassed a subset of the '
                                'queries but failed query %s',
                                issue.id, issue.title, query)
                        matching_issues.remove(issue)
                    break
        return matching_issues


    def filter_issues(self, queries='', labels=None, fast=True):
        """Run the queries, labels filters by crawling crbug.

        @param queries: A space seperated string of queries, usually passed
            through the command line.
        @param labels: A space seperated string of labels, usually passed
            through the command line.
        @param fast: If specified, skip creating comments for issues since this
            can be a slow process. This value is only a suggestion, since it is
            ignored if multiple queries are specified.
        """
        queries = shlex.split(queries)
        labels = shlex.split(labels) if labels else None

        # We'll need comments to filter multiple queries.
        if len(queries) > 1:
            fast = False
        matching_issues = self.exhaustive_crawl(
                query=queries.pop(0) if queries else '',
                label=labels.pop(0) if labels else '', fast=fast)
        matching_issues = self.filter_labels(matching_issues, labels)
        matching_issues = self.filter_queries(matching_issues, queries)
        self.issues = list(matching_issues)


    def dump_issues(self, limit=None):
        """Print issues.
        """
        if limit and limit < len(self.issues):
            issues = self.issues[:limit]
        else:
            issues = self.issues
        #TODO: Modify formatting, include some paging etc.
        for issue in issues:
            try:
                print ('[%s] %s crbug.com/%s %s' %
                       (self._get_autofiled_count(issue),
                        issue.status, issue.id, issue.title))
            except UnicodeEncodeError as e:
                print "Unicdoe error decoding issue id %s" % issue.id
                continue


def _update_test(args):
    """A simple update test, to record usage.
    """
    updater = UpdateManager(autocommit=True)
    for issue in issues:
        updater.update(issue,
                       Update(comment='this is bogus', labels=['bogus'],
                              status='Assigned'))
    updater.revert()


def configure_logging(quiet=False):
    """Configure logging.

    @param quiet: True to turn off warning messages.
    """
    logging.basicConfig()
    logger = logging.getLogger()
    level = logging.WARNING
    if quiet:
        level = logging.ERROR
    logger.setLevel(level)


def main(args):
    crawler = Crawler()
    if args.reap:
        if args.queries or args.labels:
            logging.error('Query based ranking of bugs not supported yet.')
            return
        queries = ''
        labels = crawler.all_autofiled_label
    else:
        queries = args.queries
        labels = args.labels
    crawler.filter_issues(queries=queries, labels=labels,
            fast=False if queries else True)
    crawler.dump_issues(int(args.num))
    logging.warning('\nThis is a truncated list of %s results, use --num %s '
            'to get them all. If you want more informative results/better '
            'querying capabilities try crbug_shell.py.',
            args.num, len(crawler.issues))


if __name__ == '__main__':
    args = _parse_args(sys.argv[1:])
    configure_logging(args.quiet)
    main(args)

