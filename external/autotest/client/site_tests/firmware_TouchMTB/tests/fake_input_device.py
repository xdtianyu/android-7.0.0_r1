# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""This module provides a fake InputDevice for validator unit tests."""


class FakeInputDevice(object):
    def __init__(self, number_fingers=0, slots={}):
        self.number_fingers = number_fingers
        self.slots = slots

    def get_num_fingers(self):
        return self.number_fingers

    def get_slots(self):
        return self.slots
