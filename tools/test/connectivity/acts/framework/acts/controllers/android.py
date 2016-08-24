#/usr/bin/env python3.4
#
# Copyright (C) 2009 Google Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License"); you may not
# use this file except in compliance with the License. You may obtain a copy of
# the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
# License for the specific language governing permissions and limitations under
# the License.

"""
JSON RPC interface to android scripting engine.
"""

from builtins import str

import json
import os
import socket
import threading
import time

HOST = os.environ.get('AP_HOST', None)
PORT = os.environ.get('AP_PORT', 9999)

class SL4AException(Exception):
    pass

class SL4AAPIError(SL4AException):
    """Raised when remote API reports an error."""

class SL4AProtocolError(SL4AException):
    """Raised when there is some error in exchanging data with server on device."""
    NO_RESPONSE_FROM_HANDSHAKE = "No response from handshake."
    NO_RESPONSE_FROM_SERVER = "No response from server."
    MISMATCHED_API_ID = "Mismatched API id."

def IDCounter():
    i = 0
    while True:
        yield i
        i += 1

class Android(object):
    COUNTER = IDCounter()

    _SOCKET_CONNECT_TIMEOUT = 60

    def __init__(self, cmd='initiate', uid=-1, port=PORT, addr=HOST, timeout=None):
        self.lock = threading.RLock()
        self.client = None  # prevent close errors on connect failure
        self.uid = None
        timeout_time = time.time() + self._SOCKET_CONNECT_TIMEOUT
        while True:
            try:
                self.conn = socket.create_connection(
                        (addr, port), max(1,timeout_time - time.time()))
                self.conn.settimeout(timeout)
                break
            except (TimeoutError, socket.timeout):
                print("Failed to create socket connection!")
                raise
            except (socket.error, IOError):
                # TODO: optimize to only forgive some errors here
                # error values are OS-specific so this will require
                # additional tuning to fail faster
                if time.time() + 1 >= timeout_time:
                    print("Failed to create socket connection!")
                    raise
                time.sleep(1)

        self.client = self.conn.makefile(mode="brw")

        resp = self._cmd(cmd, uid)
        if not resp:
            raise SL4AProtocolError(SL4AProtocolError.NO_RESPONSE_FROM_HANDSHAKE)
        result = json.loads(str(resp, encoding="utf8"))
        if result['status']:
            self.uid = result['uid']
        else:
            self.uid = -1

    def close(self):
        if self.conn is not None:
            self.conn.close()
            self.conn = None

    def _cmd(self, command, uid=None):
        if not uid:
            uid = self.uid
        self.client.write(
            json.dumps({'cmd': command, 'uid': uid})
                .encode("utf8")+b'\n')
        self.client.flush()
        return self.client.readline()

    def _rpc(self, method, *args):
        self.lock.acquire()
        apiid = next(Android.COUNTER)
        self.lock.release()
        data = {'id': apiid,
                'method': method,
                'params': args}
        request = json.dumps(data)
        self.client.write(request.encode("utf8")+b'\n')
        self.client.flush()
        response = self.client.readline()
        if not response:
            raise SL4AProtocolError(SL4AProtocolError.NO_RESPONSE_FROM_SERVER)
        result = json.loads(str(response, encoding="utf8"))
        if result['error']:
            raise SL4AAPIError(result['error'])
        if result['id'] != apiid:
            raise SL4AProtocolError(SL4AProtocolError.MISMATCHED_API_ID)
        return result['result']

    def __getattr__(self, name):
        def rpc_call(*args):
            return self._rpc(name, *args)
        return rpc_call
