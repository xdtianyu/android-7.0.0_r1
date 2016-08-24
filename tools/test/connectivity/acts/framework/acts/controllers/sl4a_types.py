#!/usr/bin/env python3.4
#
#   Copyright 2016 - The Android Open Source Project
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

from acts.dict_object import DictObject

class Sl4aEvent(DictObject):
    """Event returned by sl4a calls to eventPoll() and eventWait()

    The 'name' field uniquely identifies the contents of 'data'.

    """
    def __init__(self, name=None, time=None, data=None):
        DictObject.__init__(
                self,
                name=name,
                time=time,
                data=data)
