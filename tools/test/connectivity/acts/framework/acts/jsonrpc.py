#!/usr/bin/env python3.4
#
#   Copyright 2016- Google, Inc.
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.

"""
A simple JSON RPC client.
"""
import json
import time
from urllib import request

class HTTPError(Exception):
    pass

class RemoteError(Exception):
    pass

def JSONCounter():
    """A counter that generates JSON RPC call IDs.

    Follows the increasing integer sequence. Every time this function is
    called, the next number in the sequence is returned.
    """
    i = 0
    while True:
        yield i
        i += 1

class JSONRPCClient:
    COUNTER = JSONCounter()
    headers = {'content-type': 'application/json'}
    def __init__(self, baseurl):
        self._baseurl = baseurl

    def call(self, path, methodname=None, *args):
        """Wrapper for the internal _call method.

        A retry is performed if the initial call fails to compensate for
        unstable networks.

        Params:
            path: Path of the rpc service to be appended to the base url.
            methodname: Method name of the RPC call.
            args: A tuple of arguments for the RPC call.

        Returns:
            The returned message of the JSON RPC call from the server.
        """
        try:
            return self._call(path, methodname, *args)
        except:
            # Take five and try again
            time.sleep(5)
            return self._call(path, methodname, *args)

    def _post_json(self, url, payload):
        """Performs an HTTP POST request with a JSON payload.

        Params:
            url: The full URL to post the payload to.
            payload: A JSON string to be posted to server.

        Returns:
            The HTTP response code and text.
        """
        req = request.Request(url)
        req.add_header('Content-Type', 'application/json')
        resp = request.urlopen(req, data=payload.encode("utf-8"))
        txt = resp.read()
        return resp.code, txt.decode('utf-8')

    def _call(self, path, methodname=None, *args):
        """Performs a JSON RPC call and return the response.

        Params:
            path: Path of the rpc service to be appended to the base url.
            methodname: Method name of the RPC call.
            args: A tuple of arguments for the RPC call.

        Returns:
            The returned message of the JSON RPC call from the server.

        Raises:
            HTTPError: Raised if the http post return code is not 200.
            RemoteError: Raised if server returned an error.
        """
        jsonid = next(JSONRPCClient.COUNTER)
        payload = json.dumps({"method": methodname,
                              "params": args,
                              "id": jsonid})
        url = self._baseurl + path
        status_code, text = self._post_json(url, payload)
        if status_code != 200:
            raise HTTPError(text)
        r = json.loads(text)
        if r['error']:
            raise RemoteError(r['error'])
        return r['result']

    def sys(self, *args):
        return self.call("sys", *args)

    def __getattr__(self, name):
        def rpc_call(*args):
            return self.call('uci', name, *args)
        return rpc_call
