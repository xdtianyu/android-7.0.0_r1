#!/usr/bin/python

# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
import logging

import common

import httplib
import httplib2
from autotest_lib.server.cros.dynamic_suite import constants
from chromite.lib import gdata_lib

try:
  from apiclient.discovery import build as apiclient_build
  from apiclient import errors as apiclient_errors
  from oauth2client import file as oauth_client_fileio
except ImportError as e:
  apiclient_build = None
  logging.debug("API client for bug filing disabled. %s", e)


class ProjectHostingApiException(Exception):
    """
    Raised when an api call fails, since the actual
    HTTP error can be cryptic.
    """


class BaseIssue(gdata_lib.Issue):
    """Base issue class with the minimum data to describe a tracker bug.
    """
    def __init__(self, t_issue):
        kwargs = {}
        kwargs.update((keys, t_issue.get(keys))
                       for keys in gdata_lib.Issue.SlotDefaults.keys())
        super(BaseIssue, self).__init__(**kwargs)


class Issue(BaseIssue):
    """
    Class representing an Issue and it's related metadata.
    """
    def __init__(self, t_issue):
        """
        Initialize |self| from tracker issue |t_issue|

        @param t_issue: The base issue we want to use to populate
                        the member variables of this object.

        @raises ProjectHostingApiException: If the tracker issue doesn't
            contain all expected fields needed to create a complete issue.
        """
        super(Issue, self).__init__(t_issue)

        try:
            # The value keyed under 'summary' in the tracker issue
            # is, unfortunately, not the summary but the title. The
            # actual summary is the update at index 0.
            self.summary = t_issue.get('updates')[0]
            self.comments = t_issue.get('updates')[1:]

            # open or closed statuses are classified according to labels like
            # unconfirmed, verified, fixed etc just like through the front end.
            self.state = t_issue.get(constants.ISSUE_STATE)
            self.merged_into = None
            if (t_issue.get(constants.ISSUE_STATUS)
                    == constants.ISSUE_DUPLICATE and
                constants.ISSUE_MERGEDINTO in t_issue):
                parent_issue_dict = t_issue.get(constants.ISSUE_MERGEDINTO)
                self.merged_into = parent_issue_dict.get('issueId')
        except KeyError as e:
            raise ProjectHostingApiException('Cannot create a '
                    'complete issue %s, tracker issue: %s' % (e, t_issue))


