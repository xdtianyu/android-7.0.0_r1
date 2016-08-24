# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Provides utility methods for controlling powerd in ChromiumOS."""

import errno
import logging
import multiprocessing
import os
import rtc
import time
import upstart

SYSFS_POWER_STATE = '/sys/power/state'
SYSFS_WAKEUP_COUNT = '/sys/power/wakeup_count'


class SuspendFailure(Exception):
    """Base class for a failure during a single suspend/resume cycle."""
    pass


class SuspendTimeout(SuspendFailure):
    """Suspend took too long, got wakeup event (RTC tick) before it was done."""
    pass


class KernelError(SuspendFailure):
    """Kernel problem encountered during suspend/resume."""
    WHITELIST = [
            # crosbug.com/37594: debug tracing clock desync we don't care about
            (r'kernel/trace/ring_buffer.c:\d+ rb_reserve_next_event',
             r'Delta way too big!'),
        ]


class FirmwareError(SuspendFailure):
    """String 'ERROR' found in firmware log after resume."""
    WHITELIST = [
            # crosbug.com/36762: no one knows, but it has always been there
            ('^stumpy', r'PNP: 002e\.4 70 irq size: 0x0000000001 not assigned'),
            # crbug.com/221538: no one knows what ME errors mean anyway
            ('^parrot', r'ME failed to respond'),
        ]


class SpuriousWakeupError(SuspendFailure):
    """Received spurious wakeup while suspending or woke before schedule."""
    S3_WHITELIST = [  # (<board>, <eventlog wake source>, <syslog wake source>)
            # crbug.com/220014: spurious trackpad IRQs
            ('^link', 'Wake Source | GPIO | 12', ''),
            # crbug.com/345327: unknown, probably same as crbug.com/290923
            ('^x86-alex', '', ''),   # alex can report neither, blanket ignore
            # crbug.com/355106: unknown, possibly related to crbug.com/290923
            ('^lumpy|^parrot', '', 'PM1_STS: WAK PWRBTN'),
        ]
    S0_WHITELIST = [  # (<board>, <kernel wake source>)
            # crbug.com/290923: spurious keyboard IRQ, believed to be from Servo
            ('^x86-alex|^lumpy|^parrot|^butterfly', 'serio0'),
        ]

class MemoryError(SuspendFailure):
    """memory_suspend_test found memory corruption."""
    pass


class SuspendNotAllowed(SuspendFailure):
    """Suspend was not allowed to be performed."""
    pass


def prepare_wakeup(seconds):
    """Prepare the device to wake up from an upcoming suspend.

    @param seconds: The number of seconds to allow the device to suspend.
    """
    # May cause DUT not wake from sleep if the suspend time is 1 second.
    # It happens when the current clock (floating point) is close to the
    # next integer, as the RTC sysfs interface only accepts integers.
    # Make sure it is larger than or equal to 2.
    assert seconds >= 2
    wakeup_count = read_wakeup_count()
    alarm = int(rtc.get_seconds() + seconds)
    logging.debug('Suspend for %d seconds, wakealarm = %d', seconds, alarm)
    rtc.set_wake_alarm(alarm)
    return (alarm, wakeup_count)


def check_wakeup(alarm):
    """Verify that the device did not wakeup early.

    @param alarm: The time at which the device was expected to wake up.
    """
    now = rtc.get_seconds()
    if now < alarm:
        logging.error('Woke up early at %d', now)
        raise SpuriousWakeupError('Woke from suspend early')


def do_suspend(suspend_seconds, delay_seconds=0):
    """Do a suspend using the power manager.

    Wait for |delay_seconds|, suspend the system to RAM (S3), waking up again
    after having been suspended for |suspend_seconds|, using the
    powerd_dbus_suspend program. Function will block until suspend/resume
    has completed or failed. Returns the wake alarm time from the RTC as epoch.

    @param suspend_seconds: Number of seconds to suspend the DUT.
    @param delay_seconds: Number of seconds wait before suspending the DUT.

    """
    alarm, wakeup_count = prepare_wakeup(suspend_seconds)
    upstart.ensure_running('powerd')
    command = ('/usr/bin/powerd_dbus_suspend --delay=%d --timeout=30 '
               '--wakeup_count=%d') % (delay_seconds, wakeup_count)
    logging.info("Running '%s'", command)
    os.system(command)
    check_wakeup(alarm)
    return alarm


