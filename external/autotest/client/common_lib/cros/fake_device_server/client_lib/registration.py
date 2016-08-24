# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Module contains a simple client lib to the registration RPC."""

import json
import logging
import urllib2

import common
from fake_device_server.client_lib import common_client
from fake_device_server import registration_tickets


class RegistrationClient(common_client.CommonClient):
    """Client library for registrationTickets method."""

    def __init__(self, *args, **kwargs):
        common_client.CommonClient.__init__(
                self, registration_tickets.REGISTRATION_PATH, *args, **kwargs)


    def get_registration_ticket(self, ticket_id):
        """Returns info about the given |ticket_id|.

        @param ticket_id: valid id for a ticket.
        """
        url_h = urllib2.urlopen(self.get_url([ticket_id]))
        return json.loads(url_h.read())


    def update_registration_ticket(self, ticket_id, data,
                                   additional_headers=None, replace=False):
        """Updates the given registration ticket with the new data.

        @param ticket_id: id of the ticket to update.
        @param data: data to update.
        @param additional_headers: additional HTTP headers to pass (expects a
                list of tuples).
        @param replace: If True, replace all data with the given data using the
                PUT operation.
        """
        if not data:
            return

        headers = {'Content-Type': 'application/json'}
        if additional_headers:
            headers.update(additional_headers)

        request = urllib2.Request(self.get_url([ticket_id]), json.dumps(data),
                                  headers=headers)
        if replace:
            request.get_method = lambda: 'PUT'
        else:
            request.get_method = lambda: 'PATCH'

        url_h = urllib2.urlopen(request)
        return json.loads(url_h.read())


    def create_registration_ticket(self):
        """Creates a new registration ticket."""
        # We're going to fall back onto this test access token, if we don't
        # have a real one.  Tests rely on this behavior.
        token = registration_tickets.RegistrationTickets.TEST_ACCESS_TOKEN
        headers = {'Content-Type': 'application/json',
                   'Authorization': 'Bearer %s' % token,
        }
        auth_headers = self.add_auth_headers()
        headers.update(auth_headers)
        data = {'userEmail': 'me'}
        request = urllib2.Request(self.get_url(), json.dumps(data), headers)
        url_h = urllib2.urlopen(request)
        return json.loads(url_h.read())


    def finalize_registration_ticket(self, ticket_id):
        """Finalizes a registration ticket by creating a new device.

        @param ticket_id: id of ticket to finalize.
        """
        request = urllib2.Request(self.get_url([ticket_id, 'finalize']),
                                  data='')
        url_h = urllib2.urlopen(request)
        return json.loads(url_h.read())


    def register_device(self, system_name, channel,
                        oauth_client_id, **kwargs):
        """Goes through the entire registration process using the device args.

        @param system_name: name to give the system.
        @param channel: supported communication channel.
        @param oauth_client_id: see oauth docs.
        @param kwargs: additional dictionary of args to put in config.
        """
        ticket = self.create_registration_ticket()
        logging.info('Initial Ticket: %s', ticket)
        ticket_id = ticket['id']

        device_draft = dict(name=system_name,
                            channel=dict(supportedType=channel),
                            **kwargs)

        ticket = self.update_registration_ticket(
                ticket_id,
                {'deviceDraft': device_draft,
                 'userEmail': 'me',
                 'oauthClientId': oauth_client_id})

        logging.info('Updated Ticket After Claiming: %s', ticket)
        return self.finalize_registration_ticket(ticket_id)
