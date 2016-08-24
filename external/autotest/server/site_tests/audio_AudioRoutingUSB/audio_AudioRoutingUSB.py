# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging, time

from autotest_lib.server import test
from autotest_lib.client.common_lib import error

# After connecting/disconnecting the USB headset, we wait a while for the event
# to be discovered, and CRAS to switch the output device.
SWITCH_DELAY = 7

class audio_AudioRoutingUSB(test.test):
    version = 1

    def get_opened_device(self, host):
        """Returns the opened pcm device under /dev/snd."""
        output = host.run('lsof -Fn +D /dev/snd', ignore_status=True).stdout
        return parse_pcm_device(output);

    def run_once(self, host):
        try:
            host.run('aplay /dev/zero </dev/null >/dev/null 2>&1 &')
            self.run_test_while_audio_is_playing(host)
        finally:
            host.run('killall aplay')

    def run_test_while_audio_is_playing(self, host):
        host.servo.set('dut_usb2_prtctl', 'on')

        # First disconnect the headset from DUT
        host.servo.set('usb_mux_oe2', 'off')
        time.sleep(SWITCH_DELAY)
        dev1 = self.get_opened_device(host)

        # Connect the headset to DUT
        host.servo.set('usb_mux_oe2', 'on')
        time.sleep(SWITCH_DELAY)
        dev2 = self.get_opened_device(host)

        # Disconnect the headset from DUT
        host.servo.set('usb_mux_oe2', 'off')
        time.sleep(SWITCH_DELAY)
        dev3 = self.get_opened_device(host)

        logging.info('dev1: %s, dev2: %s, dev3:%s', dev1, dev2, dev3)
        if dev1 == dev2:
            raise error.TestFail('Same audio device used when the headset is '
                                 'connected. Make sure a USB headset is '
                                 'plugged into DUT_USB (TYPE A/J4), and '
                                 'DUT_IN (TYPE MICRO-B/J5) is '
                                 'connected to a USB port on the device')
        if dev1 != dev3:
            raise error.TestFail('The audio device didn\'t switch back to the '
                                 'original one after the USB headset is '
                                 'unplugged')

def parse_pcm_device(input):
  """
  Parses the output of lsof command. Returns the pcm device opened.

  >>> input = '''
  ... p1847
  ... n/dev/snd/pcmC0D0p
  ... n/dev/snd/controlC0
  ... n/dev/snd/controlC0
  ... n/dev/snd/controlC0
  ... '''
  >>> parse_pcm_device(input)
  '/dev/snd/pcmC0D0p'
  """
  devices = set()
  for line in input.split('\n'):
    if line and line.startswith('n/dev/snd/pcmC'):
      devices.add(line[1:])
      logging.info('opened devices: %s', devices)
      if len(devices) != 1:
        raise error.TestError('Should open one and only one device')
      return devices.pop()
