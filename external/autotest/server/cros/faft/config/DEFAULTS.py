# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Default configuration values for FAFT tests go into this file.

For the ability to override these values on a platform specific basis, please
refer to the config object implementation.
"""


class Values(object):
    """We have a class here to allow for inheritence. This is less important
    defaults, but very helpful for platform overrides.
    """

    mode_switcher_type = 'keyboard_dev_switcher'
    fw_bypasser_type = 'ctrl_d_bypasser'

    chrome_ec = False
    chrome_usbpd = False
    dark_resume_capable = False
    has_lid = True
    has_keyboard = True
    has_powerbutton = True
    rec_button_dev_switch = False
    long_rec_combo = False
    use_u_boot = False
    ec_capability = list()
    gbb_version = 1.1
    wp_voltage = 'pp1800'
    spi_voltage = 'pp1800'
    key_checker = [[0x29, 'press'],
                   [0x32, 'press'],
                   [0x32, 'release'],
                   [0x29, 'release'],
                   [0x28, 'press'],
                   [0x28, 'release']]
    key_checker_strict = [[0x29, 'press'],
                          [0x29, 'release'],
                          [0x32, 'press'],
                          [0x32, 'release'],
                          [0x28, 'press'],
                          [0x28, 'release'],
                          [0x61, 'press'],
                          [0x61, 'release']]

    # Has eventlog support including proper timestamps. (Only for old boards!
    # Never disable this "temporarily, until we get around to implementing it"!)
    has_eventlog = True

    # Delay between power-on and firmware screen
    firmware_screen = 10

    # Delay between reboot and first ping response from the DUT
    # When this times out, it indicates we're stuck at a firmware screen.
    # Hence, bypass action has to be taken if we want to proceed.
    delay_reboot_to_ping = 30

    # Delay between keypresses in firmware screen
    confirm_screen = 3

    # Only True on Alex/ZGB which needs a transition state to enter dev mode.
    need_dev_transition = False

    # Delay between passing firmware screen and text mode warning screen
    legacy_text_screen = 20

    # The developer screen timeouts fit our spec
    dev_screen_timeout = 30

    # Delay for waiting beep done
    beep = 1

    # Delay between power-on and plug USB
    usb_plug = 10

    # Delay for waiting client to shutdown
    shutdown = 30

    # Timeout of confirming DUT shutdown
    shutdown_timeout = 60

    # Delay between EC boot and ChromeEC console functional
    ec_boot_to_console = 1.2

    # Delay between EC boot and pressing power button
    ec_boot_to_pwr_button = 0.5

    # Delay of EC software sync hash calculating time
    software_sync = 6

    # Delay of EC software sync updating EC
    software_sync_update = 2

    # Duration of holding power button to power off DUT normally
    hold_pwr_button_poweroff = 2

    # Duration of holding power button to power on DUT normally
    # (also known as SHORT_DELAY in hdctools)
    hold_pwr_button_poweron = 0.2

    # devserver startup time
    devserver = 10

    # Delay for user to power cycle the device
    user_power_cycle = 20

    # Delay after /sbin/shutdown before pressing power button
    powerup_ready = 10

    # Time in second to wait after changing servo state for programming
    servo_prog_state_delay = 0
