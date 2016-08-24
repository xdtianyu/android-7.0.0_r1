# Copyriht (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging

# Following Beaglebone GPIO pins are used to control the 8 port multiplexer.
MUX_EN = '20'
MUX_S0 = '115'
MUX_S1 = '117'
MUX_S2 = '49'

ENABLE_MUX = '1'
DISABLE_MUX = '0'

# Various commands used to control the GPIO pins at the kernel level.
LS_GPIO_DIRECTORY = 'ls /sys/class/gpio'
EXPORT_GPIO_PIN = 'echo %s > /sys/class/gpio/export'
SET_GPIO_DIRECTION = 'echo high > /sys/class/gpio/gpio%s/direction'
SET_GPIO_VALUE = 'echo %s > /sys/class/gpio/gpio%s/value'
UNEXPORT_GPIO_PIN = 'echo %s > /sys/class/gpio/unexport'

# Values passed to each GPIO pin to enable a specific port.
# Bit sequence: MUX_S2, MUX_S1, MUX_S0
# Example: To enable port 5, MUX_S2 will be set to 1, MUX_S1 will be set to 0
# and MUX_S0 will be set to 1
ports = {0:'000', 1:'001', 2:'010', 3:'011', 4:'100', 5:'101', 6:'110', 7:'111'}

class USBMuxController(object):
    """Class to control individual ports on a 8 port USB switch/hub.

    This class is responsible for enabling all the GPIO pins on the beaglebone
    needed to control the 8 port USB switch/hub. In order to use this USB mux
    controller you need custom hardware setup which connects to the beaglebone
    and drives the 8 port relay switch to turn the individual ports on the USB
    hub 'on'/'off'.

    TODO(harpreet) Write a USB mux hardware design document and provide a link
    here.

    """

    version = 1

    def __init__(self, host):
        """Initializes this USB Mux Controller instance.

        @param host: Host where the test will be run.

        """
        self.host = host

    def __del__(self):
        """Destructor of USBMuxController.

        Disables all GPIO pins used that control the multiplexer.

        """
        self.mux_teardown()

    def mux_setup(self):
        """
        Enable GPIO pins that control the multiplexer.

        """
        logging.info('Enable GPIO pins that control the multiplexer.')
        self.enable_gpio_pins(MUX_EN)
        self.disable_all_ports()
        self.enable_gpio_pins(MUX_S2)
        self.enable_gpio_pins(MUX_S1)
        self.enable_gpio_pins(MUX_S0)

    def mux_teardown(self):
        """
        Disable the multiplexer and unexport all GPIO pins.

        """
        logging.info('Start USB multiplexer teardown.')
        self.disable_all_ports()
        # unexport gpio pins
        logging.info('Unexport all GPIO pins.')
        self.host.servo.system(UNEXPORT_GPIO_PIN % MUX_S0)
        self.host.servo.system(UNEXPORT_GPIO_PIN % MUX_S1)
        self.host.servo.system(UNEXPORT_GPIO_PIN % MUX_S2)
        self.host.servo.system(UNEXPORT_GPIO_PIN % MUX_EN)
        logging.info('Completed USB multiplexer teardown. All USB ports should'
                     'now be turned off.')

    def enable_gpio_pins(self, pin):
        """
        Enables the given GPIO pin by exporting the pin and setting the
        direction.

        @param pin: GPIO pin to be enabled.

        """
        if 'gpio' + pin not in self.host.servo.system_output(LS_GPIO_DIRECTORY):
            self.host.servo.system(EXPORT_GPIO_PIN % pin)
            self.host.servo.system(SET_GPIO_DIRECTION % pin)

    def enable_port(self, usb_port):
        """
        Enables the given port on the USB hub.

        @param usb_port: USB port to be enabled.

        """
        port = ports[usb_port]
        logging.info('Enable port %s.', port)
        self.mux_setup()
        self.disable_all_ports()

        logging.info('Set GPIO pins to correct logic levels.')
        self.host.servo.system(SET_GPIO_VALUE % (port[0], MUX_S2))
        self.host.servo.system(SET_GPIO_VALUE % (port[1], MUX_S1))
        self.host.servo.system(SET_GPIO_VALUE % (port[2], MUX_S0))

        logging.info('Enable USB multiplexer. Appropriate port should now be'
                     'enabled')
        self.host.servo.system(SET_GPIO_VALUE % (ENABLE_MUX, MUX_EN))

    def disable_all_ports(self):
        """
        Disables all USB ports that are currently enabled.

        """
        if 'gpio20' in self.host.servo.system_output(LS_GPIO_DIRECTORY):
            logging.info('Disable USB ports.')
            self.host.servo.system(SET_GPIO_VALUE % (DISABLE_MUX, MUX_EN))
