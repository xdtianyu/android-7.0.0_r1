# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.


#
# This test is overly simple right now, it'll just make sure that there are
# at least a few satellites seen (i.e. that signal can be received by the GPS).
#
# There are no checks to make sure that a fix can be had, nor of the precision
# or accurracy of said fix. That can either be handled by higher-level tests
# or added here later.
#


import logging, re
from autotest_lib.client.bin import test
from autotest_lib.client.common_lib import error, utils


class hardware_GobiGPS(test.test):
    version = 1

    def run_once(self):
        sats_seen = {}
        sats_signal = {}
        got_fix = False
        pos_lat = ""
        pos_long = ""

        try:
            nmea = utils.system_output('head -300 /tmp/gobi-nmea', timeout=60)
        except:
            raise error.TestFail('GPS access failed')
            return

        logging.debug(nmea)
        for line in nmea.split('\n'):
            line = line.strip()

            # Satellites in view
            if line.startswith('$GPGSV'):
                line = line.rstrip('*0123456789ABCDEF')
                fields = line.split(',')[4:]
                while fields:
                    sat = fields[0]
                    if fields[3]:
                        sats_seen[sat] = True
                        sats_signal[sat] = fields[3]
                    else:
                        sats_seen[sat] = True
                    fields = fields[4:]

            # Recommended minimum specific GPS/Transit data
            if line.startswith('$GPRMC'):
                # Looks like Gobi has non-standard GPRMC with 13 fields, not 12.
                match = re.search(
                    r'^\$GPRMC\,(.*)\,(.*)\,(.*)\,(.*)\,(.*)\,(.*)\,(.*)\,'
                    r'(.*)\,(.*)\,(.*)\,(.*),(.*)\*(.*)$',
                    line)

                if match and match.group(2) == 'A' and not got_fix:
                    logging.debug('Got fix:')
                    logging.debug('Time = %s', match.group(1))
                    logging.debug('Status = %s', match.group(2))
                    logging.debug('Latitude = %s %s', match.group(3),
                                  match.group(4))
                    logging.debug('Longitude = %s %s', match.group(5),
                                  match.group(6))
                    logging.debug('Speed = %s', match.group(7))
                    logging.debug('Track Angle = %s', match.group(8))
                    logging.debug('Date = %s', match.group(9))
                    logging.debug('Magnetic Variation = %s %s', match.group(10),
                                  match.group(11))
                    got_fix = True
                    pos_lat = '%s%s' % (match.group(3), match.group(4))
                    pos_long = '%s%s' % (match.group(5), match.group(6))
                    break

        logging.debug('number of satellites seen %d: %s',
                       len(sats_seen), sats_seen)
        logging.debug('number of satellites seen with signal strength %d: %s',
                       len(sats_signal), sats_signal)

        if got_fix:
            logging.info('Got fix: %s %s' % (pos_lat, pos_long))
            return

        # Somewhat random criteria: Pass if you can see 5 at all, and at least 2
        # enough to get a signal reading
        if len(sats_signal) < 2 and len(sats_seen) < 5:
            raise error.TestFail('Unable to find GPS signal')
        else:
            logging.info('Saw %d GPS satellites, %d with signal strength',
                         len(sats_seen), len(sats_signal))
