# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Module containing helpers for interacting with oauth2."""


import json
import urllib
import urllib2


DEFAULT_SCOPE = 'https://www.googleapis.com/auth/clouddevices'
OAUTH_URL = 'https://accounts.google.com/o/oauth2'
# Constant used in oauth2 protocol for device requests.
REDIRECT_URI = 'urn:ietf:wg:oauth:2.0:oob'


def get_oauth2_auth_url(client_id, scope=DEFAULT_SCOPE):
    auth_url = '%s/%s' % (OAUTH_URL, 'auth')
    params = dict(client_id=client_id,
                  scope=scope,
                  response_type='code',
                  redirect_uri=REDIRECT_URI)
    return '%s?%s' % (auth_url, urllib.urlencode(params))


def get_oauth2_user_token(client_id, client_secret, code):
    """Returns the oauth2 token for a user given the auth code."""
    token_url = '%s/%s' % (OAUTH_URL, 'token')
    headers = {'Content-Type': 'application/x-www-form-urlencoded'}
    data = dict(code=code,
                client_id=client_id,
                client_secret=client_secret,
                redirect_uri=REDIRECT_URI,
                grant_type='authorization_code')

    request = urllib2.Request(token_url, data=urllib.urlencode(data),
                              headers=headers)
    url_h = urllib2.urlopen(request)
    auth_result = json.loads(url_h.read())
    return '%s %s' % (auth_result['token_type'],
                      auth_result['access_token'])


def get_oauth2_robot_token(client_id, client_secret, code):
    """Returns the oauth2 token for a robot account to use."""
    token_url = '%s/%s' % (OAUTH_URL, 'token')
    headers = {'Content-Type': 'application/x-www-form-urlencoded'}
    data = dict(code=code,
                client_id=client_id,
                client_secret=client_secret,
                redirect_uri='oob',
                grant_type='authorization_code')

    request = urllib2.Request(token_url, data=urllib.urlencode(data),
                              headers=headers)
    url_h = urllib2.urlopen(request)
    auth_result = json.loads(url_h.read())
    return '%s %s' % (auth_result['token_type'],
                      auth_result['access_token'])
