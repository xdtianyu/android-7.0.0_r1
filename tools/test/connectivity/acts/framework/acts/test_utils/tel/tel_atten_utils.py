#!/usr/bin/env python3.4
#
#   Copyright 2016 - Google
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

import time
import math
from acts.test_utils.tel.tel_defines import ATTEN_MAX_VALUE
from acts.test_utils.tel.tel_defines import ATTEN_MIN_VALUE
from acts.test_utils.tel.tel_defines import MAX_RSSI_RESERVED_VALUE
from acts.test_utils.tel.tel_defines import MIN_RSSI_RESERVED_VALUE


def get_atten(log, atten_obj):
    """Get attenuator current attenuation value.

    Args:
        log: log object.
        atten_obj: attenuator object.
    Returns:
        Current attenuation value.
    """
    return atten_obj.get_atten()


def set_atten(log, atten_obj, target_atten, step_size=0, time_per_step=0):
    """Set attenuator attenuation value.

    Args:
        log: log object.
        atten_obj: attenuator object.
        target_atten: target attenuation value.
        step_size: step size (in unit of dBm) for 'attenuation value setting'.
            This is optional. Default value is 0. If step_size is 0, it means
            the setting will be done in only one step.
        time_per_step: delay time (in unit of second) per step when setting
            the attenuation value.
            This is optional. Default value is 0.
    Returns:
        True is no error happened. Otherwise false.
    """
    try:
        print_name = atten_obj.path
    except AttributeError:
        print_name = str(atten_obj)

    current_atten = get_atten(log, atten_obj)
    info = "set_atten {} from {} to {}".format(print_name, current_atten,
                                               target_atten)
    if step_size > 0:
        info += ", step size {}, time per step {}s.".format(step_size,
                                                            time_per_step)
    log.info(info)
    try:
        delta = target_atten - current_atten
        if step_size > 0:
            number_of_steps = int(abs(delta) / step_size)
            while number_of_steps > 0:
                number_of_steps -= 1
                current_atten += math.copysign(step_size,
                                               (target_atten - current_atten))
                atten_obj.set_atten(current_atten)
                time.sleep(time_per_step)
        atten_obj.set_atten(target_atten)
    except Exception as e:
        log.error("set_atten error happened: {}".format(e))
        return False
    return True


def set_rssi(log,
             atten_obj,
             calibration_rssi,
             target_rssi,
             step_size=0,
             time_per_step=0):
    """Set RSSI value by changing attenuation.

    Args:
        log: log object.
        atten_obj: attenuator object.
        calibration_rssi: RSSI calibration information.
        target_rssi: target RSSI value.
        step_size: step size (in unit of dBm) for 'RSSI value setting'.
            This is optional. Default value is 0. If step_size is 0, it means
            the setting will be done in only one step.
        time_per_step: delay time (in unit of second) per step when setting
            the attenuation value.
            This is optional. Default value is 0.
    Returns:
        True is no error happened. Otherwise false.
    """
    try:
        print_name = atten_obj.path
    except AttributeError:
        print_name = str(atten_obj)

    if target_rssi == MAX_RSSI_RESERVED_VALUE:
        target_atten = ATTEN_MIN_VALUE
    elif target_rssi == MIN_RSSI_RESERVED_VALUE:
        target_atten = ATTEN_MAX_VALUE
    else:
        log.info("set_rssi {} to {}.".format(print_name, target_rssi))
        target_atten = calibration_rssi - target_rssi

    if target_atten < 0:
        log.info("set_rssi: WARNING - you are setting an unreachable RSSI.")
        log.info(
            "max RSSI value on {} is {}. Setting attenuation to 0.".format(
                print_name, calibration_rssi))
        target_atten = 0
    if not set_atten(log, atten_obj, target_atten, step_size, time_per_step):
        log.error("set_rssi to {}failed".format(target_rssi))
        return False
    return True
