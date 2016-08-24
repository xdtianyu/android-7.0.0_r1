# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import inspect
import logging
import sys


TYPE_KEY = 'xmlrpc_struct_type_key'


def deserialize(serialized, module=None):
    """Deserialize an XmlRpcStruct.

    Because Python XMLRPC doesn't understand anything more than basic
    types, we're forced to reinvent type serialization.  This is one way
    to do it.

    This function looks at the constructor and figures out what subset of
    |serialized| will be accepted as arguments.

    @param serialized dict representing a serialized XmlRpcStruct.
    @param module module object to pull classes from.  Defaults to
            this file.
    @return an XmlRpcStruct object built from |serialized|.

    """
    if TYPE_KEY not in serialized:
        logging.error('Failed to deserialize XmlRpcStruct because of '
                      'missing type.')
        logging.error('Got serialized object: %r', serialized)
        return None

    if module is None:
        module = sys.modules[__name__]
    klass = getattr(module, serialized[TYPE_KEY])
    constructor_args = inspect.getargspec(klass.__init__)
    optional_args = []
    if constructor_args.defaults:
        # Valid args should now be a list of all the parameters that have
        # default values.
        optional_args = constructor_args.args[-len(constructor_args.defaults):]
        # skip to argument 1 because the first argument is always |self|.
        required_args = constructor_args.args[1:-len(constructor_args.defaults)]
    args = []
    for arg in required_args:
        if arg not in serialized:
            logging.error('Failed to find non-keyword argument %s', arg)
            return None

        args.append(serialized[arg])
    kwargs = dict(filter(lambda (k, v): k in optional_args,
                         serialized.iteritems()))
    logging.debug('Constructing %s object with args=%r, kwargs=%r',
                  serialized[TYPE_KEY], args, kwargs)
    return klass(*args, **kwargs)


class XmlRpcStruct(object):
    """Enables deserialization by injecting the proper class type.

    To make deserilization work for you, write a constructor like:

    def __init__(self, arg1, arg2='foo'):
        # It is important that self.arg1 = arg1.  Otherwise arguments
        # won't be sent over the wire correctly.
        self.arg1 = arg1
        self.arg2 = arg2

    Serialization happens automatically when using XMLRPC proxies, since
    Python's XMLRPC framework will take all fields from your object and drop
    them into a dict to go over the wire.  To deserialize your object on the
    other side, call deserialize() and pass in the module defining the
    requisite class.

    serialize() is provided to allow objects to fake that they are XMLRPC
    proxies.

    """

    def __init__(self):
        super(XmlRpcStruct, self).__init__()
        setattr(self, TYPE_KEY, self.__class__.__name__)


    def serialize(self):
        """@return dict of object fields."""
        return self.__dict__
