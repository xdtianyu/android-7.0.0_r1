# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import utils, dbus
from autotest_lib.client.bin import test
from autotest_lib.client.common_lib import error

class desktopui_SpeechSynthesisSemiAuto(test.test):
  version = 1

  def run_once(self):
    # Start the speech_synthesizer DBus service
    utils.system('sudo /usr/sbin/speech_synthesizer &')
    # Test if the TTS service works by using the DBus API
    # If successful, the synthesized audio should be heard
    bus = dbus.SystemBus()
    proxy = bus.get_object("org.chromium.SpeechSynthesizer",
                           "/org/chromium/SpeechSynthesizer")
    speech = dbus.Interface(proxy, "org.chromium.SpeechSynthesizerInterface")
    res = speech.Speak("Welcome to Chromium O S")
    if res == False:
      raise error.TestFail('Speak call failed.')
