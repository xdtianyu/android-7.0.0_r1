# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Module contains code used in dealing with data resources."""

import logging
import uuid

import common
from fake_device_server import server_errors


class ResourceDelegate(object):
    """Delegate for resources held by the various server methods.

    The fake_device_server methods are all fairly similar in that they
    have similar dictionary representations. Server methods use this class to
    delegate access to their data.

    Data is stored based on a combination of <id> + <api_key>
    tuples. The api_key can be passed in to any command with ?key=<api_key>.
    This isn't necessary though as using a default of None is ok.
    """

    def __init__(self, data):
        # Dictionary of data blobs with keys of <id, api_key> pairs that map
        # to the data e.g. for devices, the values are the device dicts, for
        # registration tickets, the values are the ticket dicts.
        self._data = data


    def get_data_val(self, id, api_key):
        """Returns the data value for the given id, api_key pair.

        @param id: ID for data val.
        @param api_key: optional api_key for the data_val.

        Raises:
            server_errors.HTTPError if the data_val doesn't exist.
        """
        key = (id, api_key)
        data_val = self._data.get(key)
        if not data_val:
            # Put the tuple we want inside another tuple, so that Python doesn't
            # unroll |key| and complain that we haven't asked to printf two
            # values.
            raise server_errors.HTTPError(400, 'Invalid data key: %r' % (key,))
        return data_val


    def get_data_vals(self):
        """Returns a list of all data values."""
        return self._data.values()


    def del_data_val(self, id, api_key):
        """Deletes the data value for the given id, api_key pair.

        @param id: ID for data val.
        @param api_key: optional api_key for the data_val.

        Raises:
            server_errors.HTTPError if the data_val doesn't exist.
        """
        key = (id, api_key)
        if key not in self._data:
            # Put the tuple we want inside another tuple, so that Python doesn't
            # unroll |key| and complain that we haven't asked to printf two
            # values.
            raise server_errors.HTTPError(400, 'Invalid data key: %r' % (key,))
        del self._data[key]


    def update_data_val(self, id, api_key, data_in=None, update=True):
        """Helper method for all mutations to data vals.

        If the id isn't given, creates a new template default with a new id.
        Otherwise updates/replaces the given dict with the data based on update.

        @param id: id (if None, creates a new data val).
        @param api_key: optional api_key.
        @param data_in: data dictionary to either update or replace current.
        @param update: fully replace data_val given by id, api_key with data_in.

        Raises:
            server_errors.HTTPError if the id is non-None and not in self._data.
        """
        data_val = None
        if not id:
            # This is an insertion.
            if not data_in:
                raise ValueError('Either id OR data_in must be specified.')

            # Create a new id and insert the data blob into our dictionary.
            id = uuid.uuid4().hex[0:6]
            data_in['id'] = id
            self._data[(id, api_key)] = data_in
            return data_in

        data_val = self.get_data_val(id, api_key)
        if not data_in:
            logging.warning('Received empty data update. Doing nothing.')
            return data_val

        # Update or replace the existing data val.
        if update:
            data_val.update(data_in)
        else:
            if data_val.get('id') != data_in.get('id'):
                raise server_errors.HTTPError(400, "Ticket id doesn't match")

            data_val = data_in
            self._data[(id, api_key)] = data_in

        return data_val