class ProjectHostingApiClient():
    """
    Client class for interaction with the project hosting api.
    """

    # Maximum number of results we would like when querying the tracker.
    _max_results_for_issue = 50
    _start_index = 1


    def __init__(self, oauth_credentials, project_name):
        if apiclient_build is None:
            raise ProjectHostingApiException('Cannot get apiclient library.')

        if not oauth_credentials:
            raise ProjectHostingApiException('No oauth_credentials is provided.')

        storage = oauth_client_fileio.Storage(oauth_credentials)
        credentials = storage.get()
        if credentials is None or credentials.invalid:
            raise ProjectHostingApiException('Invalid credentials for Project '
                                             'Hosting api. Cannot file bugs.')

        http = credentials.authorize(httplib2.Http())
        try:
            self._codesite_service = apiclient_build('projecthosting',
                                                     'v2', http=http)
        except (apiclient_errors.Error, httplib2.HttpLib2Error,
                httplib.BadStatusLine) as e:
            raise ProjectHostingApiException(str(e))
        self._project_name = project_name


    def _execute_request(self, request):
        """
        Executes an api request.

        @param request: An apiclient.http.HttpRequest object representing the
                        request to be executed.
        @raises: ProjectHostingApiException if we fail to execute the request.
                 This could happen if we receive an http response that is not a
                 2xx, or if the http object itself encounters an error.

        @return: A deserialized object model of the response body returned for
                 the request.
        """
        try:
            return request.execute()
        except (apiclient_errors.Error, httplib2.HttpLib2Error,
                httplib.BadStatusLine) as e:
            msg = 'Unable to execute your request: %s'
            raise ProjectHostingApiException(msg % e)


    def _get_field(self, field):
        """
        Gets a field from the project.

        This method directly queries the project hosting API using bugdroids1's,
        api key.

        @param field: A selector, which corresponds loosely to a field in the
                      new bug description of the crosbug frontend.
        @raises: ProjectHostingApiException, if the request execution fails.

        @return: A json formatted python dict of the specified field's options,
                 or None if we can't find the api library. This dictionary
                 represents the javascript literal used by the front end tracker
                 and can hold multiple filds.

                The returned dictionary follows a template, but it's structure
                is only loosely defined as it needs to match whatever the front
                end describes via javascript.
                For a new issue interface which looks like:

                field 1: text box
                              drop down: predefined value 1 = description
                                         predefined value 2 = description
                field 2: text box
                              similar structure as field 1

                you will get a dictionary like:
                {
                    'field name 1': {
                        'project realted config': 'config value'
                        'property': [
                            {predefined value for property 1, description},
                            {predefined value for property 2, description}
                        ]
                    },

                    'field name 2': {
                        similar structure
                    }
                    ...
                }
        """
        project = self._codesite_service.projects()
        request = project.get(projectId=self._project_name,
                              fields=field)
        return self._execute_request(request)


    def _list_updates(self, issue_id):
        """
        Retrieve all updates for a given issue including comments, changes to
        it's labels, status etc. The first element in the dictionary returned
        by this method, is by default, the 0th update on the bug; which is the
        entry that created it. All the text in a given update is keyed as
        'content', and updates that contain no text, eg: a change to the status
        of a bug, will contain the emtpy string instead.

        @param issue_id: The id of the issue we want detailed information on.
        @raises: ProjectHostingApiException, if the request execution fails.

        @return: A json formatted python dict that has an entry for each update
                 performed on this issue.
        """
        issue_comments = self._codesite_service.issues().comments()
        request = issue_comments.list(projectId=self._project_name,
                                      issueId=issue_id,
                                      maxResults=self._max_results_for_issue)
        return self._execute_request(request)


    def _get_issue(self, issue_id):
        """
        Gets an issue given it's id.

        @param issue_id: A string representing the issue id.
        @raises: ProjectHostingApiException, if failed to get the issue.

        @return: A json formatted python dict that has the issue content.
        """
        issues = self._codesite_service.issues()
        try:
            request = issues.get(projectId=self._project_name,
                                 issueId=issue_id)
        except TypeError as e:
            raise ProjectHostingApiException(
                'Unable to get issue %s from project %s: %s' %
                (issue_id, self._project_name, str(e)))
        return self._execute_request(request)


    def set_max_results(self, max_results):
        """Set the max results to return.

        @param max_results: An integer representing the maximum number of
            matching results to return per query.
        """
        self._max_results_for_issue = max_results


    def set_start_index(self, start_index):
        """Set the start index, for paging.

        @param start_index: The new start index to use.
        """
        self._start_index = start_index


    def list_issues(self, **kwargs):
        """
        List issues containing the search marker. This method will only list
        the summary, title and id of an issue, though it searches through the
        comments. Eg: if we're searching for the marker '123', issues that
        contain a comment of '123' will appear in the output, but the string
        '123' itself may not, because the output only contains issue summaries.

        @param kwargs:
            q: The anchor string used in the search.
            can: a string representing the search space that is passed to the
                 google api, can be 'all', 'new', 'open', 'owned', 'reported',
                 'starred', or 'to-verify', defaults to 'all'.
            label: A string representing a single label to match.

        @return: A json formatted python dict of all matching issues.

        @raises: ProjectHostingApiException, if the request execution fails.
        """
        issues = self._codesite_service.issues()

        # Asking for issues with None or '' labels will restrict the query
        # to those issues without labels.
        if not kwargs['label']:
            del kwargs['label']

        request = issues.list(projectId=self._project_name,
                              startIndex=self._start_index,
                              maxResults=self._max_results_for_issue,
                              **kwargs)
        return self._execute_request(request)


    def _get_property_values(self, prop_dict):
        """
        Searches a dictionary as returned by _get_field for property lists,
        then returns each value in the list. Effectively this gives us
        all the accepted values for a property. For example, in crosbug,
        'properties' map to things like Status, Labels, Owner etc, each of these
        will have a list within the issuesConfig dict.

        @param prop_dict: dictionary which contains a list of properties.
        @yield: each value in a property list. This can be a dict or any other
                type of datastructure, the caller is responsible for handling
                it correctly.
        """
        for name, property in prop_dict.iteritems():
            if isinstance(property, list):
                for values in property:
                    yield values


    def _get_cros_labels(self, prop_dict):
        """
        Helper function to isolate labels from the labels dictionary. This
        dictionary is of the form:
            {
                "label": "Cr-OS-foo",
                "description": "description"
            },
        And maps to the frontend like so:
            Labels: Cr-???
                    Cr-OS-foo = description
        where Cr-OS-foo is a conveniently predefined value for Label Cr-OS-???.

        @param prop_dict: a dictionary we expect the Cros label to be in.
        @return: A lower case product area, eg: video, factory, ui.
        """
        label = prop_dict.get('label')
        if label and 'Cr-OS-' in label:
            return label.split('Cr-OS-')[1]


    def get_areas(self):
        """
        Parse issue options and return a list of 'Cr-OS' labels.

        @return: a list of Cr-OS labels from crosbug, eg: ['kernel', 'systems']
        """
        if apiclient_build is None:
            logging.error('Missing Api-client import. Cannot get area-labels.')
            return []

        try:
            issue_options_dict = self._get_field('issuesConfig')
        except ProjectHostingApiException as e:
            logging.error('Unable to determine area labels: %s', str(e))
            return []

        # Since we can request multiple fields at once we need to
        # retrieve each one from the field options dictionary, even if we're
        # really only asking for one field.
        issue_options = issue_options_dict.get('issuesConfig')
        if issue_options is None:
            logging.error('The IssueConfig field does not contain issue '
                          'configuration as a member anymore; The project '
                          'hosting api might have changed.')
            return []

        return filter(None, [self._get_cros_labels(each)
                      for each in self._get_property_values(issue_options)
                      if isinstance(each, dict)])


    def create_issue(self, request_body):
        """
        Convert the request body into an issue on the frontend tracker.

        @param request_body: A python dictionary with key-value pairs
                             that represent the fields of the issue.
                             eg: {
                                'title': 'bug title',
                                'description': 'bug description',
                                'labels': ['Type-Bug'],
                                'owner': {'name': 'owner@'},
                                'cc': [{'name': 'cc1'}, {'name': 'cc2'}]
                             }
                             Note the title and descriptions fields of a
                             new bug are not optional, all other fields are.
        @raises: ProjectHostingApiException, if request execution fails.

        @return: The response body, which will contain the metadata of the
                 issue created, or an error response code and information
                 about a failure.
        """
        issues = self._codesite_service.issues()
        request = issues.insert(projectId=self._project_name, sendEmail=True,
                                body=request_body)
        return self._execute_request(request)


    def update_issue(self, issue_id, request_body):
        """
        Convert the request body into an update on an issue.

        @param request_body: A python dictionary with key-value pairs
                             that represent the fields of the update.
                             eg:
                             {
                                'content': 'comment to add',
                                'updates':
                                {
                                    'labels': ['Type-Bug', 'another label'],
                                    'owner': 'owner@',
                                    'cc': ['cc1@', cc2@'],
                                }
                             }
                             Note the owner and cc fields need to be email
                             addresses the tracker recognizes.
        @param issue_id: The id of the issue to update.
        @raises: ProjectHostingApiException, if request execution fails.

        @return: The response body, which will contain information about the
                 update of said issue, or an error response code and information
                 about a failure.
        """
        issues = self._codesite_service.issues()
        request = issues.comments().insert(projectId=self._project_name,
                                           issueId=issue_id, sendEmail=False,
                                           body=request_body)
        return self._execute_request(request)


    def _populate_issue_updates(self, t_issue):
        """
        Populates a tracker issue with updates.

        Any issue is useless without it's updates, since the updates will
        contain both the summary and the comments. We need at least one of
        those to successfully dedupe. The Api doesn't allow us to grab all this
        information in one shot because viewing the comments on an issue
        requires more authority than just viewing it's title.

        @param t_issue: The basic tracker issue, to populate with updates.
        @raises: ProjectHostingApiException, if request execution fails.

        @returns: A tracker issue, with it's updates.
        """
        updates = self._list_updates(t_issue['id'])
        t_issue['updates'] = [update['content'] for update in
                              self._get_property_values(updates)
                              if update.get('content')]
        return t_issue


    def get_tracker_issues_by_text(self, search_text, full_text=True,
                                   include_dupes=False, label=None):
        """
        Find all Tracker issues that contain the specified search text.

        @param search_text: Anchor text to use in the search.
        @param full_text: True if we would like an extensive search through
                          issue comments. If False the search will be restricted
                          to just summaries and titles.
        @param include_dupes: If True, search over both open issues as well as
                          closed issues whose status is 'Duplicate'. If False,
                          only search over open issues.
        @param label: A string representing a single label to match.

        @return: A list of issues that contain the search text, or an empty list
                 when we're either unable to list issues or none match the text.
        """
        issue_list = []
        try:
            search_space = 'all' if include_dupes else 'open'
            feed = self.list_issues(q=search_text, can=search_space,
                                    label=label)
        except ProjectHostingApiException as e:
            logging.error('Unable to search for issues with marker %s: %s',
                          search_text, e)
            return issue_list

        for t_issue in self._get_property_values(feed):
            state = t_issue.get(constants.ISSUE_STATE)
            status = t_issue.get(constants.ISSUE_STATUS)
            is_open_or_dup = (state == constants.ISSUE_OPEN or
                              (state == constants.ISSUE_CLOSED
                               and status == constants.ISSUE_DUPLICATE))
            # All valid issues will have an issue id we can use to retrieve
            # more information about it. If we encounter a failure mode that
            # returns a bad Http response code but doesn't throw an exception
            # we won't find an issue id in the returned json.
            if t_issue.get('id') and is_open_or_dup:
                # TODO(beeps): If this method turns into a performance
                # bottle neck yield each issue and refactor the reporter.
                # For now passing all issues allows us to detect when
                # deduping fails, because multiple issues will match a
                # given query exactly.
                try:
                    if full_text:
                        issue = Issue(self._populate_issue_updates(t_issue))
                    else:
                        issue = BaseIssue(t_issue)
                except ProjectHostingApiException as e:
                    logging.error('Unable to list the updates of issue %s: %s',
                                  t_issue.get('id'), str(e))
                else:
                    issue_list.append(issue)
        return issue_list


    def get_tracker_issue_by_id(self, issue_id):
        """
        Returns an issue object given the id.

        @param issue_id: A string representing the issue id.

        @return: An Issue object on success or None on failure.
        """
        try:
            t_issue = self._get_issue(issue_id)
            return Issue(self._populate_issue_updates(t_issue))
        except ProjectHostingApiException as e:
            logging.error('Creation of an Issue object for %s fails: %s',
                          issue_id, str(e))
            return None
