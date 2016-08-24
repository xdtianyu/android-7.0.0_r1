
"""
  Copyright (c) 2007 Jan-Klaas Kollhof

  This file is part of jsonrpc.

  jsonrpc is free software; you can redistribute it and/or modify
  it under the terms of the GNU Lesser General Public License as published by
  the Free Software Foundation; either version 2.1 of the License, or
  (at your option) any later version.

  This software is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public License
  along with this software; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
"""

import os
import urllib2
from autotest_lib.client.common_lib import error as exceptions

from json import decoder

from json import encoder as json_encoder
json_encoder_class = json_encoder.JSONEncoder


# Try to upgrade to the Django JSON encoder. It uses the standard json encoder
# but can handle DateTime
try:
    # See http://crbug.com/418022 too see why the try except is needed here.
    from django import conf as django_conf
    # The serializers can't be imported if django isn't configured.
    # Using try except here doesn't work, as test_that initializes it's own
    # django environment (setup_django_lite_environment) which raises import
    # errors if the django dbutils have been previously imported, as importing
    # them leaves some state behind.
    # This the variable name must not be undefined or empty string.
    if os.environ.get(django_conf.ENVIRONMENT_VARIABLE, None):
        from django.core.serializers import json as django_encoder
        json_encoder_class = django_encoder.DjangoJSONEncoder
except ImportError:
    pass


class JSONRPCException(Exception):
    pass

class ValidationError(JSONRPCException):
    """Raised when the RPC is malformed."""
    def __init__(self, error, formatted_message):
        """Constructor.

        @param error: a dict of error info like so:
                      {error['name']: 'ErrorKind',
                       error['message']: 'Pithy error description.',
                       error['traceback']: 'Multi-line stack trace'}
        @formatted_message: string representation of this exception.
        """
        self.problem_keys = eval(error['message'])
        self.traceback = error['traceback']
        super(ValidationError, self).__init__(formatted_message)

def BuildException(error):
    """Exception factory.

    Given a dict of error info, determine which subclass of
    JSONRPCException to build and return.  If can't determine the right one,
    just return a JSONRPCException with a pretty-printed error string.

    @param error: a dict of error info like so:
                  {error['name']: 'ErrorKind',
                   error['message']: 'Pithy error description.',
                   error['traceback']: 'Multi-line stack trace'}
    """
    error_message = '%(name)s: %(message)s\n%(traceback)s' % error
    for cls in JSONRPCException.__subclasses__():
        if error['name'] == cls.__name__:
            return cls(error, error_message)
    for cls in (exceptions.CrosDynamicSuiteException.__subclasses__() +
                exceptions.RPCException.__subclasses__()):
        if error['name'] == cls.__name__:
            return cls(error_message)
    return JSONRPCException(error_message)

class ServiceProxy(object):
    def __init__(self, serviceURL, serviceName=None, headers=None):
        self.__serviceURL = serviceURL
        self.__serviceName = serviceName
        self.__headers = headers or {}

    def __getattr__(self, name):
        if self.__serviceName is not None:
            name = "%s.%s" % (self.__serviceName, name)
        return ServiceProxy(self.__serviceURL, name, self.__headers)

    def __call__(self, *args, **kwargs):
        postdata = json_encoder_class().encode({'method': self.__serviceName,
                                                'params': args + (kwargs,),
                                                'id': 'jsonrpc'})
        request = urllib2.Request(self.__serviceURL, data=postdata,
                                  headers=self.__headers)
        respdata = urllib2.urlopen(request).read()
        try:
            resp = decoder.JSONDecoder().decode(respdata)
        except ValueError:
            raise JSONRPCException('Error decoding JSON reponse:\n' + respdata)
        if resp['error'] is not None:
            raise BuildException(resp['error'])
        else:
            return resp['result']
