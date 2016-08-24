"""\
RPC request handler Django.  Exposed RPC interface functions should be
defined in rpc_interface.py.
"""

__author__ = 'showard@google.com (Steve Howard)'

import traceback, pydoc, re, urllib, logging, logging.handlers, inspect
from autotest_lib.frontend.afe.json_rpc import serviceHandler
from autotest_lib.frontend.afe import models, rpc_utils
from autotest_lib.client.common_lib import global_config
from autotest_lib.frontend.afe import rpcserver_logging

LOGGING_REGEXPS = [r'.*add_.*',
                   r'delete_.*',
                   r'.*remove_.*',
                   r'modify_.*',
                   r'create.*',
                   r'set_.*']
FULL_REGEXP = '(' + '|'.join(LOGGING_REGEXPS) + ')'
COMPILED_REGEXP = re.compile(FULL_REGEXP)


def should_log_message(name):
    return COMPILED_REGEXP.match(name)


class RpcMethodHolder(object):
    'Dummy class to hold RPC interface methods as attributes.'


class RpcHandler(object):
    def __init__(self, rpc_interface_modules, document_module=None):
        self._rpc_methods = RpcMethodHolder()
        self._dispatcher = serviceHandler.ServiceHandler(self._rpc_methods)

        # store all methods from interface modules
        for module in rpc_interface_modules:
            self._grab_methods_from(module)

        # get documentation for rpc_interface we can send back to the
        # user
        if document_module is None:
            document_module = rpc_interface_modules[0]
        self.html_doc = pydoc.html.document(document_module)


    def get_rpc_documentation(self):
        return rpc_utils.raw_http_response(self.html_doc)


    def raw_request_data(self, request):
        if request.method == 'POST':
            return request.raw_post_data
        return urllib.unquote(request.META['QUERY_STRING'])


    def execute_request(self, json_request):
        return self._dispatcher.handleRequest(json_request)


    def decode_request(self, json_request):
        return self._dispatcher.translateRequest(json_request)


    def dispatch_request(self, decoded_request):
        return self._dispatcher.dispatchRequest(decoded_request)


    def log_request(self, user, decoded_request, decoded_result,
                    remote_ip, log_all=False):
        if log_all or should_log_message(decoded_request['method']):
            msg = '%s| %s:%s %s'  % (remote_ip, decoded_request['method'],
                                     user, decoded_request['params'])
            if decoded_result['err']:
                msg += '\n' + decoded_result['err_traceback']
                rpcserver_logging.rpc_logger.error(msg)
            else:
                rpcserver_logging.rpc_logger.info(msg)


    def encode_result(self, results):
        return self._dispatcher.translateResult(results)


    def handle_rpc_request(self, request):
        remote_ip = self._get_remote_ip(request)
        user = models.User.current_user()
        json_request = self.raw_request_data(request)
        decoded_request = self.decode_request(json_request)

        decoded_request['remote_ip'] = remote_ip
        decoded_result = self.dispatch_request(decoded_request)
        result = self.encode_result(decoded_result)
        if rpcserver_logging.LOGGING_ENABLED:
            self.log_request(user, decoded_request, decoded_result,
                             remote_ip)
        return rpc_utils.raw_http_response(result)


    def handle_jsonp_rpc_request(self, request):
        request_data = request.GET['request']
        callback_name = request.GET['callback']
        # callback_name must be a simple identifier
        assert re.search(r'^\w+$', callback_name)

        result = self.execute_request(request_data)
        padded_result = '%s(%s)' % (callback_name, result)
        return rpc_utils.raw_http_response(padded_result,
                                           content_type='text/javascript')


    @staticmethod
    def _allow_keyword_args(f):
        """\
        Decorator to allow a function to take keyword args even though
        the RPC layer doesn't support that.  The decorated function
        assumes its last argument is a dictionary of keyword args and
        passes them to the original function as keyword args.
        """
        def new_fn(*args):
            assert args
            keyword_args = args[-1]
            args = args[:-1]
            return f(*args, **keyword_args)
        new_fn.func_name = f.func_name
        return new_fn


    def _grab_methods_from(self, module):
        for name in dir(module):
            if name.startswith('_'):
                continue
            attribute = getattr(module, name)
            if not inspect.isfunction(attribute):
                continue
            decorated_function = RpcHandler._allow_keyword_args(attribute)
            setattr(self._rpc_methods, name, decorated_function)


    def _get_remote_ip(self, request):
        """Get the ip address of a RPC caller.

        Returns the IP of the request, accounting for the possibility of
        being behind a proxy.
        If a Django server is behind a proxy, request.META["REMOTE_ADDR"] will
        return the proxy server's IP, not the client's IP.
        The proxy server would provide the client's IP in the
        HTTP_X_FORWARDED_FOR header.

        @param request: django.core.handlers.wsgi.WSGIRequest object.

        @return: IP address of remote host as a string.
                 Empty string if the IP cannot be found.
        """
        remote = request.META.get('HTTP_X_FORWARDED_FOR', None)
        if remote:
            # X_FORWARDED_FOR returns client1, proxy1, proxy2,...
            remote = remote.split(',')[0].strip()
        else:
            remote = request.META.get('REMOTE_ADDR', '')
        return remote
