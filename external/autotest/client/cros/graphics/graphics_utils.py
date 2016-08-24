# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""
Provides graphics related utils, like capturing screenshots or checking on
the state of the graphics driver.
"""

import collections
import glob
import logging
import os
import re
import sys
import time
import traceback
# Please limit the use of the uinput library to this file. Try not to spread
# dependencies and abstract as much as possible to make switching to a different
# input library in the future easier.
import uinput

from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.cros import power_utils
from autotest_lib.client.cros.graphics import drm


# TODO(ihf): Remove xcommand for non-freon builds.
def xcommand(cmd, user=None):
    """
    Add the necessary X setup to a shell command that needs to connect to the X
    server.
    @param cmd: the command line string
    @param user: if not None su command to desired user.
    @return a modified command line string with necessary X setup
    """
    logging.warning('xcommand will be deprecated under freon!')
    traceback.print_stack()
    if user is not None:
        cmd = 'su %s -c \'%s\'' % (user, cmd)
    if not utils.is_freon():
        cmd = 'DISPLAY=:0 XAUTHORITY=/home/chronos/.Xauthority ' + cmd
    return cmd

# TODO(ihf): Remove xsystem for non-freon builds.
def xsystem(cmd, user=None):
    """
    Run the command cmd, using utils.system, after adding the necessary
    setup to connect to the X server.

    @param cmd: The command.
    @param user: The user to switch to, or None for the current user.
    @param timeout: Optional timeout.
    @param ignore_status: Whether to check the return code of the command.
    """
    return utils.system(xcommand(cmd, user))


# TODO(ihf): Remove XSET for non-freon builds.
XSET = 'LD_LIBRARY_PATH=/usr/local/lib xset'

def screen_disable_blanking():
    """ Called from power_Backlight to disable screen blanking. """
    if utils.is_freon():
        # We don't have to worry about unexpected screensavers or DPMS here.
        return
    xsystem(XSET + ' s off')
    xsystem(XSET + ' dpms 0 0 0')
    xsystem(XSET + ' -dpms')


def screen_disable_energy_saving():
    """ Called from power_Consumption to immediately disable energy saving. """
    if utils.is_freon():
        # All we need to do here is enable displays via Chrome.
        power_utils.set_display_power(power_utils.DISPLAY_POWER_ALL_ON)
        return
    # Disable X screen saver
    xsystem(XSET + ' s 0 0')
    # Disable DPMS Standby/Suspend/Off
    xsystem(XSET + ' dpms 0 0 0')
    # Force monitor on
    screen_switch_on(on=1)
    # Save off X settings
    xsystem(XSET + ' q')


def screen_switch_on(on):
    """Turn the touch screen on/off."""
    if on:
        xsystem(XSET + ' dpms force on')
    else:
        xsystem(XSET + ' dpms force off')


def screen_toggle_fullscreen():
    """Toggles fullscreen mode."""
    if utils.is_freon():
        press_keys(['KEY_F11'])
    else:
        press_key_X('F11')


def screen_toggle_mirrored():
    """Toggles the mirrored screen."""
    if utils.is_freon():
        press_keys(['KEY_LEFTCTRL', 'KEY_F4'])
    else:
        press_key_X('ctrl+F4')


def hide_cursor():
    """Hides mouse cursor."""
    # Send a keystroke to hide the cursor.
    if utils.is_freon():
        press_keys(['KEY_UP'])
    else:
        press_key_X('Up')


def screen_wakeup():
    """Wake up the screen if it is dark."""
    # Move the mouse a little bit to wake up the screen.
    if utils.is_freon():
        device = _get_uinput_device_mouse_rel()
        _uinput_emit(device, 'REL_X', 1)
        _uinput_emit(device, 'REL_X', -1)
    else:
        xsystem('xdotool mousemove_relative 1 1')


def switch_screen_on(on):
    """
    Turn the touch screen on/off.

    @param on: On or off.
    """
    if on:
        xsystem(XSET + ' dpms force on')
    else:
        xsystem(XSET + ' dpms force off')


# Don't create a device during build_packages or for tests that don't need it.
uinput_device_keyboard = None
uinput_device_touch = None
uinput_device_mouse_rel = None

# Don't add more events to this list than are used. For a complete list of
# available events check python2.7/site-packages/uinput/ev.py.
UINPUT_DEVICE_EVENTS_KEYBOARD = [
    uinput.KEY_F4,
    uinput.KEY_F11,
    uinput.KEY_KPPLUS,
    uinput.KEY_KPMINUS,
    uinput.KEY_LEFTCTRL,
    uinput.KEY_TAB,
    uinput.KEY_UP,
    uinput.KEY_DOWN,
    uinput.KEY_LEFT,
    uinput.KEY_RIGHT
]
# TODO(ihf): Find an ABS sequence that actually works.
UINPUT_DEVICE_EVENTS_TOUCH = [
    uinput.BTN_TOUCH,
    uinput.ABS_MT_SLOT,
    uinput.ABS_MT_POSITION_X + (0, 2560, 0, 0),
    uinput.ABS_MT_POSITION_Y + (0, 1700, 0, 0),
    uinput.ABS_MT_TRACKING_ID + (0, 10, 0, 0),
    uinput.BTN_TOUCH
]
UINPUT_DEVICE_EVENTS_MOUSE_REL = [
    uinput.REL_X,
    uinput.REL_Y,
    uinput.BTN_MOUSE,
    uinput.BTN_LEFT,
    uinput.BTN_RIGHT
]


def _get_uinput_device_keyboard():
    """
    Lazy initialize device and return it. We don't want to create a device
    during build_packages or for tests that don't need it, hence init with None.
    """
    global uinput_device_keyboard
    if uinput_device_keyboard is None:
        uinput_device_keyboard = uinput.Device(UINPUT_DEVICE_EVENTS_KEYBOARD)
    return uinput_device_keyboard


def _get_uinput_device_mouse_rel():
    """
    Lazy initialize device and return it. We don't want to create a device
    during build_packages or for tests that don't need it, hence init with None.
    """
    global uinput_device_mouse_rel
    if uinput_device_mouse_rel is None:
        uinput_device_mouse_rel = uinput.Device(UINPUT_DEVICE_EVENTS_MOUSE_REL)
    return uinput_device_mouse_rel


def _get_uinput_device_touch():
    """
    Lazy initialize device and return it. We don't want to create a device
    during build_packages or for tests that don't need it, hence init with None.
    """
    global uinput_device_touch
    if uinput_device_touch is None:
        uinput_device_touch = uinput.Device(UINPUT_DEVICE_EVENTS_TOUCH)
    return uinput_device_touch


def _uinput_translate_name(event_name):
    """
    Translates string |event_name| to uinput event.
    """
    return getattr(uinput, event_name)


def _uinput_emit(device, event_name, value, syn=True):
    """
    Wrapper for uinput.emit. Emits event with value.
    Example: ('REL_X', 20), ('BTN_RIGHT', 1)
    """
    event = _uinput_translate_name(event_name)
    device.emit(event, value, syn)


def _uinput_emit_click(device, event_name, syn=True):
    """
    Wrapper for uinput.emit_click. Emits click event. Only KEY and BTN events
    are accepted, otherwise ValueError is raised. Example: 'KEY_A'
    """
    event = _uinput_translate_name(event_name)
    device.emit_click(event, syn)


def _uinput_emit_combo(device, event_names, syn=True):
    """
    Wrapper for uinput.emit_combo. Emits sequence of events.
    Example: ['KEY_LEFTCTRL', 'KEY_LEFTALT', 'KEY_F5']
    """
    events = [_uinput_translate_name(en) for en in event_names]
    device.emit_combo(events, syn)


def press_keys(key_list):
    """Presses the given keys as one combination.

    Please do not leak uinput dependencies outside of the file.

    @param key: A list of key strings, e.g. ['LEFTCTRL', 'F4']
    """
    _uinput_emit_combo(_get_uinput_device_keyboard(), key_list)


# TODO(ihf): Remove press_key_X for non-freon builds.
def press_key_X(key_str):
    """Presses the given keys as one combination.
    @param key: A string of keys, e.g. 'ctrl+F4'.
    """
    if utils.is_freon():
        raise error.TestFail('freon: press_key_X not implemented')
    command = 'xdotool key %s' % key_str
    xsystem(command)


def click_mouse():
    """Just click the mouse.
    Presumably only hacky tests use this function.
    """
    logging.info('click_mouse()')
    # Move a little to make the cursor appear.
    device = _get_uinput_device_mouse_rel()
    _uinput_emit(device, 'REL_X', 1)
    # Some sleeping is needed otherwise events disappear.
    time.sleep(0.1)
    # Move cursor back to not drift.
    _uinput_emit(device, 'REL_X', -1)
    time.sleep(0.1)
    # Click down.
    _uinput_emit(device, 'BTN_LEFT', 1)
    time.sleep(0.2)
    # Release click.
    _uinput_emit(device, 'BTN_LEFT', 0)


# TODO(ihf): this function is broken. Make it work.
def activate_focus_at(rel_x, rel_y):
    """Clicks with the mouse at screen position (x, y).

    This is a pretty hacky method. Using this will probably lead to
    flaky tests as page layout changes over time.
    @param rel_x: relative horizontal position between 0 and 1.
    @param rel_y: relattive vertical position between 0 and 1.
    """
    width, height = get_internal_resolution()
    device = _get_uinput_device_touch()
    _uinput_emit(device, 'ABS_MT_SLOT', 0, syn=False)
    _uinput_emit(device, 'ABS_MT_TRACKING_ID', 1, syn=False)
    _uinput_emit(device, 'ABS_MT_POSITION_X', int(rel_x * width), syn=False)
    _uinput_emit(device, 'ABS_MT_POSITION_Y', int(rel_y * height), syn=False)
    _uinput_emit(device, 'BTN_TOUCH', 1, syn=True)
    time.sleep(0.2)
    _uinput_emit(device, 'BTN_TOUCH', 0, syn=True)


def take_screenshot(resultsdir, fname_prefix, extension='png'):
    """Take screenshot and save to a new file in the results dir.
    Args:
      @param resultsdir:   Directory to store the output in.
      @param fname_prefix: Prefix for the output fname.
      @param extension:    String indicating file format ('png', 'jpg', etc).
    Returns:
      the path of the saved screenshot file
    """

    old_exc_type = sys.exc_info()[0]

    next_index = len(glob.glob(
        os.path.join(resultsdir, '%s-*.%s' % (fname_prefix, extension))))
    screenshot_file = os.path.join(
        resultsdir, '%s-%d.%s' % (fname_prefix, next_index, extension))
    logging.info('Saving screenshot to %s.', screenshot_file)

    try:
        image = drm.crtcScreenshot()
        image.save(screenshot_file)
    except Exception as err:
        # Do not raise an exception if the screenshot fails while processing
        # another exception.
        if old_exc_type is None:
            raise
        logging.error(err)

    return screenshot_file


def take_screenshot_crop_by_height(fullpath, final_height, x_offset_pixels,
                                   y_offset_pixels):
    """
    Take a screenshot, crop to final height starting at given (x, y) coordinate.
    Image width will be adjusted to maintain original aspect ratio).

    @param fullpath: path, fullpath of the file that will become the image file.
    @param final_height: integer, height in pixels of resulting image.
    @param x_offset_pixels: integer, number of pixels from left margin
                            to begin cropping.
    @param y_offset_pixels: integer, number of pixels from top margin
                            to begin cropping.
    """
    image = drm.crtcScreenshot()
    image.crop()
    width, height = image.size
    # Preserve aspect ratio: Wf / Wi == Hf / Hi
    final_width = int(width * (float(final_height) / height))
    box = (x_offset_pixels, y_offset_pixels,
           x_offset_pixels + final_width, y_offset_pixels + final_height)
    cropped = image.crop(box)
    cropped.save(fullpath)
    return fullpath


def take_screenshot_crop_x(fullpath, box=None):
    """
    Take a screenshot using import tool, crop according to dim given by the box.
    @param fullpath: path, full path to save the image to.
    @param box: 4-tuple giving the upper left and lower right pixel coordinates.
    """

    if box:
        img_w, img_h, upperx, uppery = box
        cmd = ('/usr/local/bin/import -window root -depth 8 -crop '
                      '%dx%d+%d+%d' % (img_w, img_h, upperx, uppery))
    else:
        cmd = ('/usr/local/bin/import -window root -depth 8')

    old_exc_type = sys.exc_info()[0]
    try:
        xsystem('%s %s' % (cmd, fullpath))
    except Exception as err:
        # Do not raise an exception if the screenshot fails while processing
        # another exception.
        if old_exc_type is None:
            raise
        logging.error(err)


def take_screenshot_crop(fullpath, box=None, crtc_id=None):
    """
    Take a screenshot using import tool, crop according to dim given by the box.
    @param fullpath: path, full path to save the image to.
    @param box: 4-tuple giving the upper left and lower right pixel coordinates.
    """
    if not utils.is_freon():
        return take_screenshot_crop_x(fullpath, box)
    if crtc_id is not None:
        image = drm.crtcScreenshot(crtc_id)
    else:
        image = drm.crtcScreenshot(get_internal_crtc())
    if box:
        image = image.crop(box)
    image.save(fullpath)
    return fullpath


_MODETEST_CONNECTOR_PATTERN = re.compile(
    r'^(\d+)\s+\d+\s+(connected|disconnected)\s+(\S+)\s+\d+x\d+\s+\d+\s+\d+')

_MODETEST_MODE_PATTERN = re.compile(
    r'\s+.+\d+\s+(\d+)\s+\d+\s+\d+\s+\d+\s+(\d+)\s+\d+\s+\d+\s+\d+\s+flags:.+type:'
    r' preferred')

_MODETEST_CRTCS_START_PATTERN = re.compile(r'^id\s+fb\s+pos\s+size')

_MODETEST_CRTC_PATTERN = re.compile(
    r'^(\d+)\s+(\d+)\s+\((\d+),(\d+)\)\s+\((\d+)x(\d+)\)')

Connector = collections.namedtuple(
    'Connector', [
        'cid',  # connector id (integer)
        'ctype',  # connector type, e.g. 'eDP', 'HDMI-A', 'DP'
        'connected',  # boolean
        'size',  # current screen size, e.g. (1024, 768)
        'encoder',  # encoder id (integer)
        # list of resolution tuples, e.g. [(1920,1080), (1600,900), ...]
        'modes',
    ])

CRTC = collections.namedtuple(
    'CRTC', [
        'id',  # crtc id
        'fb',  # fb id
        'pos',  # position, e.g. (0,0)
        'size',  # size, e.g. (1366,768)
    ])


def get_display_resolution():
    """
    Parses output of modetest to determine the display resolution of the dut.
    @return: tuple, (w,h) resolution of device under test.
    """
    if not utils.is_freon():
        return _get_display_resolution_x()

    connectors = get_modetest_connectors()
    for connector in connectors:
        if connector.connected:
            return connector.size
    return None


def _get_display_resolution_x():
    """
    Used temporarily while Daisy's modetest isn't working
    TODO(dhaddock): remove when no longer needed
    @return: tuple, (w,h) resolution of device under test.
    """
    env_vars = 'DISPLAY=:0.0 ' \
                              'XAUTHORITY=/home/chronos/.Xauthority'
    cmd = '%s xrandr | egrep -o "current [0-9]* x [0-9]*"' % env_vars
    output = utils.system_output(cmd)
    match = re.search(r'(\d+) x (\d+)', output)
    if len(match.groups()) == 2:
        return int(match.group(1)), int(match.group(2))
    return None


def _get_num_outputs_connected():
    """
    Parses output of modetest to determine the number of connected displays
    @return: The number of connected displays
    """
    connected = 0
    connectors = get_modetest_connectors()
    for connector in connectors:
        if connector.connected:
            connected = connected + 1

    return connected


def get_num_outputs_on():
    """
    Retrieves the number of connected outputs that are on.

    Return value: integer value of number of connected outputs that are on.
    """

    return _get_num_outputs_connected()


def call_xrandr(args_string=''):
    """
    Calls xrandr with the args given by args_string.

    e.g. call_xrandr('--output LVDS1 --off') will invoke:
        'xrandr --output LVDS1 --off'

    @param args_string: A single string containing all arguments.

    Return value: Output of xrandr
    """
    return utils.system_output(xcommand('xrandr %s' % args_string))


def get_modetest_connectors():
    """
    Retrieves a list of Connectors using modetest.

    Return value: List of Connectors.
    """
    connectors = []
    modetest_output = utils.system_output('modetest -c')
    for line in modetest_output.splitlines():
        # First search for a new connector.
        connector_match = re.match(_MODETEST_CONNECTOR_PATTERN, line)
        if connector_match is not None:
            cid = int(connector_match.group(1))
            connected = False
            if connector_match.group(2) == 'connected':
                connected = True
            ctype = connector_match.group(3)
            size = (-1, -1)
            encoder = -1
            modes = None
            connectors.append(
                Connector(cid, ctype, connected, size, encoder, modes))
        else:
            # See if we find corresponding line with modes, sizes etc.
            mode_match = re.match(_MODETEST_MODE_PATTERN, line)
            if mode_match is not None:
                size = (int(mode_match.group(1)), int(mode_match.group(2)))
                # Update display size of last connector in list.
                c = connectors.pop()
                connectors.append(
                    Connector(
                        c.cid, c.ctype, c.connected, size, c.encoder,
                        c.modes))
    return connectors


def get_modetest_crtcs():
    """
    Returns a list of CRTC data.

    Sample:
        [CRTC(id=19, fb=50, pos=(0, 0), size=(1366, 768)),
         CRTC(id=22, fb=54, pos=(0, 0), size=(1920, 1080))]
    """
    crtcs = []
    modetest_output = utils.system_output('modetest -p')
    found = False
    for line in modetest_output.splitlines():
        if found:
            crtc_match = re.match(_MODETEST_CRTC_PATTERN, line)
            if crtc_match is not None:
                crtc_id = int(crtc_match.group(1))
                fb = int(crtc_match.group(2))
                x = int(crtc_match.group(3))
                y = int(crtc_match.group(4))
                width = int(crtc_match.group(5))
                height = int(crtc_match.group(6))
                # CRTCs with fb=0 are disabled, but lets skip anything with
                # trivial width/height just in case.
                if not (fb == 0 or width == 0 or height == 0):
                    crtcs.append(CRTC(crtc_id, fb, (x, y), (width, height)))
            elif line and not line[0].isspace():
                return crtcs
        if re.match(_MODETEST_CRTCS_START_PATTERN, line) is not None:
            found = True
    return crtcs


def get_modetest_output_state():
    """
    Reduce the output of get_modetest_connectors to a dictionary of connector/active states.
    """
    connectors = get_modetest_connectors()
    outputs = {}
    for connector in connectors:
        # TODO(ihf): Figure out why modetest output needs filtering.
        if connector.connected:
            outputs[connector.ctype] = connector.connected
    return outputs


def get_output_rect(output):
    """Gets the size and position of the given output on the screen buffer.

    @param output: The output name as a string.

    @return A tuple of the rectangle (width, height, fb_offset_x,
            fb_offset_y) of ints.
    """
    connectors = get_modetest_connectors()
    for connector in connectors:
        if connector.ctype == output:
            # Concatenate two 2-tuples to 4-tuple.
            return connector.size + (0, 0)  # TODO(ihf): Should we use CRTC.pos?
    return (0, 0, 0, 0)


def get_internal_resolution():
    if utils.is_freon():
        if has_internal_display():
            crtcs = get_modetest_crtcs()
            if len(crtcs) > 0:
                return crtcs[0].size
        return (-1, -1)
    else:
        connector = get_internal_connector_name()
        width, height, _, _ = get_output_rect_x(connector)
        return (width, height)


def has_internal_display():
    """Checks whether the DUT is equipped with an internal display.

    @return True if internal display is present; False otherwise.
    """
    return bool(get_internal_connector_name())


def get_external_resolution():
    """Gets the resolution of the external display.

    @return A tuple of (width, height) or None if no external display is
            connected.
    """
    if utils.is_freon():
        offset = 1 if has_internal_display() else 0
        crtcs = get_modetest_crtcs()
        if len(crtcs) > offset and crtcs[offset].size != (0, 0):
            return crtcs[offset].size
        return None
    else:
        connector = get_external_connector_name()
        width, height, _, _ = get_output_rect_x(connector)
        if width == 0 and height == 0:
            return None
        return (width, height)


def get_output_rect_x(output):
    """Gets the size and position of the given output on the screen buffer.

    @param output: The output name as a string.

    @return A tuple of the rectangle (width, height, fb_offset_x,
            fb_offset_y) of ints.
    """
    regexp = re.compile(
            r'^([-A-Za-z0-9]+)\s+connected\s+(\d+)x(\d+)\+(\d+)\+(\d+)',
            re.M)
    match = regexp.findall(call_xrandr())
    for m in match:
        if m[0] == output:
            return (int(m[1]), int(m[2]), int(m[3]), int(m[4]))
    return (0, 0, 0, 0)


def get_display_output_state():
    """
    Retrieves output status of connected display(s).

    Return value: dictionary of connected display states.
    """
    if utils.is_freon():
        return get_modetest_output_state()
    else:
        return get_xrandr_output_state()


def get_xrandr_output_state():
    """
    Retrieves output status of connected display(s) using xrandr.

    When xrandr report a display is "connected", it doesn't mean the
    display is active. For active display, it will have '*' after display mode.

    Return value: dictionary of connected display states.
                  key = output name
                  value = True if the display is active; False otherwise.
    """
    output = call_xrandr().split('\n')
    xrandr_outputs = {}
    current_output_name = ''

    # Parse output of xrandr, line by line.
    for line in output:
        if line.startswith('Screen'):
            continue
        # If the line contains "connected", it is a connected display, as
        # opposed to a disconnected output.
        if line.find(' connected') != -1:
            current_output_name = line.split()[0]
            # Temporarily mark it as inactive until we see a '*' afterward.
            xrandr_outputs[current_output_name] = False
            continue

        # If "connected" was not found, this is a line that shows a display
        # mode, e.g:    1920x1080      50.0     60.0     24.0
        # Check if this has an asterisk indicating it's on.
        if line.find('*') != -1 and current_output_name:
            xrandr_outputs[current_output_name] = True
            # Reset the output name since this should not be set more than once.
            current_output_name = ''

    return xrandr_outputs


def set_xrandr_output(output_name, enable):
    """
    Sets the output given by |output_name| on or off.

    Parameters:
        output_name       name of output, e.g. 'HDMI1', 'LVDS1', 'DP1'
        enable            True or False, indicating whether to turn on or off
    """
    call_xrandr('--output %s --%s' % (output_name, 'auto' if enable else 'off'))


def set_modetest_output(output_name, enable):
    # TODO(ihf): figure out what to do here. Don't think this is the right command.
    # modetest -s <connector_id>[,<connector_id>][@<crtc_id>]:<mode>[-<vrefresh>][@<format>]  set a mode
    pass


def set_display_output(output_name, enable):
    """
    Sets the output given by |output_name| on or off.
    """
    set_modetest_output(output_name, enable)


# TODO(ihf): Fix this for multiple external connectors.
def get_external_crtc(index=0):
    offset = 1 if has_internal_display() else 0
    crtcs = get_modetest_crtcs()
    if len(crtcs) > offset + index:
        return crtcs[offset + index].id
    return -1


def get_internal_crtc():
    if has_internal_display():
        crtcs = get_modetest_crtcs()
        if len(crtcs) > 0:
            return crtcs[0].id
    return -1


# TODO(ihf): Fix this for multiple external connectors.
def get_external_connector_name():
    """Gets the name of the external output connector.

    @return The external output connector name as a string, if any.
            Otherwise, return False.
    """
    outputs = get_display_output_state()
    for output in outputs.iterkeys():
        if outputs[output] and (output.startswith('HDMI')
                or output.startswith('DP')
                or output.startswith('DVI')
                or output.startswith('VGA')):
            return output
    return False


def get_internal_connector_name():
    """Gets the name of the internal output connector.

    @return The internal output connector name as a string, if any.
            Otherwise, return False.
    """
    outputs = get_display_output_state()
    for output in outputs.iterkeys():
        # reference: chromium_org/chromeos/display/output_util.cc
        if (output.startswith('eDP')
                or output.startswith('LVDS')
                or output.startswith('DSI')):
            return output
    return False


def wait_output_connected(output):
    """Wait for output to connect.

    @param output: The output name as a string.

    @return: True if output is connected; False otherwise.
    """
    def _is_connected(output):
        """Helper function."""
        outputs = get_display_output_state()
        if output not in outputs:
            return False
        return outputs[output]

    return utils.wait_for_value(lambda: _is_connected(output),
                                expected_value=True)


def set_content_protection(output_name, state):
    """
    Sets the content protection to the given state.

    @param output_name: The output name as a string.
    @param state: One of the states 'Undesired', 'Desired', or 'Enabled'

    """
    if utils.is_freon():
        raise error.TestFail('freon: set_content_protection not implemented')
    call_xrandr('--output %s --set "Content Protection" %s' %
                (output_name, state))


def get_content_protection(output_name):
    """
    Gets the state of the content protection.

    @param output_name: The output name as a string.
    @return: A string of the state, like 'Undesired', 'Desired', or 'Enabled'.
             False if not supported.

    """
    if utils.is_freon():
        raise error.TestFail('freon: get_content_protection not implemented')

    output = call_xrandr('--verbose').split('\n')
    current_output_name = ''

    # Parse output of xrandr, line by line.
    for line in output:
        # If the line contains 'connected', it is a connected display.
        if line.find(' connected') != -1:
            current_output_name = line.split()[0]
            continue
        if current_output_name != output_name:
            continue
        # Search the line like: 'Content Protection:     Undesired'
        match = re.search(r'Content Protection:\t(\w+)', line)
        if match:
            return match.group(1)

    return False


def wflinfo_cmd():
    """
    Return a wflinfo command appropriate to the current graphics platform/api.
    """
    return 'wflinfo -p %s -a %s' % (utils.graphics_platform(),
                                    utils.graphics_api())


def is_sw_rasterizer():
    """Return true if OpenGL is using a software rendering."""
    cmd = wflinfo_cmd() + ' | grep "OpenGL renderer string"'
    output = utils.run(cmd)
    result = output.stdout.splitlines()[0]
    logging.info('wflinfo: %s', result)
    # TODO(ihf): Find exhaustive error conditions (especially ARM).
    return 'llvmpipe' in result.lower() or 'soft' in result.lower()


class GraphicsKernelMemory(object):
    """
    Reads from sysfs to determine kernel gem objects and memory info.
    """
    # These are sysfs fields that will be read by this test.  For different
    # architectures, the sysfs field paths are different.  The "paths" are given
    # as lists of strings because the actual path may vary depending on the
    # system.  This test will read from the first sysfs path in the list that is
    # present.
    # e.g. ".../memory" vs ".../gpu_memory" -- if the system has either one of
    # these, the test will read from that path.

    exynos_fields = {
        'gem_objects': ['/sys/kernel/debug/dri/0/exynos_gem_objects'],
        'memory': ['/sys/class/misc/mali0/device/memory',
                   '/sys/class/misc/mali0/device/gpu_memory'],
    }
    # TODO Add memory nodes once the GPU patches landed.
    rockchip_fields = {}
    tegra_fields = {
        'memory': ['/sys/kernel/debug/memblock/memory'],
    }
    x86_fields = {
        'gem_objects': ['/sys/kernel/debug/dri/0/i915_gem_objects'],
        'memory': ['/sys/kernel/debug/dri/0/i915_gem_gtt'],
    }
    arm_fields = {}
    arch_fields = {
        'exynos5': exynos_fields,
        'tegra': tegra_fields,
        'rockchip': rockchip_fields,
        'i386': x86_fields,
        'x86_64': x86_fields,
        'arm': arm_fields,
    }

    num_errors = 0

    def get_memory_keyvals(self):
        """
        Reads the graphics memory values and returns them as keyvals.
        """
        keyvals = {}

        # Get architecture type and list of sysfs fields to read.
        arch = utils.get_cpu_soc_family()

        if not arch in self.arch_fields:
            raise error.TestFail('Architecture "%s" not yet supported.' % arch)
        fields = self.arch_fields[arch]

        for field_name in fields:
            possible_field_paths = fields[field_name]
            field_value = None
            for path in possible_field_paths:
                if utils.system('ls %s' % path):
                    continue
                field_value = utils.system_output('cat %s' % path)
                break

            if not field_value:
                logging.error('Unable to find any sysfs paths for field "%s"',
                              field_name)
                self.num_errors += 1
                continue

            parsed_results = GraphicsKernelMemory._parse_sysfs(field_value)

            for key in parsed_results:
                keyvals['%s_%s' % (field_name, key)] = parsed_results[key]

            if 'bytes' in parsed_results and parsed_results['bytes'] == 0:
                logging.error('%s reported 0 bytes', field_name)
                self.num_errors += 1

        keyvals['meminfo_MemUsed'] = (utils.read_from_meminfo('MemTotal') -
                                      utils.read_from_meminfo('MemFree'))
        keyvals['meminfo_SwapUsed'] = (utils.read_from_meminfo('SwapTotal') -
                                       utils.read_from_meminfo('SwapFree'))
        return keyvals

    @staticmethod
    def _parse_sysfs(output):
        """
        Parses output of graphics memory sysfs to determine the number of
        buffer objects and bytes.

        Arguments:
            output      Unprocessed sysfs output
        Return value:
            Dictionary containing integer values of number bytes and objects.
            They may have the keys 'bytes' and 'objects', respectively.  However
            the result may not contain both of these values.
        """
        results = {}
        labels = ['bytes', 'objects']

        for line in output.split('\n'):
            # Strip any commas to make parsing easier.
            line_words = line.replace(',', '').split()

            prev_word = None
            for word in line_words:
                # When a label has been found, the previous word should be the
                # value. e.g. "3200 bytes"
                if word in labels and word not in results and prev_word:
                    logging.info(prev_word)
                    results[word] = int(prev_word)

                prev_word = word

            # Once all values has been parsed, return.
            if len(results) == len(labels):
                return results

        return results


class GraphicsStateChecker(object):
    """
    Analyzes the state of the GPU and log history. Should be instantiated at the
    beginning of each graphics_* test.
    """
    crash_blacklist = []
    dirty_writeback_centisecs = 0
    existing_hangs = {}

    _BROWSER_VERSION_COMMAND = '/opt/google/chrome/chrome --version'
    _HANGCHECK = ['drm:i915_hangcheck_elapsed', 'drm:i915_hangcheck_hung',
                  'Hangcheck timer elapsed...']
    _HANGCHECK_WARNING = ['render ring idle']
    _MESSAGES_FILE = '/var/log/messages'

    def __init__(self, raise_error_on_hang=True):
        """
        Analyzes the initial state of the GPU and log history.
        """
        # Attempt flushing system logs every second instead of every 10 minutes.
        self.dirty_writeback_centisecs = utils.get_dirty_writeback_centisecs()
        utils.set_dirty_writeback_centisecs(100)
        self._raise_error_on_hang = raise_error_on_hang
        logging.info(utils.get_board_with_frequency_and_memory())
        self.graphics_kernel_memory = GraphicsKernelMemory()

        if utils.get_cpu_arch() != 'arm':
            if is_sw_rasterizer():
                raise error.TestFail('Refusing to run on SW rasterizer.')
            logging.info('Initialize: Checking for old GPU hangs...')
            messages = open(self._MESSAGES_FILE, 'r')
            for line in messages:
                for hang in self._HANGCHECK:
                    if hang in line:
                        logging.info(line)
                        self.existing_hangs[line] = line
            messages.close()

    def finalize(self):
        """
        Analyzes the state of the GPU, log history and emits warnings or errors
        if the state changed since initialize. Also makes a note of the Chrome
        version for later usage in the perf-dashboard.
        """
        utils.set_dirty_writeback_centisecs(self.dirty_writeback_centisecs)
        new_gpu_hang = False
        new_gpu_warning = False
        if utils.get_cpu_arch() != 'arm':
            logging.info('Cleanup: Checking for new GPU hangs...')
            messages = open(self._MESSAGES_FILE, 'r')
            for line in messages:
                for hang in self._HANGCHECK:
                    if hang in line:
                        if not line in self.existing_hangs.keys():
                            logging.info(line)
                            for warn in self._HANGCHECK_WARNING:
                                if warn in line:
                                    new_gpu_warning = True
                                    logging.warning(
                                        'Saw GPU hang warning during test.')
                                else:
                                    logging.warning('Saw GPU hang during test.')
                                    new_gpu_hang = True
            messages.close()

            if is_sw_rasterizer():
                logging.warning('Finished test on SW rasterizer.')
                raise error.TestFail('Finished test on SW rasterizer.')
            if self._raise_error_on_hang and new_gpu_hang:
                raise error.TestError('Detected GPU hang during test.')
            if new_gpu_hang:
                raise error.TestWarn('Detected GPU hang during test.')
            if new_gpu_warning:
                raise error.TestWarn('Detected GPU warning during test.')


    def get_memory_access_errors(self):
        """ Returns the number of errors while reading memory stats. """
        return self.graphics_kernel_memory.num_errors

    def get_memory_keyvals(self):
        """ Returns memory stats. """
        return self.graphics_kernel_memory.get_memory_keyvals()