def suspend_bg_for_dark_resume(delay_seconds=0):
    """Do a non-blocking indefinite suspend using power manager. ONLY USE THIS
    IF YOU ARE ABSOLUTELY CERTAIN YOU NEED TO.

    Wait for |delay_seconds|, then suspend to RAM (S3). This does not set an RTC
    alarm and does not pass an external wakeup count. It is meant to be used for
    dark resume testing, where the server-side API exposes it in such a fashion
    that the DUT will be woken by the server no matter how the test is exited.

    @param delay_seconds: Number of seconds wait before suspending the DUT.

    """
    upstart.ensure_running('powerd')
    command = ('/usr/bin/powerd_dbus_suspend --delay=%d '
               '--timeout=30') % delay_seconds
    logging.info("Running '%s'", command)
    process = multiprocessing.Process(target=os.system, args=(command,))
    process.start()


def kernel_suspend(seconds):
    """Do a kernel suspend.

    Suspend the system to RAM (S3), waking up again after |seconds|, by directly
    writing to /sys/power/state. Function will block until suspend/resume has
    completed or failed.

    @param seconds: The number of seconds to suspend the device.
    """
    alarm, wakeup_count = prepare_wakeup(seconds)
    logging.debug('Saving wakeup count: %d', wakeup_count)
    write_wakeup_count(wakeup_count)
    try:
        logging.info('Suspending at %d', rtc.get_seconds())
        with open(SYSFS_POWER_STATE, 'w') as sysfs_file:
            sysfs_file.write('mem')
    except IOError as e:
        logging.exception('Writing to %s failed', SYSFS_POWER_STATE)
        if e.errno == errno.EBUSY and rtc.get_seconds() >= alarm:
            # The kernel returns EBUSY if it has to abort because
            # another wakeup fires before we've reached suspend.
            raise SpuriousWakeupError('Received spurious wakeup in kernel.')
        else:
            # Some driver probably failed to suspend properly.
            # A hint as to what failed is in errno.
            raise KernelError('Suspend failed: %s' % e.strerror)
    else:
        logging.info('Woke from suspend at %d', rtc.get_seconds())
    logging.debug('New wakeup count: %d', read_wakeup_count())
    check_wakeup(alarm)


def idle_suspend(seconds):
    """
    Wait for the system to suspend to RAM (S3), scheduling the RTC to wake up
    |seconds| after this function was called. Caller must ensure that the system
    will idle-suspend in time for this to happen. Returns the wake alarm time
    from the RTC as epoch.

    @param seconds: The number of seconds before wakeup.
    """
    alarm, _ = prepare_wakeup(seconds)
    while rtc.get_seconds() < alarm:
        time.sleep(0.2)

    # tell powerd something happened, or it will immediately idle-suspend again
    # TODO: switch to cros.power_utils#call_powerd_dbus_method once this
    # horrible mess with the WiFi tests and this file's imports is solved
    logging.debug('Simulating user activity after idle suspend...')
    os.system('dbus-send --type=method_call --system '
              '--dest=org.chromium.PowerManager /org/chromium/PowerManager '
              'org.chromium.PowerManager.HandleUserActivity')

    return alarm


def memory_suspend(seconds, size=0):
    """Do a memory suspend.

    Suspend the system to RAM (S3), waking up again after |seconds|, using
    the memory_suspend_test tool. Function will block until suspend/resume has
    completed or failed. Returns the wake alarm time from the RTC as epoch.

    @param seconds: The number of seconds to suspend the device.
    @param size: Amount of memory to allocate, in bytes. 
                 Set to 0 to let memory_suspend_test determine amount of memory.   
    """
    # since we cannot have utils.system_output in here, we need a workaround
    output = '/tmp/memory_suspend_output'
    alarm, wakeup_count = prepare_wakeup(seconds)
    status = os.system('/usr/bin/memory_suspend_test --wakeup_count=%d '
                       '--size=%d > %s' % (wakeup_count, size, output))
    status = os.WEXITSTATUS(status)
    if status == 2:
        logging.error('memory_suspend_test found the following errors:')
        for line in open(output, 'r'):
            logging.error(line)
        raise MemoryError('Memory corruption found after resume')
    elif status == 1:
        raise SuspendFailure('Failure in powerd_suspend during memory test')
    elif status:
        raise SuspendFailure('Unknown failure in memory_suspend_test (crash?)')
    check_wakeup(alarm)
    return alarm


def read_wakeup_count():
    """
    Retrieves the current value of /sys/power/wakeup_count.
    """
    with open(SYSFS_WAKEUP_COUNT) as sysfs_file:
        return int(sysfs_file.read().rstrip('\n'))


def write_wakeup_count(wakeup_count):
    """Writes a value to /sys/power/wakeup_count.

    @param wakeup_count: The wakeup count value to write.
    """
    try:
        with open(SYSFS_WAKEUP_COUNT, 'w') as sysfs_file:
            sysfs_file.write(str(wakeup_count))
    except IOError as e:
        if (e.errno == errno.EINVAL and read_wakeup_count() != wakeup_count):
            raise SpuriousWakeupError('wakeup_count changed before suspend')
        else:
            raise
