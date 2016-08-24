#!/usr/bin/python
#
# Copyright (c) 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import errno, logging, os, select, signal, subprocess, time

from autotest_lib.client.bin import utils, test
from autotest_lib.client.common_lib import error


class platform_CompressedSwap(test.test):
    """
    Verify compressed swap is configured and basically works.
    """
    version = 1
    executable = 'hog'
    swap_enable_file = '/home/chronos/.swap_enabled'
    swap_disksize_file = '/sys/block/zram0/disksize'


    def setup(self):
        os.chdir(self.srcdir)
        utils.make(self.executable)

    def check_for_oom(self, hogs):
        for p in hogs:
            retcode = p.poll() # returns None if the thread is still running
            if retcode is not None:
                logging.info('hog %d of %d is gone, assume oom: retcode %s' %
                             (hogs.index(p) + 1, len(hogs), retcode))
                return True
        return False

    # Check for low memory notification by polling /dev/chromeos-low-mem.
    def getting_low_mem_notification(self):
        lowmem_fd = open('/dev/chromeos-low-mem', 'r')
        lowmem_poller = select.poll()
        lowmem_poller.register(lowmem_fd, select.POLLIN)
        events=lowmem_poller.poll(0)
        lowmem_fd.close()
        for fd, flag in events:
            if flag & select.POLLIN:
                return True
        return False

    def run_once(self, just_checking_lowmem=False, checking_for_oom=False):

        memtotal = utils.read_from_meminfo('MemTotal')
        swaptotal = utils.read_from_meminfo('SwapTotal')
        free_target = (memtotal + swaptotal) * 0.03

        # Check for proper swap space configuration.
        # If the swap enable file says "0", swap.conf does not create swap.
        if not just_checking_lowmem and not checking_for_oom:
            if os.path.exists(self.swap_enable_file):
                enable_size = utils.read_one_line(self.swap_enable_file)
            else:
                enable_size = "nonexistent" # implies nonzero
            if enable_size == "0":
                if swaptotal != 0:
                    raise error.TestFail('The swap enable file said 0, but'
                                         ' swap was still enabled for %d.' %
                                         swaptotal)
                logging.info('Swap enable (0), swap disabled.')
            else:
                # Rather than parsing swap.conf logic to calculate a size,
                # use the value it writes to /sys/block/zram0/disksize.
                if not os.path.exists(self.swap_disksize_file):
                    raise error.TestFail('The %s swap enable file should have'
                                         ' caused zram to load, but %s was'
                                         ' not found.' %
                                         (enable_size, self.swap_disksize_file))
                disksize = utils.read_one_line(self.swap_disksize_file)
                swaprequested = int(disksize) / 1000
                if (swaptotal < swaprequested * 0.9 or
                    swaptotal > swaprequested * 1.1):
                    raise error.TestFail('Our swap of %d K is not within 10%'
                                         ' of the %d K we requested.' %
                                         (swaptotal, swaprequested))
                logging.info('Swap enable (%s), requested %d, total %d'
                             % (enable_size, swaprequested, swaptotal))

        first_oom = 0
        first_lowmem = 0
        cleared_low_mem_notification = False

        # Loop over hog creation until MemFree+SwapFree approaches 0.
        # Confirm we do not see any OOMs (procs killed due to Out Of Memory).
        hogs = []
        cmd = [ self.srcdir + '/' + self.executable, '50' ]
        logging.debug('Memory hog command line is %s' % cmd)
        while len(hogs) < 200:
            memfree = utils.read_from_meminfo('MemFree')
            swapfree = utils.read_from_meminfo('SwapFree')
            total_free = memfree + swapfree
            logging.debug('nhogs %d: memfree %d, swapfree %d' %
                          (len(hogs), memfree, swapfree))
            if not checking_for_oom and total_free < free_target:
                break;

            p = subprocess.Popen(cmd)
            utils.write_one_line('/proc/%d/oom_score_adj' % p.pid, '1000')
            hogs.append(p)

            time.sleep(2)

            if self.check_for_oom(hogs):
                first_oom = len(hogs)
                break

            # Check for low memory notification.
            if self.getting_low_mem_notification():
                if first_lowmem == 0:
                    first_lowmem = len(hogs)
                logging.info('Got low memory notification after hog %d' %
                             len(hogs))

        logging.info('Finished creating %d hogs, SwapFree %d, MemFree %d, '
                     'low mem at %d, oom at %d' %
                     (len(hogs), swapfree, memfree, first_lowmem, first_oom))

        if not checking_for_oom and first_oom > 0:
            utils.system("killall -TERM hog")
            raise error.TestFail('Oom detected after %d hogs created' %
                                 len(hogs))

        # Before cleaning up all the hogs, verify that killing hogs back to
        # our initial low memory notification causes notification to end.
        if first_lowmem > 0:
            hogs_killed = 0;
            for p in hogs:
                if not self.getting_low_mem_notification():
                    cleared_low_mem_notification = True
                    logging.info('Cleared low memory notification after %d '
                                 'hogs were killed' % hogs_killed)
                    break;
                try:
                    p.kill()
                except OSError, e:
                    if e.errno == errno.ESRCH:
                        logging.info('Hog %d not found to kill, assume Oomed' %
                                     (hogs.index(p) + 1));
                    else:
                        logging.warning('Hog %d kill failed: %s' %
                                        (hogs.index(p) + 1,
                                         os.strerror(e.errno)));
                else:
                    hogs_killed += 1
                time.sleep(2)

        # Clean up the rest of our hogs since they otherwise live forever.
        utils.system("killall -TERM hog")
        time.sleep(5)
        swapfree2 = utils.read_from_meminfo('SwapFree')
        logging.info('SwapFree was %d before cleanup, %d after.' %
                     (swapfree, swapfree2))

        # Raise exceptions due to low memory notification failures.
        if first_lowmem == 0:
            raise error.TestFail('We did not get low memory notification!')
        elif not cleared_low_mem_notification:
            raise error.TestFail('We did not clear low memory notification!')
        elif len(hogs) - hogs_killed < first_lowmem - 3:
            raise error.TestFail('We got low memory notification at hog %d, '
                                 'but we did not clear it until we dropped to '
                                 'hog %d' %
                                 (first_lowmem, len(hogs) - hogs_killed))
