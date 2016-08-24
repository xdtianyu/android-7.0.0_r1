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


from acts.controllers.android import Android
import json
import os
import socket
import threading

HOST = os.environ.get('AP_HOST', None)
PORT = os.environ.get('AP_PORT', 9999)

class SL4NException(Exception):
    pass

class SL4NAPIError(SL4NException):
    """Raised when remote API reports an error."""

class SL4NProtocolError(SL4NException):
    """Raised when there is some error in exchanging data with server on device."""
    NO_RESPONSE_FROM_HANDSHAKE = "No response from handshake."
    NO_RESPONSE_FROM_SERVER = "No response from server."
    MISMATCHED_API_ID = "Mismatched API id."

def IDCounter():
    i = 0
    while True:
        yield i
        i += 1

class NativeAndroid(Android):
    COUNTER = IDCounter()

    def _rpc(self, method, *args):
        self.lock.acquire()
        apiid = next(NativeAndroid.COUNTER)
        self.lock.release()
        data = {'id': apiid,
                'method': method,
                'params': args}
        request = json.dumps(data)
        self.client.write(request.encode("utf8")+b'\n')
        self.client.flush()
        response = self.client.readline()
        if not response:
            raise SL4NProtocolError(SL4NProtocolError.NO_RESPONSE_FROM_SERVER)
        #TODO: (tturney) fix the C side from sending \x00 char over the socket.
        result = json.loads(
                str(response, encoding="utf8").rstrip().replace("\x00", ""))

        if result['error']:
            raise SL4NAPIError(result['error'])
        if result['id'] != apiid:
            raise SL4NProtocolError(SL4NProtocolError.MISMATCHED_API_ID)
        return result['result']